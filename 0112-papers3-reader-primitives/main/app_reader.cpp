#include "app_reader.h"

#include <cstdio>
#include <cstring>

#include "esp_log.h"

#include "app_display.h"
#include "app_reader_book.h"
#include "app_storage.h"
#include "app_ui.h"
#include "s3paper/content.h"
#include "s3paper/page.h"
#include "s3paper/paginator.h"
#include "s3paper/text.h"
#include "s3paper/widget.h"

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

// ---- Phase 9: both screens are retained widget pages (design §5).
// The reading page keeps its tree across page turns (SetText updates the
// chrome); the library rebuilds per show because its rows change.

struct ReadingUi {
    s3paper::WidgetHandle title{};
    s3paper::WidgetHandle star{};
    s3paper::WidgetHandle footer_text{};
    s3paper::WidgetHandle book{};
    s3paper::PageSlots slots{};
};

ReadingUi s_reading_ui;
s3paper::PageId s_page_reading = s3paper::kNoPage;
s3paper::PageId s_page_library = s3paper::kNoPage;

void EnsurePagesRegistered() {
    if (s_page_reading != s3paper::kNoPage) {
        return;
    }
    s3paper::PageRouter &router = UiRouter();
    s_page_reading =
        router.Register("reading", s3paper::PageSlots{}).value;
    s_page_library =
        router.Register("library", s3paper::PageSlots{}).value;
}

// Route transition helper: prefer Back when it lands on `page` (bounded
// stack), else Push. Every transition presents with a screen-change full.
void RouteTo(s3paper::PageId page) {
    s3paper::PageRouter &router = UiRouter();
    const s3paper::Result<s3paper::PageId> current = router.Current();
    if (current.ok() && current.value == page) {
        return;
    }
    if (router.stack_depth() >= 2) {
        const s3paper::Result<s3paper::PageId> back = router.Back();
        if (back.ok() && back.value == page) {
            return;
        }
        if (back.ok()) {
            (void)router.Push(current.value);  // undo: wrong destination
        }
    }
    if (!router.Push(page).ok()) {
        // Bounded stack exhausted by odd navigation: reset to this page.
        ESP_LOGW(kTag, "route stack full; keeping current page");
    }
}

// Extra draw ops after widget compilation: the book body lines, using the
// paginator's absolute baselines (LayoutKey margins are unchanged from
// Phase 8, so persisted positions and goldens stay valid).
StatusCode ComposeBodyOps(s3paper::FrameBuilder &fb,
                          const s3paper::LayoutEntry *entries,
                          uint32_t entry_count) {
    (void)entries;
    (void)entry_count;
    const s3paper::FontLineMetrics body =
        s3paper::GetFontLineMetrics(s3paper::kFontBody).value;
    for (uint32_t i = 0; i < s_state.page.line_count; ++i) {
        const s3paper::PageLine &line = s_state.page.lines[i];
        char buf[256];
        const uint32_t len = line.byte_len < sizeof(buf)
                                 ? line.byte_len
                                 : static_cast<uint32_t>(sizeof(buf));
        const s3paper::Result<uint32_t> got = s_state.active->ReadAt(
            line.byte_start, reinterpret_cast<uint8_t *>(buf), len);
        if (!got.ok()) return got.code;
        const s3paper::Status st = fb.GlyphRun(
            s3paper::Rect{kMarginX, line.baseline_y - body.ascent,
                          line.width, body.line_height},
            line.baseline_y, s3paper::kFontBody, 0, buf, got.value, 0);
        if (!st.ok()) return st.code;
    }
    return StatusCode::Ok;
}

// Builds the retained reading page: header row (title + bookmark star),
// Book content node reserving the body, footer (divider + status line).
StatusCode BuildReadingTree() {
    s3paper::WidgetArena &a = UiArena();
    a.Reset();

    s3paper::WidgetHandle header = s3paper::NewCol(a).value;
    s3paper::WidgetNode *hn = a.Configure(header);
    if (hn == nullptr) {
        return StatusCode::CapacityExceeded;
    }
    hn->padding = s3paper::Insets{16, kMarginX, 4, kMarginX};
    hn->gap = 8;
    s3paper::WidgetHandle title_row = s3paper::NewRow(a).value;
    s_reading_ui.title = s3paper::NewText(a, "", s3paper::kFontUi, 0).value;
    a.Configure(s_reading_ui.title)->flex = 1;
    s_reading_ui.star = s3paper::NewText(a, "", s3paper::kFontUi, 0,
                                         s3paper::TextAlign::End)
                            .value;
    a.Configure(s_reading_ui.star)->fixed_w = 24;
    (void)a.AddChild(title_row, s_reading_ui.title);
    (void)a.AddChild(title_row, s_reading_ui.star);
    (void)a.AddChild(header, title_row);
    (void)a.AddChild(header, s3paper::NewDivider(a, 1, 0).value);

    s_reading_ui.book = s3paper::NewBook(a, 1).value;

    s3paper::WidgetHandle footer = s3paper::NewCol(a).value;
    s3paper::WidgetNode *fn = a.Configure(footer);
    fn->padding = s3paper::Insets{4, kMarginX, 10, kMarginX};
    fn->gap = 6;
    (void)a.AddChild(footer, s3paper::NewDivider(a, 1, 0).value);
    s_reading_ui.footer_text =
        s3paper::NewText(a, "", s3paper::kFontUi, 0).value;
    (void)a.AddChild(footer, s_reading_ui.footer_text);

    s_reading_ui.slots = s3paper::PageSlots{header, s_reading_ui.book,
                                            footer, s3paper::kNullWidget};
    EnsurePagesRegistered();
    (void)UiRouter().SetSlots(s_page_reading, s_reading_ui.slots);
    return StatusCode::Ok;
}

// Renders the current page through the generic widget pipeline.
StatusCode RenderCurrentPage() {
    s3paper::WidgetArena &a = UiArena();
    // Rebuild when never built or when another screen reset the arena.
    if (a.Get(s_reading_ui.title) == nullptr) {
        const StatusCode built = BuildReadingTree();
        if (built != StatusCode::Ok) {
            return built;
        }
    }
    (void)a.SetText(s_reading_ui.title, s_state.title);
    const bool marked =
        StorageMounted() &&
        BookmarkIsSet(s_state.content_hash, s_state.current.byte_offset);
    (void)a.SetText(s_reading_ui.star, marked ? "*" : "");
    const s3paper::Result<uint32_t> progress =
        s_state.paginator->ProgressPermille(s_state.page.next);
    const uint32_t marks = BookmarkCountFor(s_state.content_hash);
    char footer[s3paper::TextProps::kCapacity];
    int footer_len = snprintf(
        footer, sizeof(footer), "%u%%%s   turns %u",
        progress.ok() ? static_cast<unsigned>(progress.value / 10) : 0,
        s_state.page.at_end ? " (end)" : "",
        static_cast<unsigned>(s_state.page_turns));
    if (marks > 0 && footer_len > 0 &&
        footer_len < static_cast<int>(sizeof(footer))) {
        snprintf(footer + footer_len, sizeof(footer) - footer_len,
                 "   marks %u", static_cast<unsigned>(marks));
    }
    (void)a.SetText(s_reading_ui.footer_text, footer);

    const UiPresentResult presented =
        UiPresentPage(s_reading_ui.slots, s3paper::PresentIntent::TextPage,
                      false, nullptr, 0, &ComposeBodyOps);
    if (presented.status == StatusCode::Ok) {
        RouteTo(s_page_reading);
        ESP_LOGI(kTag, "page at %llu lines=%u full=%d",
                 static_cast<unsigned long long>(
                     s_state.current.byte_offset),
                 static_cast<unsigned>(s_state.page.line_count),
                 presented.full_refresh ? 1 : 0);
    }
    return presented.status;
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

// Library rows are widgets with hit ids: embedded = kRegionEmbedded, SD
// book i = i + 1 (hit id 0 means "not tappable", so indices shift by one).
StatusCode AddLibraryRow(s3paper::WidgetArena &a, s3paper::WidgetHandle list,
                         const char *text, uint32_t hit_id, bool dark) {
    const s3paper::Result<s3paper::WidgetHandle> row = s3paper::NewCol(a);
    if (!row.ok()) {
        return row.code;
    }
    s3paper::WidgetNode *rn = a.Configure(row.value);
    rn->padding = s3paper::Insets{10, kMarginX, 8, kMarginX};
    rn->gap = 8;
    if (hit_id != 0) {
        rn->hit_id = hit_id;
        rn->hit_z = 1;
    }
    const s3paper::Result<s3paper::WidgetHandle> label =
        s3paper::NewText(a, text, s3paper::kFontBody,
                         dark ? 0 : 96);
    if (!label.ok()) {
        return label.code;
    }
    (void)a.AddChild(row.value, label.value);
    const s3paper::Result<s3paper::WidgetHandle> rule =
        s3paper::NewDivider(a, 1, 176);
    if (!rule.ok()) {
        return rule.code;
    }
    (void)a.AddChild(row.value, rule.value);
    (void)a.AddChild(list, row.value);
    return StatusCode::Ok;
}

StatusCode LibraryShow() {
    s3paper::WidgetArena &a = UiArena();
    a.Reset();  // the reading tree rebuilds on next render

    s3paper::WidgetHandle header = s3paper::NewCol(a).value;
    s3paper::WidgetNode *hn = a.Configure(header);
    if (hn == nullptr) {
        return StatusCode::CapacityExceeded;
    }
    hn->padding = s3paper::Insets{16, kMarginX, 4, kMarginX};
    hn->gap = 8;
    (void)a.AddChild(header,
                     s3paper::NewText(a, "Library", s3paper::kFontUi, 0)
                         .value);
    (void)a.AddChild(header, s3paper::NewDivider(a, 1, 0).value);

    s3paper::WidgetHandle list = s3paper::NewList(a).value;
    a.Configure(list)->padding = s3paper::Insets{12, 0, 0, 0};

    char row[s3paper::TextProps::kCapacity];
    const s3paper::ContentHash embedded_hash =
        s_state.embedded_source.Hash().value;
    snprintf(row, sizeof(row), "%s (embedded) %u%%", kEmbeddedBookTitle,
             static_cast<unsigned>(PersistedPercent(
                 embedded_hash, sizeof(kEmbeddedBookText) - 1)));
    StatusCode st = AddLibraryRow(a, list, row, kRegionEmbedded, true);
    if (st != StatusCode::Ok) return st;

    if (!StorageMounted()) {
        st = AddLibraryRow(a, list, "(no SD card - use 'sd mount')", 0,
                           false);
        if (st != StatusCode::Ok) return st;
    } else if (LibraryCount() == 0) {
        st = AddLibraryRow(a, list, "(no .txt books on card)", 0, false);
        if (st != StatusCode::Ok) return st;
    } else {
        for (uint32_t i = 0; i < LibraryCount(); ++i) {
            const BookEntry *book = LibraryGet(i);
            snprintf(row, sizeof(row), "%s  %uKB %u%%", book->title,
                     static_cast<unsigned>(book->size / 1024),
                     static_cast<unsigned>(
                         PersistedPercent(book->content_hash, book->size)));
            st = AddLibraryRow(a, list, row, i + 1, true);
            if (st == StatusCode::CapacityExceeded) {
                ESP_LOGW(kTag, "library arena full after %u rows",
                         static_cast<unsigned>(i));
                break;  // bounded: List paginates what was built
            }
            if (st != StatusCode::Ok) return st;
        }
    }

    s3paper::WidgetHandle footer = s3paper::NewCol(a).value;
    s3paper::WidgetNode *fn = a.Configure(footer);
    fn->padding = s3paper::Insets{4, kMarginX, 10, kMarginX};
    (void)a.AddChild(footer, s3paper::NewText(a, "tap a book to read",
                                              s3paper::kFontUi, 96)
                                 .value);

    EnsurePagesRegistered();
    const s3paper::PageSlots slots{header, list, footer,
                                   s3paper::kNullWidget};
    (void)UiRouter().SetSlots(s_page_library, slots);
    const UiPresentResult presented = UiPresentPage(
        slots, s3paper::PresentIntent::CleanFull, true, s_state.regions,
        kMaxBooks + 1, nullptr);
    if (presented.status == StatusCode::Ok) {
        s_state.region_count = presented.hit_count;
        s_state.screen = Screen::Library;
        RouteTo(s_page_library);
        StorageFlushNow();  // leaving reading: persist immediately
        ESP_LOGI(kTag, "library screen: %u row region(s)",
                 static_cast<unsigned>(s_state.region_count));
    }
    return presented.status;
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

void ReaderFormatLibraryLine(uint32_t index, char *out, uint32_t out_size) {
    if (index == 0xFFFFFFFF) {
        const s3paper::ContentHash embedded_hash =
            s_state.embedded_source.Hash().value;
        snprintf(out, out_size, "%s (embedded) %u%%", kEmbeddedBookTitle,
                 static_cast<unsigned>(PersistedPercent(
                     embedded_hash, sizeof(kEmbeddedBookText) - 1)));
        return;
    }
    const BookEntry *book = LibraryGet(index);
    if (book == nullptr) {
        snprintf(out, out_size, "(no book %u)", static_cast<unsigned>(index));
        return;
    }
    snprintf(out, out_size, "%s  %uKB %u%%", book->title,
             static_cast<unsigned>(book->size / 1024),
             static_cast<unsigned>(
                 PersistedPercent(book->content_hash, book->size)));
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
            (void)ReaderOpenSd(hit.value - 1);  // widget hit ids are 1-based
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
