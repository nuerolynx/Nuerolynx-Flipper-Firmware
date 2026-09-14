#include "../nlx_credential_suite_i.h"

#include <stdlib.h>

#define NLX_INSPECTION_DIR  "/ext/apps_data/nlx_credential_suite/inspections"
#define NLX_REPORT_VIEW_MAX 4096U

void nlx_scene_technology_select_on_enter(void* context) {
    NlxCredentialSuiteApp* app = context;
    submenu_reset(app->submenu);
    submenu_set_header(app->submenu, "Choose technology");
    furi_string_printf(
        app->text,
        "%s - %s",
        nlx_technology_name(app->result.technology),
        app->result.card_family[0] ? app->result.card_family : app->result.protocol);
    submenu_add_item(
        app->submenu,
        furi_string_get_cstr(app->text),
        NlxEventSelectPrimary,
        nlx_submenu_callback,
        app);
    furi_string_printf(
        app->text,
        "%s - %s",
        nlx_technology_name(app->secondary_result.technology),
        app->secondary_result.card_family[0] ? app->secondary_result.card_family :
                                               app->secondary_result.protocol);
    submenu_add_item(
        app->submenu,
        furi_string_get_cstr(app->text),
        NlxEventSelectSecondary,
        nlx_submenu_callback,
        app);
    view_dispatcher_switch_to_view(app->view_dispatcher, NlxViewSubmenu);
}

bool nlx_scene_technology_select_on_event(void* context, SceneManagerEvent event) {
    NlxCredentialSuiteApp* app = context;
    if(event.type != SceneManagerEventTypeCustom) return false;
    if(event.event != NlxEventSelectPrimary && event.event != NlxEventSelectSecondary) {
        return false;
    }

    const NlxResultAction action = app->pending_result_action;
    app->selected_result_component = event.event == NlxEventSelectSecondary ?
                                         NlxResultComponentSecondary :
                                         NlxResultComponentPrimary;
    return nlx_app_launch_result_action(app, action);
}

void nlx_scene_technology_select_on_exit(void* context) {
    NlxCredentialSuiteApp* app = context;
    submenu_reset(app->submenu);
    app->selected_result_component = NlxResultComponentNone;
    app->pending_result_action = NlxResultActionNone;
}

static bool nlx_saved_report_load(NlxCredentialSuiteApp* app, const char* path) {
    File* file = storage_file_alloc(app->storage);
    bool success = false;
    if(storage_file_open(file, path, FSAM_READ, FSOM_OPEN_EXISTING)) {
        size_t size = storage_file_size(file);
        if(size > NLX_REPORT_VIEW_MAX) size = NLX_REPORT_VIEW_MAX;
        char* buffer = malloc(size + 1U);
        if(buffer) {
            const size_t read = storage_file_read(file, buffer, size);
            buffer[read] = '\0';
            furi_string_set(app->text, buffer);
            free(buffer);
            success = read > 0U;
        }
        storage_file_close(file);
    }
    storage_file_free(file);
    return success;
}

void nlx_scene_saved_on_enter(void* context) {
    NlxCredentialSuiteApp* app = context;
    storage_common_mkdir(app->storage, "/ext/apps_data/nlx_credential_suite");
    storage_common_mkdir(app->storage, NLX_INSPECTION_DIR);
    furi_string_set(app->file_path, NLX_INSPECTION_DIR);

    DialogsFileBrowserOptions options;
    dialog_file_browser_set_basic_options(&options, ".txt", NULL);
    options.base_path = NLX_INSPECTION_DIR;
    options.hide_ext = false;
    options.skip_assets = true;

    if(!dialog_file_browser_show(app->dialogs, app->file_path, app->file_path, &options)) {
        scene_manager_previous_scene(app->scene_manager);
        return;
    }

    if(!nlx_saved_report_load(app, furi_string_get_cstr(app->file_path))) {
        furi_string_set(
            app->text, "\e#Unable to open report\e#\nCheck the SD card and try again.");
    }
    text_box_set_text(app->text_box, furi_string_get_cstr(app->text));
    view_dispatcher_switch_to_view(app->view_dispatcher, NlxViewTextBox);
}

bool nlx_scene_saved_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void nlx_scene_saved_on_exit(void* context) {
    NlxCredentialSuiteApp* app = context;
    text_box_reset(app->text_box);
}

static void nlx_diagnostics_append_risk(
    FuriString* text,
    const NlxCredentialResult* result,
    const char* label) {
    NlxRiskAssessment assessment;
    if(!nlx_risk_assess(result, &assessment)) return;

    furi_string_cat_printf(
        text,
        "\n\e#%s: %s\e#\nExposure: %u/100 (%s)%s\n",
        label,
        nlx_technology_name(result->technology),
        assessment.exposure_score,
        nlx_risk_level_name(assessment.level),
        assessment.provisional ? " - provisional" : "");
    if(assessment.indicators & NlxRiskIndicatorLegacy) {
        furi_string_cat(text, "- Legacy credential family\n");
    }
    if(assessment.indicators & NlxRiskIndicatorUnencrypted) {
        furi_string_cat(text, "- Unencrypted signal/data\n");
    }
    if(assessment.indicators & NlxRiskIndicatorProtected) {
        furi_string_cat(text, "- Encryption/auth observed\n");
    }
    if(assessment.indicators & NlxRiskIndicatorIncomplete) {
        furi_string_cat(text, "- Shallow classification only\n");
    }
    if(assessment.indicators & NlxRiskIndicatorLowConfidence) {
        furi_string_cat(text, "- Evidence confidence is low\n");
    }
    if(assessment.indicators & NlxRiskIndicatorAmbiguous) {
        furi_string_cat(text, "- Multiple format candidates\n");
    }
}

static bool nlx_diagnostics_tool_ready(
    NlxCredentialSuiteApp* app,
    NlxToolId tool_id) {
    const NlxToolDefinition* tool = nlx_tool_registry_get(tool_id);
    if(!tool) return false;
    return tool->launch_target[0] != '/' || storage_file_exists(app->storage, tool->launch_target);
}

static void nlx_diagnostics_append_tool(
    NlxCredentialSuiteApp* app,
    NlxToolId tool_id) {
    const NlxToolDefinition* tool = nlx_tool_registry_get(tool_id);
    if(!tool) return;
    furi_string_cat_printf(
        app->text,
        "%s %s%s\n",
        nlx_diagnostics_tool_ready(app, tool_id) ? "+" : "-",
        tool->name,
        tool->external_hardware_may_be_required ? " *" : "");
}

void nlx_scene_diagnostics_on_enter(void* context) {
    NlxCredentialSuiteApp* app = context;
    submenu_reset(app->submenu);
    submenu_set_header(app->submenu, "Audit & Diagnostics");
    submenu_add_item(
        app->submenu, "Credential Risk Review", NlxEventDiagnosticsRisk, nlx_submenu_callback, app);
    submenu_add_item(
        app->submenu,
        "HF/LF Field Detector",
        NlxEventDiagnosticsFieldDetector,
        nlx_submenu_callback,
        app);
    submenu_add_item(
        app->submenu, "Tool Availability", NlxEventDiagnosticsTools, nlx_submenu_callback, app);
    view_dispatcher_switch_to_view(app->view_dispatcher, NlxViewSubmenu);
}

bool nlx_scene_diagnostics_on_event(void* context, SceneManagerEvent event) {
    NlxCredentialSuiteApp* app = context;
    if(event.type != SceneManagerEventTypeCustom) return false;

    if(event.event == NlxEventDiagnosticsRisk) {
        if(app->result.technology == NlxTechnologyUnknown) {
            nlx_app_show_message(
                app, "No inspection", "Run Read Credential first, then return here for review.");
            return true;
        }
        furi_string_set(app->text, "\e#NLX Credential Risk Review\e#");
        nlx_diagnostics_append_risk(app->text, &app->result, "Primary");
        if(app->secondary_result_present) {
            nlx_diagnostics_append_risk(app->text, &app->secondary_result, "Secondary");
        }
        furi_string_cat(
            app->text,
            "\nHigher means more exposed. Screening estimate only; verify controls and reader configuration before drawing conclusions.");
        scene_manager_next_scene(app->scene_manager, NlxSceneMessage);
        return true;
    }
    if(event.event == NlxEventDiagnosticsFieldDetector) {
        return nlx_app_launch_tool(app, NlxToolFieldDetector);
    }
    if(event.event == NlxEventDiagnosticsTools) {
        furi_string_set(app->text, "\e#Credential tools\e#\n+ ready  - missing\n");
        nlx_diagnostics_append_tool(app, NlxToolBuiltInNfc);
        nlx_diagnostics_append_tool(app, NlxToolBuiltInLfRfid);
        nlx_diagnostics_append_tool(app, NlxToolPicopass);
        nlx_diagnostics_append_tool(app, NlxToolMfkey);
        nlx_diagnostics_append_tool(app, NlxToolNfcMagic);
        nlx_diagnostics_append_tool(app, NlxToolFieldDetector);
        nlx_diagnostics_append_tool(app, NlxToolSeader);
        nlx_diagnostics_append_tool(app, NlxToolRfidFuzzer);
        nlx_diagnostics_append_tool(app, NlxToolMifareFuzzer);
        furi_string_cat(app->text, "\n* compatible expansion hardware may be required");
        scene_manager_next_scene(app->scene_manager, NlxSceneMessage);
        return true;
    }
    return false;
}

void nlx_scene_diagnostics_on_exit(void* context) {
    NlxCredentialSuiteApp* app = context;
    submenu_reset(app->submenu);
}

void nlx_scene_authorization_on_enter(void* context) {
    NlxCredentialSuiteApp* app = context;
    dialog_ex_reset(app->dialog);
    dialog_ex_set_header(app->dialog, "Authorized Lab", 64, 4, AlignCenter, AlignTop);
    dialog_ex_set_text(
        app->dialog,
        "Use only on credentials and\nsystems you own or have\npermission to test.",
        64,
        20,
        AlignCenter,
        AlignTop);
    dialog_ex_set_left_button_text(app->dialog, "Cancel");
    dialog_ex_set_right_button_text(app->dialog, "I Agree");
    dialog_ex_set_context(app->dialog, app);
    dialog_ex_set_result_callback(app->dialog, nlx_dialog_callback);
    view_dispatcher_switch_to_view(app->view_dispatcher, NlxViewDialog);
}

bool nlx_scene_authorization_on_event(void* context, SceneManagerEvent event) {
    NlxCredentialSuiteApp* app = context;
    if(event.type != SceneManagerEventTypeCustom) return false;
    if(event.event == NlxEventAuthorizedAccepted) {
        app->authorized_lab_acknowledged = true;
        if(app->pending_result_action != NlxResultActionNone) {
            const NlxResultAction action = app->pending_result_action;
            app->pending_result_action = NlxResultActionNone;
            return nlx_app_launch_result_action(app, action);
        }
        scene_manager_next_scene(app->scene_manager, NlxSceneAuthorizedLab);
        return true;
    }
    if(event.event == NlxEventAuthorizedRejected) {
        app->pending_result_action = NlxResultActionNone;
        scene_manager_previous_scene(app->scene_manager);
        return true;
    }
    return false;
}

void nlx_scene_authorization_on_exit(void* context) {
    NlxCredentialSuiteApp* app = context;
    dialog_ex_reset(app->dialog);
}

void nlx_scene_authorized_lab_on_enter(void* context) {
    NlxCredentialSuiteApp* app = context;
    submenu_reset(app->submenu);
    submenu_set_header(app->submenu, "Authorized Lab");
    submenu_add_item(
        app->submenu, "Read then Emulate", NlxEventLabEmulate, nlx_submenu_callback, app);
    submenu_add_item(
        app->submenu, "Read then Copy/Write", NlxEventLabCopyWrite, nlx_submenu_callback, app);
    submenu_add_item(
        app->submenu, "MFKey / MFKey32", NlxEventLabMfkey, nlx_submenu_callback, app);
    submenu_add_item(app->submenu, "NFC APDU Runner", NlxEventLabApdu, nlx_submenu_callback, app);
    submenu_add_item(app->submenu, "NFC Magic", NlxEventLabMagic, nlx_submenu_callback, app);
    submenu_add_item(
        app->submenu, "ISO15693 Writer", NlxEventLabIso15693, nlx_submenu_callback, app);
    submenu_add_item(app->submenu, "Seader", NlxEventLabSeader, nlx_submenu_callback, app);
    submenu_add_item(app->submenu, "SEOS", NlxEventLabSeos, nlx_submenu_callback, app);
    submenu_add_item(
        app->submenu, "RFID Fuzzer", NlxEventLabRfidFuzzer, nlx_submenu_callback, app);
    submenu_add_item(
        app->submenu, "MIFARE Fuzzer", NlxEventLabMifareFuzzer, nlx_submenu_callback, app);
    submenu_add_item(
        app->submenu, "iButton Fuzzer", NlxEventLabIbuttonFuzzer, nlx_submenu_callback, app);
    view_dispatcher_switch_to_view(app->view_dispatcher, NlxViewSubmenu);
}

bool nlx_scene_authorized_lab_on_event(void* context, SceneManagerEvent event) {
    NlxCredentialSuiteApp* app = context;
    if(event.type != SceneManagerEventTypeCustom) return false;
    switch(event.event) {
    case NlxEventLabEmulate:
        return nlx_app_launch_result_action(app, NlxResultActionEmulate);
    case NlxEventLabCopyWrite:
        return nlx_app_launch_result_action(app, NlxResultActionCopyWrite);
    case NlxEventLabMfkey:
        return nlx_app_launch_tool(app, NlxToolMfkey);
    case NlxEventLabApdu:
        return nlx_app_launch_tool(app, NlxToolNfcApduRunner);
    case NlxEventLabMagic:
        return nlx_app_launch_tool(app, NlxToolNfcMagic);
    case NlxEventLabIso15693:
        return nlx_app_launch_tool(app, NlxToolIso15693Writer);
    case NlxEventLabSeader:
        return nlx_app_launch_tool(app, NlxToolSeader);
    case NlxEventLabSeos:
        return nlx_app_launch_tool(app, NlxToolSeos);
    case NlxEventLabRfidFuzzer:
        return nlx_app_launch_tool(app, NlxToolRfidFuzzer);
    case NlxEventLabMifareFuzzer:
        return nlx_app_launch_tool(app, NlxToolMifareFuzzer);
    case NlxEventLabIbuttonFuzzer:
        return nlx_app_launch_tool(app, NlxToolIbuttonFuzzer);
    default:
        return false;
    }
}

void nlx_scene_authorized_lab_on_exit(void* context) {
    NlxCredentialSuiteApp* app = context;
    submenu_reset(app->submenu);
}

void nlx_scene_hardware_required_on_enter(void* context) {
    NlxCredentialSuiteApp* app = context;
    const NlxToolDefinition* tool = nlx_tool_registry_get(app->pending_tool);
    dialog_ex_reset(app->dialog);
    dialog_ex_set_header(app->dialog, "External hardware", 64, 3, AlignCenter, AlignTop);
    furi_string_printf(
        app->text,
        "%s may require a compatible\nSAM/serial expansion module.\nAttach it before launch.",
        tool ? tool->name : "This tool");
    dialog_ex_set_text(
        app->dialog, furi_string_get_cstr(app->text), 64, 18, AlignCenter, AlignTop);
    dialog_ex_set_left_button_text(app->dialog, "Back");
    dialog_ex_set_right_button_text(app->dialog, "Launch");
    dialog_ex_set_context(app->dialog, app);
    dialog_ex_set_result_callback(app->dialog, nlx_hardware_dialog_callback);
    view_dispatcher_switch_to_view(app->view_dispatcher, NlxViewDialog);
}

bool nlx_scene_hardware_required_on_event(void* context, SceneManagerEvent event) {
    NlxCredentialSuiteApp* app = context;
    if(event.type != SceneManagerEventTypeCustom) return false;
    if(event.event == NlxEventHardwareLaunch) {
        app->hardware_preflight_bypass = true;
        return nlx_app_launch_tool(app, app->pending_tool);
    }
    if(event.event == NlxEventHardwareCancel) {
        scene_manager_previous_scene(app->scene_manager);
        return true;
    }
    return false;
}

void nlx_scene_hardware_required_on_exit(void* context) {
    NlxCredentialSuiteApp* app = context;
    dialog_ex_reset(app->dialog);
}

void nlx_scene_settings_on_enter(void* context) {
    NlxCredentialSuiteApp* app = context;
    submenu_reset(app->submenu);
    submenu_set_header(app->submenu, "Settings");
    submenu_add_item(
        app->submenu, "Site / job label", NlxEventSettingsSiteJob, nlx_submenu_callback, app);
    submenu_add_item(
        app->submenu, "Scan profile", NlxEventSettingsScanProfile, nlx_submenu_callback, app);
    view_dispatcher_switch_to_view(app->view_dispatcher, NlxViewSubmenu);
}

bool nlx_scene_settings_on_event(void* context, SceneManagerEvent event) {
    NlxCredentialSuiteApp* app = context;
    if(event.type != SceneManagerEventTypeCustom) return false;
    if(event.event == NlxEventSettingsSiteJob) {
        scene_manager_next_scene(app->scene_manager, NlxSceneSiteJob);
        return true;
    }
    if(event.event == NlxEventSettingsScanProfile) {
        scene_manager_next_scene(app->scene_manager, NlxSceneScanProfile);
        return true;
    }
    return false;
}

void nlx_scene_settings_on_exit(void* context) {
    NlxCredentialSuiteApp* app = context;
    submenu_reset(app->submenu);
}

void nlx_scene_site_job_on_enter(void* context) {
    NlxCredentialSuiteApp* app = context;
    text_input_reset(app->text_input);
    text_input_set_header_text(app->text_input, "Optional site / job");
    text_input_set_result_callback(
        app->text_input, nlx_text_input_callback, app, app->site_job, sizeof(app->site_job), true);
    view_dispatcher_switch_to_view(app->view_dispatcher, NlxViewTextInput);
}

bool nlx_scene_site_job_on_event(void* context, SceneManagerEvent event) {
    NlxCredentialSuiteApp* app = context;
    if(event.type == SceneManagerEventTypeCustom && event.event == NlxEventSiteJobSaved) {
        scene_manager_previous_scene(app->scene_manager);
        return true;
    }
    return false;
}

void nlx_scene_site_job_on_exit(void* context) {
    NlxCredentialSuiteApp* app = context;
    text_input_reset(app->text_input);
}

void nlx_scene_scan_profile_on_enter(void* context) {
    NlxCredentialSuiteApp* app = context;
    furi_string_set(
        app->text,
        "\e#Balanced scan\e#\nNFC: 1.50 s\nPicopass: 1.25 s\nLF RFID: 1.75 s\n\n"
        "Radio pass: 4.50 s\niButton contact: 0.75 s\n\n"
        "HF hits continue through LF to check dual-tech cards.\n\n"
        "No continuous GUI timer. Back cancels safely.");
    text_box_set_text(app->text_box, furi_string_get_cstr(app->text));
    view_dispatcher_switch_to_view(app->view_dispatcher, NlxViewTextBox);
}

bool nlx_scene_scan_profile_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void nlx_scene_scan_profile_on_exit(void* context) {
    NlxCredentialSuiteApp* app = context;
    text_box_reset(app->text_box);
}

void nlx_scene_about_on_enter(void* context) {
    NlxCredentialSuiteApp* app = context;
    furi_string_printf(
        app->text,
        "\e#NLX Credential Suite\e#\nv0.6 | Nuerolynx\n%u parser families\n\n"
        "Passive inspection is the default. No automatic writing or emulation.\n\n"
        "Audit & Diagnostics includes a transparent NLX exposure review and the passive HF/LF reader-field detector.\n\n"
        "Firmware: GPLv3\nThird-party tools retain their own licenses and remain independently maintained. "
        "SEOS is AGPLv3 and is only launched as a separate application.",
        (unsigned int)nlx_parser_registry_count());
    text_box_set_text(app->text_box, furi_string_get_cstr(app->text));
    view_dispatcher_switch_to_view(app->view_dispatcher, NlxViewTextBox);
}

bool nlx_scene_about_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void nlx_scene_about_on_exit(void* context) {
    NlxCredentialSuiteApp* app = context;
    text_box_reset(app->text_box);
}

void nlx_scene_message_on_enter(void* context) {
    NlxCredentialSuiteApp* app = context;
    text_box_reset(app->text_box);
    text_box_set_text(app->text_box, furi_string_get_cstr(app->text));
    view_dispatcher_switch_to_view(app->view_dispatcher, NlxViewTextBox);
}

bool nlx_scene_message_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void nlx_scene_message_on_exit(void* context) {
    NlxCredentialSuiteApp* app = context;
    text_box_reset(app->text_box);
}
