#pragma once

extern "C" {
#include "lvgl.h"
}

enum class DemoId {
    Menu = 0,
    Basics = 1,
    Pomodoro = 2,
};

struct DemoManager {
    lv_indev_t *kb_indev = nullptr;
    lv_group_t *group = nullptr;
    DemoId active = DemoId::Menu;
    uint32_t last_key = 0;
};

// Initializes global LVGL focus behavior for demos (group, indev binding, etc).
void demo_manager_init(DemoManager *mgr, lv_indev_t *kb_indev);

// Loads a demo (creates a new screen and swaps it in).
void demo_manager_load(DemoManager *mgr, DemoId id);

// Convenience used by the app loop to treat Esc as "back to menu".
bool demo_manager_handle_global_key(DemoManager *mgr, uint32_t key);
