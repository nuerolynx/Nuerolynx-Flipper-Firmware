#include "nlx_inspection_store.h"

#include "../core/nlx_report.h"

#include <furi_hal_rtc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NLX_DATA_DIR           "/ext/apps_data/nlx_credential_suite"
#define NLX_INSPECTION_DIR     NLX_DATA_DIR "/inspections"
#define NLX_REPORT_BUFFER_SIZE 7168U

static bool
    nlx_write_file_atomic(Storage* storage, const char* path, const void* data, size_t length) {
    char temporary_path[NLX_RESULT_PATH_MAX + 8U];
    snprintf(temporary_path, sizeof(temporary_path), "%s.tmp", path);
    storage_common_remove(storage, temporary_path);

    File* file = storage_file_alloc(storage);
    bool success = false;
    if(storage_file_open(file, temporary_path, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        success = storage_file_write(file, data, length) == length;
        success = storage_file_sync(file) && success;
        storage_file_close(file);
    }
    storage_file_free(file);
    if(!success) {
        storage_common_remove(storage, temporary_path);
        return false;
    }

    if(storage_common_rename(storage, temporary_path, path) != FSE_OK) {
        storage_common_remove(storage, temporary_path);
        return false;
    }
    return true;
}

bool nlx_inspection_store_save(
    Storage* storage,
    const NlxCredentialResult* result,
    const NlxCredentialResult* secondary_result,
    char* saved_base_path,
    size_t saved_base_path_size) {
    if(!storage || !result || !saved_base_path || saved_base_path_size == 0U) return false;
    const FS_Error data_dir = storage_common_mkdir(storage, NLX_DATA_DIR);
    if(data_dir != FSE_OK && data_dir != FSE_EXIST) return false;
    const FS_Error inspection_dir = storage_common_mkdir(storage, NLX_INSPECTION_DIR);
    if(inspection_dir != FSE_OK && inspection_dir != FSE_EXIST) return false;

    DateTime now;
    furi_hal_rtc_get_datetime(&now);
    char timestamp_stem[32];
    snprintf(
        timestamp_stem,
        sizeof(timestamp_stem),
        "%04u%02u%02u-%02u%02u%02u",
        now.year,
        now.month,
        now.day,
        now.hour,
        now.minute,
        now.second);

    char json_path[NLX_RESULT_PATH_MAX];
    for(uint8_t suffix = 0U; suffix < 100U; ++suffix) {
        if(suffix == 0U) {
            snprintf(
                saved_base_path, saved_base_path_size, NLX_INSPECTION_DIR "/%s", timestamp_stem);
        } else {
            snprintf(
                saved_base_path,
                saved_base_path_size,
                NLX_INSPECTION_DIR "/%s-%02u",
                timestamp_stem,
                suffix);
        }
        snprintf(json_path, sizeof(json_path), "%s.json", saved_base_path);
        if(!storage_file_exists(storage, json_path)) break;
        if(suffix == 99U) return false;
    }

    char text_path[NLX_RESULT_PATH_MAX];
    char raw_path[NLX_RESULT_PATH_MAX];
    char secondary_raw_path[NLX_RESULT_PATH_MAX];
    snprintf(text_path, sizeof(text_path), "%s.txt", saved_base_path);
    snprintf(raw_path, sizeof(raw_path), "%s.raw", saved_base_path);
    snprintf(secondary_raw_path, sizeof(secondary_raw_path), "%s-secondary.raw", saved_base_path);

    NlxCredentialResult report_result = *result;
    NlxCredentialResult secondary_report_result;
    if(secondary_result) secondary_report_result = *secondary_result;
    snprintf(
        report_result.timestamp,
        sizeof(report_result.timestamp),
        "%04u-%02u-%02uT%02u:%02u:%02u",
        now.year,
        now.month,
        now.day,
        now.hour,
        now.minute,
        now.second);
    if(report_result.raw_data_length > 0U) {
        snprintf(report_result.raw_file, sizeof(report_result.raw_file), "%s", raw_path);
    } else {
        report_result.raw_file[0] = '\0';
    }
    if(secondary_result) {
        snprintf(
            secondary_report_result.timestamp,
            sizeof(secondary_report_result.timestamp),
            "%s",
            report_result.timestamp);
        if(secondary_report_result.raw_data_length > 0U) {
            snprintf(
                secondary_report_result.raw_file,
                sizeof(secondary_report_result.raw_file),
                "%s",
                secondary_raw_path);
        } else {
            secondary_report_result.raw_file[0] = '\0';
        }
    }

    char* report_buffer = malloc(NLX_REPORT_BUFFER_SIZE);
    if(!report_buffer) return false;

    if(report_result.raw_data_length > 0U &&
       !nlx_write_file_atomic(
           storage, raw_path, report_result.raw_data, report_result.raw_data_length)) {
        free(report_buffer);
        return false;
    }
    if(secondary_result && secondary_report_result.raw_data_length > 0U &&
       !nlx_write_file_atomic(
           storage,
           secondary_raw_path,
           secondary_report_result.raw_data,
           secondary_report_result.raw_data_length)) {
        if(report_result.raw_data_length > 0U) storage_common_remove(storage, raw_path);
        free(report_buffer);
        return false;
    }
    const size_t json_length = secondary_result ?
                                   nlx_report_json_pair(
                                       &report_result,
                                       &secondary_report_result,
                                       report_buffer,
                                       NLX_REPORT_BUFFER_SIZE) :
                                   nlx_report_json(
                                       &report_result, report_buffer, NLX_REPORT_BUFFER_SIZE);
    if(json_length == 0U ||
       !nlx_write_file_atomic(storage, json_path, report_buffer, json_length)) {
        if(report_result.raw_data_length > 0U) storage_common_remove(storage, raw_path);
        if(secondary_result && secondary_report_result.raw_data_length > 0U) {
            storage_common_remove(storage, secondary_raw_path);
        }
        free(report_buffer);
        return false;
    }
    const size_t text_length = secondary_result ?
                                   nlx_report_text_pair(
                                       &report_result,
                                       &secondary_report_result,
                                       report_buffer,
                                       NLX_REPORT_BUFFER_SIZE) :
                                   nlx_report_text(
                                       &report_result, report_buffer, NLX_REPORT_BUFFER_SIZE);
    if(text_length == 0U ||
       !nlx_write_file_atomic(storage, text_path, report_buffer, text_length)) {
        storage_common_remove(storage, json_path);
        if(report_result.raw_data_length > 0U) storage_common_remove(storage, raw_path);
        if(secondary_result && secondary_report_result.raw_data_length > 0U) {
            storage_common_remove(storage, secondary_raw_path);
        }
        free(report_buffer);
        return false;
    }
    free(report_buffer);
    return true;
}
