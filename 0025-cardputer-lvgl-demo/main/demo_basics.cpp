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

static BasicsState *s_basics = nullptr;

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

static void root_delete_cb(lv_event_t *e) {
    auto *st = static_cast<BasicsState *>(lv_event_get_user_data(e));
    if (!st) return;

    if (st->status_timer) {
        lv_timer_del(st->status_timer);
        st->status_timer = nullptr;
    }

    st->root = nullptr;
    st->ta = nullptr;
    st->btn = nullptr;
    st->status = nullptr;

    if (s_basics == st) {
        s_basics = nullptr;
    }
    delete st;
}

} // namespace

lv_obj_t *demo_basics_create(DemoManager *mgr) {
    auto *st = new BasicsState{};
    st->mgr = mgr;
    s_basics = st;

    st->root = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(st->root, lv_color_black(), 0);
    lv_obj_set_style_text_color(st->root, lv_palette_main(LV_PALETTE_GREEN), 0);

    lv_obj_t *title = lv_label_create(st->root);
    lv_label_set_text(title, "Basics");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    st->ta = lv_textarea_create(st->root);
    lv_obj_set_width(st->ta, 240 - 16);
    lv_obj_set_height(st->ta, 60);
    lv_obj_align(st->ta, LV_ALIGN_TOP_MID, 0, 24);
    lv_textarea_set_placeholder_text(st->ta, "Type here...");
    lv_textarea_set_one_line(st->ta, false);

    st->btn = lv_btn_create(st->root);
    lv_obj_set_size(st->btn, 80, 28);
    lv_obj_align(st->btn, LV_ALIGN_TOP_MID, 0, 92);
    lv_obj_add_flag(st->btn, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_add_event_cb(st->btn, on_clear, LV_EVENT_CLICKED, st);
    lv_obj_t *btn_label = lv_label_create(st->btn);
    lv_label_set_text(btn_label, "Clear");
    lv_obj_center(btn_label);

    st->status = lv_label_create(st->root);
    lv_obj_set_style_text_font(st->status, lvgl_font_small(), 0);
    lv_label_set_text(st->status, "heap=? dma=? last=?");
    lv_obj_align(st->status, LV_ALIGN_BOTTOM_MID, 0, -14);

    lv_obj_t *hint = lv_label_create(st->root);
    lv_obj_set_style_text_font(hint, lvgl_font_small(), 0);
    lv_label_set_text(hint, "Type  Tab focus  Enter click  Fn+` menu");
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -2);

    lv_obj_add_event_cb(st->root, root_delete_cb, LV_EVENT_DELETE, st);
    st->status_timer = lv_timer_create(status_timer_cb, 250, st);

    return st->root;
}

void demo_basics_bind_group(DemoManager *mgr) {
    if (!mgr || !mgr->group) return;
    if (!s_basics || !s_basics->ta || !s_basics->btn) return;

    lv_group_add_obj(mgr->group, s_basics->ta);
    lv_group_add_obj(mgr->group, s_basics->btn);
    lv_group_focus_obj(s_basics->ta);
}
