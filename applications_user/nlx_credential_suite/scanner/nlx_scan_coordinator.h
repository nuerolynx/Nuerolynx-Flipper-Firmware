#pragma once

#include "../core/nlx_credential_result.h"
#include "../core/nlx_scan_schedule.h"

#include <furi.h>

typedef void (*NlxScanStatusCallback)(NlxScanStage stage, uint32_t cycle, void* context);
typedef void (*NlxScanResultCallback)(
    const NlxCredentialResult* primary,
    const NlxCredentialResult* secondary,
    void* context);

typedef struct NlxScanCoordinator NlxScanCoordinator;

NlxScanCoordinator* nlx_scan_coordinator_alloc(
    NlxScanStatusCallback status_callback,
    NlxScanResultCallback result_callback,
    void* context);
void nlx_scan_coordinator_free(NlxScanCoordinator* coordinator);
bool nlx_scan_coordinator_start(NlxScanCoordinator* coordinator);
void nlx_scan_coordinator_cancel(NlxScanCoordinator* coordinator);
void nlx_scan_coordinator_join(NlxScanCoordinator* coordinator);
bool nlx_scan_coordinator_is_running(const NlxScanCoordinator* coordinator);
