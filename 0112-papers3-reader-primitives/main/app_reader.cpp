#include "app_reader.h"

#include <cstdio>
#include <cstring>

#include "esp_log.h"

#include "app_display.h"
#include "app_reader_book.h"
#include "s3paper/content.h"
#include "s3paper/paginator.h"
#include "s3paper/text.h"

namespace reader {
namespace {

const char *kTag = "reader_ctl";

constexpr int32_t kMarginX = 40;
constexpr int32_t kMarginTop = 72;   // leaves room for the header
constexpr int32_t kMarginBottom = 56;  // leaves room for the footer

struct ReaderState {
    bool open = false;
    s3paper::MemoryContentSource source{
        kEmbeddedBookText, sizeof(kEmbeddedBookText) - 1};
    s3paper::Paginator *paginator = nullptr;
    s3paper::TextLocator current{};
    s3paper::PageLayout page{};
    uint32_t page_turns = 0;
};

ReaderState s_state;

s3paper::LayoutKey MakeKey() {
    s3paper::LayoutKey key{};
    key.content = s_state.source.Hash().value;
    key.font_id = s3paper::kFontBody;
    key.viewport_w = 540;
    key.viewport_h = 960;
    key.margin_x = kMarginX;
    key.margin_top = kMarginTop;
    key.margin_bottom = kMarginBottom;
    key.engine_version = s3paper::kLayoutEngineVersion;
    return key;
}

// Renders the current page: header (title), body lines, footer (progress).
StatusCode RenderCurrentPage() {
    s3paper::FrameBuilder &fb = FrameBuilderRef();
    const s3paper::FontLineMetrics ui =
        s3paper::GetFontLineMetrics(s3paper::kFontUi).value;
    const s3paper::FontLineMetrics body =
        s3paper::GetFontLineMetrics(s3paper::kFontBody).value;
    fb.Begin();
    s3paper::Status st = fb.FillRect(s3paper::Rect{0, 0, 540, 960}, 255);
    if (!st.ok()) return st.code;
    // Header.
    st = fb.GlyphRun(s3paper::Rect{kMarginX, 16, 460, ui.line_height + 4},
                     16 + ui.line_height, s3paper::kFontUi, 0,
                     kEmbeddedBookTitle, sizeof(kEmbeddedBookTitle) - 1, 0);
    if (!st.ok()) return st.code;
    st = fb.HLine(kMarginX, 56, 540 - 2 * kMarginX, 0);
    if (!st.ok()) return st.code;
    // Body lines: read each line's bytes from the content source into a
    // stack buffer; GlyphRun copies them into the frame arena.
    for (uint32_t i = 0; i < s_state.page.line_count; ++i) {
        const s3paper::PageLine &line = s_state.page.lines[i];
        char buf[256];
        const uint32_t len = line.byte_len < sizeof(buf)
                                 ? line.byte_len
                                 : static_cast<uint32_t>(sizeof(buf));
        const s3paper::Result<uint32_t> got = s_state.source.ReadAt(
            line.byte_start, reinterpret_cast<uint8_t *>(buf), len);
        if (!got.ok()) return got.code;
        st = fb.GlyphRun(
            s3paper::Rect{kMarginX, line.baseline_y - body.ascent,
                          line.width, body.line_height},
            line.baseline_y, s3paper::kFontBody, 0, buf, got.value, 0);
        if (!st.ok()) return st.code;
    }
    // Footer: progress + page-turn count.
    const s3paper::Result<uint32_t> progress =
        s_state.paginator->ProgressPermille(s_state.page.next);
    char footer[64];
    const int footer_len = snprintf(
        footer, sizeof(footer), "%u%%%s   turns %u",
        progress.ok() ? static_cast<unsigned>(progress.value / 10) : 0,
        s_state.page.at_end ? " (end)" : "",
        static_cast<unsigned>(s_state.page_turns));
    st = fb.HLine(kMarginX, 960 - 44, 540 - 2 * kMarginX, 0);
    if (!st.ok()) return st.code;
    st = fb.GlyphRun(s3paper::Rect{kMarginX, 960 - 40, 460, 32}, 960 - 14,
                     s3paper::kFontUi, 0, footer,
                     static_cast<uint32_t>(footer_len), 0);
    if (!st.ok()) return st.code;

    const s3paper::Result<s3paper::RenderFrame> frame = FinishFrame();
    if (!frame.ok()) return frame.code;
    const s3paper::Status init = EnsureM5Init();
    if (!init.ok()) return init.code;
    const PlannedPresent presented = PresentFramePlanned(
        frame.value, s3paper::PresentIntent::TextPage, true);
    if (presented.present.status == StatusCode::Ok) {
        ESP_LOGI(kTag, "page at %llu lines=%u full=%d reason=%s",
                 static_cast<unsigned long long>(
                     s_state.current.byte_offset),
                 static_cast<unsigned>(s_state.page.line_count),
                 presented.plan.full_refresh ? 1 : 0,
                 s3paper::RefreshReasonName(presented.plan.reason));
    }
    return presented.present.status;
}

// Takes the locator BY VALUE: callers pass s_state.page.next, which
// ComposePage overwrites mid-call; a reference would alias the storage.
StatusCode ComposeAndRender(s3paper::TextLocator at) {
    const s3paper::Status composed =
        s_state.paginator->ComposePage(at, &s_state.page);
    if (!composed.ok()) {
        return composed.code;
    }
    s_state.current = at;
    return RenderCurrentPage();
}

}  // namespace

StatusCode ReaderOpen() {
    static s3paper::Paginator paginator(&s_state.source, MakeKey());
    s_state.paginator = &paginator;
    const s3paper::Result<s3paper::TextLocator> begin = paginator.Begin();
    if (!begin.ok()) {
        return begin.code;
    }
    s_state.open = true;
    s_state.page_turns = 0;
    Planner().NoteScreenChange();  // opening a book is a screen change
    return ComposeAndRender(begin.value);
}

StatusCode ReaderNext() {
    if (!s_state.open) {
        return StatusCode::Busy;
    }
    if (s_state.page.at_end) {
        return StatusCode::InvalidArgument;  // explicit end-of-content
    }
    s_state.page_turns++;
    return ComposeAndRender(s_state.page.next);
}

StatusCode ReaderPrev() {
    if (!s_state.open) {
        return StatusCode::Busy;
    }
    const s3paper::Result<s3paper::TextLocator> prev =
        s_state.paginator->PreviousPageStart(s_state.current);
    if (!prev.ok()) {
        return prev.code;
    }
    if (prev.value.byte_offset == s_state.current.byte_offset) {
        return StatusCode::InvalidArgument;  // already at the beginning
    }
    s_state.page_turns++;
    return ComposeAndRender(prev.value);
}

void FillReaderSnapshot(ReaderSnapshot *out) {
    std::memset(out, 0, sizeof(*out));
    out->open = s_state.open ? 1 : 0;
    if (!s_state.open) {
        return;
    }
    out->at_end = s_state.page.at_end ? 1 : 0;
    out->byte_offset = s_state.current.byte_offset;
    out->line_count = s_state.page.line_count;
    out->page_turns = s_state.page_turns;
    const s3paper::Result<uint32_t> progress =
        s_state.paginator->ProgressPermille(s_state.page.next);
    out->progress_permille = progress.ok() ? progress.value : 0;
    out->checkpoints = s_state.paginator->checkpoint_count();
}

bool ReaderHandleGesture(const s3paper::GestureEvent &gesture) {
    if (!s_state.open) {
        return false;
    }
    switch (gesture.kind) {
        case s3paper::GestureKind::SwipeLeft:
            (void)ReaderNext();
            return true;
        case s3paper::GestureKind::SwipeRight:
            (void)ReaderPrev();
            return true;
        case s3paper::GestureKind::Tap:
            // Right half advances, left half goes back.
            if (gesture.pos.x >= 270) {
                (void)ReaderNext();
            } else {
                (void)ReaderPrev();
            }
            return true;
        default:
            return false;
    }
}

}  // namespace reader
