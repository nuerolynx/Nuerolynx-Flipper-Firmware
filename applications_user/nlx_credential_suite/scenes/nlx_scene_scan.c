#include "../nlx_credential_suite_i.h"

#include <stdio.h>
#include <string.h>

static const char* nlx_stage_name(NlxScanStage stage) {
    switch(stage) {
    case NlxScanStageNfc:
        return "NFC / smart card";
    case NlxScanStagePicopass:
        return "Picopass / iCLASS";
    case NlxScanStageLfRfid:
        return "125 kHz / dual-tech";
    case NlxScanStageIbutton:
        return "iButton contact";
    case NlxScanStageDetected:
        return "Credential detected";
    default:
        return "Preparing";
    }
}

static void nlx_scan_refresh(NlxCredentialSuiteApp* app) {
    furi_string_printf(
        app->text,
        "\e#Read Credential\e#\nCycle %lu\n\nChecking:\n%s\n\nBack: cancel",
        app->scan_cycle,
        nlx_stage_name(app->scan_stage));
    text_box_reset(app->text_box);
    text_box_set_text(app->text_box, furi_string_get_cstr(app->text));
    view_dispatcher_switch_to_view(app->view_dispatcher, NlxViewTextBox);
}

static void nlx_scan_status_callback(NlxScanStage stage, uint32_t cycle, void* context) {
    NlxCredentialSuiteApp* app = context;
    app->scan_stage = stage;
    app->scan_cycle = cycle;
    view_dispatcher_send_custom_event(
        app->view_dispatcher, NlxEventScanStatusBase + (uint32_t)stage);
}

static void nlx_scan_result_callback(
    const NlxCredentialResult* result,
    const NlxCredentialResult* secondary,
    void* context) {
    NlxCredentialSuiteApp* app = context;
    memcpy(&app->result, result, sizeof(app->result));
    snprintf(app->result.site_job, sizeof(app->result.site_job), "%s", app->site_job);
    app->secondary_result_present = secondary != NULL;
    if(secondary) {
        memcpy(&app->secondary_result, secondary, sizeof(app->secondary_result));
        snprintf(
            app->secondary_result.site_job,
            sizeof(app->secondary_result.site_job),
            "%s",
            app->site_job);
    } else {
        nlx_credential_result_reset(&app->secondary_result);
    }
    app->selected_result_component = NlxResultComponentNone;
    view_dispatcher_send_custom_event(app->view_dispatcher, NlxEventScanResult);
}

void nlx_scene_universal_scan_on_enter(void* context) {
    NlxCredentialSuiteApp* app = context;
    app->scan_stage = NlxScanStageIdle;
    app->scan_cycle = 1U;
    nlx_scan_refresh(app);
    if(!app->scanner) {
        app->scanner =
            nlx_scan_coordinator_alloc(nlx_scan_status_callback, nlx_scan_result_callback, app);
    }
    if(!nlx_scan_coordinator_start(app->scanner)) {
        nlx_app_show_message(app, "Scanner busy", "The credential scanner could not start.");
    }
}

bool nlx_scene_universal_scan_on_event(void* context, SceneManagerEvent event) {
    NlxCredentialSuiteApp* app = context;
    if(event.type == SceneManagerEventTypeBack) {
        nlx_app_stop_scanner(app);
        return false;
    }
    if(event.type != SceneManagerEventTypeCustom) return false;
    if(event.event == NlxEventScanResult) {
        scene_manager_next_scene(app->scene_manager, NlxSceneResult);
        return true;
    }
    if(event.event >= NlxEventScanStatusBase &&
       event.event <= NlxEventScanStatusBase + NlxScanStageCancelled) {
        nlx_scan_refresh(app);
        return true;
    }
    return false;
}

void nlx_scene_universal_scan_on_exit(void* context) {
    NlxCredentialSuiteApp* app = context;
    nlx_app_stop_scanner(app);
    text_box_reset(app->text_box);
}
