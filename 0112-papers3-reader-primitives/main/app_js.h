// MicroQuickJS host for the s3paper JS facade (Phase 12). Owner-task-only.
//
// One long-lived context in a fixed internal-RAM arena runs the embedded
// ES5 facade plus acceptance apps. Scripts always execute under a
// deadline; native code never stores JS values across calls (dispatch
// looks the handler up by name each time), so the compacting GC needs no
// long-lived roots here. Widget/page handles cross the boundary as the
// same generation-checked integers s3paper_core uses.
#pragma once

#include "app_events.h"
#include "s3paper/input.h"

namespace reader {

// Lazily creates the context and evaluates the facade. Idempotent.
StatusCode JsInit();

// Runs an embedded acceptance app: 1 = hello, 2 = status (tap counter),
// 3 = library, 4 = fault, 5 = trace-compare, 6 = reader.
StatusCode JsRunApp(uint32_t which);

// Injects a gesture into the JS dispatcher for console-driven validation
// (kind = s3paper::GestureKind). Busy when no JS screen is active.
StatusCode JsSyntheticGesture(uint32_t kind, int32_t x, int32_t y);

// True while the JS app's page is the one on the panel (no other screen
// presented since). Gestures then go to JS instead of the reader.
bool JsScreenActive();

// Routes a gesture to the JS dispatcher (hit test over the JS page's hit
// regions, then s3Dispatch(kind, x, y, hit)). Returns true when consumed.
bool JsHandleGesture(const s3paper::GestureEvent &gesture);

void FillJsSnapshot(JsSnapshot *out);

}  // namespace reader
