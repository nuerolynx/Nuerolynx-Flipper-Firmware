#pragma once

#include "nlx_credential_result.h"

#define NLX_FORMAT_CANDIDATE_MAX 8U

typedef enum {
    NlxParitySchemeNone = 0,
    NlxParitySchemeLinearPair,
    NlxParitySchemeCorporate1000,
} NlxParityScheme;

typedef struct {
    const char* name;
    uint8_t bit_length;
    uint8_t facility_offset;
    uint8_t facility_length;
    uint8_t credential_offset;
    uint8_t credential_length;
    NlxParityScheme parity_scheme;
    uint8_t parity_a_bit;
    uint8_t parity_a_data_offset;
    uint8_t parity_a_data_length;
    bool parity_a_odd;
    uint8_t parity_b_bit;
    uint8_t parity_b_data_offset;
    uint8_t parity_b_data_length;
    bool parity_b_odd;
    uint8_t base_score;
} NlxFormatDefinition;

typedef struct {
    const NlxFormatDefinition* definition;
    uint32_t facility_code;
    uint64_t credential_number;
    NlxParityStatus parity;
    uint8_t score;
} NlxFormatCandidate;

typedef struct {
    NlxFormatCandidate candidates[NLX_FORMAT_CANDIDATE_MAX];
    size_t count;
    bool ambiguous;
} NlxDecodeResult;

size_t nlx_format_definition_count(void);
const NlxFormatDefinition* nlx_format_definition_at(size_t index);
bool nlx_parity_validate(const NlxFormatDefinition* format, uint64_t raw_value);
bool nlx_format_decode(uint64_t raw_value, uint8_t bit_length, NlxDecodeResult* result);
bool nlx_format_decode_hid_generic(
    const uint8_t* data,
    size_t data_size,
    uint8_t* bit_length,
    NlxDecodeResult* result);
void nlx_format_apply_best(const NlxDecodeResult* decoded, NlxCredentialResult* result);
