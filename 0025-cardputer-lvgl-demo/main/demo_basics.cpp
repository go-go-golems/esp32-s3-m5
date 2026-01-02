#include "demo_manager.h"

#include <inttypes.h>

#include "esp_heap_caps.h"
#include "esp_system.h"

#include "lvgl_font_util.h"

namespace {

struct BasicsState {
    DemoManager *mgr = nullptr;
    lv_obj_t *root = nullptr;
    lv_obj_t *ta = nullptr;
    lv_obj_t *btn = nullptr;
    lv_obj_t *status = nullptr;
    lv_timer_t *status_timer = nullptr;
};

static BasicsState s_basics;

static void on_clear(lv_event_t *e) {
    auto *st = static_cast<BasicsState *>(lv_event_get_user_data(e));
    if (!st || !st->ta) return;
    lv_textarea_set_text(st->ta, "");
}

static void status_timer_cb(lv_timer_t *t) {
    auto *st = static_cast<BasicsState *>(t->user_data);
    if (!st || !st->status) return;

    const uint32_t heap_free = esp_get_free_heap_size();
    const uint32_t dma_free = (uint32_t)heap_caps_get_free_size(MALLOC_CAP_DMA);
    const uint32_t last_key = st->mgr ? st->mgr->last_key : 0;

    char buf[96];
    snprintf(buf, sizeof(buf), "heap=%" PRIu32 " dma=%" PRIu32 " last=%" PRIu32, heap_free, dma_free, last_key);
    lv_label_set_text(st->status, buf);
}

} // namespace

lv_obj_t *demo_basics_create(DemoManager *mgr) {
    s_basics = BasicsState{};
    s_basics.mgr = mgr;

    s_basics.root = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(s_basics.root, lv_color_black(), 0);
    lv_obj_set_style_text_color(s_basics.root, lv_palette_main(LV_PALETTE_GREEN), 0);

    lv_obj_t *title = lv_label_create(s_basics.root);
    lv_label_set_text(title, "Basics");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    s_basics.ta = lv_textarea_create(s_basics.root);
    lv_obj_set_width(s_basics.ta, 240 - 16);
    lv_obj_set_height(s_basics.ta, 60);
    lv_obj_align(s_basics.ta, LV_ALIGN_TOP_MID, 0, 24);
    lv_textarea_set_placeholder_text(s_basics.ta, "Type here...");
    lv_textarea_set_one_line(s_basics.ta, false);

    s_basics.btn = lv_btn_create(s_basics.root);
    lv_obj_set_size(s_basics.btn, 80, 28);
    lv_obj_align(s_basics.btn, LV_ALIGN_TOP_MID, 0, 92);
    lv_obj_add_flag(s_basics.btn, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_add_event_cb(s_basics.btn, on_clear, LV_EVENT_CLICKED, &s_basics);
    lv_obj_t *btn_label = lv_label_create(s_basics.btn);
    lv_label_set_text(btn_label, "Clear");
    lv_obj_center(btn_label);

    s_basics.status = lv_label_create(s_basics.root);
    lv_obj_set_style_text_font(s_basics.status, lvgl_font_small(), 0);
    lv_label_set_text(s_basics.status, "heap=? dma=? last=?");
    lv_obj_align(s_basics.status, LV_ALIGN_BOTTOM_MID, 0, -14);

    lv_obj_t *hint = lv_label_create(s_basics.root);
    lv_obj_set_style_text_font(hint, lvgl_font_small(), 0);
    lv_label_set_text(hint, "Type  Tab focus  Enter click  Fn+` menu");
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -2);

    s_basics.status_timer = lv_timer_create(status_timer_cb, 250, &s_basics);

    return s_basics.root;
}

void demo_basics_bind_group(DemoManager *mgr) {
    if (!mgr || !mgr->group) return;
    if (!s_basics.ta || !s_basics.btn) return;

    lv_group_add_obj(mgr->group, s_basics.ta);
    lv_group_add_obj(mgr->group, s_basics.btn);
    lv_group_focus_obj(s_basics.ta);
}
