#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    NlxScanStageIdle = 0,
    NlxScanStageNfc,
    NlxScanStagePicopass,
    NlxScanStageLfRfid,
    NlxScanStageIbutton,
    NlxScanStageDetected,
    NlxScanStageCancelled,
} NlxScanStage;

typedef struct {
    NlxScanStage stage;
    uint32_t cycle;
} NlxScanSchedule;

void nlx_scan_schedule_init(NlxScanSchedule* schedule);
void nlx_scan_schedule_advance(NlxScanSchedule* schedule);
void nlx_scan_schedule_detected(NlxScanSchedule* schedule);
void nlx_scan_schedule_cancel(NlxScanSchedule* schedule);
bool nlx_scan_schedule_is_terminal(const NlxScanSchedule* schedule);
bool nlx_scan_wait_result_has(uint32_t wait_result, uint32_t signal);
