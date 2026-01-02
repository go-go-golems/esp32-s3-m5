/*
 * Demo suite scaffold for ticket 0021-M5-GFX-DEMO:
 * - Cardputer display bring-up via M5GFX autodetect
 * - keyboard scan (Cardputer matrix) -> key events
 * - reusable ListView widget (A2) rendered into a full-screen canvas
 */

#include <inttypes.h>
#include <stdint.h>

#include <string>
#include <vector>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"

#include "M5GFX.h"

#include "input_keyboard.h"
#include "ui_list_view.h"

static const char *TAG = "cardputer_m5gfx_demo_suite";

static NavKey nav_key_from_event(const KeyEvent &ev, bool *out_is_nav) {
    *out_is_nav = true;
    if (ev.key == "w" || ev.key == "W" || ev.key == "k" || ev.key == "K") {
        return NavKey::Up;
    }
    if (ev.key == "s" || ev.key == "S" || ev.key == "j" || ev.key == "J") {
        return NavKey::Down;
    }
    if (ev.key == "," || ev.key == "<") {
        return NavKey::PageUp;
    }
    if (ev.key == "." || ev.key == ">") {
        return NavKey::PageDown;
    }
    if (ev.key == "enter") {
        return NavKey::Enter;
    }
    *out_is_nav = false;
    return NavKey::Enter;
}

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

    M5Canvas canvas(&display);
    canvas.setPsram(false);
    canvas.setColorDepth(16);
    if (!canvas.createSprite(w, h)) {
        ESP_LOGE(TAG, "canvas createSprite failed (%dx%d)", w, h);
        return;
    }

    CardputerKeyboard keyboard;
    if (keyboard.init() != ESP_OK) {
        ESP_LOGE(TAG, "keyboard init failed");
        return;
    }

    ListView list;
    list.set_bounds(0, 0, w, h);
    list.set_style(ListViewStyle{});
    list.set_items({
        "A2 List view + selection (menu pattern)",
        "A1 Status bar + HUD overlay (TODO)",
        "B2 Frame-time / perf overlay (TODO)",
        "E1 Terminal / log console (TODO)",
        "B3 Screenshot to serial (TODO)",
    });

    bool dirty = true;

    while (true) {
        bool selection_opened = false;

        for (const auto &ev : keyboard.poll()) {
            bool is_nav = false;
            NavKey k = nav_key_from_event(ev, &is_nav);
            if (is_nav) {
                if (list.on_key(k)) {
                    dirty = true;
                }
                if (k == NavKey::Enter) {
                    selection_opened = true;
                }
                continue;
            }

            if (ev.key == "del") {
                ESP_LOGI(TAG, "back (del) pressed");
                continue;
            }
        }

        if (selection_opened) {
            ESP_LOGI(TAG, "open: idx=%d", list.selected_index());
        }

        if (dirty) {
            list.render(canvas);
            canvas.pushSprite(0, 0);
            display.waitDMA();
            dirty = false;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

