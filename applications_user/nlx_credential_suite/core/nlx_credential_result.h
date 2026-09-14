#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define NLX_RESULT_TEXT_SMALL    32U
#define NLX_RESULT_TEXT_MEDIUM   48U
#define NLX_RESULT_PATH_MAX      192U
#define NLX_RESULT_UID_MAX       16U
#define NLX_RESULT_RAW_MAX       32U
#define NLX_RESULT_CANDIDATE_MAX 3U

typedef enum {
    NlxTechnologyUnknown = 0,
    NlxTechnologyNfc,
    NlxTechnologyLfRfid,
    NlxTechnologyPicopass,
    NlxTechnologyIbutton,
} NlxTechnology;

typedef enum {
    NlxParityUnknown = 0,
    NlxParityValid,
    NlxParityInvalid,
    NlxParityNotApplicable,
} NlxParityStatus;

typedef enum {
    NlxSecurityUnknown = 0,
    NlxSecurityUnencrypted,
    NlxSecurityEncrypted,
    NlxSecurityAuthenticationRequired,
} NlxSecurityStatus;

typedef enum {
    NlxCompletenessNone = 0,
    NlxCompletenessClassified,
    NlxCompletenessIdentifier,
    NlxCompletenessFull,
} NlxReadCompleteness;

typedef enum {
    NlxConfidenceNone = 0,
    NlxConfidenceLow,
    NlxConfidenceMedium,
    NlxConfidenceHigh,
} NlxConfidence;

typedef struct {
    char format[NLX_RESULT_TEXT_MEDIUM];
    bool facility_code_present;
    uint32_t facility_code;
    bool credential_number_present;
    uint64_t credential_number;
    NlxParityStatus parity;
    uint8_t score;
} NlxCredentialCandidate;

typedef struct {
    char timestamp[NLX_RESULT_TEXT_SMALL];
    char site_job[NLX_RESULT_TEXT_MEDIUM];
    NlxTechnology technology;
    uint32_t frequency_khz;
    char protocol[NLX_RESULT_TEXT_MEDIUM];
    char card_family[NLX_RESULT_TEXT_MEDIUM];
    char probable_format[NLX_RESULT_TEXT_MEDIUM];
    uint16_t bit_length;
    bool facility_code_present;
    bool facility_code_inferred;
    uint32_t facility_code;
    bool credential_number_present;
    bool credential_number_inferred;
    uint64_t credential_number;
    uint8_t uid[NLX_RESULT_UID_MAX];
    size_t uid_length;
    char manufacturer[NLX_RESULT_TEXT_MEDIUM];
    NlxParityStatus parity;
    NlxSecurityStatus security;
    NlxReadCompleteness completeness;
    NlxConfidence confidence;
    uint8_t raw_data[NLX_RESULT_RAW_MAX];
    size_t raw_data_length;
    char raw_file[NLX_RESULT_PATH_MAX];
    char recommended_tool[NLX_RESULT_TEXT_MEDIUM];
    char parser_version[NLX_RESULT_TEXT_SMALL];
    NlxCredentialCandidate candidates[NLX_RESULT_CANDIDATE_MAX];
    size_t candidate_count;
} NlxCredentialResult;

void nlx_credential_result_reset(NlxCredentialResult* result);
const char* nlx_technology_name(NlxTechnology technology);
const char* nlx_parity_name(NlxParityStatus status);
const char* nlx_security_name(NlxSecurityStatus status);
const char* nlx_completeness_name(NlxReadCompleteness completeness);
const char* nlx_confidence_name(NlxConfidence confidence);
