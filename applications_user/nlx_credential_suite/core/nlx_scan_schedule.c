#include "nlx_scan_schedule.h"

#define NLX_SCAN_WAIT_ERROR_MASK 0x80000000U

void nlx_scan_schedule_init(NlxScanSchedule* schedule) {
    if(!schedule) return;
    schedule->stage = NlxScanStageNfc;
    schedule->cycle = 1U;
}

void nlx_scan_schedule_advance(NlxScanSchedule* schedule) {
    if(!schedule || nlx_scan_schedule_is_terminal(schedule)) return;
    switch(schedule->stage) {
    case NlxScanStageNfc:
        schedule->stage = NlxScanStagePicopass;
        break;
    case NlxScanStagePicopass:
        schedule->stage = NlxScanStageLfRfid;
        break;
    case NlxScanStageLfRfid:
        schedule->stage = NlxScanStageIbutton;
        break;
    case NlxScanStageIbutton:
    default:
        schedule->stage = NlxScanStageNfc;
        ++schedule->cycle;
        break;
    }
}

void nlx_scan_schedule_detected(NlxScanSchedule* schedule) {
    if(schedule) schedule->stage = NlxScanStageDetected;
}

void nlx_scan_schedule_cancel(NlxScanSchedule* schedule) {
    if(schedule) schedule->stage = NlxScanStageCancelled;
}

bool nlx_scan_schedule_is_terminal(const NlxScanSchedule* schedule) {
    return schedule &&
           (schedule->stage == NlxScanStageDetected || schedule->stage == NlxScanStageCancelled);
}

bool nlx_scan_wait_result_has(uint32_t wait_result, uint32_t signal) {
    return (wait_result & NLX_SCAN_WAIT_ERROR_MASK) == 0U && (wait_result & signal) != 0U;
}
