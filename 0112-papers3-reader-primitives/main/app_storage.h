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
    int64_t mtime;    // FAT mtime; catalog cache-validation key with size
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
// Records keyed by content hash; loaded on mount. Stores mark the state
// dirty; StorageFlushIfDue() coalesces the actual atomic writes (quiet
// window or age-based), and StorageFlushNow() forces one (screen change,
// shutdown). Works for the embedded book too when a card is present.
StatusCode PositionsLoad();
StatusCode PositionsSave();
bool PositionLookup(s3paper::ContentHash content,
                    s3paper::TextLocator *out);
void PositionStore(s3paper::ContentHash content,
                   const s3paper::TextLocator &locator);

// Coalesced flushing of dirty positions/bookmarks.
void StorageFlushIfDue(int64_t now_us);
void StorageFlushNow();

// ---- Last-book record (boot restore) ----
// Path of the most recently opened book ("" = embedded). Saved atomically
// on every successful open.
void LastBookStore(const char *sd_path_or_empty);
// Returns true and fills out (size >= 96) when a valid record exists.
bool LastBookGet(char *out, uint32_t out_size);

// ---- Settings (small key-value store) ----
// Named int32 records (versioned + CRC + atomic like positions), used for
// app state and high scores. Works without a card (RAM-only until mount);
// writes coalesce through the same flush machinery.
StatusCode SettingsLoad();
int32_t SettingsGet(const char *key, int32_t fallback);
void SettingsSet(const char *key, int32_t value);

// ---- Bookmarks ----
// Multiple bookmarks per book, persisted like positions (versioned +
// checksummed + atomic). Toggle semantics: same content+offset removes.
StatusCode BookmarksLoad();
StatusCode BookmarkToggle(s3paper::ContentHash content,
                          const s3paper::TextLocator &locator,
                          bool *now_set);
bool BookmarkIsSet(s3paper::ContentHash content, uint64_t byte_offset);
uint32_t BookmarkCountFor(s3paper::ContentHash content);
// nth bookmark of this content (insertion order); false when out of range.
bool BookmarkGet(s3paper::ContentHash content, uint32_t index,
                 s3paper::TextLocator *out);
void BookmarksPrint(s3paper::ContentHash content);

}  // namespace reader
