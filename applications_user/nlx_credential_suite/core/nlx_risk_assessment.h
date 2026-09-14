#pragma once

#include "nlx_credential_result.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    NlxRiskLevelLow = 0,
    NlxRiskLevelModerate,
    NlxRiskLevelHigh,
    NlxRiskLevelVeryHigh,
} NlxRiskLevel;

typedef enum {
    NlxRiskIndicatorNone = 0,
    NlxRiskIndicatorLegacy = 1U << 0U,
    NlxRiskIndicatorUnencrypted = 1U << 1U,
    NlxRiskIndicatorProtected = 1U << 2U,
    NlxRiskIndicatorIncomplete = 1U << 3U,
    NlxRiskIndicatorLowConfidence = 1U << 4U,
    NlxRiskIndicatorAmbiguous = 1U << 5U,
} NlxRiskIndicator;

typedef struct {
    uint8_t exposure_score;
    NlxRiskLevel level;
    uint32_t indicators;
    bool provisional;
} NlxRiskAssessment;

bool nlx_risk_assess(const NlxCredentialResult* result, NlxRiskAssessment* assessment);
const char* nlx_risk_level_name(NlxRiskLevel level);
