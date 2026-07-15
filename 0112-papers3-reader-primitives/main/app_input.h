// Input service (Phase 4). Owner-task-only: tracker, gesture detector,
// scheduler, and counters are application state.
//
// A separate tick-producer task posts TimerDue events at the poll rate but
// never touches M5 or the model; the owner does the GT911 read and pipeline
// work when the tick event arrives.
#pragma once

#include <stdint.h>

#include "app_events.h"

namespace reader {

// TimerDue id used by the touch tick producer.
constexpr uint32_t kTouchTimerId = 0x70C4;

// Starts the tick producer task (disabled until TouchEnable).
void InputServiceInit();

// Owner-task-only. Enabling initializes the M5 backend if needed.
StatusCode TouchEnable();
void TouchDisable();
bool TouchEnabled();

// Owner-task-only: handles one poll tick (sample -> events -> gestures ->
// quiet scheduling), updating the input counters.
void InputHandleTick();

// Owner-task-only: fills the console snapshot.
void FillTouchSnapshot(TouchSnapshot *out);

// Owner-task-only: monotonic time of the last touch input (0 = never).
// Quiet-while-active regions defer their updates against this.
int64_t InputLastInputUs();

}  // namespace reader
