/*
 * Cardputer LVGL bring-up demo:
 * - display bring-up via M5GFX autodetect
 * - LVGL display flush -> M5GFX
 * - Cardputer keyboard -> LVGL keypad indev (nav + text entry)
 */

#include <inttypes.h>
#include <stdint.h>

#include <vector>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"

#include "M5GFX.h"

#include "demo_manager.h"
#include "input_keyboard.h"
#include "lvgl_port_cardputer_kb.h"
#include "lvgl_port_m5gfx.h"

static const char *TAG = "cardputer_lvgl_demo";

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

    static DemoManager demos;
    demo_manager_init(&demos, kb_indev);
    demo_manager_load(&demos, DemoId::Menu);

    while (true) {
        const std::vector<KeyEvent> events = keyboard.poll();

        std::vector<KeyEvent> filtered;
        filtered.reserve(events.size());
        for (const auto &ev : events) {
            const uint32_t key = lvgl_port_cardputer_kb_translate(ev);
            if (key != 0) {
                demos.last_key = key;
            }
            if (key != 0 && demo_manager_handle_global_key(&demos, key)) {
                continue;
            }
            filtered.push_back(ev);
        }
        lvgl_port_cardputer_kb_feed(filtered);

        lv_timer_handler();
        // Ensure we actually block for >= 1 RTOS tick; otherwise IDLE0 can starve and trip task_wdt.
        vTaskDelay(1);
    }
}
