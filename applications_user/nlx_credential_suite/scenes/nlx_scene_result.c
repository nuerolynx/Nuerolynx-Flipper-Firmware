#include "../nlx_credential_suite_i.h"

#include "../storage/nlx_inspection_store.h"

#include <inttypes.h>

static void nlx_result_append_hex(FuriString* text, const uint8_t* data, size_t length) {
    for(size_t i = 0U; i < length; ++i) {
        furi_string_cat_printf(text, "%02X", data[i]);
    }
}

static void nlx_result_append_component(
    FuriString* text,
    const NlxCredentialResult* result,
    uint8_t component_number,
    bool numbered) {
    if(numbered) {
        furi_string_cat_printf(
            text,
            "\n\e#%u: %s\e#\n%s",
            component_number,
            nlx_technology_name(result->technology),
            result->protocol[0] ? result->protocol : "unidentified protocol");
    } else {
        furi_string_printf(
            text,
            "\e#%s\e#\n%s",
            nlx_technology_name(result->technology),
            result->protocol[0] ? result->protocol : "unidentified protocol");
    }
    if(result->frequency_khz > 0U) {
        furi_string_cat_printf(text, " | %lu kHz", result->frequency_khz);
    } else {
        furi_string_cat(text, " | contact");
    }
    furi_string_cat_printf(
        text,
        "\n%s\nFmt: %s\nBits: %u\n",
        result->card_family[0] ? result->card_family : "family not determined",
        result->probable_format[0] ? result->probable_format : "not determined",
        result->bit_length);
    if(result->facility_code_present) {
        furi_string_cat_printf(
            text,
            "FC: %s%lu\n",
            result->facility_code_inferred ? "~" : "",
            result->facility_code);
    } else {
        furi_string_cat(text, "FC: not available\n");
    }
    if(result->credential_number_present) {
        furi_string_cat_printf(
            text,
            "Card: %s%" PRIu64 "\n",
            result->credential_number_inferred ? "~" : "",
            result->credential_number);
    } else {
        furi_string_cat(text, "Card: not available\n");
    }
    furi_string_cat(text, "UID/CSN: ");
    if(result->uid_length > 0U) {
        nlx_result_append_hex(text, result->uid, result->uid_length);
    } else {
        furi_string_cat(text, "not read");
    }
    furi_string_cat_printf(
        text,
        "\nMaker: %s\nParity: %s\nSecurity: %s\nRead: %s\nConfidence: %s\nTool: %s",
        result->manufacturer[0] ? result->manufacturer : "not available",
        nlx_parity_name(result->parity),
        nlx_security_name(result->security),
        nlx_completeness_name(result->completeness),
        nlx_confidence_name(result->confidence),
        result->recommended_tool[0] ? result->recommended_tool : "none");
    if(result->candidate_count > 1U) {
        furi_string_cat(text, "\nCandidates:");
        for(size_t i = 0U; i < result->candidate_count; ++i) {
            const NlxCredentialCandidate* candidate = &result->candidates[i];
            furi_string_cat_printf(
                text,
                "\n%u. %s (%u%%, %s)",
                (unsigned int)(i + 1U),
                candidate->format,
                candidate->score,
                nlx_parity_name(candidate->parity));
            if(candidate->facility_code_present) {
                furi_string_cat_printf(text, " FC %lu", candidate->facility_code);
            }
            if(candidate->credential_number_present) {
                furi_string_cat_printf(text, " Card %" PRIu64, candidate->credential_number);
            }
        }
    }
}

void nlx_scene_result_on_enter(void* context) {
    NlxCredentialSuiteApp* app = context;
    widget_reset(app->widget);
    if(app->secondary_result_present) {
        furi_string_set(app->text, "\e#Dual-tech credential\e#");
        nlx_result_append_component(app->text, &app->result, 1U, true);
        nlx_result_append_component(app->text, &app->secondary_result, 2U, true);
    } else {
        nlx_result_append_component(app->text, &app->result, 1U, false);
    }
    widget_add_text_scroll_element(app->widget, 0, 0, 128, 52, furi_string_get_cstr(app->text));
    widget_add_button_element(app->widget, GuiButtonTypeLeft, "Save", nlx_widget_callback, app);
    widget_add_button_element(
        app->widget, GuiButtonTypeCenter, "Emulate", nlx_widget_callback, app);
    widget_add_button_element(app->widget, GuiButtonTypeRight, "Write", nlx_widget_callback, app);
    view_dispatcher_switch_to_view(app->view_dispatcher, NlxViewWidget);
}

bool nlx_scene_result_on_event(void* context, SceneManagerEvent event) {
    NlxCredentialSuiteApp* app = context;
    if(event.type != SceneManagerEventTypeCustom) return false;
    if(event.event == GuiButtonTypeLeft) {
        const bool saved = nlx_inspection_store_save(
            app->storage,
            &app->result,
            app->secondary_result_present ? &app->secondary_result : NULL,
            app->saved_path,
            sizeof(app->saved_path));
        if(saved) {
            // Preserve the normalized inspection first, then hand off to the
            // correct native reader for the complete emulatable card capture.
            return nlx_app_launch_result_action(app, NlxResultActionSaveCapture);
        }
        nlx_app_show_message(app, "Save failed", "Check the SD card and try again.");
        return true;
    }
    if(event.event == GuiButtonTypeCenter) {
        return nlx_app_launch_result_action(app, NlxResultActionEmulate);
    }
    if(event.event == GuiButtonTypeRight) {
        return nlx_app_launch_result_action(app, NlxResultActionCopyWrite);
    }
    return false;
}

void nlx_scene_result_on_exit(void* context) {
    NlxCredentialSuiteApp* app = context;
    widget_reset(app->widget);
}
