#include "nlx_credential_result.h"

#include <string.h>

void nlx_credential_result_reset(NlxCredentialResult* result) {
    if(!result) return;
    memset(result, 0, sizeof(*result));
    result->technology = NlxTechnologyUnknown;
    result->parity = NlxParityUnknown;
    result->security = NlxSecurityUnknown;
    result->completeness = NlxCompletenessNone;
    result->confidence = NlxConfidenceNone;
    memcpy(result->parser_version, "NLX-CS/1", sizeof("NLX-CS/1"));
}

const char* nlx_technology_name(NlxTechnology technology) {
    switch(technology) {
    case NlxTechnologyNfc:
        return "NFC";
    case NlxTechnologyLfRfid:
        return "LF RFID";
    case NlxTechnologyPicopass:
        return "Picopass/iCLASS";
    case NlxTechnologyIbutton:
        return "iButton/1-Wire";
    default:
        return "Unknown";
    }
}

const char* nlx_parity_name(NlxParityStatus status) {
    switch(status) {
    case NlxParityValid:
        return "valid";
    case NlxParityInvalid:
        return "invalid";
    case NlxParityNotApplicable:
        return "n/a";
    default:
        return "unknown";
    }
}

const char* nlx_security_name(NlxSecurityStatus status) {
    switch(status) {
    case NlxSecurityUnencrypted:
        return "unencrypted";
    case NlxSecurityEncrypted:
        return "encrypted";
    case NlxSecurityAuthenticationRequired:
        return "authentication required";
    default:
        return "unknown";
    }
}

const char* nlx_completeness_name(NlxReadCompleteness completeness) {
    switch(completeness) {
    case NlxCompletenessClassified:
        return "classified";
    case NlxCompletenessIdentifier:
        return "identifier read";
    case NlxCompletenessFull:
        return "complete";
    default:
        return "none";
    }
}

const char* nlx_confidence_name(NlxConfidence confidence) {
    switch(confidence) {
    case NlxConfidenceLow:
        return "low";
    case NlxConfidenceMedium:
        return "medium";
    case NlxConfidenceHigh:
        return "high";
    default:
        return "none";
    }
}
