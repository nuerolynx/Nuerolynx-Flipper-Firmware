#include "nlx_risk_assessment.h"

#include <string.h>

static uint8_t nlx_risk_base_score(const NlxCredentialResult* result, uint32_t* indicators) {
    switch(result->technology) {
    case NlxTechnologyLfRfid:
        *indicators |= NlxRiskIndicatorLegacy;
        return 75U;
    case NlxTechnologyPicopass:
        *indicators |= NlxRiskIndicatorLegacy;
        return 50U;
    case NlxTechnologyIbutton:
        *indicators |= NlxRiskIndicatorLegacy;
        return 55U;
    case NlxTechnologyNfc:
        if(strstr(result->card_family, "MIFARE Classic")) {
            *indicators |= NlxRiskIndicatorLegacy;
            return 65U;
        }
        if(strstr(result->card_family, "Ultralight") || strstr(result->card_family, "NTAG")) {
            return 55U;
        }
        if(strstr(result->card_family, "DESFire") || strstr(result->card_family, "Plus")) {
            return 30U;
        }
        return 45U;
    case NlxTechnologyUnknown:
    default:
        return 0U;
    }
}

bool nlx_risk_assess(const NlxCredentialResult* result, NlxRiskAssessment* assessment) {
    if(!result || !assessment || result->technology == NlxTechnologyUnknown) return false;

    uint32_t indicators = NlxRiskIndicatorNone;
    int score = nlx_risk_base_score(result, &indicators);

    if(result->security == NlxSecurityUnencrypted) {
        score += 10;
        indicators |= NlxRiskIndicatorUnencrypted;
    } else if(
        result->security == NlxSecurityEncrypted ||
        result->security == NlxSecurityAuthenticationRequired) {
        score -= 20;
        indicators |= NlxRiskIndicatorProtected;
    }

    bool provisional = false;
    if(result->completeness <= NlxCompletenessClassified) {
        indicators |= NlxRiskIndicatorIncomplete;
        provisional = true;
    }
    if(result->confidence <= NlxConfidenceLow) {
        indicators |= NlxRiskIndicatorLowConfidence;
        provisional = true;
    }
    if(result->candidate_count > 1U) {
        indicators |= NlxRiskIndicatorAmbiguous;
        provisional = true;
    }

    if(score < 0) score = 0;
    if(score > 100) score = 100;
    assessment->exposure_score = (uint8_t)score;
    assessment->indicators = indicators;
    assessment->provisional = provisional;
    assessment->level = score >= 80 ? NlxRiskLevelVeryHigh :
                        score >= 60 ? NlxRiskLevelHigh :
                        score >= 35 ? NlxRiskLevelModerate :
                                      NlxRiskLevelLow;
    return true;
}

const char* nlx_risk_level_name(NlxRiskLevel level) {
    switch(level) {
    case NlxRiskLevelVeryHigh:
        return "very high";
    case NlxRiskLevelHigh:
        return "high";
    case NlxRiskLevelModerate:
        return "moderate";
    case NlxRiskLevelLow:
    default:
        return "low";
    }
}
