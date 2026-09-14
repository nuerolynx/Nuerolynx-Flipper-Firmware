#include "nlx_report.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define NLX_SINGLE_REPORT_BUFFER_SIZE 3072U

typedef struct {
    char* output;
    size_t capacity;
    size_t used;
    bool valid;
} NlxReportBuilder;

static void nlx_builder_init(NlxReportBuilder* builder, char* output, size_t capacity) {
    builder->output = output;
    builder->capacity = capacity;
    builder->used = 0U;
    builder->valid = output && capacity > 0U;
    if(builder->valid) output[0] = '\0';
}

static void nlx_builder_append(NlxReportBuilder* builder, const char* format, ...) {
    if(!builder->valid) return;
    va_list arguments;
    va_start(arguments, format);
    const int written = vsnprintf(
        builder->output + builder->used, builder->capacity - builder->used, format, arguments);
    va_end(arguments);
    if(written < 0 || (size_t)written >= builder->capacity - builder->used) {
        builder->valid = false;
        return;
    }
    builder->used += (size_t)written;
}

static void nlx_builder_json_string(NlxReportBuilder* builder, const char* value) {
    nlx_builder_append(builder, "\"");
    const unsigned char* cursor = (const unsigned char*)(value ? value : "");
    while(builder->valid && *cursor) {
        switch(*cursor) {
        case '\"':
            nlx_builder_append(builder, "\\\"");
            break;
        case '\\':
            nlx_builder_append(builder, "\\\\");
            break;
        case '\n':
            nlx_builder_append(builder, "\\n");
            break;
        case '\r':
            nlx_builder_append(builder, "\\r");
            break;
        case '\t':
            nlx_builder_append(builder, "\\t");
            break;
        default:
            if(*cursor < 0x20U) {
                nlx_builder_append(builder, "\\u%04X", (unsigned int)*cursor);
            } else {
                nlx_builder_append(builder, "%c", *cursor);
            }
            break;
        }
        ++cursor;
    }
    nlx_builder_append(builder, "\"");
}

static size_t nlx_hex(const uint8_t* data, size_t length, char* output, size_t output_size) {
    if(!output || output_size == 0U) return 0U;
    size_t used = 0U;
    for(size_t i = 0; i < length && used + 2U < output_size; ++i) {
        const int written = snprintf(output + used, output_size - used, "%02X", data[i]);
        if(written < 0) break;
        used += (size_t)written;
    }
    output[used] = '\0';
    return used;
}

static void nlx_builder_json_key_string(
    NlxReportBuilder* builder,
    const char* key,
    const char* value,
    bool comma) {
    nlx_builder_json_string(builder, key);
    nlx_builder_append(builder, ":");
    nlx_builder_json_string(builder, value);
    if(comma) nlx_builder_append(builder, ",");
}

size_t nlx_report_json(const NlxCredentialResult* result, char* output, size_t output_size) {
    if(!result || !output || output_size == 0U) return 0U;
    char uid[(NLX_RESULT_UID_MAX * 2U) + 1U];
    char raw[(NLX_RESULT_RAW_MAX * 2U) + 1U];
    nlx_hex(result->uid, result->uid_length, uid, sizeof(uid));
    nlx_hex(result->raw_data, result->raw_data_length, raw, sizeof(raw));

    NlxReportBuilder builder;
    nlx_builder_init(&builder, output, output_size);
    nlx_builder_append(&builder, "{");
    nlx_builder_json_key_string(&builder, "schema", "nlx-credential-inspection/v1", true);
    nlx_builder_json_key_string(&builder, "timestamp", result->timestamp, true);
    nlx_builder_json_key_string(&builder, "site_job", result->site_job, true);
    nlx_builder_json_key_string(
        &builder, "technology", nlx_technology_name(result->technology), true);
    nlx_builder_append(&builder, "\"frequency_khz\":%" PRIu32 ",", result->frequency_khz);
    nlx_builder_json_key_string(&builder, "protocol", result->protocol, true);
    nlx_builder_json_key_string(&builder, "card_family", result->card_family, true);
    nlx_builder_json_key_string(&builder, "probable_format", result->probable_format, true);
    nlx_builder_append(&builder, "\"bit_length\":%u,", result->bit_length);
    nlx_builder_append(&builder, "\"facility_code\":");
    if(result->facility_code_present) {
        nlx_builder_append(&builder, "%" PRIu32, result->facility_code);
    } else {
        nlx_builder_append(&builder, "null");
    }
    nlx_builder_append(
        &builder,
        ",\"facility_code_present\":%s,\"facility_code_inferred\":%s,",
        result->facility_code_present ? "true" : "false",
        result->facility_code_inferred ? "true" : "false");
    nlx_builder_append(&builder, "\"credential_number\":");
    if(result->credential_number_present) {
        nlx_builder_append(&builder, "%" PRIu64, result->credential_number);
    } else {
        nlx_builder_append(&builder, "null");
    }
    nlx_builder_append(
        &builder,
        ",\"credential_number_present\":%s,\"credential_number_inferred\":%s,",
        result->credential_number_present ? "true" : "false",
        result->credential_number_inferred ? "true" : "false");
    nlx_builder_json_key_string(&builder, "uid", uid, true);
    nlx_builder_json_key_string(&builder, "manufacturer", result->manufacturer, true);
    nlx_builder_json_key_string(&builder, "parity", nlx_parity_name(result->parity), true);
    nlx_builder_json_key_string(&builder, "security", nlx_security_name(result->security), true);
    nlx_builder_json_key_string(
        &builder, "read_completeness", nlx_completeness_name(result->completeness), true);
    nlx_builder_json_key_string(
        &builder, "confidence", nlx_confidence_name(result->confidence), true);
    nlx_builder_json_key_string(&builder, "raw_data", raw, true);
    nlx_builder_json_key_string(&builder, "raw_file", result->raw_file, true);
    nlx_builder_json_key_string(&builder, "recommended_tool", result->recommended_tool, true);
    nlx_builder_json_key_string(&builder, "parser_version", result->parser_version, true);
    nlx_builder_append(&builder, "\"candidates\":[");
    for(size_t i = 0U; i < result->candidate_count; ++i) {
        const NlxCredentialCandidate* candidate = &result->candidates[i];
        if(i > 0U) nlx_builder_append(&builder, ",");
        nlx_builder_append(&builder, "{");
        nlx_builder_json_key_string(&builder, "format", candidate->format, true);
        nlx_builder_append(&builder, "\"facility_code\":");
        if(candidate->facility_code_present) {
            nlx_builder_append(&builder, "%" PRIu32, candidate->facility_code);
        } else {
            nlx_builder_append(&builder, "null");
        }
        nlx_builder_append(&builder, ",\"credential_number\":");
        if(candidate->credential_number_present) {
            nlx_builder_append(&builder, "%" PRIu64, candidate->credential_number);
        } else {
            nlx_builder_append(&builder, "null");
        }
        nlx_builder_append(&builder, ",\"parity\":");
        nlx_builder_json_string(&builder, nlx_parity_name(candidate->parity));
        nlx_builder_append(&builder, ",\"score\":%u}", candidate->score);
    }
    nlx_builder_append(&builder, "]}");
    return builder.valid ? builder.used : 0U;
}

size_t nlx_report_text(const NlxCredentialResult* result, char* output, size_t output_size) {
    if(!result || !output || output_size == 0U) return 0U;
    char uid[(NLX_RESULT_UID_MAX * 2U) + 1U];
    nlx_hex(result->uid, result->uid_length, uid, sizeof(uid));

    NlxReportBuilder builder;
    nlx_builder_init(&builder, output, output_size);
    nlx_builder_append(
        &builder,
        "NLX Credential Inspection\nTimestamp: %s\nSite/job: %s\nTechnology: %s\n",
        result->timestamp[0] ? result->timestamp : "not recorded",
        result->site_job[0] ? result->site_job : "not specified",
        nlx_technology_name(result->technology));
    if(result->frequency_khz > 0U) {
        nlx_builder_append(&builder, "Frequency: %" PRIu32 " kHz\n", result->frequency_khz);
    } else {
        nlx_builder_append(&builder, "Frequency: contact interface\n");
    }
    nlx_builder_append(
        &builder,
        "Protocol: %s\nCard family: %s\nProbable format: %s\nBit length: %u\n",
        result->protocol[0] ? result->protocol : "not determined",
        result->card_family[0] ? result->card_family : "not determined",
        result->probable_format[0] ? result->probable_format : "not determined",
        result->bit_length);
    if(result->facility_code_present) {
        nlx_builder_append(
            &builder,
            "Facility/site code: %s%" PRIu32 "\n",
            result->facility_code_inferred ? "inferred " : "",
            result->facility_code);
    } else {
        nlx_builder_append(&builder, "Facility/site code: not available\n");
    }
    if(result->credential_number_present) {
        nlx_builder_append(
            &builder,
            "Credential number: %s%" PRIu64 "\n",
            result->credential_number_inferred ? "inferred " : "",
            result->credential_number);
    } else {
        nlx_builder_append(&builder, "Credential number: not available\n");
    }
    nlx_builder_append(
        &builder,
        "UID/CSN/serial: %s\nManufacturer: %s\nParity: %s\nSecurity: %s\n"
        "Read completeness: %s\nConfidence: %s\nRaw file: %s\n"
        "Recommended NLX tool: %s\nParser: %s\n",
        uid[0] ? uid : "not read",
        result->manufacturer[0] ? result->manufacturer : "not available",
        nlx_parity_name(result->parity),
        nlx_security_name(result->security),
        nlx_completeness_name(result->completeness),
        nlx_confidence_name(result->confidence),
        result->raw_file[0] ? result->raw_file : "not captured",
        result->recommended_tool[0] ? result->recommended_tool : "none",
        result->parser_version);
    if(result->candidate_count > 1U) {
        nlx_builder_append(&builder, "Alternative interpretations:\n");
        for(size_t i = 0U; i < result->candidate_count; ++i) {
            const NlxCredentialCandidate* candidate = &result->candidates[i];
            nlx_builder_append(
                &builder,
                "  %u. %s; score %u; parity %s",
                (unsigned int)(i + 1U),
                candidate->format,
                candidate->score,
                nlx_parity_name(candidate->parity));
            if(candidate->facility_code_present) {
                nlx_builder_append(&builder, "; FC %" PRIu32, candidate->facility_code);
            }
            if(candidate->credential_number_present) {
                nlx_builder_append(
                    &builder, "; credential %" PRIu64, candidate->credential_number);
            }
            nlx_builder_append(&builder, "\n");
        }
    }
    return builder.valid ? builder.used : 0U;
}

size_t nlx_report_json_pair(
    const NlxCredentialResult* primary,
    const NlxCredentialResult* secondary,
    char* output,
    size_t output_size) {
    if(!primary || !secondary || !output || output_size == 0U) return 0U;
    char* primary_json = malloc(NLX_SINGLE_REPORT_BUFFER_SIZE);
    char* secondary_json = malloc(NLX_SINGLE_REPORT_BUFFER_SIZE);
    if(!primary_json || !secondary_json) {
        free(primary_json);
        free(secondary_json);
        return 0U;
    }

    const size_t primary_length =
        nlx_report_json(primary, primary_json, NLX_SINGLE_REPORT_BUFFER_SIZE);
    const size_t secondary_length =
        nlx_report_json(secondary, secondary_json, NLX_SINGLE_REPORT_BUFFER_SIZE);
    size_t used = 0U;
    if(primary_length > 0U && secondary_length > 0U) {
        const int written = snprintf(
            output,
            output_size,
            "{\"schema\":\"nlx-credential-inspection/v2\","
            "\"dual_technology\":true,\"components\":[%s,%s]}",
            primary_json,
            secondary_json);
        if(written > 0 && (size_t)written < output_size) used = (size_t)written;
    }
    free(primary_json);
    free(secondary_json);
    return used;
}

size_t nlx_report_text_pair(
    const NlxCredentialResult* primary,
    const NlxCredentialResult* secondary,
    char* output,
    size_t output_size) {
    if(!primary || !secondary || !output || output_size == 0U) return 0U;
    char* primary_text = malloc(NLX_SINGLE_REPORT_BUFFER_SIZE);
    char* secondary_text = malloc(NLX_SINGLE_REPORT_BUFFER_SIZE);
    if(!primary_text || !secondary_text) {
        free(primary_text);
        free(secondary_text);
        return 0U;
    }

    const size_t primary_length =
        nlx_report_text(primary, primary_text, NLX_SINGLE_REPORT_BUFFER_SIZE);
    const size_t secondary_length =
        nlx_report_text(secondary, secondary_text, NLX_SINGLE_REPORT_BUFFER_SIZE);
    size_t used = 0U;
    if(primary_length > 0U && secondary_length > 0U) {
        const int written = snprintf(
            output,
            output_size,
            "NLX Dual-Technology Credential\nComponents detected: 2\n\n"
            "--- Component 1 ---\n%s\n--- Component 2 ---\n%s",
            primary_text,
            secondary_text);
        if(written > 0 && (size_t)written < output_size) used = (size_t)written;
    }
    free(primary_text);
    free(secondary_text);
    return used;
}
