// Streaming pagination (Phase 7).
//
// Positions are locators (byte offset + context hash), never page numbers:
// a page number is a cache result that changes with font, viewport, margins,
// and engine version. Forward composition streams a bounded window from the
// ContentSource; backward navigation uses sparse in-memory checkpoints plus
// bounded forward reconstruction.
#pragma once

#include <stdint.h>

#include "s3paper/content.h"
#include "s3paper/status.h"
#include "s3paper/text.h"

namespace s3paper {

// Bump when composition behavior changes; invalidates cached pagination.
constexpr uint32_t kLayoutEngineVersion = 1;

struct TextLocator {
    uint64_t byte_offset;
    // FNV of up to 16 bytes at byte_offset; detects stale locators after
    // content changes.
    uint32_t context_hash;
};

struct LayoutKey {
    ContentHash content;
    uint8_t font_id;
    int32_t viewport_w;
    int32_t viewport_h;
    int32_t margin_x;
    int32_t margin_top;
    int32_t margin_bottom;
    uint32_t engine_version;
};

bool LayoutKeyEquals(const LayoutKey &a, const LayoutKey &b);

struct PageLine {
    uint64_t byte_start;  // absolute content offset
    uint32_t byte_len;
    int32_t width;
    int32_t baseline_y;   // absolute y in the viewport
};

struct PageLayout {
    static constexpr uint32_t kMaxLines = 40;
    TextLocator start;
    TextLocator next;     // locator of the following page (== start at end)
    PageLine lines[kMaxLines];
    uint32_t line_count;
    bool at_end;          // no content after this page
};

class Paginator {
  public:
    static constexpr uint32_t kWindowBytes = 8192;
    static constexpr uint32_t kMaxCheckpoints = 128;

    Paginator(ContentSource *source, const LayoutKey &key);

    // Builds the locator for the start of content.
    Result<TextLocator> Begin();

    // Validates a locator's context hash against current content.
    Status Validate(const TextLocator &locator);

    // Composes the page starting at `start`. Streams at most kWindowBytes;
    // a single paragraph longer than the window is split at the window edge
    // (explicit bound, no unbounded memory). Records a checkpoint.
    // `start` is taken by value: callers routinely pass out->next, which
    // this call overwrites.
    Status ComposePage(TextLocator start, PageLayout *out);

    // Locator of the page preceding `current`, via checkpoints or a bounded
    // backward paragraph scan followed by forward reconstruction. Returns
    // Ok(current-page-start) unchanged when already at the beginning.
    Result<TextLocator> PreviousPageStart(const TextLocator &current);

    // Non-blocking progress estimate in permille of content bytes.
    Result<uint32_t> ProgressPermille(const TextLocator &locator);

    uint32_t checkpoint_count() const { return checkpoint_count_; }
    const LayoutKey &key() const { return key_; }

  private:
    Result<TextLocator> MakeLocator(uint64_t offset);
    void RecordCheckpoint(uint64_t offset);

    ContentSource *source_;
    LayoutKey key_;
    // Sparse ring of known page-start offsets, kept sorted by insertion
    // recency; lookup scans all (small, bounded).
    uint64_t checkpoints_[kMaxCheckpoints];
    uint32_t checkpoint_count_ = 0;
    uint32_t checkpoint_next_ = 0;
    uint8_t window_[kWindowBytes];
};

}  // namespace s3paper
