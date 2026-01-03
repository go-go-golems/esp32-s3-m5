#include "demo_manager.h"

#include <algorithm>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include "esp_heap_caps.h"
#include "esp_system.h"

#include "lvgl_font_util.h"

extern "C" {
extern volatile uint32_t g_ui_loop_counter;
extern volatile uint32_t g_lvgl_handler_us_last;
extern volatile uint32_t g_lvgl_handler_us_avg;
}

namespace {

struct Ema32 {
    bool inited = false;
    uint32_t value = 0;

    void update(uint32_t x, uint8_t shift = 3) {
        if (!inited) {
            inited = true;
            value = x;
            return;
        }
        value = value - (value >> shift) + (x >> shift);
    }
};

struct SystemMonitorState {
    DemoManager *mgr = nullptr;
    lv_obj_t *root = nullptr;
    lv_obj_t *header = nullptr;
    lv_obj_t *footer = nullptr;

    lv_obj_t *chart_heap = nullptr;
    lv_obj_t *chart_dma = nullptr;
    lv_obj_t *chart_fps = nullptr;

    lv_chart_series_t *s_heap = nullptr;
    lv_chart_series_t *s_dma = nullptr;
    lv_chart_series_t *s_fps = nullptr;

    lv_timer_t *timer = nullptr;

    uint32_t last_tick_ms = 0;
    uint32_t last_loop_counter = 0;
    Ema32 loop_hz_ema{};
};

static SystemMonitorState *s_sysmon = nullptr;

static lv_obj_t *make_chart(lv_obj_t *parent, lv_coord_t h) {
    lv_obj_t *c = lv_chart_create(parent);
    lv_obj_set_width(c, lv_pct(100));
    lv_obj_set_height(c, h);
    lv_chart_set_type(c, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(c, 60);
    lv_chart_set_div_line_count(c, 0, 0);
    lv_obj_set_style_bg_opa(c, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(c, 1, 0);
    lv_obj_set_style_border_color(c, lv_palette_darken(LV_PALETTE_GREEN, 3), 0);
    lv_obj_set_style_pad_all(c, 2, 0);
    lv_obj_set_style_line_width(c, 1, LV_PART_ITEMS);
    // Hide per-point squares (keep a 1px line).
    lv_obj_set_style_width(c, 0, LV_PART_INDICATOR);
    lv_obj_set_style_height(c, 0, LV_PART_INDICATOR);
    lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);
    return c;
}

static void sample_cb(lv_timer_t *t) {
    auto *st = static_cast<SystemMonitorState *>(t->user_data);
    if (!st || !st->header) return;

    const uint32_t now = lv_tick_get();
    uint32_t dt = now - st->last_tick_ms;
    if (dt == 0) dt = 1;
    st->last_tick_ms = now;

    const uint32_t heap_free = esp_get_free_heap_size();
    const uint32_t dma_free = (uint32_t)heap_caps_get_free_size(MALLOC_CAP_DMA);
    const uint32_t heap_kb = heap_free / 1024;
    const uint32_t dma_kb = dma_free / 1024;

    const uint32_t loop_counter = g_ui_loop_counter;
    const uint32_t loops_delta = loop_counter - st->last_loop_counter;
    st->last_loop_counter = loop_counter;
    const uint32_t loop_hz = (loops_delta * 1000U) / dt;
    st->loop_hz_ema.update(loop_hz);

    const uint32_t lvgl_us_avg = g_lvgl_handler_us_avg;

    char buf[96];
    snprintf(buf, sizeof(buf), "h=%" PRIu32 "k d=%" PRIu32 "k L=%" PRIu32 "Hz lv=%" PRIu32 "us", heap_kb, dma_kb,
             st->loop_hz_ema.value, lvgl_us_avg);
    lv_label_set_text(st->header, buf);

    if (st->chart_heap && st->s_heap) {
        lv_chart_set_next_value(st->chart_heap, st->s_heap, (lv_coord_t)std::min<uint32_t>(heap_kb, 512));
    }
    if (st->chart_dma && st->s_dma) {
        lv_chart_set_next_value(st->chart_dma, st->s_dma, (lv_coord_t)std::min<uint32_t>(dma_kb, 128));
    }
    if (st->chart_fps && st->s_fps) {
        lv_chart_set_next_value(st->chart_fps, st->s_fps, (lv_coord_t)std::min<uint32_t>(st->loop_hz_ema.value, 60));
    }
}

static void root_delete_cb(lv_event_t *e) {
    auto *st = static_cast<SystemMonitorState *>(lv_event_get_user_data(e));
    if (!st) return;

    if (st->timer) {
        lv_timer_del(st->timer);
        st->timer = nullptr;
    }

    st->root = nullptr;
    st->header = nullptr;
    st->footer = nullptr;
    st->chart_heap = nullptr;
    st->chart_dma = nullptr;
    st->chart_fps = nullptr;
    st->s_heap = nullptr;
    st->s_dma = nullptr;
    st->s_fps = nullptr;

    if (s_sysmon == st) s_sysmon = nullptr;
    delete st;
}

} // namespace

lv_obj_t *demo_system_monitor_create(DemoManager *mgr) {
    auto *st = new SystemMonitorState{};
    st->mgr = mgr;
    s_sysmon = st;

    st->root = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(st->root, lv_color_black(), 0);
    lv_obj_set_style_text_color(st->root, lv_palette_main(LV_PALETTE_GREEN), 0);

    lv_obj_t *title = lv_label_create(st->root);
    lv_label_set_text(title, "System Monitor");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    st->header = lv_label_create(st->root);
    lv_obj_set_style_text_font(st->header, lvgl_font_small(), 0);
    lv_obj_set_style_text_align(st->header, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_width(st->header, 240 - 12);
    lv_label_set_long_mode(st->header, LV_LABEL_LONG_CLIP);
    lv_label_set_text(st->header, "h=?k d=?k L=?Hz lv=?us f=?");
    lv_obj_align(st->header, LV_ALIGN_TOP_LEFT, 6, 20);

    lv_obj_t *cont = lv_obj_create(st->root);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, 240 - 12, 72);
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 34);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    st->chart_heap = make_chart(cont, 20);
    lv_chart_set_range(st->chart_heap, LV_CHART_AXIS_PRIMARY_Y, 0, 512);
    st->s_heap = lv_chart_add_series(st->chart_heap, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);

    st->chart_dma = make_chart(cont, 20);
    lv_chart_set_range(st->chart_dma, LV_CHART_AXIS_PRIMARY_Y, 0, 128);
    st->s_dma = lv_chart_add_series(st->chart_dma, lv_palette_lighten(LV_PALETTE_GREEN, 2), LV_CHART_AXIS_PRIMARY_Y);

    st->chart_fps = make_chart(cont, 20);
    lv_chart_set_range(st->chart_fps, LV_CHART_AXIS_PRIMARY_Y, 0, 60);
    st->s_fps = lv_chart_add_series(st->chart_fps, lv_palette_main(LV_PALETTE_CYAN), LV_CHART_AXIS_PRIMARY_Y);

    st->footer = lv_obj_create(st->root);
    lv_obj_remove_style_all(st->footer);
    lv_obj_set_size(st->footer, 240, 16);
    lv_obj_set_style_bg_color(st->footer, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(st->footer, LV_OPA_COVER, 0);
    lv_obj_align(st->footer, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_t *hint = lv_label_create(st->footer);
    lv_obj_set_style_text_font(hint, lvgl_font_small(), 0);
    lv_label_set_text(hint, "Fn+` menu");
    lv_obj_align(hint, LV_ALIGN_LEFT_MID, 4, 0);

    lv_obj_add_event_cb(st->root, root_delete_cb, LV_EVENT_DELETE, st);
    lv_obj_add_flag(st->root, LV_OBJ_FLAG_CLICK_FOCUSABLE);

    st->last_tick_ms = lv_tick_get();
    st->last_loop_counter = g_ui_loop_counter;
    st->timer = lv_timer_create(sample_cb, 250, st);
    sample_cb(st->timer);

    return st->root;
}

void demo_system_monitor_bind_group(DemoManager *mgr) {
    if (!mgr || !mgr->group) return;
    if (!s_sysmon || !s_sysmon->root) return;
    lv_group_add_obj(mgr->group, s_sysmon->root);
    lv_group_focus_obj(s_sysmon->root);
}
