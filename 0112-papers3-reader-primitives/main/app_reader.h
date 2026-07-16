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
StatusCode ReaderOpen();                  // embedded fixture book
StatusCode ReaderOpenSd(uint32_t index);  // library book by scan index
StatusCode ReaderNext();
StatusCode ReaderPrev();
// Renders the on-screen library (books + hit regions; tap opens).
StatusCode LibraryShow();
// Boot flow: mount card, scan, reopen the last book at its persisted
// position, else show the library. Owner-task-only, called once at start.
StatusCode ReaderBootRestore();
// Bookmark actions on the current page start.
StatusCode ReaderBookmarkToggle();
StatusCode ReaderBookmarkGoto(uint32_t index);
void ReaderBookmarksPrint();
void FillReaderSnapshot(ReaderSnapshot *out);

// Formats a library row line ("title  12KB 45%"). index 0xFFFFFFFF formats
// the embedded book ("title (embedded) 45%"). Used by the JS library port.
void ReaderFormatLibraryLine(uint32_t index, char *out, uint32_t out_size);

// Routes a gesture to the reader when a book is open. Returns true when
// the gesture was consumed (a page turn happened or was attempted).
bool ReaderHandleGesture(const s3paper::GestureEvent &gesture);

}  // namespace reader
