#include "nlx_format_decoder.h"

#include <string.h>

/*
 * Positions are numbered from the most-significant transmitted bit at zero.
 * Parity definitions describe a parity bit plus the data range it protects.
 * Formats without a verified parity definition deliberately score lower.
 */
static const NlxFormatDefinition nlx_formats[] = {
    {.name = "HID H10301 26-bit",
     .bit_length = 26,
     .facility_offset = 1,
     .facility_length = 8,
     .credential_offset = 9,
     .credential_length = 16,
     .parity_scheme = NlxParitySchemeLinearPair,
     .parity_a_bit = 0,
     .parity_a_data_offset = 1,
     .parity_a_data_length = 12,
     .parity_a_odd = false,
     .parity_b_bit = 25,
     .parity_b_data_offset = 13,
     .parity_b_data_length = 12,
     .parity_b_odd = true,
     .base_score = 70},
    {.name = "Generic 32-bit 8/24",
     .bit_length = 32,
     .facility_offset = 0,
     .facility_length = 8,
     .credential_offset = 8,
     .credential_length = 24,
     .base_score = 35},
    {.name = "Generic 32-bit 12/20",
     .bit_length = 32,
     .facility_offset = 0,
     .facility_length = 12,
     .credential_offset = 12,
     .credential_length = 20,
     .base_score = 35},
    {.name = "HID H10306 34-bit",
     .bit_length = 34,
     .facility_offset = 1,
     .facility_length = 16,
     .credential_offset = 17,
     .credential_length = 16,
     .parity_scheme = NlxParitySchemeLinearPair,
     .parity_a_bit = 0,
     .parity_a_data_offset = 1,
     .parity_a_data_length = 16,
     .parity_a_odd = false,
     .parity_b_bit = 33,
     .parity_b_data_offset = 17,
     .parity_b_data_length = 16,
     .parity_b_odd = true,
     .base_score = 65},
    {.name = "Corporate 1000 35-bit",
     .bit_length = 35,
     .facility_offset = 2,
     .facility_length = 12,
     .credential_offset = 14,
     .credential_length = 20,
     .parity_scheme = NlxParitySchemeCorporate1000,
     .base_score = 67},
    {.name = "Generic 36-bit 18/16",
     .bit_length = 36,
     .facility_offset = 1,
     .facility_length = 18,
     .credential_offset = 19,
     .credential_length = 16,
     .base_score = 35},
    {.name = "HID H10302 37-bit",
     .bit_length = 37,
     .facility_offset = 0,
     .facility_length = 0,
     .credential_offset = 1,
     .credential_length = 35,
     .parity_scheme = NlxParitySchemeLinearPair,
     .parity_a_bit = 0,
     .parity_a_data_offset = 1,
     .parity_a_data_length = 18,
     .parity_a_odd = false,
     .parity_b_bit = 36,
     .parity_b_data_offset = 18,
     .parity_b_data_length = 18,
     .parity_b_odd = true,
     .base_score = 55},
    {.name = "HID H10304 37-bit",
     .bit_length = 37,
     .facility_offset = 1,
     .facility_length = 16,
     .credential_offset = 17,
     .credential_length = 19,
     .parity_scheme = NlxParitySchemeLinearPair,
     .parity_a_bit = 0,
     .parity_a_data_offset = 1,
     .parity_a_data_length = 18,
     .parity_a_odd = false,
     .parity_b_bit = 36,
     .parity_b_data_offset = 18,
     .parity_b_data_length = 18,
     .parity_b_odd = true,
     .base_score = 60},
    {.name = "Generic 40-bit 8/32",
     .bit_length = 40,
     .facility_offset = 0,
     .facility_length = 8,
     .credential_offset = 8,
     .credential_length = 32,
     .base_score = 35},
    {.name = "Generic 40-bit 16/24",
     .bit_length = 40,
     .facility_offset = 0,
     .facility_length = 16,
     .credential_offset = 16,
     .credential_length = 24,
     .base_score = 35},
};

static uint8_t nlx_bit_at(uint64_t raw_value, uint8_t bit_length, uint8_t from_left) {
    if((bit_length == 0U) || (from_left >= bit_length)) return 0U;
    return (uint8_t)((raw_value >> (bit_length - 1U - from_left)) & 1ULL);
}

static uint64_t
    nlx_extract(uint64_t raw_value, uint8_t bit_length, uint8_t offset, uint8_t length) {
    if((length == 0U) || (length > 63U) || (offset + length > bit_length)) return 0ULL;
    const uint8_t shift = bit_length - offset - length;
    const uint64_t mask = (1ULL << length) - 1ULL;
    return (raw_value >> shift) & mask;
}

static bool nlx_check_parity_rule(
    uint64_t raw_value,
    uint8_t bit_length,
    uint8_t parity_bit,
    uint8_t data_offset,
    uint8_t data_length,
    bool odd) {
    uint8_t ones = nlx_bit_at(raw_value, bit_length, parity_bit);
    for(uint8_t i = 0; i < data_length; ++i) {
        ones = (uint8_t)(ones + nlx_bit_at(raw_value, bit_length, data_offset + i));
    }
    return ((ones & 1U) != 0U) == odd;
}

size_t nlx_format_definition_count(void) {
    return sizeof(nlx_formats) / sizeof(nlx_formats[0]);
}

const NlxFormatDefinition* nlx_format_definition_at(size_t index) {
    return index < nlx_format_definition_count() ? &nlx_formats[index] : NULL;
}

bool nlx_parity_validate(const NlxFormatDefinition* format, uint64_t raw_value) {
    if(!format || format->parity_scheme == NlxParitySchemeNone) return false;
    if(format->parity_scheme == NlxParitySchemeLinearPair) {
        return nlx_check_parity_rule(
                   raw_value,
                   format->bit_length,
                   format->parity_a_bit,
                   format->parity_a_data_offset,
                   format->parity_a_data_length,
                   format->parity_a_odd) &&
               nlx_check_parity_rule(
                   raw_value,
                   format->bit_length,
                   format->parity_b_bit,
                   format->parity_b_data_offset,
                   format->parity_b_data_length,
                   format->parity_b_odd);
    }

    /* Corporate 1000 uses three interleaved parity masks. This is the same
     * public layout used by the independently maintained Picopass/Seader tools. */
    const uint32_t mid = (uint32_t)(raw_value >> 32U);
    const uint32_t bot = (uint32_t)raw_value;
    uint32_t parity_a = (mid & 0x1U) ^ (bot & 0xB6DB6DB6U);
    uint32_t parity_b = (mid & 0x3U) ^ (bot & 0x6DB6DB6CU);
    uint32_t parity_c = (mid & 0x3U) ^ bot;
    uint8_t ones_a = 0U;
    uint8_t ones_b = 0U;
    uint8_t ones_c = 0U;
    while(parity_a) {
        ones_a ^= (uint8_t)(parity_a & 1U);
        parity_a >>= 1U;
    }
    while(parity_b) {
        ones_b ^= (uint8_t)(parity_b & 1U);
        parity_b >>= 1U;
    }
    while(parity_c) {
        ones_c ^= (uint8_t)(parity_c & 1U);
        parity_c >>= 1U;
    }
    const uint8_t expected_even = ones_a;
    const uint8_t expected_odd_b = (uint8_t)(ones_b ^ 1U);
    const uint8_t expected_odd_c = (uint8_t)(ones_c ^ 1U);
    return ((mid >> 1U) & 1U) == expected_even && (bot & 1U) == expected_odd_b &&
           ((mid >> 2U) & 1U) == expected_odd_c;
}

static void nlx_sort_candidates(NlxDecodeResult* result) {
    for(size_t i = 1; i < result->count; ++i) {
        NlxFormatCandidate candidate = result->candidates[i];
        size_t j = i;
        while((j > 0U) && (result->candidates[j - 1U].score < candidate.score)) {
            result->candidates[j] = result->candidates[j - 1U];
            --j;
        }
        result->candidates[j] = candidate;
    }
}

bool nlx_format_decode(uint64_t raw_value, uint8_t bit_length, NlxDecodeResult* result) {
    if(!result) return false;
    memset(result, 0, sizeof(*result));

    for(size_t i = 0; i < nlx_format_definition_count(); ++i) {
        const NlxFormatDefinition* format = &nlx_formats[i];
        if(format->bit_length != bit_length) continue;
        if(result->count >= NLX_FORMAT_CANDIDATE_MAX) break;

        NlxFormatCandidate* candidate = &result->candidates[result->count++];
        candidate->definition = format;
        candidate->facility_code = (uint32_t)nlx_extract(
            raw_value, bit_length, format->facility_offset, format->facility_length);
        candidate->credential_number = nlx_extract(
            raw_value, bit_length, format->credential_offset, format->credential_length);
        candidate->score = format->base_score;
        if(format->parity_scheme != NlxParitySchemeNone) {
            const bool valid = nlx_parity_validate(format, raw_value);
            candidate->parity = valid ? NlxParityValid : NlxParityInvalid;
            candidate->score = valid ?
                                   (uint8_t)(candidate->score + 25U) :
                                   (uint8_t)(candidate->score > 30U ? candidate->score - 30U : 0U);
        } else {
            candidate->parity = NlxParityUnknown;
        }
    }

    nlx_sort_candidates(result);
    const uint8_t score_delta =
        result->count > 1U ? (uint8_t)(result->candidates[0].score - result->candidates[1].score) :
                             UINT8_MAX;
    result->ambiguous = result->count > 1U && score_delta < 15U;
    return result->count > 0U;
}

static uint8_t nlx_data_bit(const uint8_t* data, size_t bit_index) {
    return (uint8_t)((data[bit_index / 8U] >> (7U - (bit_index % 8U))) & 1U);
}

bool nlx_format_decode_hid_generic(
    const uint8_t* data,
    size_t data_size,
    uint8_t* bit_length,
    NlxDecodeResult* result) {
    if(!data || data_size != 6U || !bit_length || !result) return false;

    uint8_t decoded_length = 0U;
    for(size_t bit_index = 0U; bit_index < 6U; ++bit_index) {
        if(nlx_data_bit(data, bit_index)) {
            decoded_length = (uint8_t)(48U - bit_index - 1U);
            break;
        }
    }
    if(decoded_length == 0U) {
        if(nlx_data_bit(data, 6U) == 0U) {
            decoded_length = 37U;
        } else {
            size_t bit_index = 7U;
            decoded_length = 36U;
            while(decoded_length >= 26U && nlx_data_bit(data, bit_index) == 0U) {
                --decoded_length;
                ++bit_index;
            }
            if(decoded_length < 26U) decoded_length = 0U;
        }
    }

    *bit_length = decoded_length;
    if(decoded_length == 0U || decoded_length > 40U) {
        memset(result, 0, sizeof(*result));
        return false;
    }

    const size_t first_data_bit = 48U - decoded_length;
    uint64_t raw_value = 0ULL;
    for(size_t bit_index = first_data_bit; bit_index < 48U; ++bit_index) {
        raw_value = (raw_value << 1U) | nlx_data_bit(data, bit_index);
    }
    return nlx_format_decode(raw_value, decoded_length, result);
}

void nlx_format_apply_best(const NlxDecodeResult* decoded, NlxCredentialResult* result) {
    if(!decoded || !result || decoded->count == 0U) return;
    const NlxFormatCandidate* best = &decoded->candidates[0];
    const size_t format_capacity = sizeof(result->probable_format);
    size_t format_length = strlen(best->definition->name);
    if(format_length >= format_capacity) format_length = format_capacity - 1U;
    memcpy(result->probable_format, best->definition->name, format_length);
    result->probable_format[format_length] = '\0';
    result->bit_length = best->definition->bit_length;
    result->parity = best->parity;
    result->facility_code_present = best->definition->facility_length > 0U;
    result->facility_code_inferred = decoded->ambiguous;
    result->facility_code = best->facility_code;
    result->credential_number_present = best->definition->credential_length > 0U;
    result->credential_number_inferred = decoded->ambiguous;
    result->credential_number = best->credential_number;
    result->confidence = decoded->ambiguous ? NlxConfidenceLow :
                         best->score >= 85U ? NlxConfidenceHigh :
                                              NlxConfidenceMedium;

    result->candidate_count =
        decoded->count > NLX_RESULT_CANDIDATE_MAX ? NLX_RESULT_CANDIDATE_MAX : decoded->count;
    for(size_t i = 0U; i < result->candidate_count; ++i) {
        const NlxFormatCandidate* source = &decoded->candidates[i];
        NlxCredentialCandidate* destination = &result->candidates[i];
        size_t name_length = strlen(source->definition->name);
        if(name_length >= sizeof(destination->format)) {
            name_length = sizeof(destination->format) - 1U;
        }
        memcpy(destination->format, source->definition->name, name_length);
        destination->format[name_length] = '\0';
        destination->facility_code_present = source->definition->facility_length > 0U;
        destination->facility_code = source->facility_code;
        destination->credential_number_present = source->definition->credential_length > 0U;
        destination->credential_number = source->credential_number;
        destination->parity = source->parity;
        destination->score = source->score;
    }
}
