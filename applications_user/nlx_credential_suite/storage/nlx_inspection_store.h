#pragma once

#include "../core/nlx_credential_result.h"

#include <storage/storage.h>

bool nlx_inspection_store_save(
    Storage* storage,
    const NlxCredentialResult* result,
    const NlxCredentialResult* secondary_result,
    char* saved_base_path,
    size_t saved_base_path_size);
