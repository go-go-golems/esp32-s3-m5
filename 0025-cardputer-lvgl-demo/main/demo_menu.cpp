#include "demo_manager.h"

#include <algorithm>

#include "lvgl_font_util.h"

namespace {

static constexpr int kItemCount = 5;

struct MenuState {
    DemoManager *mgr = nullptr;
    lv_obj_t *root = nullptr;
    lv_obj_t *rows[kItemCount] = {nullptr};
    lv_obj_t *labels[kItemCount] = {nullptr};
    int selected = 0;
};

static MenuState s_menu;

static void apply_row_style(MenuState *st, int idx, bool selected) {
    if (!st || idx < 0 || idx >= kItemCount) return;
    lv_obj_t *row = st->rows[idx];
    lv_obj_t *label = st->labels[idx];
    if (!row || !label) return;

    if (selected) {
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(row, lv_palette_main(LV_PALETTE_GREEN), 0);
        lv_obj_set_style_text_color(label, lv_color_black(), 0);
    } else {
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_text_color(label, lv_palette_main(LV_PALETTE_GREEN), 0);
    }
}

static void refresh(MenuState *st) {
    if (!st) return;
    st->selected = std::clamp(st->selected, 0, kItemCount - 1);
    for (int i = 0; i < kItemCount; i++) {
        apply_row_style(st, i, st->selected == i);
    }
}

static void open_selected(MenuState *st) {
    if (!st || !st->mgr) return;
    if (st->selected == 0) {
        demo_manager_load(st->mgr, DemoId::Basics);
    } else if (st->selected == 1) {
        demo_manager_load(st->mgr, DemoId::Pomodoro);
    } else if (st->selected == 2) {
        demo_manager_load(st->mgr, DemoId::SplitConsole);
    } else if (st->selected == 3) {
        demo_manager_load(st->mgr, DemoId::SystemMonitor);
    } else {
        demo_manager_load(st->mgr, DemoId::FileBrowser);
    }
}

static void key_cb(lv_event_t *e) {
    auto *st = static_cast<MenuState *>(lv_event_get_user_data(e));
    const uint32_t key = lv_event_get_key(e);

    if (key == LV_KEY_UP) {
        st->selected = (st->selected + kItemCount - 1) % kItemCount;
        refresh(st);
        return;
    }
    if (key == LV_KEY_DOWN) {
        st->selected = (st->selected + 1) % kItemCount;
        refresh(st);
        return;
    }
    if (key == LV_KEY_ENTER || key == (uint32_t)' ') {
        open_selected(st);
        return;
    }

    // Allow "Esc/back" to re-open the selected demo (useful muscle memory).
    if (key == LV_KEY_ESC) {
        open_selected(st);
        return;
    }
}

} // namespace

lv_obj_t *demo_menu_create(DemoManager *mgr) {
    s_menu = MenuState{};
    s_menu.mgr = mgr;
    s_menu.selected = 0;

    s_menu.root = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(s_menu.root, lv_color_black(), 0);
    lv_obj_set_style_text_color(s_menu.root, lv_palette_main(LV_PALETTE_GREEN), 0);

    lv_obj_t *title = lv_label_create(s_menu.root);
    lv_label_set_text(title, "LVGL Demos");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    // Menu rows.
    static const char *kItems[kItemCount] = {"Basics", "Pomodoro", "Console", "SysMon", "Files"};
    for (int i = 0; i < kItemCount; i++) {
        lv_obj_t *row = lv_obj_create(s_menu.root);
        s_menu.rows[i] = row;

        lv_obj_set_size(row, 240 - 16, 18);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_radius(row, 6, 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);

        lv_obj_align(row, LV_ALIGN_TOP_MID, 0, 22 + i * 20);

        lv_obj_t *label = lv_label_create(row);
        s_menu.labels[i] = label;
        lv_label_set_text(label, kItems[i]);
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 8, 0);
    }

    lv_obj_t *hint = lv_label_create(s_menu.root);
    lv_label_set_text(hint, "Up/Down select  Enter open");
    lv_obj_set_style_text_font(hint, lvgl_font_small(), 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -2);

    lv_obj_add_event_cb(s_menu.root, key_cb, LV_EVENT_KEY, &s_menu);
    lv_obj_add_flag(s_menu.root, LV_OBJ_FLAG_CLICK_FOCUSABLE);

    refresh(&s_menu);
    return s_menu.root;
}

void demo_menu_bind_group(DemoManager *mgr) {
    if (!mgr || !mgr->group) return;
    if (!s_menu.root) return;
    lv_group_add_obj(mgr->group, s_menu.root);
    lv_group_focus_obj(s_menu.root);
}
