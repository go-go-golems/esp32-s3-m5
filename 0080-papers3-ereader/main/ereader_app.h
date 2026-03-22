#pragma once

#include "dirty_tracker.h"
#include "gnosis_types.h"

#include <M5Unified.hpp>
#include <cstdint>

namespace ereader {

using gnosis::Node;
using gnosis::NodePool;
using gnosis::Rect;
using gnosis::Screen;

static constexpr int16_t kScreenW = 960;
static constexpr int16_t kScreenH = 540;
static constexpr int kPageBufSize = 4096;

enum class AppScreen {
    LIBRARY,
    READING,
};

class EReaderApp {
public:
    void Run();

    // Console access
    void ListBooks();
    void OpenBook(int index);
    void GotoPage(int page);
    void ShowInfo();
    void ForceRefresh();

    NodePool& Pool() { return pool_; }

private:
    void InitBoard();
    void BuildLibraryScreen();
    void BuildReadingScreen();
    void FullRefresh();
    void HandleTouch();
    void ProcessDirtyRefresh();

    void NextPage();
    void PreviousPage();
    void LoadCurrentPage();
    void UpdateStatusLabels();
    void SwitchScreen(AppScreen target);

    NodePool pool_;
    Screen screen_{};
    AppScreen current_screen_ = AppScreen::READING;

    bool touch_down_ = false;
    std::uint32_t partial_refresh_count_ = 0;

    // Page content
    char page_buffer_[kPageBufSize]{};

    // Reading state
    int current_page_ = 0;
    int total_pages_ = 1;

    // Node pointers for dynamic updates
    Node* text_node_ = nullptr;
    Node* page_label_ = nullptr;
    Node* progress_bar_ = nullptr;
    Node* title_label_ = nullptr;
    Node* pct_label_ = nullptr;
};

EReaderApp& GetApp();

}  // namespace ereader
