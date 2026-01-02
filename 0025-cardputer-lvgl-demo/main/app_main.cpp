/*
 * Cardputer LVGL bring-up demo:
 * - display bring-up via M5GFX autodetect
 * - LVGL display flush -> M5GFX
 * - Cardputer keyboard -> LVGL keypad indev (nav + text entry)
 */

#include <inttypes.h>
#include <stdint.h>

#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"

#include "M5GFX.h"

#include "input_keyboard.h"
#include "lvgl_port_cardputer_kb.h"
#include "lvgl_port_m5gfx.h"

static const char *TAG = "cardputer_lvgl_demo";

namespace {

struct UiState {
    lv_obj_t *ta = nullptr;
    lv_obj_t *status = nullptr;
    uint32_t last_key = 0;
};

static void on_clear(lv_event_t *e) {
    auto *st = static_cast<UiState *>(lv_event_get_user_data(e));
    if (!st || !st->ta) return;
    lv_textarea_set_text(st->ta, "");
}

static void status_timer_cb(lv_timer_t *t) {
    auto *st = static_cast<UiState *>(t->user_data);
    if (!st || !st->status) return;

    const uint32_t heap_free = esp_get_free_heap_size();
    char buf[96];
    snprintf(buf, sizeof(buf), "heap=%" PRIu32 "  last_key=%" PRIu32, heap_free, st->last_key);
    lv_label_set_text(st->status, buf);
}

static void create_demo_ui(UiState *st, int w, int h, lv_indev_t *kb_indev) {
    if (!st) return;
    *st = UiState{};

    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_color(scr, lv_color_hex(0xFFFFFF), 0);

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Cardputer LVGL Demo");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);

    st->ta = lv_textarea_create(scr);
    lv_obj_set_width(st->ta, w - 16);
    lv_obj_set_height(st->ta, h / 2);
    lv_obj_align(st->ta, LV_ALIGN_TOP_MID, 0, 28);
    lv_textarea_set_placeholder_text(st->ta, "Type here...");
    lv_textarea_set_one_line(st->ta, false);

    lv_obj_t *btn = lv_btn_create(scr);
    lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 28 + (h / 2) + 10);
    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Clear");
    lv_obj_center(btn_label);
    lv_obj_add_event_cb(btn, on_clear, LV_EVENT_CLICKED, st);

    st->status = lv_label_create(scr);
    lv_label_set_text(st->status, "heap=? last_key=?");
    lv_obj_align(st->status, LV_ALIGN_BOTTOM_MID, 0, -6);

    lv_group_t *g = lv_group_create();
    lv_group_set_default(g);
    lv_group_add_obj(g, st->ta);
    lv_group_add_obj(g, btn);
    if (kb_indev) {
        lv_indev_set_group(kb_indev, g);
    }
    lv_group_focus_obj(st->ta);

    lv_timer_create(status_timer_cb, 250, st);
}

} // namespace

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "boot; free_heap=%" PRIu32 " dma_free=%" PRIu32, esp_get_free_heap_size(),
             (uint32_t)heap_caps_get_free_size(MALLOC_CAP_DMA));

    m5gfx::M5GFX display;
    ESP_LOGI(TAG, "display init via M5GFX autodetect...");
    display.init();
    display.setColorDepth(16);

    const int w = (int)display.width();
    const int h = (int)display.height();
    ESP_LOGI(TAG, "display ready: width=%d height=%d", w, h);

    CardputerKeyboard keyboard;
    if (keyboard.init() != ESP_OK) {
        ESP_LOGE(TAG, "keyboard init failed");
        return;
    }

    LvglM5gfxConfig lv_cfg{};
    lv_cfg.buffer_lines = 40;
    lv_cfg.double_buffer = true;
    lv_cfg.swap_bytes = false;
    lv_cfg.tick_ms = 2;

    if (!lvgl_port_m5gfx_init(display, lv_cfg)) {
        ESP_LOGE(TAG, "lvgl_port_m5gfx_init failed");
        return;
    }
    lv_indev_t *kb_indev = lvgl_port_cardputer_kb_init();

    // LVGL stores callback user_data pointers; keep UiState stable for the lifetime of the app.
    static UiState ui;
    create_demo_ui(&ui, w, h, kb_indev);

    while (true) {
        const std::vector<KeyEvent> events = keyboard.poll();
        for (const auto &ev : events) {
            if (ev.key.size() == 1) {
                ui.last_key = (uint32_t)(unsigned char)ev.key[0];
            }
        }
        lvgl_port_cardputer_kb_feed(events);

        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
