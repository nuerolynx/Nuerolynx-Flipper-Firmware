#include "../nlx_credential_suite_i.h"

void nlx_scene_main_on_enter(void* context) {
    NlxCredentialSuiteApp* app = context;
    submenu_reset(app->submenu);
    submenu_set_header(app->submenu, "NLX Credential Suite");
    submenu_add_item(
        app->submenu, "Read Credential", NlxEventMenuUniversalScan, nlx_submenu_callback, app);
    submenu_add_item(app->submenu, "NFC / Smart Card", NlxEventMenuNfc, nlx_submenu_callback, app);
    submenu_add_item(app->submenu, "LF RFID", NlxEventMenuLfRfid, nlx_submenu_callback, app);
    submenu_add_item(
        app->submenu, "Picopass / iCLASS", NlxEventMenuPicopass, nlx_submenu_callback, app);
    submenu_add_item(app->submenu, "iButton", NlxEventMenuIbutton, nlx_submenu_callback, app);
    submenu_add_item(
        app->submenu, "Saved Inspections", NlxEventMenuSaved, nlx_submenu_callback, app);
    submenu_add_item(
        app->submenu, "Audit & Diagnostics", NlxEventMenuDiagnostics, nlx_submenu_callback, app);
    submenu_add_item(
        app->submenu, "Authorized Lab", NlxEventMenuAuthorizedLab, nlx_submenu_callback, app);
    submenu_add_item(app->submenu, "Settings", NlxEventMenuSettings, nlx_submenu_callback, app);
    submenu_add_item(
        app->submenu, "About / Licenses", NlxEventMenuAbout, nlx_submenu_callback, app);
    view_dispatcher_switch_to_view(app->view_dispatcher, NlxViewSubmenu);
}

bool nlx_scene_main_on_event(void* context, SceneManagerEvent event) {
    NlxCredentialSuiteApp* app = context;
    if(event.type == SceneManagerEventTypeBack) {
        view_dispatcher_stop(app->view_dispatcher);
        return true;
    }
    if(event.type != SceneManagerEventTypeCustom) return false;

    switch(event.event) {
    case NlxEventMenuUniversalScan:
        scene_manager_next_scene(app->scene_manager, NlxSceneUniversalScan);
        return true;
    case NlxEventMenuNfc:
        return nlx_app_launch_tool(app, NlxToolBuiltInNfc);
    case NlxEventMenuLfRfid:
        return nlx_app_launch_tool(app, NlxToolBuiltInLfRfid);
    case NlxEventMenuPicopass:
        return nlx_app_launch_tool(app, NlxToolPicopass);
    case NlxEventMenuIbutton:
        return nlx_app_launch_tool(app, NlxToolBuiltInIbutton);
    case NlxEventMenuSaved:
        scene_manager_next_scene(app->scene_manager, NlxSceneSaved);
        return true;
    case NlxEventMenuDiagnostics:
        scene_manager_next_scene(app->scene_manager, NlxSceneDiagnostics);
        return true;
    case NlxEventMenuAuthorizedLab:
        scene_manager_next_scene(
            app->scene_manager,
            app->authorized_lab_acknowledged ? NlxSceneAuthorizedLab : NlxSceneAuthorization);
        return true;
    case NlxEventMenuSettings:
        scene_manager_next_scene(app->scene_manager, NlxSceneSettings);
        return true;
    case NlxEventMenuAbout:
        scene_manager_next_scene(app->scene_manager, NlxSceneAbout);
        return true;
    default:
        return false;
    }
}

void nlx_scene_main_on_exit(void* context) {
    NlxCredentialSuiteApp* app = context;
    submenu_reset(app->submenu);
}
