#include "ereader_app.h"
#include "layout_engine.h"
#include "node_builder.h"
#include "widget_renderer.h"

#include <cstdio>
#include <cstring>
#include <esp_timer.h>

namespace ereader {

using namespace gnosis;

static constexpr std::uint32_t kLoopDelayMs = 16;
static constexpr std::uint32_t kFullRefreshEveryN = 60;
static constexpr int kCharsPerLine = 100;  // conservative for size 1 in 896px wide area
static constexpr int kLinesPerPage = 31;

static EReaderApp s_app;
EReaderApp& GetApp() { return s_app; }

void EReaderApp::InitBoard()
{
    auto cfg = M5.config();
    cfg.clear_display = true;
    M5.begin(cfg);
    M5.Display.setRotation(1);
    M5.Display.setTextDatum(top_left);
    M5.Display.setTextFont(2);
    M5.Display.setTextColor(kColorFg, kColorBg);
}

void EReaderApp::MountStorage()
{
    if (!book_store_.Mount()) {
        std::printf("SPIFFS mount failed: %s\n", book_store_.LastStatus());
        return;
    }
    std::printf("%s\n", book_store_.LastStatus());

    if (!book_store_.LoadIndex()) {
        std::printf("index: %s\n", book_store_.LastStatus());
    } else {
        std::printf("index: %s\n", book_store_.LastStatus());
    }

    paginator_.Init(kCharsPerLine, kLinesPerPage);
}

void EReaderApp::BuildReadingScreen()
{
    pool_.Reset();
    text_node_ = nullptr;
    page_label_ = nullptr;
    progress_bar_ = nullptr;
    title_label_ = nullptr;
    pct_label_ = nullptr;

    // Title from current book
    const char* book_title = "No book";
    if (current_book_ >= 0 && current_book_ < book_store_.BookCount()) {
        book_title = book_store_.GetBook(current_book_).title;
    }

    title_label_ = Label(pool_, book_title, 1, 0);
    page_label_ = Label(pool_, "P.1/1", 1, 1);
    pct_label_ = Label(pool_, "0%", 1, 2);

    screen_.bar = HBox(pool_, {
        title_label_,
        Spacer(pool_),
        page_label_,
        Label(pool_, " ", 1, 0, 0, 0, 12),
        pct_label_,
        Dot(pool_, 24),
    }, 32);
    if (screen_.bar) screen_.bar->border_b = true;

    // Body: full-width text block
    text_node_ = TextBlock(pool_, "", 14, 32, 8, 896, 440);
    text_node_->ext_text = page_buffer_;

    screen_.body = Fixed(pool_, { text_node_ });

    // Nav bar
    Node* lib_label = Label(pool_, "LIBRARY", 1, 1);
    progress_bar_ = Bar(pool_, 100, 0, 3, 0, 0, 400);
    Node* bm_label = Label(pool_, "BM", 1, 1);

    screen_.nav = HBox(pool_, {
        lib_label,
        Spacer(pool_),
        progress_bar_,
        Spacer(pool_),
        bm_label,
    }, 32);
    if (screen_.nav) screen_.nav->border_t = true;
}

void EReaderApp::BuildLibraryScreen()
{
    pool_.Reset();
    text_node_ = nullptr;
    page_label_ = nullptr;
    progress_bar_ = nullptr;
    title_label_ = nullptr;
    pct_label_ = nullptr;

    char count_str[16];
    std::snprintf(count_str, sizeof(count_str), "%d BOOK%s",
                  book_store_.BookCount(), book_store_.BookCount() != 1 ? "S" : "");

    screen_.bar = HBox(pool_, {
        Label(pool_, "EREADER//1.0"),
        Spacer(pool_),
        Label(pool_, count_str, 1, 1),
        Dot(pool_, 24),
    }, 32);
    if (screen_.bar) screen_.bar->border_b = true;

    Node* book_list = List(pool_, 56, 8, 40, 60, 880, 360);
    for (int i = 0; i < book_store_.BookCount(); ++i) {
        const BookInfo& book = book_store_.GetBook(i);
        char progress[16];
        int pct = book.total_pages > 0 ? 0 : 0;  // TODO: from bookmarks
        std::snprintf(progress, sizeof(progress), "%d%%", pct);
        ListAddRow(book_list, book.title, 500, 0, progress, 100, 2);

        char detail[48];
        std::snprintf(detail, sizeof(detail), "%s", book.author);
        char pages[24];
        std::snprintf(pages, sizeof(pages), "%d pages", static_cast<int>(book.total_pages));
        ListAddRow(book_list, detail, 500, 2, pages, 200, 2);
    }
    if (current_book_ >= 0) {
        book_list->list_selected = static_cast<int8_t>(current_book_ * 2);
    }

    screen_.body = Fixed(pool_, {
        Label(pool_, "LIBRARY", 2, 0, 40, 16),
        book_list,
    });

    screen_.nav = HBox(pool_, {
        Icon(pool_, 0, 48),
        Icon(pool_, 1, 48),
        Icon(pool_, 2, 48),
        Icon(pool_, 3, 48),
        Spacer(pool_),
        Badge(pool_, "READ", 72),
    }, 32);
    if (screen_.nav) screen_.nav->border_t = true;
}

void EReaderApp::ComputeTotalPages()
{
    if (current_book_ < 0 || current_book_ >= book_store_.BookCount()) {
        total_pages_ = 1;
        return;
    }

    const BookInfo& book = book_store_.GetBook(current_book_);

    // If cached page count exists and is > 0, use it
    if (book.total_pages > 0) {
        total_pages_ = book.total_pages;
        return;
    }

    // Compute by paginating the entire file
    paginator_.Reset();
    int32_t file_size = book.file_size;
    if (file_size <= 0) {
        total_pages_ = 1;
        return;
    }

    // Paginate until we've covered the whole file
    for (int page = 1; page < kMaxPageOffsets; ++page) {
        paginator_.EnsurePage(book_store_, book.filename, page);
        if (paginator_.PagesComputed() <= page) break;  // EOF reached
    }

    total_pages_ = paginator_.PagesComputed() - 1;
    if (total_pages_ < 1) total_pages_ = 1;

    // Cache in index
    book_store_.GetBookMut(current_book_).total_pages = total_pages_;
    book_store_.SaveIndex();
    std::printf("computed %d pages for '%s'\n", total_pages_, book.title);
}

void EReaderApp::LoadCurrentPage()
{
    if (current_book_ < 0 || current_book_ >= book_store_.BookCount()) {
        std::strncpy(page_buffer_, "No book loaded.\nUse 'ereader open 0' or tap a book.", kPageBufSize - 1);
        page_buffer_[kPageBufSize - 1] = '\0';
    } else {
        const BookInfo& book = book_store_.GetBook(current_book_);
        int written = paginator_.GetPageText(book_store_, book.filename, current_page_,
                                              page_buffer_, kPageBufSize);
        if (written <= 0) {
            std::strncpy(page_buffer_, "[end of book]", kPageBufSize - 1);
        }
    }

    if (text_node_) {
        text_node_->ext_text = page_buffer_;
        gnosis::MarkDirty(text_node_);
    }
    UpdateStatusLabels();
}

void EReaderApp::UpdateStatusLabels()
{
    if (page_label_) {
        char buf[24];
        std::snprintf(buf, sizeof(buf), "P.%d/%d", current_page_ + 1, total_pages_);
        std::strncpy(page_label_->text, buf, kMaxTextLen - 1);
        gnosis::MarkDirty(page_label_);
    }
    if (pct_label_) {
        int pct = total_pages_ > 1 ? (current_page_ * 100 / (total_pages_ - 1)) : 0;
        if (pct > 100) pct = 100;
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%d%%", pct);
        std::strncpy(pct_label_->text, buf, kMaxTextLen - 1);
        gnosis::MarkDirty(pct_label_);
    }
    if (progress_bar_) {
        int pct = total_pages_ > 1 ? (current_page_ * 100 / (total_pages_ - 1)) : 0;
        if (pct > 100) pct = 100;
        progress_bar_->props[1] = static_cast<int16_t>(pct);
        gnosis::MarkDirty(progress_bar_);
    }
}

void EReaderApp::NextPage()
{
    if (current_page_ + 1 >= total_pages_) return;
    ++current_page_;
    LoadCurrentPage();
}

void EReaderApp::PreviousPage()
{
    if (current_page_ <= 0) return;
    --current_page_;
    LoadCurrentPage();
}

void EReaderApp::FullRefresh()
{
    M5.Display.waitDisplay();
    M5.Display.setEpdMode(epd_mode_t::epd_quality);
    M5.Display.startWrite();
    M5.Display.fillScreen(kColorBg);

    Rect full = {0, 0, kScreenW, kScreenH};
    RenderSubtree(M5.Display, screen_.bar, full);
    RenderSubtree(M5.Display, screen_.body, full);
    RenderSubtree(M5.Display, screen_.nav, full);

    M5.Display.endWrite();
    M5.Display.waitDisplay();

    auto clear = [](Node* n, auto& self) -> void {
        if (!n) return;
        n->dirty = false;
        for (int i = 0; i < n->n_children; ++i) self(n->children[i], self);
    };
    clear(screen_.bar, clear);
    clear(screen_.body, clear);
    clear(screen_.nav, clear);
    partial_refresh_count_ = 0;
}

void EReaderApp::HandleTouch()
{
    const bool has_touch = M5.Touch.getCount() > 0;

    if (has_touch && !touch_down_) {
        touch_down_ = true;
        const auto& detail = M5.Touch.getDetail();
        int16_t tx = static_cast<int16_t>(detail.x);
        int16_t ty = static_cast<int16_t>(detail.y);

        if (current_screen_ == AppScreen::READING) {
            if (screen_.body && screen_.body->rect.Contains(tx, ty)) {
                int x_pct = (tx - screen_.body->rect.x) * 100 / screen_.body->rect.w;
                if (x_pct < 25) {
                    PreviousPage();
                } else if (x_pct >= 75) {
                    NextPage();
                }
            }
            if (screen_.nav && screen_.nav->rect.Contains(tx, ty)) {
                if (screen_.nav->n_children > 0 &&
                    screen_.nav->children[0]->rect.Contains(tx, ty)) {
                    SwitchScreen(AppScreen::LIBRARY);
                }
            }
        } else if (current_screen_ == AppScreen::LIBRARY) {
            if (screen_.body && screen_.body->rect.Contains(tx, ty)) {
                // Find which book row was tapped
                // The list is the second child of body (after the LIBRARY label)
                if (screen_.body->n_children >= 2) {
                    Node* list_node = screen_.body->children[1];
                    if (list_node && list_node->rect.Contains(tx, ty)) {
                        int16_t row_h = list_node->props[0] > 0 ? list_node->props[0] : 56;
                        int row = (ty - list_node->rect.y) / row_h;
                        int book_idx = row / 2;  // 2 rows per book
                        if (book_idx >= 0 && book_idx < book_store_.BookCount()) {
                            OpenBook(book_idx);
                        }
                    }
                }
            }
            if (screen_.nav && screen_.nav->rect.Contains(tx, ty)) {
                for (int i = 0; i < screen_.nav->n_children; ++i) {
                    Node* c = screen_.nav->children[i];
                    if (c && c->type == gnosis::NodeType::BADGE &&
                        c->rect.Contains(tx, ty)) {
                        if (current_book_ >= 0) {
                            SwitchScreen(AppScreen::READING);
                        }
                    }
                }
            }
        }
    } else if (!has_touch) {
        touch_down_ = false;
    }
}

void EReaderApp::SwitchScreen(AppScreen target)
{
    current_screen_ = target;
    if (target == AppScreen::READING) {
        BuildReadingScreen();
        gnosis::LayoutScreen(screen_, kScreenW, kScreenH);
        LoadCurrentPage();
        FullRefresh();
    } else {
        BuildLibraryScreen();
        gnosis::LayoutScreen(screen_, kScreenW, kScreenH);
        FullRefresh();
    }
}

void EReaderApp::ProcessDirtyRefresh()
{
    gnosis::DirtyCollector dc;
    dc.Collect(screen_.bar);
    dc.Collect(screen_.body);
    dc.Collect(screen_.nav);

    if (dc.count == 0) return;
    dc.Merge();

    for (std::size_t i = 0; i < dc.count; ++i) {
        Rect& r = dc.rects[i];
        epd_mode_t mode = epd_mode_t::epd_text;
        if (dc.waveforms[i] == gnosis::Waveform::FAST)
            mode = epd_mode_t::epd_fast;

        M5.Display.waitDisplay();
        M5.Display.setEpdMode(mode);
        M5.Display.startWrite();
        M5.Display.setClipRect(r.x, r.y, r.w, r.h);
        M5.Display.fillRect(r.x, r.y, r.w, r.h, kColorBg);

        RenderSubtree(M5.Display, screen_.bar, r);
        RenderSubtree(M5.Display, screen_.body, r);
        RenderSubtree(M5.Display, screen_.nav, r);

        M5.Display.clearClipRect();
        M5.Display.endWrite();
    }

    partial_refresh_count_ += dc.count;
    if (partial_refresh_count_ >= kFullRefreshEveryN) {
        FullRefresh();
    }
}

void EReaderApp::ListBooks()
{
    if (book_store_.BookCount() == 0) {
        std::printf("no books loaded\n");
        return;
    }
    std::printf("library (%d books):\n", book_store_.BookCount());
    for (int i = 0; i < book_store_.BookCount(); ++i) {
        const BookInfo& b = book_store_.GetBook(i);
        std::printf("  [%d] %s - %s (%d pages, %d bytes)%s\n",
                    i, b.title, b.author, static_cast<int>(b.total_pages),
                    static_cast<int>(b.file_size),
                    i == current_book_ ? " *" : "");
    }
}

void EReaderApp::OpenBook(int index)
{
    if (index < 0 || index >= book_store_.BookCount()) {
        std::printf("book index %d out of range (0-%d)\n", index, book_store_.BookCount() - 1);
        return;
    }
    current_book_ = index;
    current_page_ = 0;
    paginator_.Reset();
    ComputeTotalPages();
    SwitchScreen(AppScreen::READING);
    std::printf("opened [%d] %s (%d pages)\n", index,
                book_store_.GetBook(index).title, total_pages_);
}

void EReaderApp::GotoPage(int page)
{
    if (current_book_ < 0) {
        std::printf("no book open\n");
        return;
    }
    if (page < 0 || page >= total_pages_) {
        std::printf("page %d out of range (0-%d)\n", page, total_pages_ - 1);
        return;
    }
    current_page_ = page;
    LoadCurrentPage();
    std::printf("jumped to page %d\n", page + 1);
}

void EReaderApp::ShowInfo()
{
    if (current_book_ >= 0 && current_book_ < book_store_.BookCount()) {
        const BookInfo& b = book_store_.GetBook(current_book_);
        std::printf("book: %s by %s\npage: %d/%d\nfile: %s (%d bytes)\nnodes: %zu/%zu\n",
                    b.title, b.author, current_page_ + 1, total_pages_,
                    b.filename, static_cast<int>(b.file_size),
                    pool_.Used(), kMaxNodes);
    } else {
        std::printf("no book open\nnodes: %zu/%zu\n", pool_.Used(), kMaxNodes);
    }
}

void EReaderApp::ForceRefresh()
{
    FullRefresh();
    std::printf("full refresh done\n");
}

void EReaderApp::Run()
{
    InitBoard();
    MountStorage();

    // Open first book if available
    if (book_store_.BookCount() > 0) {
        OpenBook(0);
    } else {
        SwitchScreen(AppScreen::LIBRARY);
    }

    while (true) {
        M5.update();
        HandleTouch();
        ProcessDirtyRefresh();
        M5.delay(kLoopDelayMs);
    }
}

}  // namespace ereader
