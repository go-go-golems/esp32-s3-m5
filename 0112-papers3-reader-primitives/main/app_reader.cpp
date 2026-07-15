#include "app_reader.h"

#include <cstdio>
#include <cstring>

#include "esp_log.h"

#include "app_display.h"
#include "app_reader_book.h"
#include "app_storage.h"
#include "s3paper/content.h"
#include "s3paper/paginator.h"
#include "s3paper/text.h"

namespace reader {
namespace {

const char *kTag = "reader_ctl";

constexpr int32_t kMarginX = 40;
constexpr int32_t kMarginTop = 72;   // leaves room for the header
constexpr int32_t kMarginBottom = 56;  // leaves room for the footer

// Application screens (design task djij). Card-missing and empty-library
// are rendered states of the Library screen rather than separate screens.
enum class Screen : uint8_t {
    None = 0,
    Library,
    Reading,
};

constexpr uint32_t kRegionEmbedded = 0xFFFF;

struct ReaderState {
    Screen screen = Screen::None;
    // Immutable hit regions emitted by the last library render.
    s3paper::HitRegion regions[kMaxBooks + 1];
    uint32_t region_count = 0;
    bool open = false;
    bool from_sd = false;
    bool resumed = false;
    s3paper::MemoryContentSource embedded_source{
        kEmbeddedBookText, sizeof(kEmbeddedBookText) - 1};
    SdContentSource sd_source;
    s3paper::ContentSource *active = nullptr;
    s3paper::ContentHash content_hash = 0;
    char title[40] = {};
    s3paper::Paginator *paginator = nullptr;
    s3paper::TextLocator current{};
    s3paper::PageLayout page{};
    uint32_t page_turns = 0;
};

ReaderState s_state;

s3paper::LayoutKey MakeKey(s3paper::ContentHash content) {
    s3paper::LayoutKey key{};
    key.content = content;
    key.font_id = s3paper::kFontBody;
    key.viewport_w = 540;
    key.viewport_h = 960;
    key.margin_x = kMarginX;
    key.margin_top = kMarginTop;
    key.margin_bottom = kMarginBottom;
    key.engine_version = s3paper::kLayoutEngineVersion;
    return key;
}

// Records the current position (embedded book included); the storage
// layer coalesces the actual SD writes (flush-if-due / flush-now).
void PersistPosition() {
    if (!s_state.open || !StorageMounted()) {
        return;
    }
    PositionStore(s_state.content_hash, s_state.current);
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
                     s_state.title,
                     static_cast<uint32_t>(strlen(s_state.title)), 0);
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
        const s3paper::Result<uint32_t> got = s_state.active->ReadAt(
            line.byte_start, reinterpret_cast<uint8_t *>(buf), len);
        if (!got.ok()) return got.code;
        st = fb.GlyphRun(
            s3paper::Rect{kMarginX, line.baseline_y - body.ascent,
                          line.width, body.line_height},
            line.baseline_y, s3paper::kFontBody, 0, buf, got.value, 0);
        if (!st.ok()) return st.code;
    }
    // Bookmark indicator at the right edge of the header.
    if (StorageMounted() &&
        BookmarkIsSet(s_state.content_hash, s_state.current.byte_offset)) {
        st = fb.GlyphRun(s3paper::Rect{540 - kMarginX - 24, 16, 24,
                                       ui.line_height + 4},
                         16 + ui.line_height, s3paper::kFontUi, 0, "*", 1, 0);
        if (!st.ok()) return st.code;
    }
    // Footer: progress + page-turn count + bookmark count.
    const s3paper::Result<uint32_t> progress =
        s_state.paginator->ProgressPermille(s_state.page.next);
    const uint32_t marks = BookmarkCountFor(s_state.content_hash);
    char footer[80];
    int footer_len = snprintf(
        footer, sizeof(footer), "%u%%%s   turns %u",
        progress.ok() ? static_cast<unsigned>(progress.value / 10) : 0,
        s_state.page.at_end ? " (end)" : "",
        static_cast<unsigned>(s_state.page_turns));
    if (marks > 0 && footer_len > 0 &&
        footer_len < static_cast<int>(sizeof(footer))) {
        footer_len += snprintf(footer + footer_len,
                               sizeof(footer) - footer_len, "   marks %u",
                               static_cast<unsigned>(marks));
    }
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

// Shared open path: builds a fresh paginator over `source`, restores a
// persisted position when one validates, and renders the first page.
// sd_path is the VFS path for SD books, nullptr for the embedded book.
StatusCode OpenCommon(s3paper::ContentSource *source, const char *title,
                      bool from_sd, const char *sd_path = nullptr) {
    const s3paper::Result<s3paper::ContentHash> hash = source->Hash();
    if (!hash.ok()) {
        return hash.code;
    }
    delete s_state.paginator;
    s_state.paginator = new s3paper::Paginator(source, MakeKey(hash.value));
    s_state.active = source;
    s_state.content_hash = hash.value;
    s_state.from_sd = from_sd;
    s_state.resumed = false;
    snprintf(s_state.title, sizeof(s_state.title), "%s", title);

    s3paper::TextLocator start;
    s3paper::TextLocator persisted;
    if (PositionLookup(hash.value, &persisted) &&
        s_state.paginator->Validate(persisted).ok()) {
        start = persisted;
        s_state.resumed = true;
        ESP_LOGI(kTag, "resuming \"%s\" at offset %llu", s_state.title,
                 static_cast<unsigned long long>(persisted.byte_offset));
    } else {
        const s3paper::Result<s3paper::TextLocator> begin =
            s_state.paginator->Begin();
        if (!begin.ok()) {
            return begin.code;
        }
        start = begin.value;
    }
    s_state.open = true;
    s_state.page_turns = 0;
    Planner().NoteScreenChange();  // opening a book is a screen change
    const StatusCode rendered = ComposeAndRender(start);
    if (rendered == StatusCode::Ok) {
        s_state.screen = Screen::Reading;
        LastBookStore(sd_path != nullptr ? sd_path : "");
    }
    return rendered;
}

// Percent read of a book from its persisted position (0 when unknown).
uint32_t PersistedPercent(s3paper::ContentHash content, uint64_t size) {
    s3paper::TextLocator loc;
    if (size == 0 || !PositionLookup(content, &loc)) {
        return 0;
    }
    const uint64_t clamped = loc.byte_offset > size ? size : loc.byte_offset;
    return static_cast<uint32_t>((clamped * 100) / size);
}

StatusCode ReaderOpen() {
    return OpenCommon(&s_state.embedded_source, kEmbeddedBookTitle, false);
}

StatusCode LibraryShow() {
    s3paper::FrameBuilder &fb = FrameBuilderRef();
    const s3paper::FontLineMetrics ui =
        s3paper::GetFontLineMetrics(s3paper::kFontUi).value;
    const s3paper::FontLineMetrics body =
        s3paper::GetFontLineMetrics(s3paper::kFontBody).value;
    fb.Begin();
    s3paper::Status st = fb.FillRect(s3paper::Rect{0, 0, 540, 960}, 255);
    if (!st.ok()) return st.code;
    static const char kHeader[] = "Library";
    st = fb.GlyphRun(s3paper::Rect{kMarginX, 16, 460, ui.line_height + 4},
                     16 + ui.line_height, s3paper::kFontUi, 0, kHeader,
                     sizeof(kHeader) - 1, 0);
    if (!st.ok()) return st.code;
    st = fb.HLine(kMarginX, 56, 540 - 2 * kMarginX, 0);
    if (!st.ok()) return st.code;

    s_state.region_count = 0;
    const int32_t row_height = body.line_height + 20;
    int32_t row_top = 76;
    char row[96];

    // Row renderer shared by the embedded book and SD books.
    const auto add_row = [&](const char *text, uint32_t region_id,
                             bool dark) -> s3paper::Status {
        const int32_t baseline = row_top + body.ascent + 8;
        s3paper::Status row_st = fb.GlyphRun(
            s3paper::Rect{kMarginX, row_top, 540 - 2 * kMarginX,
                          row_height},
            baseline, s3paper::kFontBody, 0, text,
            static_cast<uint32_t>(strlen(text)), dark ? 0 : 96);
        if (!row_st.ok()) return row_st;
        row_st = fb.HLine(kMarginX, row_top + row_height - 2,
                          540 - 2 * kMarginX, 176);
        if (!row_st.ok()) return row_st;
        if (region_id != UINT32_MAX &&
            s_state.region_count < kMaxBooks + 1) {
            s_state.regions[s_state.region_count++] = s3paper::HitRegion{
                s3paper::Rect{0, row_top - 4, 540, row_height + 8},
                region_id, 1};
        }
        row_top += row_height;
        return s3paper::OkStatus();
    };

    // Embedded book is always available, even without a card.
    const s3paper::ContentHash embedded_hash =
        s_state.embedded_source.Hash().value;
    snprintf(row, sizeof(row), "%s (embedded) %u%%", kEmbeddedBookTitle,
             static_cast<unsigned>(PersistedPercent(
                 embedded_hash, sizeof(kEmbeddedBookText) - 1)));
    st = add_row(row, kRegionEmbedded, true);
    if (!st.ok()) return st.code;

    if (!StorageMounted()) {
        st = add_row("(no SD card - use 'sd mount')", UINT32_MAX, false);
        if (!st.ok()) return st.code;
    } else if (LibraryCount() == 0) {
        st = add_row("(no .txt books on card)", UINT32_MAX, false);
        if (!st.ok()) return st.code;
    } else {
        for (uint32_t i = 0; i < LibraryCount(); ++i) {
            if (row_top + row_height > 940) {
                break;  // bounded: first screenful (scroll arrives later)
            }
            const BookEntry *book = LibraryGet(i);
            snprintf(row, sizeof(row), "%s  %uKB %u%%", book->title,
                     static_cast<unsigned>(book->size / 1024),
                     static_cast<unsigned>(
                         PersistedPercent(book->content_hash, book->size)));
            st = add_row(row, i, true);
            if (!st.ok()) return st.code;
        }
    }

    static const char kHint[] = "tap a book to read";
    st = fb.GlyphRun(s3paper::Rect{kMarginX, 960 - 40, 460, 32}, 960 - 14,
                     s3paper::kFontUi, 0, kHint, sizeof(kHint) - 1, 96);
    if (!st.ok()) return st.code;

    const s3paper::Result<s3paper::RenderFrame> frame = FinishFrame();
    if (!frame.ok()) return frame.code;
    const s3paper::Status init = EnsureM5Init();
    if (!init.ok()) return init.code;
    Planner().NoteScreenChange();
    const PlannedPresent presented = PresentFramePlanned(
        frame.value, s3paper::PresentIntent::CleanFull, true);
    if (presented.present.status == StatusCode::Ok) {
        s_state.screen = Screen::Library;
        StorageFlushNow();  // leaving reading: persist immediately
        ESP_LOGI(kTag, "library screen: %u row region(s)",
                 static_cast<unsigned>(s_state.region_count));
    }
    return presented.present.status;
}

StatusCode ReaderBookmarkToggle() {
    if (!s_state.open) {
        return StatusCode::Busy;
    }
    if (!StorageMounted()) {
        return StatusCode::Busy;  // bookmarks live on the card
    }
    bool now_set = false;
    const StatusCode result =
        BookmarkToggle(s_state.content_hash, s_state.current, &now_set);
    if (result != StatusCode::Ok) {
        return result;
    }
    ESP_LOGI(kTag, "bookmark %s at offset %llu",
             now_set ? "set" : "removed",
             static_cast<unsigned long long>(s_state.current.byte_offset));
    // Re-render so the header indicator reflects the change.
    return RenderCurrentPage();
}

StatusCode ReaderBookmarkGoto(uint32_t index) {
    if (!s_state.open) {
        return StatusCode::Busy;
    }
    s3paper::TextLocator mark;
    if (!BookmarkGet(s_state.content_hash, index, &mark)) {
        return StatusCode::InvalidArgument;
    }
    if (!s_state.paginator->Validate(mark).ok()) {
        return StatusCode::CorruptData;  // content changed under the mark
    }
    s_state.page_turns++;
    const StatusCode result = ComposeAndRender(mark);
    if (result == StatusCode::Ok) {
        PersistPosition();
    }
    return result;
}

void ReaderBookmarksPrint() {
    if (!s_state.open) {
        printf("bookmarks: no book open\n");
        return;
    }
    BookmarksPrint(s_state.content_hash);
}

StatusCode ReaderOpenSd(uint32_t index) {
    const BookEntry *book = LibraryGet(index);
    if (book == nullptr) {
        return StatusCode::InvalidArgument;
    }
    const StatusCode opened = s_state.sd_source.Open(book->path);
    if (opened != StatusCode::Ok) {
        return opened;
    }
    const StatusCode result =
        OpenCommon(&s_state.sd_source, book->title, true, book->path);
    if (result != StatusCode::Ok) {
        s_state.sd_source.Close();
    }
    return result;
}

StatusCode ReaderBootRestore() {
    // Best-effort boot flow: mount, scan, reopen the last book (position
    // restore happens inside OpenCommon), else show the library.
    (void)StorageMount();
    if (StorageMounted()) {
        (void)LibraryScan(nullptr);
        char last[96];
        if (LastBookGet(last, sizeof(last))) {
            if (last[0] == '\0') {
                if (ReaderOpen() == StatusCode::Ok) {
                    return StatusCode::Ok;
                }
            } else {
                for (uint32_t i = 0; i < LibraryCount(); ++i) {
                    if (strcmp(LibraryGet(i)->path, last) == 0) {
                        if (ReaderOpenSd(i) == StatusCode::Ok) {
                            return StatusCode::Ok;
                        }
                        break;
                    }
                }
                ESP_LOGW(kTag, "last book \"%s\" not restorable", last);
            }
        }
    }
    return LibraryShow();
}

StatusCode ReaderNext() {
    if (!s_state.open) {
        return StatusCode::Busy;
    }
    if (s_state.page.at_end) {
        return StatusCode::InvalidArgument;  // explicit end-of-content
    }
    s_state.page_turns++;
    const StatusCode result = ComposeAndRender(s_state.page.next);
    if (result == StatusCode::Ok) {
        PersistPosition();
    }
    return result;
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
    const StatusCode result = ComposeAndRender(prev.value);
    if (result == StatusCode::Ok) {
        PersistPosition();
    }
    return result;
}

void FillReaderSnapshot(ReaderSnapshot *out) {
    std::memset(out, 0, sizeof(*out));
    out->open = s_state.open ? 1 : 0;
    if (!s_state.open) {
        return;
    }
    out->screen = static_cast<uint8_t>(s_state.screen);
    out->at_end = s_state.page.at_end ? 1 : 0;
    out->source = s_state.from_sd ? 1 : 0;
    out->resumed = s_state.resumed ? 1 : 0;
    out->bookmarked = StorageMounted() &&
                              BookmarkIsSet(s_state.content_hash,
                                            s_state.current.byte_offset)
                          ? 1
                          : 0;
    out->bookmark_count = BookmarkCountFor(s_state.content_hash);
    snprintf(out->title, sizeof(out->title), "%s", s_state.title);
    out->byte_offset = s_state.current.byte_offset;
    out->line_count = s_state.page.line_count;
    out->page_turns = s_state.page_turns;
    const s3paper::Result<uint32_t> progress =
        s_state.paginator->ProgressPermille(s_state.page.next);
    out->progress_permille = progress.ok() ? progress.value : 0;
    out->checkpoints = s_state.paginator->checkpoint_count();
}

bool ReaderHandleGesture(const s3paper::GestureEvent &gesture) {
    if (s_state.screen == Screen::Library) {
        if (gesture.kind != s3paper::GestureKind::Tap) {
            return false;
        }
        const s3paper::Result<uint32_t> hit = s3paper::HitTest(
            s_state.regions, s_state.region_count, gesture.pos);
        if (!hit.ok()) {
            return true;  // consumed: tap on empty library space
        }
        if (hit.value == kRegionEmbedded) {
            (void)ReaderOpen();
        } else {
            (void)ReaderOpenSd(hit.value);
        }
        return true;
    }
    if (s_state.screen != Screen::Reading || !s_state.open) {
        return false;
    }
    switch (gesture.kind) {
        case s3paper::GestureKind::SwipeLeft:
            (void)ReaderNext();
            return true;
        case s3paper::GestureKind::SwipeRight:
            (void)ReaderPrev();
            return true;
        case s3paper::GestureKind::SwipeDown:
            (void)LibraryShow();  // back navigation
            return true;
        case s3paper::GestureKind::LongPress:
            (void)ReaderBookmarkToggle();
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
