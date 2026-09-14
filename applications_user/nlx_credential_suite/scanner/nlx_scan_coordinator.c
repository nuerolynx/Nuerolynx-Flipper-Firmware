#include "nlx_scan_coordinator.h"

#include "../core/nlx_format_decoder.h"

#include <furi_hal_nfc.h>
#include <nfc/nfc.h>
#include <nfc/nfc_device.h>
#include <nfc/nfc_poller.h>
#include <nfc/protocols/nfc_protocol.h>
#include <lfrfid/lfrfid_worker.h>
#include <lfrfid/protocols/lfrfid_protocols.h>
#include <ibutton/ibutton_key.h>
#include <ibutton/ibutton_protocols.h>
#include <ibutton/ibutton_worker.h>
#include <toolbox/bit_buffer.h>
#include <toolbox/protocols/protocol_dict.h>

#include <stdio.h>
#include <string.h>

#define NLX_SCAN_FLAG_CANCEL   (1U << 0)
#define NLX_SCAN_FLAG_DETECTED (1U << 1)
#define NLX_SCAN_FLAG_MASK     (NLX_SCAN_FLAG_CANCEL | NLX_SCAN_FLAG_DETECTED)

#define NLX_PICOPASS_WINDOW_MS 1250U
#define NLX_LF_WINDOW_MS       1750U
#define NLX_IBUTTON_WINDOW_MS  750U

#define NLX_PICOPASS_ACTALL 0x0AU
#define NLX_PICOPASS_FWT_FC 100000U

struct NlxScanCoordinator {
    FuriThread* thread;
    FuriEventFlag* flags;
    NlxScanStatusCallback status_callback;
    NlxScanResultCallback result_callback;
    void* callback_context;
    volatile bool running;
    volatile bool cancel_requested;
    NfcProtocol nfc_protocol;
    ProtocolId lf_protocol;
    ProtocolDict* lf_dict;
    iButtonProtocols* ibutton_protocols;
    iButtonKey* ibutton_key;
    NlxCredentialResult result;
    NlxCredentialResult secondary_result;
    bool secondary_present;
    NlxCredentialResult phase_secondary_result;
    bool phase_secondary_present;
};

typedef struct {
    NlxScanCoordinator* coordinator;
    Nfc* nfc;
    BitBuffer* tx;
    BitBuffer* rx;
    volatile bool stop_requested;
} NlxPicopassDetector;

static void nlx_copy_text(char* destination, size_t capacity, const char* source) {
    if(!destination || capacity == 0U) return;
    if(!source) source = "";
    snprintf(destination, capacity, "%s", source);
}

static bool nlx_scan_cancelled(NlxScanCoordinator* coordinator) {
    return coordinator->cancel_requested;
}

static uint32_t nlx_wait_for_detection(NlxScanCoordinator* coordinator, uint32_t timeout_ms) {
    return furi_event_flag_wait(
        coordinator->flags, NLX_SCAN_FLAG_MASK, FuriFlagWaitAny, timeout_ms);
}

static uint8_t nlx_nfc_protocol_depth(NfcProtocol protocol) {
    uint8_t depth = 0U;
    while(protocol != NfcProtocolInvalid) {
        ++depth;
        const NfcProtocol parent = nfc_protocol_get_parent(protocol);
        if(parent == protocol) break;
        protocol = parent;
    }
    return depth;
}

static const char* nlx_nfc_family(NfcProtocol protocol) {
    switch(protocol) {
    case NfcProtocolMfClassic:
        return "MIFARE Classic";
    case NfcProtocolMfUltralight:
        return "MIFARE Ultralight/NTAG";
    case NfcProtocolMfPlus:
        return "MIFARE Plus";
    case NfcProtocolMfDesfire:
        return "MIFARE DESFire";
    case NfcProtocolFelica:
        return "FeliCa";
    case NfcProtocolIso15693_3:
    case NfcProtocolSlix:
        return "ISO15693";
    case NfcProtocolIso14443_3a:
    case NfcProtocolIso14443_4a:
        return "ISO14443-A";
    case NfcProtocolIso14443_3b:
    case NfcProtocolIso14443_4b:
        return "ISO14443-B";
    default:
        return "Smart card/tag";
    }
}

static void nlx_set_nfc_result(
    NlxScanCoordinator* coordinator,
    const NfcDevice* detected_device) {
    NlxCredentialResult* result = &coordinator->result;
    nlx_credential_result_reset(result);
    result->technology = NlxTechnologyNfc;
    result->frequency_khz = 13560U;
    nlx_copy_text(
        result->protocol,
        sizeof(result->protocol),
        nfc_device_get_protocol_name(coordinator->nfc_protocol));
    nlx_copy_text(
        result->card_family,
        sizeof(result->card_family),
        nlx_nfc_family(coordinator->nfc_protocol));
    nlx_copy_text(result->recommended_tool, sizeof(result->recommended_tool), "Built-in NFC");
    result->parity = NlxParityNotApplicable;
    result->security = (coordinator->nfc_protocol == NfcProtocolMfDesfire ||
                        coordinator->nfc_protocol == NfcProtocolMfPlus) ?
                           NlxSecurityAuthenticationRequired :
                           NlxSecurityUnknown;
    result->completeness = NlxCompletenessClassified;
    result->confidence = NlxConfidenceHigh;
    if(detected_device && nfc_device_get_protocol(detected_device) != NfcProtocolInvalid) {
        size_t uid_length = 0U;
        const uint8_t* uid = nfc_device_get_uid(detected_device, &uid_length);
        result->uid_length = uid_length > NLX_RESULT_UID_MAX ? NLX_RESULT_UID_MAX : uid_length;
        if(uid && result->uid_length > 0U) {
            memcpy(result->uid, uid, result->uid_length);
            result->raw_data_length = result->uid_length;
            memcpy(result->raw_data, result->uid, result->uid_length);
            result->completeness = NlxCompletenessIdentifier;
        }
    }
}

static bool nlx_nfc_try_protocol(
    NlxScanCoordinator* coordinator,
    Nfc* nfc,
    NfcDevice* detected_device,
    NfcProtocol protocol,
    uint8_t* best_depth) {
    if(nlx_scan_cancelled(coordinator)) return false;

    NfcPoller* poller = nfc_poller_alloc(nfc, protocol);
    const bool detected = nfc_poller_detect(poller);
    if(detected) {
        const uint8_t depth = nlx_nfc_protocol_depth(protocol);
        if(coordinator->nfc_protocol == NfcProtocolInvalid || depth > *best_depth) {
            coordinator->nfc_protocol = protocol;
            *best_depth = depth;
            // The base poller owns the authoritative identifier at detection time.
            // Deeper pollers may only have enough state to classify the family.
            if(depth == 1U) {
                nfc_device_set_data(detected_device, protocol, nfc_poller_get_data(poller));
            }
        }
    }
    nfc_poller_free(poller);
    return detected;
}

static bool nlx_scan_nfc(NlxScanCoordinator* coordinator) {
    if(furi_hal_nfc_is_hal_ready() != FuriHalNfcErrorNone) {
        FURI_LOG_E("NLXCred", "NFC hardware is not ready");
        return false;
    }
    FURI_LOG_I("NLXCred", "NFC phase start");
    coordinator->nfc_protocol = NfcProtocolInvalid;
    Nfc* nfc = nfc_alloc();
    NfcDevice* detected_device = nfc_device_alloc();
    bool detected_bases[NfcProtocolNum] = {};
    bool detected_protocols[NfcProtocolNum] = {};
    uint8_t best_depth = 0U;
    coordinator->phase_secondary_present = false;
    nlx_credential_result_reset(&coordinator->phase_secondary_result);

    // Run each base poller once. Unlike NfcScanner, this phase has no nested
    // scanner thread and is never torn down in the middle of a protocol poll.
    for(NfcProtocol protocol = 0; protocol < NfcProtocolNum; ++protocol) {
        if(nfc_protocol_get_parent(protocol) != NfcProtocolInvalid) continue;
        detected_bases[protocol] = nlx_nfc_try_protocol(
            coordinator, nfc, detected_device, protocol, &best_depth);
        detected_protocols[protocol] = detected_bases[protocol];
        if(nlx_scan_cancelled(coordinator)) break;
    }

    // Refine a detected technology into the most specific supported family.
    if(coordinator->nfc_protocol != NfcProtocolInvalid && !nlx_scan_cancelled(coordinator)) {
        for(NfcProtocol protocol = 0; protocol < NfcProtocolNum; ++protocol) {
            const NfcProtocol parent = nfc_protocol_get_parent(protocol);
            if(parent == NfcProtocolInvalid) continue;
            bool eligible = false;
            for(NfcProtocol base = 0; base < NfcProtocolNum; ++base) {
                if(detected_bases[base] && nfc_protocol_has_parent(protocol, base)) {
                    eligible = true;
                    break;
                }
            }
            if(eligible) {
                detected_protocols[protocol] = nlx_nfc_try_protocol(
                    coordinator, nfc, detected_device, protocol, &best_depth);
            }
            if(nlx_scan_cancelled(coordinator)) break;
        }
    }

    const bool detected =
        !nlx_scan_cancelled(coordinator) && coordinator->nfc_protocol != NfcProtocolInvalid;
    if(detected) {
        const NfcProtocol primary_protocol = coordinator->nfc_protocol;
        nlx_set_nfc_result(coordinator, detected_device);
        const NlxCredentialResult primary_result = coordinator->result;

        NfcProtocol secondary_protocol = NfcProtocolInvalid;
        uint8_t secondary_depth = 0U;
        for(NfcProtocol protocol = 0; protocol < NfcProtocolNum; ++protocol) {
            if(!detected_protocols[protocol] || protocol == primary_protocol) continue;
            // Parent and child detections describe one chip. Only unrelated
            // protocol branches are evidence of a second embedded HF chip.
            if(nfc_protocol_has_parent(primary_protocol, protocol) ||
               nfc_protocol_has_parent(protocol, primary_protocol)) {
                continue;
            }
            const uint8_t depth = nlx_nfc_protocol_depth(protocol);
            if(secondary_protocol == NfcProtocolInvalid || depth > secondary_depth) {
                secondary_protocol = protocol;
                secondary_depth = depth;
            }
        }

        if(secondary_protocol != NfcProtocolInvalid) {
            coordinator->nfc_protocol = secondary_protocol;
            nlx_set_nfc_result(coordinator, NULL);
            coordinator->phase_secondary_result = coordinator->result;
            coordinator->phase_secondary_result.confidence = NlxConfidenceMedium;
            coordinator->phase_secondary_present = true;
            coordinator->result = primary_result;
            coordinator->nfc_protocol = primary_protocol;
        }
    }
    nfc_device_free(detected_device);
    nfc_free(nfc);
    FURI_LOG_I("NLXCred", "NFC phase complete: %s", detected ? "detected" : "none");
    return detected;
}

static NfcCommand nlx_picopass_callback(NfcEvent event, void* context) {
    NlxPicopassDetector* detector = context;
    if(detector->stop_requested || nlx_scan_cancelled(detector->coordinator)) {
        return NfcCommandStop;
    }
    if(event.type != NfcEventTypePollerReady) return NfcCommandContinue;

    bit_buffer_reset(detector->tx);
    bit_buffer_append_byte(detector->tx, NLX_PICOPASS_ACTALL);
    const NfcError error =
        nfc_poller_trx(detector->nfc, detector->tx, detector->rx, NLX_PICOPASS_FWT_FC);

    if(error == NfcErrorIncompleteFrame) {
        furi_event_flag_set(detector->coordinator->flags, NLX_SCAN_FLAG_DETECTED);
        return NfcCommandStop;
    }
    furi_delay_ms(75U);
    return NfcCommandContinue;
}

static bool nlx_scan_picopass(NlxScanCoordinator* coordinator) {
    if(furi_hal_nfc_is_hal_ready() != FuriHalNfcErrorNone) return false;
    FURI_LOG_I("NLXCred", "Picopass phase start");
    furi_event_flag_clear(coordinator->flags, NLX_SCAN_FLAG_DETECTED);
    Nfc* nfc = nfc_alloc();
    NlxPicopassDetector detector = {
        .coordinator = coordinator,
        .nfc = nfc,
        .tx = bit_buffer_alloc(16U),
        .rx = bit_buffer_alloc(16U),
    };
    nfc_config(nfc, NfcModePoller, NfcTechIso15693);
    nfc_set_guard_time_us(nfc, 10000U);
    nfc_set_fdt_poll_fc(nfc, 5000U);
    nfc_set_fdt_poll_poll_us(nfc, 1000U);
    nfc_start(nfc, nlx_picopass_callback, &detector);
    const uint32_t flags = nlx_wait_for_detection(coordinator, NLX_PICOPASS_WINDOW_MS);
    // Raw Nfc pollers do not have NfcPoller's StopRequest wrapper. Signal our
    // callback before joining, otherwise a no-card timeout waits forever here.
    detector.stop_requested = true;
    nfc_stop(nfc);
    bit_buffer_free(detector.tx);
    bit_buffer_free(detector.rx);
    nfc_free(nfc);

    if(nlx_scan_wait_result_has(flags, NLX_SCAN_FLAG_DETECTED)) {
        NlxCredentialResult* result = &coordinator->result;
        nlx_credential_result_reset(result);
        result->technology = NlxTechnologyPicopass;
        result->frequency_khz = 13560U;
        nlx_copy_text(result->protocol, sizeof(result->protocol), "Picopass");
        nlx_copy_text(result->card_family, sizeof(result->card_family), "HID iCLASS/Picopass");
        nlx_copy_text(result->recommended_tool, sizeof(result->recommended_tool), "Picopass");
        result->security = NlxSecurityUnknown;
        result->parity = NlxParityUnknown;
        result->completeness = NlxCompletenessClassified;
        result->confidence = NlxConfidenceHigh;
        FURI_LOG_I("NLXCred", "Picopass phase complete: detected");
        return true;
    }
    FURI_LOG_I("NLXCred", "Picopass phase complete: none");
    return false;
}

static void nlx_lf_callback(LFRFIDWorkerReadResult result, ProtocolId protocol, void* context) {
    NlxScanCoordinator* coordinator = context;
    if(result == LFRFIDWorkerReadDone) {
        coordinator->lf_protocol = protocol;
        furi_event_flag_set(coordinator->flags, NLX_SCAN_FLAG_DETECTED);
    }
}

static void nlx_set_lf_result(NlxScanCoordinator* coordinator) {
    NlxCredentialResult* result = &coordinator->result;
    nlx_credential_result_reset(result);
    result->technology = NlxTechnologyLfRfid;
    result->frequency_khz = 125U;
    nlx_copy_text(
        result->protocol,
        sizeof(result->protocol),
        protocol_dict_get_name(coordinator->lf_dict, coordinator->lf_protocol));
    nlx_copy_text(
        result->manufacturer,
        sizeof(result->manufacturer),
        protocol_dict_get_manufacturer(coordinator->lf_dict, coordinator->lf_protocol));
    nlx_copy_text(result->card_family, sizeof(result->card_family), "125 kHz credential");
    nlx_copy_text(result->recommended_tool, sizeof(result->recommended_tool), "125 kHz RFID");
    result->security = NlxSecurityUnencrypted;
    result->completeness = NlxCompletenessIdentifier;
    result->confidence = NlxConfidenceHigh;

    size_t data_size = protocol_dict_get_data_size(coordinator->lf_dict, coordinator->lf_protocol);
    if(data_size > NLX_RESULT_RAW_MAX) data_size = NLX_RESULT_RAW_MAX;
    protocol_dict_get_data(
        coordinator->lf_dict, coordinator->lf_protocol, result->raw_data, data_size);
    result->raw_data_length = data_size;

    if(coordinator->lf_protocol == LFRFIDProtocolH10301 && data_size >= 3U) {
        nlx_copy_text(result->card_family, sizeof(result->card_family), "HID Prox");
        nlx_copy_text(
            result->probable_format, sizeof(result->probable_format), "HID H10301 26-bit");
        result->bit_length = 26U;
        result->facility_code_present = true;
        result->facility_code = result->raw_data[0];
        result->credential_number_present = true;
        result->credential_number = ((uint16_t)result->raw_data[1] << 8U) | result->raw_data[2];
        result->parity = NlxParityValid;
    } else if(coordinator->lf_protocol == LFRFIDProtocolHidGeneric && data_size == 6U) {
        NlxDecodeResult decoded;
        uint8_t bit_length = 0U;
        nlx_copy_text(result->card_family, sizeof(result->card_family), "HID Prox");
        if(nlx_format_decode_hid_generic(result->raw_data, data_size, &bit_length, &decoded)) {
            nlx_format_apply_best(&decoded, result);
        } else {
            result->bit_length = bit_length;
            nlx_copy_text(
                result->probable_format,
                sizeof(result->probable_format),
                bit_length ? "Unmapped HID format" : "HID format undetermined");
            result->parity = NlxParityUnknown;
            result->confidence = NlxConfidenceMedium;
        }
    } else if(
        coordinator->lf_protocol == LFRFIDProtocolEM4100 ||
        coordinator->lf_protocol == LFRFIDProtocolEM410032 ||
        coordinator->lf_protocol == LFRFIDProtocolEM410016) {
        result->bit_length = (uint16_t)(data_size * 8U);
        nlx_copy_text(result->probable_format, sizeof(result->probable_format), "EM4100 serial");
        result->parity = NlxParityValid;
    } else {
        nlx_copy_text(
            result->probable_format,
            sizeof(result->probable_format),
            result->protocol[0] ? result->protocol : "Protocol-specific");
        result->parity = NlxParityUnknown;
    }
}

static bool nlx_scan_lf(NlxScanCoordinator* coordinator) {
    FURI_LOG_I("NLXCred", "LF RFID phase start");
    furi_event_flag_clear(coordinator->flags, NLX_SCAN_FLAG_DETECTED);
    coordinator->lf_protocol = PROTOCOL_NO;
    coordinator->lf_dict = protocol_dict_alloc(lfrfid_protocols, LFRFIDProtocolMax);
    LFRFIDWorker* worker = lfrfid_worker_alloc(coordinator->lf_dict);
    lfrfid_worker_start_thread(worker);
    lfrfid_worker_read_start(worker, LFRFIDWorkerReadTypeAuto, nlx_lf_callback, coordinator);
    const uint32_t flags = nlx_wait_for_detection(coordinator, NLX_LF_WINDOW_MS);
    lfrfid_worker_stop(worker);
    lfrfid_worker_stop_thread(worker);

    const bool detected = nlx_scan_wait_result_has(flags, NLX_SCAN_FLAG_DETECTED) &&
                          coordinator->lf_protocol != PROTOCOL_NO;
    if(detected) nlx_set_lf_result(coordinator);
    lfrfid_worker_free(worker);
    protocol_dict_free(coordinator->lf_dict);
    coordinator->lf_dict = NULL;
    FURI_LOG_I("NLXCred", "LF RFID phase complete: %s", detected ? "detected" : "none");
    return detected;
}

static void nlx_ibutton_callback(void* context) {
    NlxScanCoordinator* coordinator = context;
    furi_event_flag_set(coordinator->flags, NLX_SCAN_FLAG_DETECTED);
}

static bool nlx_scan_ibutton(NlxScanCoordinator* coordinator) {
    FURI_LOG_I("NLXCred", "iButton phase start");
    furi_event_flag_clear(coordinator->flags, NLX_SCAN_FLAG_DETECTED);
    coordinator->ibutton_protocols = ibutton_protocols_alloc();
    coordinator->ibutton_key =
        ibutton_key_alloc(ibutton_protocols_get_max_data_size(coordinator->ibutton_protocols));
    iButtonWorker* worker = ibutton_worker_alloc(coordinator->ibutton_protocols);
    ibutton_worker_start_thread(worker);
    ibutton_worker_read_set_callback(worker, nlx_ibutton_callback, coordinator);
    ibutton_worker_read_start(worker, coordinator->ibutton_key);
    const uint32_t flags = nlx_wait_for_detection(coordinator, NLX_IBUTTON_WINDOW_MS);
    ibutton_worker_stop(worker);
    ibutton_worker_stop_thread(worker);

    const bool detected =
        nlx_scan_wait_result_has(flags, NLX_SCAN_FLAG_DETECTED) &&
        ibutton_protocols_is_valid(coordinator->ibutton_protocols, coordinator->ibutton_key);
    if(detected) {
        NlxCredentialResult* result = &coordinator->result;
        nlx_credential_result_reset(result);
        const iButtonProtocolId protocol = ibutton_key_get_protocol_id(coordinator->ibutton_key);
        result->technology = NlxTechnologyIbutton;
        result->frequency_khz = 0U;
        nlx_copy_text(
            result->protocol,
            sizeof(result->protocol),
            ibutton_protocols_get_name(coordinator->ibutton_protocols, protocol));
        nlx_copy_text(result->card_family, sizeof(result->card_family), "1-Wire/contact key");
        nlx_copy_text(
            result->manufacturer,
            sizeof(result->manufacturer),
            ibutton_protocols_get_manufacturer(coordinator->ibutton_protocols, protocol));
        nlx_copy_text(result->recommended_tool, sizeof(result->recommended_tool), "iButton");
        iButtonEditableData editable = {};
        ibutton_protocols_get_editable_data(
            coordinator->ibutton_protocols, coordinator->ibutton_key, &editable);
        result->raw_data_length = editable.size > NLX_RESULT_RAW_MAX ? NLX_RESULT_RAW_MAX :
                                                                       editable.size;
        if(editable.ptr && result->raw_data_length > 0U) {
            memcpy(result->raw_data, editable.ptr, result->raw_data_length);
            result->uid_length = result->raw_data_length > NLX_RESULT_UID_MAX ?
                                     NLX_RESULT_UID_MAX :
                                     result->raw_data_length;
            memcpy(result->uid, result->raw_data, result->uid_length);
        }
        result->parity = NlxParityNotApplicable;
        result->security = NlxSecurityUnknown;
        result->completeness = NlxCompletenessIdentifier;
        result->confidence = NlxConfidenceHigh;
    }

    ibutton_worker_free(worker);
    ibutton_key_free(coordinator->ibutton_key);
    ibutton_protocols_free(coordinator->ibutton_protocols);
    coordinator->ibutton_key = NULL;
    coordinator->ibutton_protocols = NULL;
    FURI_LOG_I("NLXCred", "iButton phase complete: %s", detected ? "detected" : "none");
    return detected;
}

static int32_t nlx_scan_thread(void* context) {
    NlxScanCoordinator* coordinator = context;
    coordinator->running = true;
    bool detected = false;
    bool hf_detected = false;
    bool second_hf_detected = false;
    NlxCredentialResult hf_result;
    NlxCredentialResult second_hf_result;
    nlx_credential_result_reset(&hf_result);
    nlx_credential_result_reset(&second_hf_result);
    coordinator->secondary_present = false;
    nlx_credential_result_reset(&coordinator->secondary_result);
    NlxScanSchedule schedule;
    nlx_scan_schedule_init(&schedule);

    while(!nlx_scan_cancelled(coordinator) && !nlx_scan_schedule_is_terminal(&schedule)) {
        coordinator->status_callback(
            schedule.stage, schedule.cycle, coordinator->callback_context);
        switch(schedule.stage) {
        case NlxScanStageNfc:
            detected = nlx_scan_nfc(coordinator);
            break;
        case NlxScanStagePicopass:
            detected = nlx_scan_picopass(coordinator);
            break;
        case NlxScanStageLfRfid:
            detected = nlx_scan_lf(coordinator);
            break;
        case NlxScanStageIbutton:
            detected = nlx_scan_ibutton(coordinator);
            break;
        default:
            break;
        }

        if(nlx_scan_cancelled(coordinator)) {
            nlx_scan_schedule_cancel(&schedule);
        } else if(schedule.stage == NlxScanStageNfc) {
            if(detected) {
                memcpy(&hf_result, &coordinator->result, sizeof(hf_result));
                hf_detected = true;
                if(coordinator->phase_secondary_present) {
                    memcpy(
                        &second_hf_result,
                        &coordinator->phase_secondary_result,
                        sizeof(second_hf_result));
                    second_hf_detected = true;
                }
            }
            nlx_scan_schedule_advance(&schedule);
        } else if(schedule.stage == NlxScanStagePicopass) {
            if(detected) {
                const bool primary_is_generic_iso15693 =
                    hf_detected && strcmp(hf_result.card_family, "ISO15693") == 0;
                const bool secondary_is_generic_iso15693 =
                    second_hf_detected &&
                    strcmp(second_hf_result.card_family, "ISO15693") == 0;
                if(primary_is_generic_iso15693) {
                    memcpy(&hf_result, &coordinator->result, sizeof(hf_result));
                } else if(secondary_is_generic_iso15693) {
                    memcpy(&second_hf_result, &coordinator->result, sizeof(second_hf_result));
                } else if(hf_detected && !second_hf_detected) {
                    memcpy(&second_hf_result, &coordinator->result, sizeof(second_hf_result));
                    second_hf_detected = true;
                } else if(!hf_detected) {
                    memcpy(&hf_result, &coordinator->result, sizeof(hf_result));
                    hf_detected = true;
                }
            }
            nlx_scan_schedule_advance(&schedule);
        } else if(schedule.stage == NlxScanStageLfRfid) {
            if(detected) {
                if(hf_detected) {
                    memcpy(
                        &coordinator->secondary_result,
                        &coordinator->result,
                        sizeof(coordinator->secondary_result));
                    memcpy(&coordinator->result, &hf_result, sizeof(coordinator->result));
                    coordinator->secondary_present = true;
                    if(second_hf_detected) {
                        FURI_LOG_W(
                            "NLXCred",
                            "More than two credential components detected; reporting HF and LF");
                    }
                }
                nlx_scan_schedule_detected(&schedule);
            } else if(hf_detected) {
                memcpy(&coordinator->result, &hf_result, sizeof(coordinator->result));
                if(second_hf_detected) {
                    memcpy(
                        &coordinator->secondary_result,
                        &second_hf_result,
                        sizeof(coordinator->secondary_result));
                    coordinator->secondary_present = true;
                }
                detected = true;
                nlx_scan_schedule_detected(&schedule);
            } else {
                nlx_scan_schedule_advance(&schedule);
            }
        } else if(detected) {
            nlx_scan_schedule_detected(&schedule);
        } else {
            nlx_scan_schedule_advance(&schedule);
        }
    }

    if(detected && !nlx_scan_cancelled(coordinator)) {
        coordinator->status_callback(
            NlxScanStageDetected, schedule.cycle, coordinator->callback_context);
        coordinator->result_callback(
            &coordinator->result,
            coordinator->secondary_present ? &coordinator->secondary_result : NULL,
            coordinator->callback_context);
    } else {
        coordinator->status_callback(
            NlxScanStageCancelled, schedule.cycle, coordinator->callback_context);
    }
    coordinator->running = false;
    return 0;
}

NlxScanCoordinator* nlx_scan_coordinator_alloc(
    NlxScanStatusCallback status_callback,
    NlxScanResultCallback result_callback,
    void* context) {
    furi_check(status_callback);
    furi_check(result_callback);
    NlxScanCoordinator* coordinator = malloc(sizeof(*coordinator));
    memset(coordinator, 0, sizeof(*coordinator));
    coordinator->flags = furi_event_flag_alloc();
    coordinator->status_callback = status_callback;
    coordinator->result_callback = result_callback;
    coordinator->callback_context = context;
    nlx_credential_result_reset(&coordinator->result);
    nlx_credential_result_reset(&coordinator->secondary_result);
    nlx_credential_result_reset(&coordinator->phase_secondary_result);
    return coordinator;
}

void nlx_scan_coordinator_free(NlxScanCoordinator* coordinator) {
    if(!coordinator) return;
    nlx_scan_coordinator_cancel(coordinator);
    nlx_scan_coordinator_join(coordinator);
    furi_event_flag_free(coordinator->flags);
    free(coordinator);
}

bool nlx_scan_coordinator_start(NlxScanCoordinator* coordinator) {
    furi_check(coordinator);
    if(coordinator->thread || coordinator->running) return false;
    furi_event_flag_clear(coordinator->flags, NLX_SCAN_FLAG_MASK);
    coordinator->cancel_requested = false;
    coordinator->thread = furi_thread_alloc_ex("NlxCredScan", 4096U, nlx_scan_thread, coordinator);
    furi_thread_start(coordinator->thread);
    return true;
}

void nlx_scan_coordinator_cancel(NlxScanCoordinator* coordinator) {
    if(!coordinator) return;
    coordinator->cancel_requested = true;
    furi_event_flag_set(coordinator->flags, NLX_SCAN_FLAG_CANCEL);
}

void nlx_scan_coordinator_join(NlxScanCoordinator* coordinator) {
    if(!coordinator || !coordinator->thread) return;
    furi_thread_join(coordinator->thread);
    furi_thread_free(coordinator->thread);
    coordinator->thread = NULL;
}

bool nlx_scan_coordinator_is_running(const NlxScanCoordinator* coordinator) {
    return coordinator && coordinator->running;
}
