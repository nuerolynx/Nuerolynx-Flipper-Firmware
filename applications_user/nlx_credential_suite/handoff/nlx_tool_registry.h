#pragma once

#include "../core/nlx_credential_result.h"

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    NlxToolBuiltInNfc = 0,
    NlxToolBuiltInLfRfid,
    NlxToolBuiltInIbutton,
    NlxToolPicopass,
    NlxToolMfcEditor,
    NlxToolMfkey,
    NlxToolNfcApduRunner,
    NlxToolNfcMagic,
    NlxToolMetroflip,
    NlxToolSeader,
    NlxToolSeos,
    NlxToolIso15693Writer,
    NlxToolFieldDetector,
    NlxToolRfidFuzzer,
    NlxToolMifareFuzzer,
    NlxToolIbuttonFuzzer,
    NlxToolCount,
} NlxToolId;

typedef struct {
    NlxToolId id;
    const char* name;
    const char* launch_target;
    bool authorized_lab_only;
    bool external_hardware_may_be_required;
} NlxToolDefinition;

const NlxToolDefinition* nlx_tool_registry_get(NlxToolId id);
NlxToolId nlx_tool_registry_recommend(const NlxCredentialResult* result);
