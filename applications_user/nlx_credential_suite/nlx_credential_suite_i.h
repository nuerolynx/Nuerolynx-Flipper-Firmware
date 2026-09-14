#pragma once

#include "core/nlx_credential_result.h"
#include "core/nlx_risk_assessment.h"
#include "handoff/nlx_tool_registry.h"
#include "parsers/nlx_parser_registry.h"
#include "scanner/nlx_scan_coordinator.h"

#include <furi.h>
#include <dialogs/dialogs.h>
#include <gui/gui.h>
#include <gui/modules/dialog_ex.h>
#include <gui/modules/submenu.h>
#include <gui/modules/text_box.h>
#include <gui/modules/text_input.h>
#include <gui/modules/widget.h>
#include <gui/scene_manager.h>
#include <gui/view_dispatcher.h>
#include <loader/loader.h>
#include <storage/storage.h>

typedef struct NlxCredentialSuiteApp NlxCredentialSuiteApp;

typedef enum {
    NlxViewSubmenu = 0,
    NlxViewTextBox,
    NlxViewWidget,
    NlxViewDialog,
    NlxViewTextInput,
} NlxView;

typedef enum {
    NlxSceneMain = 0,
    NlxSceneUniversalScan,
    NlxSceneResult,
    NlxSceneTechnologySelect,
    NlxSceneSaved,
    NlxSceneDiagnostics,
    NlxSceneAuthorization,
    NlxSceneAuthorizedLab,
    NlxSceneHardwareRequired,
    NlxSceneSettings,
    NlxSceneSiteJob,
    NlxSceneScanProfile,
    NlxSceneAbout,
    NlxSceneMessage,
    NlxSceneCount,
} NlxScene;

typedef enum {
    NlxEventMenuUniversalScan = 10,
    NlxEventMenuNfc,
    NlxEventMenuLfRfid,
    NlxEventMenuPicopass,
    NlxEventMenuIbutton,
    NlxEventMenuSaved,
    NlxEventMenuDiagnostics,
    NlxEventMenuAuthorizedLab,
    NlxEventMenuSettings,
    NlxEventMenuAbout,
    NlxEventScanStatusBase = 100,
    NlxEventScanResult = 120,
    NlxEventAuthorizedAccepted = 130,
    NlxEventAuthorizedRejected,
    NlxEventLabMfkey = 150,
    NlxEventLabApdu,
    NlxEventLabMagic,
    NlxEventLabIso15693,
    NlxEventLabSeader,
    NlxEventLabSeos,
    NlxEventLabEmulate,
    NlxEventLabCopyWrite,
    NlxEventLabRfidFuzzer,
    NlxEventLabMifareFuzzer,
    NlxEventLabIbuttonFuzzer,
    NlxEventResultSave = 170,
    NlxEventResultTool,
    NlxEventSettingsSiteJob = 180,
    NlxEventSettingsScanProfile,
    NlxEventSiteJobSaved,
    NlxEventHardwareLaunch = 190,
    NlxEventHardwareCancel,
    NlxEventSelectPrimary,
    NlxEventSelectSecondary,
    NlxEventDiagnosticsRisk,
    NlxEventDiagnosticsFieldDetector,
    NlxEventDiagnosticsTools,
} NlxEvent;

typedef enum {
    NlxResultActionNone = 0,
    NlxResultActionSaveCapture,
    NlxResultActionEmulate,
    NlxResultActionCopyWrite,
} NlxResultAction;

typedef enum {
    NlxResultComponentNone = 0,
    NlxResultComponentPrimary,
    NlxResultComponentSecondary,
} NlxResultComponent;

struct NlxCredentialSuiteApp {
    Gui* gui;
    Storage* storage;
    Loader* loader;
    DialogsApp* dialogs;
    ViewDispatcher* view_dispatcher;
    SceneManager* scene_manager;
    Submenu* submenu;
    TextBox* text_box;
    Widget* widget;
    DialogEx* dialog;
    TextInput* text_input;
    FuriString* text;
    FuriString* file_path;
    NlxScanCoordinator* scanner;
    NlxCredentialResult result;
    NlxCredentialResult secondary_result;
    bool secondary_result_present;
    NlxScanStage scan_stage;
    uint32_t scan_cycle;
    bool authorized_lab_acknowledged;
    bool hardware_preflight_bypass;
    NlxToolId pending_tool;
    NlxResultAction pending_result_action;
    NlxResultComponent selected_result_component;
    char site_job[NLX_RESULT_TEXT_MEDIUM];
    char saved_path[NLX_RESULT_PATH_MAX];
};

extern const SceneManagerHandlers nlx_scene_handlers;

void nlx_app_show_message(NlxCredentialSuiteApp* app, const char* title, const char* message);
bool nlx_app_launch_tool(NlxCredentialSuiteApp* app, NlxToolId tool_id);
bool nlx_app_launch_result_action(NlxCredentialSuiteApp* app, NlxResultAction action);
void nlx_app_stop_scanner(NlxCredentialSuiteApp* app);
void nlx_submenu_callback(void* context, uint32_t index);
void nlx_widget_callback(GuiButtonType result, InputType type, void* context);
void nlx_dialog_callback(DialogExResult result, void* context);
void nlx_hardware_dialog_callback(DialogExResult result, void* context);
void nlx_text_input_callback(void* context);

void nlx_scene_main_on_enter(void* context);
bool nlx_scene_main_on_event(void* context, SceneManagerEvent event);
void nlx_scene_main_on_exit(void* context);

void nlx_scene_universal_scan_on_enter(void* context);
bool nlx_scene_universal_scan_on_event(void* context, SceneManagerEvent event);
void nlx_scene_universal_scan_on_exit(void* context);

void nlx_scene_result_on_enter(void* context);
bool nlx_scene_result_on_event(void* context, SceneManagerEvent event);
void nlx_scene_result_on_exit(void* context);

void nlx_scene_technology_select_on_enter(void* context);
bool nlx_scene_technology_select_on_event(void* context, SceneManagerEvent event);
void nlx_scene_technology_select_on_exit(void* context);

void nlx_scene_saved_on_enter(void* context);
bool nlx_scene_saved_on_event(void* context, SceneManagerEvent event);
void nlx_scene_saved_on_exit(void* context);

void nlx_scene_diagnostics_on_enter(void* context);
bool nlx_scene_diagnostics_on_event(void* context, SceneManagerEvent event);
void nlx_scene_diagnostics_on_exit(void* context);

void nlx_scene_authorization_on_enter(void* context);
bool nlx_scene_authorization_on_event(void* context, SceneManagerEvent event);
void nlx_scene_authorization_on_exit(void* context);

void nlx_scene_authorized_lab_on_enter(void* context);
bool nlx_scene_authorized_lab_on_event(void* context, SceneManagerEvent event);
void nlx_scene_authorized_lab_on_exit(void* context);

void nlx_scene_hardware_required_on_enter(void* context);
bool nlx_scene_hardware_required_on_event(void* context, SceneManagerEvent event);
void nlx_scene_hardware_required_on_exit(void* context);

void nlx_scene_settings_on_enter(void* context);
bool nlx_scene_settings_on_event(void* context, SceneManagerEvent event);
void nlx_scene_settings_on_exit(void* context);

void nlx_scene_site_job_on_enter(void* context);
bool nlx_scene_site_job_on_event(void* context, SceneManagerEvent event);
void nlx_scene_site_job_on_exit(void* context);

void nlx_scene_scan_profile_on_enter(void* context);
bool nlx_scene_scan_profile_on_event(void* context, SceneManagerEvent event);
void nlx_scene_scan_profile_on_exit(void* context);

void nlx_scene_about_on_enter(void* context);
bool nlx_scene_about_on_event(void* context, SceneManagerEvent event);
void nlx_scene_about_on_exit(void* context);

void nlx_scene_message_on_enter(void* context);
bool nlx_scene_message_on_event(void* context, SceneManagerEvent event);
void nlx_scene_message_on_exit(void* context);
