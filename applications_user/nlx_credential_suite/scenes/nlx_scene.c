#include "../nlx_credential_suite_i.h"

static const AppSceneOnEnterCallback nlx_on_enter_handlers[NlxSceneCount] = {
    [NlxSceneMain] = nlx_scene_main_on_enter,
    [NlxSceneUniversalScan] = nlx_scene_universal_scan_on_enter,
    [NlxSceneResult] = nlx_scene_result_on_enter,
    [NlxSceneTechnologySelect] = nlx_scene_technology_select_on_enter,
    [NlxSceneSaved] = nlx_scene_saved_on_enter,
    [NlxSceneDiagnostics] = nlx_scene_diagnostics_on_enter,
    [NlxSceneAuthorization] = nlx_scene_authorization_on_enter,
    [NlxSceneAuthorizedLab] = nlx_scene_authorized_lab_on_enter,
    [NlxSceneHardwareRequired] = nlx_scene_hardware_required_on_enter,
    [NlxSceneSettings] = nlx_scene_settings_on_enter,
    [NlxSceneSiteJob] = nlx_scene_site_job_on_enter,
    [NlxSceneScanProfile] = nlx_scene_scan_profile_on_enter,
    [NlxSceneAbout] = nlx_scene_about_on_enter,
    [NlxSceneMessage] = nlx_scene_message_on_enter,
};

static const AppSceneOnEventCallback nlx_on_event_handlers[NlxSceneCount] = {
    [NlxSceneMain] = nlx_scene_main_on_event,
    [NlxSceneUniversalScan] = nlx_scene_universal_scan_on_event,
    [NlxSceneResult] = nlx_scene_result_on_event,
    [NlxSceneTechnologySelect] = nlx_scene_technology_select_on_event,
    [NlxSceneSaved] = nlx_scene_saved_on_event,
    [NlxSceneDiagnostics] = nlx_scene_diagnostics_on_event,
    [NlxSceneAuthorization] = nlx_scene_authorization_on_event,
    [NlxSceneAuthorizedLab] = nlx_scene_authorized_lab_on_event,
    [NlxSceneHardwareRequired] = nlx_scene_hardware_required_on_event,
    [NlxSceneSettings] = nlx_scene_settings_on_event,
    [NlxSceneSiteJob] = nlx_scene_site_job_on_event,
    [NlxSceneScanProfile] = nlx_scene_scan_profile_on_event,
    [NlxSceneAbout] = nlx_scene_about_on_event,
    [NlxSceneMessage] = nlx_scene_message_on_event,
};

static const AppSceneOnExitCallback nlx_on_exit_handlers[NlxSceneCount] = {
    [NlxSceneMain] = nlx_scene_main_on_exit,
    [NlxSceneUniversalScan] = nlx_scene_universal_scan_on_exit,
    [NlxSceneResult] = nlx_scene_result_on_exit,
    [NlxSceneTechnologySelect] = nlx_scene_technology_select_on_exit,
    [NlxSceneSaved] = nlx_scene_saved_on_exit,
    [NlxSceneDiagnostics] = nlx_scene_diagnostics_on_exit,
    [NlxSceneAuthorization] = nlx_scene_authorization_on_exit,
    [NlxSceneAuthorizedLab] = nlx_scene_authorized_lab_on_exit,
    [NlxSceneHardwareRequired] = nlx_scene_hardware_required_on_exit,
    [NlxSceneSettings] = nlx_scene_settings_on_exit,
    [NlxSceneSiteJob] = nlx_scene_site_job_on_exit,
    [NlxSceneScanProfile] = nlx_scene_scan_profile_on_exit,
    [NlxSceneAbout] = nlx_scene_about_on_exit,
    [NlxSceneMessage] = nlx_scene_message_on_exit,
};

const SceneManagerHandlers nlx_scene_handlers = {
    .on_enter_handlers = nlx_on_enter_handlers,
    .on_event_handlers = nlx_on_event_handlers,
    .on_exit_handlers = nlx_on_exit_handlers,
    .scene_num = NlxSceneCount,
};
