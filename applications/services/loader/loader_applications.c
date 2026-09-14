#include "loader.h"
#include "loader_applications.h"
#include <ctype.h>
#include <dialogs/dialogs.h>
#include <flipper_application/flipper_application.h>
#include <assets_icons.h>
#include <gui/gui.h>
#include <gui/view_holder.h>
#include <gui/modules/loading.h>
#include <gui/modules/submenu.h>
#include <gui/modules/text_input.h>
#include <dolphin/dolphin.h>
#include <lib/toolbox/dir_walk.h>
#include <lib/toolbox/path.h>

#define TAG "LoaderApplications"

#ifdef JS_RUNNER_FAP
#define JS_RUNNER_APP EXT_PATH("apps/assets/js_app.fap")
#else
#define JS_RUNNER_APP "JS Runner"
#endif

struct LoaderApplications {
    FuriThread* thread;
    void (*closed_cb)(void*);
    void* context;
    bool search_mode;
};

static int32_t loader_applications_thread(void* p);

LoaderApplications*
    loader_applications_alloc(void (*closed_cb)(void*), void* context, bool search_mode) {
    LoaderApplications* loader_applications = malloc(sizeof(LoaderApplications));
    loader_applications->thread =
        furi_thread_alloc_ex(TAG, 2048, loader_applications_thread, (void*)loader_applications);
    loader_applications->closed_cb = closed_cb;
    loader_applications->context = context;
    loader_applications->search_mode = search_mode;
    furi_thread_start(loader_applications->thread);
    return loader_applications;
}

void loader_applications_free(LoaderApplications* loader_applications) {
    furi_assert(loader_applications);
    furi_thread_join(loader_applications->thread);
    furi_thread_free(loader_applications->thread);
    free(loader_applications);
}

typedef struct {
    FuriString* file_path;
    DialogsApp* dialogs;
    Storage* storage;
    Loader* loader;

    Gui* gui;
    ViewHolder* view_holder;
    Loading* loading;

    FuriThreadId thread_id;
    TextInput* search_input;
    Submenu* search_results;
    char search_query[33];
    uint32_t selected_result;
    size_t result_count;
    FuriString* result_paths[20];
    FuriString* result_names[20];
    uint16_t result_scores[20];
} LoaderApplicationsApp;

static LoaderApplicationsApp* loader_applications_app_alloc(bool search_mode) {
    LoaderApplicationsApp* app = malloc(sizeof(LoaderApplicationsApp)); //-V799
    app->file_path = furi_string_alloc_set(EXT_PATH("apps"));
    app->dialogs = furi_record_open(RECORD_DIALOGS);
    app->storage = furi_record_open(RECORD_STORAGE);
    app->loader = furi_record_open(RECORD_LOADER);

    app->gui = furi_record_open(RECORD_GUI);
    app->view_holder = view_holder_alloc();
    app->loading = loading_alloc();
    app->thread_id = furi_thread_get_current_id();
    app->search_input = NULL;
    app->search_results = NULL;
    app->search_query[0] = '\0';
    app->selected_result = 0;
    app->result_count = 0;

    if(search_mode) {
        app->search_input = text_input_alloc();
        app->search_results = submenu_alloc();
        for(size_t i = 0; i < COUNT_OF(app->result_paths); i++) {
            app->result_paths[i] = furi_string_alloc();
            app->result_names[i] = furi_string_alloc();
            app->result_scores[i] = 0;
        }
    }

    view_holder_attach_to_gui(app->view_holder, app->gui);

    return app;
} //-V773

static void loader_applications_app_free(LoaderApplicationsApp* app) {
    furi_assert(app);

    if(app->search_input) {
        text_input_free(app->search_input);
        submenu_free(app->search_results);
        for(size_t i = 0; i < COUNT_OF(app->result_paths); i++) {
            furi_string_free(app->result_paths[i]);
            furi_string_free(app->result_names[i]);
        }
    }

    view_holder_free(app->view_holder);
    loading_free(app->loading);
    furi_record_close(RECORD_GUI);

    furi_record_close(RECORD_LOADER);
    furi_record_close(RECORD_DIALOGS);
    furi_record_close(RECORD_STORAGE);
    furi_string_free(app->file_path);
    free(app);
}

static bool loader_applications_item_callback(
    FuriString* path,
    void* context,
    uint8_t** icon_ptr,
    FuriString* item_name) {
    LoaderApplicationsApp* loader_applications_app = context;
    furi_assert(loader_applications_app);
    if(furi_string_end_with(path, ".fap")) {
        return flipper_application_load_name_and_icon(
            path, loader_applications_app->storage, icon_ptr, item_name);
    } else {
        path_extract_filename(path, item_name, false);
        memcpy(*icon_ptr, icon_get_frame_data(&I_js_script_10px, 0), FAP_MANIFEST_MAX_ICON_SIZE);
        return true;
    }
}

static bool loader_applications_select_app(LoaderApplicationsApp* loader_applications_app) {
    const DialogsFileBrowserOptions browser_options = {
        .extension = ".fap|.js",
        .skip_assets = true,
        .icon = &I_unknown_10px,
        .hide_ext = true,
        .item_loader_callback = loader_applications_item_callback,
        .item_loader_context = loader_applications_app,
        .base_path = EXT_PATH("apps"),
    };

    return dialog_file_browser_show(
        loader_applications_app->dialogs,
        loader_applications_app->file_path,
        loader_applications_app->file_path,
        &browser_options);
}

#define APPLICATION_STOP_EVENT 1
#define SEARCH_INPUT_EVENT     (1U << 1)
#define SEARCH_SELECT_EVENT    (1U << 2)
#define SEARCH_BACK_EVENT      (1U << 3)
#define SEARCH_EVENT_MASK      (SEARCH_INPUT_EVENT | SEARCH_SELECT_EVENT | SEARCH_BACK_EVENT)
#define SEARCH_NO_RESULT       UINT32_MAX

static void loader_pubsub_callback(const void* message, void* context) {
    const LoaderEvent* event = message;
    const FuriThreadId thread_id = (FuriThreadId)context;

    if(event->type == LoaderEventTypeNoMoreAppsInQueue) {
        furi_thread_flags_set(thread_id, APPLICATION_STOP_EVENT);
    }
}

static void
    loader_applications_start_app(LoaderApplicationsApp* app, const char* name, const char* args) {
    if(!furi_string_start_with_str(app->file_path, EXT_PATH("apps/Games/")) &&
       !furi_string_start_with_str(app->file_path, EXT_PATH("apps/Media/"))) {
        dolphin_deed(DolphinDeedPluginInternalStart);
    }

    // load app
    FuriThreadId thread_id = furi_thread_get_current_id();
    FuriPubSubSubscription* subscription =
        furi_pubsub_subscribe(loader_get_pubsub(app->loader), loader_pubsub_callback, thread_id);

    LoaderStatus status = loader_start_with_gui_error(app->loader, name, args);

    if(status == LoaderStatusOk) {
        furi_thread_flags_wait(APPLICATION_STOP_EVENT, FuriFlagWaitAny, FuriWaitForever);
    }

    furi_pubsub_unsubscribe(loader_get_pubsub(app->loader), subscription);
    furi_thread_flags_clear(APPLICATION_STOP_EVENT);
}

typedef struct {
    const char* filename;
    const char* keywords;
} LoaderSearchKeywords;

static const LoaderSearchKeywords loader_search_keywords[] = {
    {"nlx_credential_suite.fap",
     "credential badge card access universal scan dual technology hf lf nfc rfid picopass "
     "iclass mifare facility code save emulate copy write physical security"},
    {"voltcalc_app.fap",
     "voltage drop calculator ohms law resistance current wire gauge awg electrical cable "
     "wore guage"},
    {"gpio_logic_analyzer.fap",
     "logic analyzer digital signal capture gpio protocol diagnostic"},
    {"smart_meter_monitor.fap", "smart utility energy electric meter monitor subghz"},
    {"nfc_rfid_detector.fap", "field detector frequency identify reader nfc rfid hf lf"},
    {"picopass.fap", "picopass iclass hid access credential card"},
    {"mfkey.fap", "mfkey mfkey32 mifare classic key recovery nonce"},
    {"nfc_magic.fap", "nfc magic card uid clone copy write gen1 gen2 gen4"},
    {"mfc_editor.fap", "mifare classic editor sector block card"},
    {"seader.fap", "seader cedar iclass se seos sam credential"},
    {"seos.fap", "seos hid credential access card"},
    {"blackhat.fap", "wifi wireless audit pentest security esp32 blackhat"},
    {"esp32_wifi_marauder.fap", "wifi wireless audit pentest security marauder esp32"},
    {"evil_portal.fap", "wifi wireless portal audit pentest security"},
    {"sub_analyzer.fap", "subghz radio signal analyzer frequency capture"},
    {"spectrum_analyzer.fap", "spectrum radio signal analyzer frequency subghz"},
    {"flipperscope.fap", "oscilloscope scope analog signal gpio diagnostic"},
    {"i2ctools.fap", "i2c bus scan sensor gpio diagnostic"},
    {"wire_tester.fap", "wire cable continuity tester gpio field diagnostic"},
    {"bad_kb.fap", "badusb bad usb keyboard hid script automation"},
    {"ir_remote.fap", "infrared ir universal remote control"},
};

static const char* loader_applications_search_get_keywords(const char* path) {
    const char* filename = strrchr(path, '/');
    filename = filename ? filename + 1 : path;
    for(size_t i = 0; i < COUNT_OF(loader_search_keywords); i++) {
        if(strcmp(filename, loader_search_keywords[i].filename) == 0) {
            return loader_search_keywords[i].keywords;
        }
    }
    return "";
}

static bool loader_applications_search_word_starts_with(
    const char* text,
    const char* word) {
    const size_t word_length = strlen(word);
    for(const char* match = strcasestr(text, word); match; match = strcasestr(match + 1, word)) {
        if(match == text || !isalnum((unsigned char)match[-1])) {
            if(strncasecmp(match, word, word_length) == 0) return true;
        }
    }
    return false;
}

static uint16_t loader_applications_search_score(
    const char* query,
    const char* app_name,
    const char* path) {
    char tokens[6][17] = {0};
    size_t token_count = 0;
    const char* cursor = query;

    while(*cursor && token_count < COUNT_OF(tokens)) {
        while(*cursor == ' ') cursor++;
        if(!*cursor) break;
        size_t length = 0;
        while(cursor[length] && cursor[length] != ' ' && length < sizeof(tokens[0]) - 1) {
            tokens[token_count][length] = cursor[length];
            length++;
        }
        tokens[token_count][length] = '\0';
        token_count++;
        while(cursor[length] && cursor[length] != ' ') length++;
        cursor += length;
    }

    if(token_count == 0) return 0;

    const char* keywords = loader_applications_search_get_keywords(path);
    uint16_t score = 0;
    if(strcasecmp(query, app_name) == 0) {
        score += 1000;
    } else if(strncasecmp(app_name, query, strlen(query)) == 0) {
        score += 350;
    } else if(strcasestr(app_name, query)) {
        score += 250;
    }

    for(size_t i = 0; i < token_count; i++) {
        const char* token = tokens[i];
        bool matched = false;
        if(loader_applications_search_word_starts_with(app_name, token)) {
            score += 140;
            matched = true;
        } else if(strcasestr(app_name, token)) {
            score += 110;
            matched = true;
        }
        if(strcasestr(path, token)) {
            score += 60;
            matched = true;
        }
        if(strcasestr(keywords, token)) {
            score += 80;
            matched = true;
        }
        if(!matched) return 0;
    }

    return score;
}

static void loader_applications_search_insert(
    LoaderApplicationsApp* app,
    const char* path,
    const char* name,
    uint16_t score) {
    size_t insert_at = 0;
    while(insert_at < app->result_count && app->result_scores[insert_at] >= score) {
        insert_at++;
    }
    if(insert_at >= COUNT_OF(app->result_paths)) return;

    size_t new_count = app->result_count;
    if(new_count < COUNT_OF(app->result_paths)) new_count++;
    for(size_t i = new_count - 1; i > insert_at; i--) {
        app->result_scores[i] = app->result_scores[i - 1];
        furi_string_set(app->result_paths[i], app->result_paths[i - 1]);
        furi_string_set(app->result_names[i], app->result_names[i - 1]);
    }
    app->result_scores[insert_at] = score;
    furi_string_set_str(app->result_paths[insert_at], path);
    furi_string_set_str(app->result_names[insert_at], name);
    app->result_count = new_count;
}

static void loader_applications_search_build(LoaderApplicationsApp* app) {
    app->result_count = 0;
    FuriString* path = furi_string_alloc();
    FuriString* name = furi_string_alloc();
    FileInfo file_info;
    DirWalk* dir_walk = dir_walk_alloc(app->storage);

    if(dir_walk_open(dir_walk, EXT_PATH("apps"))) {
        while(dir_walk_read(dir_walk, path, &file_info) == DirWalkOK) {
            if(file_info_is_dir(&file_info) ||
               furi_string_start_with_str(path, EXT_PATH("apps/assets/"))) {
                continue;
            }

            const bool is_fap = furi_string_end_with(path, ".fap");
            const bool is_js = furi_string_end_with(path, ".js");
            if(!is_fap && !is_js) continue;

            if(is_fap) {
                uint8_t icon_data[FAP_MANIFEST_MAX_ICON_SIZE];
                uint8_t* icon = icon_data;
                flipper_application_load_name_and_icon(path, app->storage, &icon, name);
            } else {
                path_extract_filename(path, name, true);
            }

            const uint16_t score = loader_applications_search_score(
                app->search_query, furi_string_get_cstr(name), furi_string_get_cstr(path));
            if(score) {
                loader_applications_search_insert(
                    app,
                    furi_string_get_cstr(path),
                    furi_string_get_cstr(name),
                    score);
            }
        }
        dir_walk_close(dir_walk);
    }

    dir_walk_free(dir_walk);
    furi_string_free(name);
    furi_string_free(path);
}

static void loader_applications_search_input_callback(void* context) {
    LoaderApplicationsApp* app = context;
    furi_thread_flags_set(app->thread_id, SEARCH_INPUT_EVENT);
}

static void loader_applications_search_select_callback(void* context, uint32_t index) {
    LoaderApplicationsApp* app = context;
    app->selected_result = index;
    furi_thread_flags_set(app->thread_id, SEARCH_SELECT_EVENT);
}

static void loader_applications_search_back_callback(void* context) {
    LoaderApplicationsApp* app = context;
    furi_thread_flags_set(app->thread_id, SEARCH_BACK_EVENT);
}

static bool loader_applications_search_prompt(LoaderApplicationsApp* app) {
    text_input_reset(app->search_input);
    text_input_set_header_text(app->search_input, "Find app or tool:");
    text_input_set_minimum_length(app->search_input, 1);
    text_input_set_result_callback(
        app->search_input,
        loader_applications_search_input_callback,
        app,
        app->search_query,
        sizeof(app->search_query),
        false);

    furi_thread_flags_clear(SEARCH_EVENT_MASK);
    view_holder_set_back_callback(
        app->view_holder, loader_applications_search_back_callback, app);
    view_holder_set_view(app->view_holder, text_input_get_view(app->search_input));
    const uint32_t event =
        furi_thread_flags_wait(SEARCH_INPUT_EVENT | SEARCH_BACK_EVENT, FuriFlagWaitAny, FuriWaitForever);
    return (event & SEARCH_INPUT_EVENT) != 0;
}

static bool loader_applications_search_choose(LoaderApplicationsApp* app) {
    submenu_reset(app->search_results);
    submenu_set_header(app->search_results, "Best matches");

    FuriString* label = furi_string_alloc();
    if(app->result_count == 0) {
        submenu_add_item(
            app->search_results,
            "No matches - search again",
            SEARCH_NO_RESULT,
            loader_applications_search_select_callback,
            app);
    } else {
        for(size_t i = 0; i < app->result_count; i++) {
            const char* path = furi_string_get_cstr(app->result_paths[i]);
            const char* category = strstr(path, "/apps/");
            category = category ? category + strlen("/apps/") : path;
            const char* category_end = strchr(category, '/');
            if(category_end) {
                furi_string_printf(
                    label,
                    "%s [%.*s]",
                    furi_string_get_cstr(app->result_names[i]),
                    (int)(category_end - category),
                    category);
            } else {
                furi_string_set(label, app->result_names[i]);
            }
            submenu_add_item(
                app->search_results,
                furi_string_get_cstr(label),
                i,
                loader_applications_search_select_callback,
                app);
        }
    }
    furi_string_free(label);

    app->selected_result = SEARCH_NO_RESULT;
    furi_thread_flags_clear(SEARCH_EVENT_MASK);
    view_holder_set_view(app->view_holder, submenu_get_view(app->search_results));
    const uint32_t event = furi_thread_flags_wait(
        SEARCH_SELECT_EVENT | SEARCH_BACK_EVENT, FuriFlagWaitAny, FuriWaitForever);
    return (event & SEARCH_SELECT_EVENT) != 0;
}

static void loader_applications_search(LoaderApplicationsApp* app) {
    while(loader_applications_search_prompt(app)) {
        view_holder_set_back_callback(app->view_holder, NULL, NULL);
        view_holder_set_view(app->view_holder, loading_get_view(app->loading));
        loader_applications_search_build(app);

        if(!loader_applications_search_choose(app)) continue;
        if(app->selected_result == SEARCH_NO_RESULT ||
           app->selected_result >= app->result_count) {
            continue;
        }

        furi_string_set(app->file_path, app->result_paths[app->selected_result]);
        view_holder_set_back_callback(app->view_holder, NULL, NULL);
        view_holder_set_view(app->view_holder, loading_get_view(app->loading));
        if(furi_string_end_with(app->file_path, ".js")) {
            loader_applications_start_app(
                app, JS_RUNNER_APP, furi_string_get_cstr(app->file_path));
        } else {
            loader_applications_start_app(app, furi_string_get_cstr(app->file_path), NULL);
        }
    }
}

static int32_t loader_applications_thread(void* p) {
    LoaderApplications* loader_applications = p;
    LoaderApplicationsApp* app =
        loader_applications_app_alloc(loader_applications->search_mode);

    // start loading animation
    view_holder_set_view(app->view_holder, loading_get_view(app->loading));

    if(loader_applications->search_mode) {
        loader_applications_search(app);
    } else {
        while(loader_applications_select_app(app)) {
            if(!furi_string_end_with(app->file_path, ".js")) {
                loader_applications_start_app(app, furi_string_get_cstr(app->file_path), NULL);
            } else {
                loader_applications_start_app(
                    app, JS_RUNNER_APP, furi_string_get_cstr(app->file_path));
            }
        }
    }

    // stop loading animation
    view_holder_set_back_callback(app->view_holder, NULL, NULL);
    view_holder_set_view(app->view_holder, NULL);

    loader_applications_app_free(app);

    if(loader_applications->closed_cb) {
        loader_applications->closed_cb(loader_applications->context);
    }

    return 0;
}
