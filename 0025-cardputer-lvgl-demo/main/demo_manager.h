#pragma once

#include <string>

extern "C" {
#include "lvgl.h"
}

enum class DemoId {
    Menu = 0,
    Basics = 1,
    Pomodoro = 2,
    SplitConsole = 3,
    SystemMonitor = 4,
    FileBrowser = 5,
    FileViewer = 6,
};

struct DemoManager {
    lv_indev_t *kb_indev = nullptr;
    lv_group_t *group = nullptr;
    DemoId active = DemoId::Menu;
    uint32_t last_key = 0;
    int pomodoro_minutes = 25;
    std::string file_browser_cwd{};
    std::string file_viewer_path{};
};

// Initializes global LVGL focus behavior for demos (group, indev binding, etc).
void demo_manager_init(DemoManager *mgr, lv_indev_t *kb_indev);

// Loads a demo (creates a new screen and swaps it in).
void demo_manager_load(DemoManager *mgr, DemoId id);

// Convenience used by the app loop to treat Esc as "back to menu".
bool demo_manager_handle_global_key(DemoManager *mgr, uint32_t key);

// Update the default Pomodoro duration and apply it if the screen is active.
void demo_manager_pomodoro_set_minutes(DemoManager *mgr, int minutes);
