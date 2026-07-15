#include "s3paper/paginator.h"

namespace s3paper {

bool LayoutKeyEquals(const LayoutKey &a, const LayoutKey &b) {
    return a.content == b.content && a.font_id == b.font_id &&
           a.viewport_w == b.viewport_w && a.viewport_h == b.viewport_h &&
           a.margin_x == b.margin_x && a.margin_top == b.margin_top &&
           a.margin_bottom == b.margin_bottom &&
           a.engine_version == b.engine_version;
}

Paginator::Paginator(ContentSource *source, const LayoutKey &key)
    : source_(source), key_(key) {}

Result<TextLocator> Paginator::MakeLocator(uint64_t offset) {
    uint8_t context[16];
    const Result<uint32_t> n = source_->ReadAt(offset, context, sizeof(context));
    if (!n.ok()) {
        return Result<TextLocator>::Err(n.code);
    }
    TextLocator locator{};
    locator.byte_offset = offset;
    locator.context_hash = Fnv1a(context, n.value);
    return Result<TextLocator>::Ok(locator);
}

Result<TextLocator> Paginator::Begin() { return MakeLocator(0); }

Status Paginator::Validate(const TextLocator &locator) {
    const Result<TextLocator> fresh = MakeLocator(locator.byte_offset);
    if (!fresh.ok()) {
        return ErrStatus(fresh.code);
    }
    if (fresh.value.context_hash != locator.context_hash) {
        return ErrStatus(StatusCode::CorruptData);
    }
    return OkStatus();
}

void Paginator::RecordCheckpoint(uint64_t offset) {
    for (uint32_t i = 0; i < checkpoint_count_; ++i) {
        if (checkpoints_[i] == offset) {
            return;
        }
    }
    if (checkpoint_count_ < kMaxCheckpoints) {
        checkpoints_[checkpoint_count_++] = offset;
    } else {
        checkpoints_[checkpoint_next_] = offset;
        checkpoint_next_ = (checkpoint_next_ + 1) % kMaxCheckpoints;
    }
}

Status Paginator::ComposePage(TextLocator start, PageLayout *out) {
    const Result<FontLineMetrics> line_metrics =
        GetFontLineMetrics(key_.font_id);
    if (!line_metrics.ok() || out == nullptr || source_ == nullptr) {
        return ErrStatus(StatusCode::InvalidArgument);
    }
    const Result<uint64_t> size = source_->Size();
    if (!size.ok()) {
        return ErrStatus(size.code);
    }
    const Result<uint32_t> read =
        source_->ReadAt(start.byte_offset, window_, kWindowBytes);
    if (!read.ok()) {
        return ErrStatus(read.code);
    }
    const uint32_t n = read.value;
    const bool window_truncated =
        start.byte_offset + n < size.value;

    out->start = start;
    out->line_count = 0;
    out->at_end = false;

    const int32_t text_width = key_.viewport_w - 2 * key_.margin_x;
    const int32_t line_height = line_metrics.value.line_height;
    const int32_t last_baseline = key_.viewport_h - key_.margin_bottom;
    int32_t baseline = key_.margin_top + line_height;
    if (text_width <= 0 || baseline > last_baseline) {
        return ErrStatus(StatusCode::InvalidArgument);
    }

    const char *text = reinterpret_cast<const char *>(window_);
    uint64_t consumed_abs = start.byte_offset;  // first byte NOT on the page
    bool page_full = false;

    TextSpan paras[64];
    const uint32_t para_total = SplitParagraphs(text, n, paras, 64);
    const uint32_t para_count = para_total < 64 ? para_total : 64;

    for (uint32_t p = 0; p < para_count && !page_full; ++p) {
        const TextSpan &para = paras[p];
        // Is this paragraph truncated by the window edge?
        const bool para_at_window_edge =
            (para.byte_start + para.byte_len == n) && window_truncated;
        LineSpan lines[PageLayout::kMaxLines + 1];
        const Result<uint32_t> broken =
            BreakLines(key_.font_id, text + para.byte_start, para.byte_len,
                       text_width, lines, PageLayout::kMaxLines + 1);
        uint32_t line_total;
        if (broken.ok()) {
            line_total = broken.value;
        } else if (broken.code == StatusCode::CapacityExceeded) {
            line_total = PageLayout::kMaxLines + 1;  // use what fits
        } else {
            return ErrStatus(broken.code);
        }
        uint32_t usable = line_total;
        if (para_at_window_edge && line_total > 1) {
            // Drop the final (possibly mid-word) line; the next page
            // recomposes it from a fresh window.
            usable = line_total - 1;
        }
        for (uint32_t i = 0; i < usable; ++i) {
            if (baseline > last_baseline ||
                out->line_count >= PageLayout::kMaxLines) {
                page_full = true;
                consumed_abs =
                    start.byte_offset + para.byte_start + lines[i].byte_start;
                break;
            }
            PageLine &pl = out->lines[out->line_count++];
            pl.byte_start =
                start.byte_offset + para.byte_start + lines[i].byte_start;
            pl.byte_len = lines[i].byte_len;
            pl.width = lines[i].width;
            pl.baseline_y = baseline;
            baseline += line_height;
            consumed_abs = pl.byte_start + pl.byte_len;
        }
        if (page_full) {
            break;
        }
        if (para_at_window_edge && usable < line_total) {
            // Continue on the next page at the dropped line's start.
            consumed_abs = start.byte_offset + para.byte_start +
                           lines[usable].byte_start;
            break;
        }
        // Paragraph finished: skip its newline separator and add spacing.
        const uint64_t para_end_abs =
            start.byte_offset + para.byte_start + para.byte_len;
        uint64_t after = para_end_abs;
        // Skip CR/LF bytes that SplitParagraphs excluded from the span.
        while (after < start.byte_offset + n &&
               (text[after - start.byte_offset] == '\n' ||
                text[after - start.byte_offset] == '\r')) {
            after++;
        }
        consumed_abs = after;
        baseline += line_height / 2;
    }

    if (consumed_abs <= start.byte_offset && n > 0) {
        // Guarantee progress even for degenerate content.
        consumed_abs = start.byte_offset + 1;
    }
    if (consumed_abs >= size.value) {
        consumed_abs = size.value;
        out->at_end = true;
    }
    const Result<TextLocator> next = MakeLocator(consumed_abs);
    if (!next.ok()) {
        return ErrStatus(next.code);
    }
    out->next = next.value;
    RecordCheckpoint(start.byte_offset);
    return OkStatus();
}

Result<TextLocator> Paginator::PreviousPageStart(const TextLocator &current) {
    if (current.byte_offset == 0) {
        return Result<TextLocator>::Ok(current);
    }
    // Nearest checkpoint strictly before current.
    uint64_t from = UINT64_MAX;
    for (uint32_t i = 0; i < checkpoint_count_; ++i) {
        if (checkpoints_[i] < current.byte_offset &&
            (from == UINT64_MAX || checkpoints_[i] > from)) {
            from = checkpoints_[i];
        }
    }
    if (from == UINT64_MAX) {
        // Bounded backward scan: find a paragraph boundary in the window
        // preceding current.
        const uint64_t back = current.byte_offset > kWindowBytes
                                  ? current.byte_offset - kWindowBytes
                                  : 0;
        const uint32_t want =
            static_cast<uint32_t>(current.byte_offset - back);
        const Result<uint32_t> n = source_->ReadAt(back, window_, want);
        if (!n.ok()) {
            return Result<TextLocator>::Err(n.code);
        }
        from = back;
        for (uint32_t i = n.value; i > 1; --i) {
            if (window_[i - 1] == '\n') {
                from = back + i;
                break;
            }
        }
        if (from >= current.byte_offset) {
            from = back;
        }
    }
    // Forward reconstruction from `from` until the page that ends at (or
    // beyond) current. Bounded by construction: each page consumes >= 1 byte.
    Result<TextLocator> cursor = MakeLocator(from);
    if (!cursor.ok()) {
        return cursor;
    }
    PageLayout page;
    for (uint32_t guard = 0; guard < 4096; ++guard) {
        const Status st = ComposePage(cursor.value, &page);
        if (!st.ok()) {
            return Result<TextLocator>::Err(st.code);
        }
        if (page.next.byte_offset >= current.byte_offset || page.at_end) {
            return Result<TextLocator>::Ok(page.start);
        }
        cursor = Result<TextLocator>::Ok(page.next);
    }
    return Result<TextLocator>::Err(StatusCode::Timeout);
}

Result<uint32_t> Paginator::ProgressPermille(const TextLocator &locator) {
    const Result<uint64_t> size = source_->Size();
    if (!size.ok()) {
        return Result<uint32_t>::Err(size.code);
    }
    if (size.value == 0) {
        return Result<uint32_t>::Ok(1000);
    }
    const uint64_t clamped =
        locator.byte_offset > size.value ? size.value : locator.byte_offset;
    return Result<uint32_t>::Ok(
        static_cast<uint32_t>((clamped * 1000) / size.value));
}

}  // namespace s3paper
