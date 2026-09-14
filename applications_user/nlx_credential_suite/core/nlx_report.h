#pragma once

#include "nlx_credential_result.h"

size_t nlx_report_json(const NlxCredentialResult* result, char* output, size_t output_size);
size_t nlx_report_text(const NlxCredentialResult* result, char* output, size_t output_size);
size_t nlx_report_json_pair(
    const NlxCredentialResult* primary,
    const NlxCredentialResult* secondary,
    char* output,
    size_t output_size);
size_t nlx_report_text_pair(
    const NlxCredentialResult* primary,
    const NlxCredentialResult* secondary,
    char* output,
    size_t output_size);
