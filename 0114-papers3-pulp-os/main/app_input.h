// Input service. Owner-task-only: tracker, gesture detector, and counters
// are application state.
//
// A separate tick-producer task posts TimerDue events at the poll rate but
// never touches M5 or the model; the owner does the GT911 read and pipeline
// work when the tick event arrives. Gestures route to a registered handler
// (the JS layer, Phase 6); unhandled gestures are logged only.
#pragma once

#include <stdint.h>

#include "app_events.h"
#include "s3paper/input.h"

namespace pulp {

// TimerDue id used by the touch tick producer.
constexpr uint32_t kTouchTimerId = 0x70C4;

// Owner-context gesture sink; return true when consumed.
using GestureHandler = bool (*)(const s3paper::GestureEvent &gesture);

// Starts the tick producer task (disabled until TouchEnable).
void InputServiceInit();

// Registers the gesture handler called in owner context (nullptr clears).
void InputSetGestureHandler(GestureHandler handler);

// Owner-task-only. Enabling initializes the M5 backend if needed.
StatusCode TouchEnable();
void TouchDisable();
bool TouchEnabled();

// Owner-task-only: handles one poll tick.
void InputHandleTick();

// Owner-task-only: fills the console snapshot.
void FillTouchSnapshot(TouchSnapshot *out);

// Owner-task-only: monotonic time of the last touch input (0 = never).
int64_t InputLastInputUs();

}  // namespace pulp
