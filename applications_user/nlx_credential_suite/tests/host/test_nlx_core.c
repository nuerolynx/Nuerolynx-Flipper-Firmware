#include "nlx_credential_result.h"
#include "nlx_format_decoder.h"
#include "nlx_report.h"
#include "nlx_risk_assessment.h"
#include "nlx_scan_schedule.h"
#include "nlx_tool_registry.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint64_t make_h10301(uint8_t facility, uint16_t card) {
    uint64_t value = ((uint64_t)facility << 17U) | ((uint64_t)card << 1U);
    unsigned first_ones = 0U;
    unsigned second_ones = 0U;
    for(unsigned bit = 13U; bit <= 24U; ++bit)
        first_ones += (unsigned)((value >> bit) & 1ULL);
    for(unsigned bit = 1U; bit <= 12U; ++bit)
        second_ones += (unsigned)((value >> bit) & 1ULL);
    if((first_ones & 1U) != 0U) value |= 1ULL << 25U;
    if((second_ones & 1U) == 0U) value |= 1ULL;
    return value;
}

static unsigned parity32(uint32_t value) {
    unsigned parity = 0U;
    while(value) {
        parity ^= value & 1U;
        value >>= 1U;
    }
    return parity;
}

static uint64_t make_corporate1000(uint16_t facility, uint32_t card) {
    uint32_t mid = (facility & 0x0800U) >> 11U;
    uint32_t bot = ((uint32_t)(facility & 0x07FFU) << 21U) | ((card & 0xFFFFFU) << 1U);
    mid |= parity32((mid & 0x1U) ^ (bot & 0xB6DB6DB6U)) << 1U;
    bot |= (parity32((mid & 0x3U) ^ (bot & 0x6DB6DB6CU)) ^ 1U);
    mid |= (parity32((mid & 0x3U) ^ bot) ^ 1U) << 2U;
    return ((uint64_t)mid << 32U) | bot;
}

static uint64_t
    set_field(uint64_t raw, uint8_t bit_length, uint8_t offset, uint8_t length, uint64_t value) {
    const uint8_t shift = bit_length - offset - length;
    const uint64_t mask = ((1ULL << length) - 1ULL) << shift;
    return (raw & ~mask) | ((value << shift) & mask);
}

static uint64_t make_h10304(uint16_t facility, uint32_t card) {
    uint64_t raw = 0ULL;
    raw = set_field(raw, 37U, 1U, 16U, facility);
    raw = set_field(raw, 37U, 17U, 19U, card);
    const uint64_t first_data = (raw >> (37U - 1U - 18U)) & ((1ULL << 18U) - 1ULL);
    const uint64_t second_data = (raw >> (37U - 18U - 18U)) & ((1ULL << 18U) - 1ULL);
    raw = set_field(raw, 37U, 0U, 1U, parity32((uint32_t)first_data));
    raw = set_field(raw, 37U, 36U, 1U, parity32((uint32_t)second_data) ^ 1U);
    return raw;
}

static void test_h10301(void) {
    const uint64_t raw = make_h10301(42U, 12345U);
    NlxDecodeResult decoded;
    assert(nlx_format_decode(raw, 26U, &decoded));
    assert(decoded.count == 1U);
    assert(!decoded.ambiguous);
    assert(decoded.candidates[0].facility_code == 42U);
    assert(decoded.candidates[0].credential_number == 12345U);
    assert(decoded.candidates[0].parity == NlxParityValid);
    assert(decoded.candidates[0].score >= 85U);
}

static void test_bad_parity(void) {
    const uint64_t raw = make_h10301(7U, 99U) ^ (1ULL << 25U);
    NlxDecodeResult decoded;
    assert(nlx_format_decode(raw, 26U, &decoded));
    assert(decoded.candidates[0].parity == NlxParityInvalid);
    assert(decoded.candidates[0].score < 70U);
}

static void test_corporate1000(void) {
    NlxDecodeResult decoded;
    const uint64_t raw = make_corporate1000(0x456U, 0x54321U);
    assert(nlx_format_decode(raw, 35U, &decoded));
    assert(decoded.count == 1U);
    assert(decoded.candidates[0].facility_code == 0x456U);
    assert(decoded.candidates[0].credential_number == 0x54321U);
    assert(decoded.candidates[0].parity == NlxParityValid);
    assert(decoded.candidates[0].score >= 85U);
}

static void test_ambiguity(void) {
    NlxDecodeResult decoded;
    assert(nlx_format_decode(0x12345678ULL, 32U, &decoded));
    assert(decoded.count == 2U);
    assert(decoded.ambiguous);
    assert(decoded.candidates[0].facility_code != decoded.candidates[1].facility_code);
}

static void test_37_bit_candidates(void) {
    NlxDecodeResult decoded;
    assert(nlx_format_decode(make_h10304(0x1234U, 0x45678U), 37U, &decoded));
    assert(decoded.count == 2U);
    assert(decoded.ambiguous);
    assert(strcmp(decoded.candidates[0].definition->name, "HID H10304 37-bit") == 0);
    assert(decoded.candidates[0].facility_code == 0x1234U);
    assert(decoded.candidates[0].credential_number == 0x45678U);
    assert(decoded.candidates[0].parity == NlxParityValid);

    NlxCredentialResult result;
    nlx_credential_result_reset(&result);
    nlx_format_apply_best(&decoded, &result);
    assert(result.candidate_count == 2U);
    assert(result.facility_code_inferred);
}

static void test_normalized_serialization(void) {
    NlxCredentialResult result;
    nlx_credential_result_reset(&result);
    result.technology = NlxTechnologyLfRfid;
    result.frequency_khz = 125U;
    memcpy(result.protocol, "H10301", sizeof("H10301"));
    memcpy(result.card_family, "HID Prox", sizeof("HID Prox"));
    memcpy(result.recommended_tool, "NLX LF RFID", sizeof("NLX LF RFID"));
    result.completeness = NlxCompletenessIdentifier;

    NlxDecodeResult decoded;
    assert(nlx_format_decode(make_h10301(12U, 3456U), 26U, &decoded));
    nlx_format_apply_best(&decoded, &result);

    char json[1400];
    char text[1400];
    assert(nlx_report_json(&result, json, sizeof(json)) > 0U);
    assert(strstr(json, "\"technology\":\"LF RFID\"") != NULL);
    assert(strstr(json, "\"facility_code\":12") != NULL);
    assert(strstr(json, "\"credential_number\":3456") != NULL);
    assert(strstr(json, "\"parity\":\"valid\"") != NULL);
    assert(nlx_report_text(&result, text, sizeof(text)) > 0U);
    assert(strstr(text, "Facility/site code: 12") != NULL);

    memcpy(result.site_job, "Plant \"A\"", sizeof("Plant \"A\""));
    assert(nlx_report_json(&result, json, sizeof(json)) > 0U);
    assert(strstr(json, "Plant \\\"A\\\"") != NULL);
    assert(nlx_report_json(&result, json, 16U) == 0U);
}

static void test_dual_technology_serialization(void) {
    NlxCredentialResult hf;
    NlxCredentialResult lf;
    nlx_credential_result_reset(&hf);
    nlx_credential_result_reset(&lf);
    hf.technology = NlxTechnologyNfc;
    hf.frequency_khz = 13560U;
    memcpy(hf.protocol, "MIFARE Classic", sizeof("MIFARE Classic"));
    memcpy(hf.card_family, "MIFARE Classic 1K", sizeof("MIFARE Classic 1K"));
    lf.technology = NlxTechnologyLfRfid;
    lf.frequency_khz = 125U;
    memcpy(lf.protocol, "H10301", sizeof("H10301"));
    memcpy(lf.card_family, "HID Prox", sizeof("HID Prox"));
    lf.facility_code_present = true;
    lf.facility_code = 42U;
    lf.credential_number_present = true;
    lf.credential_number = 12345U;

    char json[7168];
    char text[7168];
    assert(nlx_report_json_pair(&hf, &lf, json, sizeof(json)) > 0U);
    assert(strstr(json, "\"dual_technology\":true") != NULL);
    assert(strstr(json, "\"technology\":\"NFC\"") != NULL);
    assert(strstr(json, "\"technology\":\"LF RFID\"") != NULL);
    assert(nlx_report_text_pair(&hf, &lf, text, sizeof(text)) > 0U);
    assert(strstr(text, "Dual-Technology") != NULL);
    assert(strstr(text, "Components detected: 2") != NULL);
    assert(strstr(text, "Facility/site code: 42") != NULL);
}

static void test_scan_timeout_and_cancel(void) {
    NlxScanSchedule schedule;
    nlx_scan_schedule_init(&schedule);
    assert(schedule.stage == NlxScanStageNfc);
    assert(schedule.cycle == 1U);
    nlx_scan_schedule_advance(&schedule);
    assert(schedule.stage == NlxScanStagePicopass);
    nlx_scan_schedule_advance(&schedule);
    assert(schedule.stage == NlxScanStageLfRfid);
    nlx_scan_schedule_advance(&schedule);
    assert(schedule.stage == NlxScanStageIbutton);
    nlx_scan_schedule_advance(&schedule);
    assert(schedule.stage == NlxScanStageNfc);
    assert(schedule.cycle == 2U);
    nlx_scan_schedule_cancel(&schedule);
    assert(nlx_scan_schedule_is_terminal(&schedule));
    assert(schedule.stage == NlxScanStageCancelled);
    nlx_scan_schedule_advance(&schedule);
    assert(schedule.stage == NlxScanStageCancelled);

    assert(!nlx_scan_wait_result_has(0xFFFFFFFEU, 1U << 1U));
    assert(!nlx_scan_wait_result_has(0xFFFFFFFDU, 1U << 1U));
    assert(nlx_scan_wait_result_has(1U << 1U, 1U << 1U));
    assert(!nlx_scan_wait_result_has(1U << 0U, 1U << 1U));
}

static void test_hid_generic_frame(void) {
    const uint64_t raw = make_h10301(24U, 54321U);
    uint8_t frame[6] = {};
    frame[0] |= 1U << 1U; /* Extended HID length header begins at bit 6. */
    frame[2] |= 1U << 6U; /* Terminating marker at bit 17 selects 26 data bits. */
    for(size_t index = 0U; index < 26U; ++index) {
        if((raw >> (25U - index)) & 1ULL) {
            const size_t frame_bit = 22U + index;
            frame[frame_bit / 8U] |= (uint8_t)(1U << (7U - (frame_bit % 8U)));
        }
    }

    NlxDecodeResult decoded;
    uint8_t bit_length = 0U;
    assert(nlx_format_decode_hid_generic(frame, sizeof(frame), &bit_length, &decoded));
    assert(bit_length == 26U);
    assert(decoded.candidates[0].facility_code == 24U);
    assert(decoded.candidates[0].credential_number == 54321U);
    assert(decoded.candidates[0].parity == NlxParityValid);
}

static void test_scan_detected(void) {
    NlxScanSchedule schedule;
    nlx_scan_schedule_init(&schedule);
    nlx_scan_schedule_detected(&schedule);
    assert(nlx_scan_schedule_is_terminal(&schedule));
    assert(schedule.stage == NlxScanStageDetected);
}

static void test_tool_handoffs(void) {
    const NlxToolDefinition* mfkey = nlx_tool_registry_get(NlxToolMfkey);
    const NlxToolDefinition* seader = nlx_tool_registry_get(NlxToolSeader);
    const NlxToolDefinition* detector = nlx_tool_registry_get(NlxToolFieldDetector);
    const NlxToolDefinition* rfid_fuzzer = nlx_tool_registry_get(NlxToolRfidFuzzer);
    const NlxToolDefinition* mifare_fuzzer = nlx_tool_registry_get(NlxToolMifareFuzzer);
    assert(mfkey != NULL && mfkey->authorized_lab_only);
    assert(
        seader != NULL && seader->authorized_lab_only &&
        seader->external_hardware_may_be_required);
    assert(detector != NULL && !detector->authorized_lab_only);
    assert(strstr(detector->launch_target, "nfc_rfid_detector.fap") != NULL);
    assert(rfid_fuzzer != NULL && rfid_fuzzer->authorized_lab_only);
    assert(mifare_fuzzer != NULL && mifare_fuzzer->authorized_lab_only);
    assert(nlx_tool_registry_get(NlxToolCount) == NULL);

    NlxCredentialResult result;
    nlx_credential_result_reset(&result);
    result.technology = NlxTechnologyPicopass;
    assert(nlx_tool_registry_recommend(&result) == NlxToolPicopass);
    result.technology = NlxTechnologyIbutton;
    assert(nlx_tool_registry_recommend(&result) == NlxToolBuiltInIbutton);
}

static void test_risk_assessment(void) {
    NlxCredentialResult result;
    NlxRiskAssessment assessment;
    nlx_credential_result_reset(&result);
    assert(!nlx_risk_assess(&result, &assessment));

    result.technology = NlxTechnologyLfRfid;
    result.security = NlxSecurityUnencrypted;
    result.completeness = NlxCompletenessIdentifier;
    result.confidence = NlxConfidenceHigh;
    assert(nlx_risk_assess(&result, &assessment));
    assert(assessment.exposure_score == 85U);
    assert(assessment.level == NlxRiskLevelVeryHigh);
    assert(!assessment.provisional);
    assert(assessment.indicators & NlxRiskIndicatorLegacy);
    assert(assessment.indicators & NlxRiskIndicatorUnencrypted);

    nlx_credential_result_reset(&result);
    result.technology = NlxTechnologyNfc;
    memcpy(result.card_family, "MIFARE DESFire", sizeof("MIFARE DESFire"));
    result.security = NlxSecurityAuthenticationRequired;
    result.completeness = NlxCompletenessClassified;
    result.confidence = NlxConfidenceHigh;
    assert(nlx_risk_assess(&result, &assessment));
    assert(assessment.exposure_score == 10U);
    assert(assessment.level == NlxRiskLevelLow);
    assert(assessment.provisional);
    assert(assessment.indicators & NlxRiskIndicatorProtected);
}

int main(void) {
    test_h10301();
    test_bad_parity();
    test_corporate1000();
    test_ambiguity();
    test_37_bit_candidates();
    test_normalized_serialization();
    test_dual_technology_serialization();
    test_scan_timeout_and_cancel();
    test_scan_detected();
    test_hid_generic_frame();
    test_tool_handoffs();
    test_risk_assessment();
    puts("NLX Credential Suite host tests passed");
    return 0;
}
