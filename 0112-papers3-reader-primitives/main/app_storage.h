// Storage service (Phase 6). Owner-task-only.
//
// Non-destructive microSD mount (SDSPI, never auto-format), a bounded
// library scan of plain-text books, an SD-backed ContentSource, and atomic
// persistence of per-book reading positions (temp/write/flush/rename with
// a .bak generation). A missing or failed card is a recoverable state:
// every operation returns an explicit status and 'sd mount' retries.
#pragma once

#include <stdint.h>

#include "app_events.h"
#include "s3paper/content.h"
#include "s3paper/paginator.h"

namespace reader {

constexpr uint32_t kMaxBooks = 32;

struct BookEntry {
    char path[96];    // absolute VFS path
    char title[40];   // filename without extension
    uint64_t size;
    s3paper::ContentHash content_hash;
};

// ---- Card lifecycle ----
StatusCode StorageMount();
StatusCode StorageUnmount();
bool StorageMounted();
void FillSdSnapshot(SdSnapshot *out);

// ---- Library ----
// Scans /sdcard and /sdcard/books for *.txt (bounded, deterministic
// title-sorted). Returns the number found or an explicit error.
StatusCode LibraryScan(uint32_t *out_count);
uint32_t LibraryCount();
const BookEntry *LibraryGet(uint32_t index);
// Prints the catalog via printf (owner context, mirrors trace printing).
void LibraryPrint();

// Writes the embedded demo book to /sdcard/books/ if absent (creates the
// directory; never overwrites). Exercises the write path non-destructively.
StatusCode StorageWriteDemoBook();

// ---- SD content source ----
class SdContentSource : public s3paper::ContentSource {
  public:
    StatusCode Open(const char *path);
    void Close();
    bool IsOpen() const { return file_ != nullptr; }

    s3paper::Result<uint64_t> Size() override;
    s3paper::Result<uint32_t> ReadAt(uint64_t offset, uint8_t *buf,
                                     uint32_t len) override;
    s3paper::Result<s3paper::ContentHash> Hash() override;

  private:
    void *file_ = nullptr;  // FILE*
    uint64_t size_ = 0;
    s3paper::ContentHash hash_ = 0;
};

// ---- Reading-position persistence ----
// Records keyed by content hash; loaded on mount, saved atomically on
// change. Works for the embedded book too when a card is present.
StatusCode PositionsLoad();
StatusCode PositionsSave();
bool PositionLookup(s3paper::ContentHash content,
                    s3paper::TextLocator *out);
void PositionStore(s3paper::ContentHash content,
                   const s3paper::TextLocator &locator);

}  // namespace reader
