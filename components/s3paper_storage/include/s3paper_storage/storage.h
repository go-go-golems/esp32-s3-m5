// s3paper_storage: microSD persistence for PaperS3 firmwares (ESP-51 P2).
//
// Non-destructive SDSPI mount (never auto-format), a bounded library scan
// with a serialized catalog cache, an SD-backed ContentSource, and atomic
// versioned+checksummed persistence (positions, bookmarks, settings,
// last-book). All writes are tmp -> fflush -> fsync -> rename(bak) ->
// rename: power loss yields old-or-new, never torn. A missing card is a
// recoverable state; every operation returns an explicit status.
//
// Threading: single-owner. Call everything from ONE task (the UI owner).
// Several internal buffers are static scratch precisely because multi-KiB
// stack locals overflowed the 8 KiB owner stack once (ESP-50 diary S13).
//
// Wiring: call StorageConfigure() once before the first StorageMount().
// The pre_mount hook exists because the display driver shares the SPI bus:
// M5GFX must initialize before the SD mount or the live bus gets torn down
// (ESP-50 diary S9). The seed book keeps the "write path exercised
// non-destructively" behavior without this component embedding book text.
#pragma once

#include <stdint.h>

#include "s3paper/content.h"
#include "s3paper/paginator.h"
#include "s3paper/status.h"

namespace s3paper_storage {

using StatusCode = s3paper::StatusCode;

constexpr uint32_t kMaxBooks = 32;

struct BookEntry {
    char path[96];    // absolute VFS path
    char title[40];   // filename without extension
    uint64_t size;
    int64_t mtime;    // FAT mtime; catalog cache-validation key with size
    s3paper::ContentHash content_hash;
};

// Counters mirrored into firmware status snapshots.
struct StorageStats {
    uint8_t mounted;
    uint32_t capacity_mib;
    uint32_t book_count;
    uint32_t position_records;
    uint32_t position_writes;
    uint32_t position_write_failures;
    uint32_t scan_cached;   // last scan: hashes reused from the catalog
    uint32_t scan_hashed;   // last scan: files actually read and hashed
    uint32_t scan_ms;       // last scan duration
    uint32_t catalog_writes;
};

struct StorageConfig {
    // Runs before the SPI bus / SD mount is touched. Return Ok to proceed.
    // Use it to guarantee the display driver owns the shared bus first.
    s3paper::Status (*pre_mount)();
    // Optional seed book written to seed_path on mount-side request when
    // absent (never overwrites). Null text disables seeding.
    const char *seed_path;
    const char *seed_text;
    uint32_t seed_len;
};

void StorageConfigure(const StorageConfig &config);

// ---- Card lifecycle ----
StatusCode StorageMount();
StatusCode StorageUnmount();
bool StorageMounted();
void GetStats(StorageStats *out);

// ---- Library ----
// Scans /sdcard and /sdcard/books for *.txt (bounded, deterministic
// title-sorted). Returns the number found or an explicit error.
StatusCode LibraryScan(uint32_t *out_count);
uint32_t LibraryCount();
const BookEntry *LibraryGet(uint32_t index);
// Prints the catalog via printf (owner context, mirrors trace printing).
void LibraryPrint();

// Writes the configured seed book if absent (creates the directory; never
// overwrites). Exercises the write path non-destructively.
StatusCode StorageWriteSeedBook();

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
// shutdown). Works for embedded books too when a card is present.
StatusCode PositionsLoad();
StatusCode PositionsSave();
bool PositionLookup(s3paper::ContentHash content,
                    s3paper::TextLocator *out);
void PositionStore(s3paper::ContentHash content,
                   const s3paper::TextLocator &locator);

// Coalesced flushing of dirty positions/bookmarks/settings.
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

// ---- Fault injection (debug/testing only) ----
// Damages a state file on the card so loader recovery can be exercised
// from the console. kind: 0=positions 1=bookmarks 2=catalog 3=settings
// 4=lastbook. mode: 0=flip a byte mid-file (CRC mismatch), 1=truncate to
// half (short read), 2=delete the primary (forces .bak fallback).
StatusCode DebugCorruptStateFile(uint32_t kind, uint32_t mode);
// Re-runs every loader against the current card state and logs results.
void DebugReloadState();

}  // namespace s3paper_storage
