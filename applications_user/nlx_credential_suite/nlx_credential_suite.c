#include "nlx_credential_suite_i.h"

#include <stdio.h>
#include <string.h>

static bool nlx_custom_event_callback(void* context, uint32_t event) {
    NlxCredentialSuiteApp* app = context;
    return scene_manager_handle_custom_event(app->scene_manager, event);
}

static bool nlx_back_event_callback(void* context) {
    NlxCredentialSuiteApp* app = context;
    return scene_manager_handle_back_event(app->scene_manager);
}

void nlx_submenu_callback(void* context, uint32_t index) {
    NlxCredentialSuiteApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

void nlx_widget_callback(GuiButtonType result, InputType type, void* context) {
    if(type != InputTypeShort) return;
    NlxCredentialSuiteApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, result);
}

void nlx_dialog_callback(DialogExResult result, void* context) {
    NlxCredentialSuiteApp* app = context;
    const uint32_t event = result == DialogExResultRight ? NlxEventAuthorizedAccepted :
                                                           NlxEventAuthorizedRejected;
    view_dispatcher_send_custom_event(app->view_dispatcher, event);
}

void nlx_hardware_dialog_callback(DialogExResult result, void* context) {
    NlxCredentialSuiteApp* app = context;
    const uint32_t event = result == DialogExResultRight ? NlxEventHardwareLaunch :
                                                           NlxEventHardwareCancel;
    view_dispatcher_send_custom_event(app->view_dispatcher, event);
}

void nlx_text_input_callback(void* context) {
    NlxCredentialSuiteApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, NlxEventSiteJobSaved);
}

void nlx_app_stop_scanner(NlxCredentialSuiteApp* app) {
    if(!app || !app->scanner) return;
    nlx_scan_coordinator_cancel(app->scanner);
    nlx_scan_coordinator_join(app->scanner);
}

void nlx_app_show_message(NlxCredentialSuiteApp* app, const char* title, const char* message) {
    furi_string_printf(app->text, "\e#%s\e#\n%s", title ? title : "NLX", message ? message : "");
    scene_manager_next_scene(app->scene_manager, NlxSceneMessage);
}

bool nlx_app_launch_tool(NlxCredentialSuiteApp* app, NlxToolId tool_id) {
    const NlxToolDefinition* tool = nlx_tool_registry_get(tool_id);
    if(!tool) return false;
    if(tool->authorized_lab_only && !app->authorized_lab_acknowledged) {
        nlx_app_show_message(
            app, "Authorization required", "Open Authorized Lab and accept the warning first.");
        return false;
    }
    if(tool->launch_target[0] == '/' && !storage_file_exists(app->storage, tool->launch_target)) {
        furi_string_printf(
            app->text,
            "\e#Tool unavailable\e#\n%s is not installed at:\n%s",
            tool->name,
            tool->launch_target);
        scene_manager_next_scene(app->scene_manager, NlxSceneMessage);
        return false;
    }
    if(tool->external_hardware_may_be_required && !app->hardware_preflight_bypass) {
        app->pending_tool = tool_id;
        scene_manager_next_scene(app->scene_manager, NlxSceneHardwareRequired);
        return true;
    }
    app->hardware_preflight_bypass = false;

    nlx_app_stop_scanner(app);
    loader_enqueue_launch(app->loader, tool->launch_target, NULL, LoaderDeferredLaunchFlagGui);
    view_dispatcher_stop(app->view_dispatcher);
    return true;
}

static NlxToolId nlx_app_reader_for_result(const NlxCredentialResult* result) {
    switch(result->technology) {
    case NlxTechnologyLfRfid:
        return NlxToolBuiltInLfRfid;
    case NlxTechnologyPicopass:
        return NlxToolPicopass;
    case NlxTechnologyIbutton:
        return NlxToolBuiltInIbutton;
    case NlxTechnologyNfc:
    case NlxTechnologyUnknown:
    default:
        return NlxToolBuiltInNfc;
    }
}

bool nlx_app_launch_result_action(NlxCredentialSuiteApp* app, NlxResultAction action) {
    if(!app || action == NlxResultActionNone) return false;
    if(app->result.technology == NlxTechnologyUnknown) {
        nlx_app_show_message(app, "No credential", "Run Universal Scan before this action.");
        return true;
    }
    if(action != NlxResultActionSaveCapture && !app->authorized_lab_acknowledged) {
        app->pending_result_action = action;
        scene_manager_next_scene(app->scene_manager, NlxSceneAuthorization);
        return true;
    }

    if(app->secondary_result_present &&
       app->selected_result_component == NlxResultComponentNone) {
        app->pending_result_action = action;
        scene_manager_next_scene(app->scene_manager, NlxSceneTechnologySelect);
        return true;
    }

    const NlxCredentialResult* selected_result =
        app->selected_result_component == NlxResultComponentSecondary ?
            &app->secondary_result :
            &app->result;
    app->pending_result_action = NlxResultActionNone;
    app->selected_result_component = NlxResultComponentNone;
    return nlx_app_launch_tool(app, nlx_app_reader_for_result(selected_result));
}

static NlxCredentialSuiteApp* nlx_app_alloc(void) {
    NlxCredentialSuiteApp* app = malloc(sizeof(*app));
    memset(app, 0, sizeof(*app));
    nlx_credential_result_reset(&app->result);
    nlx_credential_result_reset(&app->secondary_result);

    app->gui = furi_record_open(RECORD_GUI);
    app->storage = furi_record_open(RECORD_STORAGE);
    app->loader = furi_record_open(RECORD_LOADER);
    app->dialogs = furi_record_open(RECORD_DIALOGS);
    app->text = furi_string_alloc();
    app->file_path = furi_string_alloc();

    app->scene_manager = scene_manager_alloc(&nlx_scene_handlers, app);
    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_custom_event_callback(app->view_dispatcher, nlx_custom_event_callback);
    view_dispatcher_set_navigation_event_callback(app->view_dispatcher, nlx_back_event_callback);

    app->submenu = submenu_alloc();
    app->text_box = text_box_alloc();
    text_box_set_font(app->text_box, TextBoxFontText);
    app->widget = widget_alloc();
    app->dialog = dialog_ex_alloc();
    app->text_input = text_input_alloc();
    view_dispatcher_add_view(app->view_dispatcher, NlxViewSubmenu, submenu_get_view(app->submenu));
    view_dispatcher_add_view(
        app->view_dispatcher, NlxViewTextBox, text_box_get_view(app->text_box));
    view_dispatcher_add_view(app->view_dispatcher, NlxViewWidget, widget_get_view(app->widget));
    view_dispatcher_add_view(app->view_dispatcher, NlxViewDialog, dialog_ex_get_view(app->dialog));
    view_dispatcher_add_view(
        app->view_dispatcher, NlxViewTextInput, text_input_get_view(app->text_input));
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);
    return app;
}

static void nlx_app_free(NlxCredentialSuiteApp* app) {
    if(!app) return;
    nlx_app_stop_scanner(app);
    if(app->scanner) nlx_scan_coordinator_free(app->scanner);
    view_dispatcher_remove_view(app->view_dispatcher, NlxViewSubmenu);
    view_dispatcher_remove_view(app->view_dispatcher, NlxViewTextBox);
    view_dispatcher_remove_view(app->view_dispatcher, NlxViewWidget);
    view_dispatcher_remove_view(app->view_dispatcher, NlxViewDialog);
    view_dispatcher_remove_view(app->view_dispatcher, NlxViewTextInput);
    submenu_free(app->submenu);
    text_box_free(app->text_box);
    widget_free(app->widget);
    dialog_ex_free(app->dialog);
    text_input_free(app->text_input);
    view_dispatcher_free(app->view_dispatcher);
    scene_manager_free(app->scene_manager);
    furi_string_free(app->file_path);
    furi_string_free(app->text);
    furi_record_close(RECORD_DIALOGS);
    furi_record_close(RECORD_LOADER);
    furi_record_close(RECORD_STORAGE);
    furi_record_close(RECORD_GUI);
    free(app);
}

int32_t nlx_credential_suite_app(void* context) {
    UNUSED(context);
    NlxCredentialSuiteApp* app = nlx_app_alloc();
    scene_manager_next_scene(app->scene_manager, NlxSceneMain);
    view_dispatcher_run(app->view_dispatcher);
    nlx_app_free(app);
    return 0;
}
