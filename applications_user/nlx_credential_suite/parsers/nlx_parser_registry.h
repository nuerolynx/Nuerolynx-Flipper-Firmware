#pragma once

#include "../core/nlx_credential_result.h"
#include "../handoff/nlx_tool_registry.h"

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    NlxParserIntegrationNative = 0,
    NlxParserIntegrationAdapter,
    NlxParserIntegrationHandoff,
} NlxParserIntegration;

typedef struct {
    const char* id;
    const char* family;
    NlxTechnology technology;
    NlxParserIntegration integration;
    NlxToolId recommended_tool;
    bool identifier_supported;
    bool credential_fields_supported;
} NlxParserDefinition;

size_t nlx_parser_registry_count(void);
const NlxParserDefinition* nlx_parser_registry_get(size_t index);
const NlxParserDefinition* nlx_parser_registry_find(const char* id);
