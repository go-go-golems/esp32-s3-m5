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

static EReaderApp s_app;
EReaderApp& GetApp() { return s_app; }

// ── Sample text for Phase 1 skeleton ──

static const char kSampleText[] =
    "The Deliverator belongs to an elite\n"
    "order, a hallowed sub-category. He's\n"
    "got esprit up to here. Right now, he\n"
    "is preparing to deliver a pizza.\n"
    "\n"
    "His car is a 1994 Pontiac. A pizza\n"
    "delivery vehicle. There are a lot of\n"
    "cars out there that are a lot better\n"
    "than his. The Deliverator does not\n"
    "care. The Deliverator drives what the\n"
    "people like.\n"
    "\n"
    "The DELIVERATOR used to make software.\n"
    "But that field went south, and he had\n"
    "to retool. Now he delivers pizzas.\n"
    "But he is the best in the business.\n"
    "Nobody delivers a pizza faster.\n"
    "\n"
    "The Deliverator stands tall, arms at\n"
    "his side. The pizza box in the back\n"
    "seat is emitting a small amount of\n"
    "heat. The car idles. The pizza has\n"
    "thirty minutes to get there.\n"
    "\n"
    "Twenty-nine minutes and fifty-eight\n"
    "seconds, to be exact. The Deliverator\n"
    "knows this because a small LED timer\n"
    "on the pizza box shows the elapsed\n"
    "time since the order was phoned in.\n"
    "\n"
    "He is going to make it. No problem.\n";

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

void EReaderApp::BuildReadingScreen()
{
    pool_.Reset();
    text_node_ = nullptr;
    page_label_ = nullptr;
    progress_bar_ = nullptr;
    title_label_ = nullptr;
    pct_label_ = nullptr;

    // Status bar
    title_label_ = Label(pool_, "Snow Crash", 1, 0);
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

    screen_.bar = HBox(pool_, {
        Label(pool_, "EREADER//1.0"),
        Spacer(pool_),
        Label(pool_, "1 BOOK", 1, 1),
        Dot(pool_, 24),
    }, 32);
    if (screen_.bar) screen_.bar->border_b = true;

    Node* book_list = List(pool_, 56, 8, 40, 60, 880, 360);
    ListAddRow(book_list, "Snow Crash", 400, 0, "0%", 100, 2);
    ListAddRow(book_list, "Neal Stephenson", 400, 2, "P.1/1", 200, 2);
    book_list->list_selected = 0;

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

void EReaderApp::LoadCurrentPage()
{
    // For now: copy sample text into page buffer
    std::strncpy(page_buffer_, kSampleText, kPageBufSize - 1);
    page_buffer_[kPageBufSize - 1] = '\0';

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
        int pct = total_pages_ > 0 ? (current_page_ * 100 / total_pages_) : 0;
        char buf[8];
        std::snprintf(buf, sizeof(buf), "%d%%", pct);
        std::strncpy(pct_label_->text, buf, kMaxTextLen - 1);
        gnosis::MarkDirty(pct_label_);
    }
    if (progress_bar_) {
        progress_bar_->props[1] = static_cast<int16_t>(
            total_pages_ > 0 ? (current_page_ * 100 / total_pages_) : 0);
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
            // Body touch: page navigation zones
            if (screen_.body && screen_.body->rect.Contains(tx, ty)) {
                int x_pct = (tx - screen_.body->rect.x) * 100 / screen_.body->rect.w;
                if (x_pct < 25) {
                    PreviousPage();
                } else if (x_pct >= 75) {
                    NextPage();
                }
            }
            // Nav bar: check LIBRARY label
            if (screen_.nav && screen_.nav->rect.Contains(tx, ty)) {
                // First child is the LIBRARY label
                if (screen_.nav->n_children > 0 &&
                    screen_.nav->children[0]->rect.Contains(tx, ty)) {
                    SwitchScreen(AppScreen::LIBRARY);
                }
            }
        } else if (current_screen_ == AppScreen::LIBRARY) {
            // Body touch: select book
            if (screen_.body && screen_.body->rect.Contains(tx, ty)) {
                // For now, any tap opens the single book
                SwitchScreen(AppScreen::READING);
            }
            // Nav badge: open reader
            if (screen_.nav && screen_.nav->rect.Contains(tx, ty)) {
                for (int i = 0; i < screen_.nav->n_children; ++i) {
                    Node* c = screen_.nav->children[i];
                    if (c && c->type == gnosis::NodeType::BADGE &&
                        c->rect.Contains(tx, ty)) {
                        SwitchScreen(AppScreen::READING);
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
    std::printf("library:\n  [0] Snow Crash - Neal Stephenson (1 page)\n");
}

void EReaderApp::OpenBook(int index)
{
    if (index != 0) {
        std::printf("only book 0 available\n");
        return;
    }
    SwitchScreen(AppScreen::READING);
    std::printf("opened book 0\n");
}

void EReaderApp::GotoPage(int page)
{
    if (page < 0 || page >= total_pages_) {
        std::printf("page %d out of range (0-%d)\n", page, total_pages_ - 1);
        return;
    }
    current_page_ = page;
    LoadCurrentPage();
    std::printf("jumped to page %d\n", page);
}

void EReaderApp::ShowInfo()
{
    std::printf("book: Snow Crash\npage: %d/%d\nnodes: %zu/%zu\nscreen: %dx%d\n",
                current_page_ + 1, total_pages_, pool_.Used(), kMaxNodes, kScreenW, kScreenH);
}

void EReaderApp::ForceRefresh()
{
    FullRefresh();
    std::printf("full refresh done\n");
}

void EReaderApp::Run()
{
    InitBoard();
    SwitchScreen(AppScreen::READING);

    while (true) {
        M5.update();
        HandleTouch();
        ProcessDirtyRefresh();
        M5.delay(kLoopDelayMs);
    }
}

}  // namespace ereader
