// MicroQuickJS host for PULP OS v2 (ESP-51 Phase 5/6). Owner-task-only.
//
// The v2 builder API: factories (text/row/col/...) return native Widget
// class instances whose opaque is the packed s3paper WidgetHandle; fluent
// methods live in ROM prototypes; closures stay in JS (the kernel's __cbs
// array roots them) and native code stores only integer callback ids.
// Every eval/call runs under a deadline; exceptions are counted and
// snapshotted, never fatal.
#pragma once

#include "app_events.h"
#include "s3paper/input.h"

namespace pulp {

// Creates the JS context (PSRAM arena), loads bytecode images, evaluates
// the kernel, and registers the gesture handler + sleep-image builder.
StatusCode JsInit();

// Runs one embedded console probe (P5.8/P6 validation surface).
StatusCode JsRunProbe(uint32_t which);

// Runs the PULP OS bytecode image (Phase 7).
StatusCode JsRunPulp();

// True while the last present on the panel is a JS page.
bool JsScreenActive();

// Owner-context gesture sink (registered with app_input). Consumes all
// gestures while the JS screen is active: hit callbacks for taps, page
// handlers per gesture kind, then the paper.home fallback for swipe-down.
bool JsHandleGesture(const s3paper::GestureEvent &gesture);

// Console injector: routes through the SAME dispatch path as real touch.
StatusCode JsSyntheticGesture(uint32_t kind, int32_t x, int32_t y);

// Owner-loop hook: fires the current page's every() tick (tick callback,
// dynamic text refresh, one diff-update present).
void JsTimerTick(int64_t now_us);

void FillJsSnapshot(JsSnapshot *out);

}  // namespace pulp
