// Reader controller (Phase 8 vertical slice). Owner-task-only.
//
// Reads the embedded fixture book through the streaming paginator and
// renders pages via the frame builder + refresh planner. Gestures map to
// page turns; the console mirrors the same operations.
#pragma once

#include "app_events.h"
#include "s3paper/input.h"

namespace reader {

// All owner-task-only.
StatusCode ReaderOpen();
StatusCode ReaderNext();
StatusCode ReaderPrev();
void FillReaderSnapshot(ReaderSnapshot *out);

// Routes a gesture to the reader when a book is open. Returns true when
// the gesture was consumed (a page turn happened or was attempted).
bool ReaderHandleGesture(const s3paper::GestureEvent &gesture);

}  // namespace reader
