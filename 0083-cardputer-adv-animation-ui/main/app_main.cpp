#include <inttypes.h>
#include <stdint.h>

#include "sdkconfig.h"

#include <M5Unified.hpp>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"

#include "ui_kb.h"
#include "ui_model.h"
#include "ui_render.h"

static const char *TAG = "0083_anim_ui";

static const char *key_kind_name(ui_key_kind_t kind)
{
    switch (kind) {
    case UI_KEY_UP:
        return "UP";
    case UI_KEY_DOWN:
        return "DOWN";
    case UI_KEY_LEFT:
        return "LEFT";
    case UI_KEY_RIGHT:
        return "RIGHT";
    case UI_KEY_ENTER:
        return "ENTER";
    case UI_KEY_BACK:
        return "BACK";
    case UI_KEY_TAB:
        return "TAB";
    case UI_KEY_SPACE:
        return "SPACE";
    case UI_KEY_DEL:
        return "DEL";
    case UI_KEY_TEXT:
        return "TEXT";
    default:
        return "UNKNOWN";
    }
}

static void log_key_event(const ui_key_event_t *ev, const UiState *ui)
{
    if (!ev || !ui) return;

    ESP_LOGI(TAG,
             "event kind=%s keynum=%u mods=0x%02x text=%s target=%.1f pos=%.1f selected=%d autoplay=%d",
             key_kind_name(ev->kind),
             (unsigned)ev->keynum,
             (unsigned)ev->mods,
             ev->text[0] ? ev->text : "-",
             (double)ui->scroll.target_px,
             (double)ui->scroll.pos_px,
             ui->selected_index,
             ui->autoplay ? 1 : 0);
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG,
             "boot; free_heap=%" PRIu32 " dma_free=%" PRIu32,
             esp_get_free_heap_size(),
             (uint32_t)heap_caps_get_free_size(MALLOC_CAP_DMA));

    ESP_LOGI(TAG, "bringing up M5Unified...");
    M5.begin();
    M5.Display.setBrightness(255);
    M5.Display.setColorDepth(16);
    auto &display = M5.Display;

    const int w = (int)display.width();
    const int h = (int)display.height();
    ESP_LOGI(TAG, "display ready: width=%d height=%d", w, h);

    M5Canvas canvas(&display);
#if CONFIG_TUTORIAL_0083_CANVAS_USE_PSRAM
    canvas.setPsram(true);
#else
    canvas.setPsram(false);
#endif
    canvas.setColorDepth(16);
    if (!canvas.createSprite(w, h)) {
        ESP_LOGE(TAG, "canvas createSprite failed (%dx%d)", w, h);
        return;
    }

    QueueHandle_t q = xQueueCreate(32, sizeof(ui_key_event_t));
    if (!q) {
        ESP_LOGE(TAG, "key queue create failed");
        return;
    }
    ui_kb_start(q);

    UiState ui{};
    ui_model_init(&ui, w, h, CONFIG_TUTORIAL_0083_LINE_COUNT, CONFIG_TUTORIAL_0083_AUTOPLAY_PERIOD_MS);

    ui_kb_debug_state_t kb_dbg{};
    bool first_frame = true;

    for (;;) {
        ui_key_event_t ev{};
        while (xQueueReceive(q, &ev, 0) == pdTRUE) {
            ui_model_handle_event(&ui, &ev);
            log_key_event(&ev, &ui);
        }

        const int64_t now_us = esp_timer_get_time();
        const bool changed = ui_model_tick(&ui, now_us);
        (void)ui_kb_debug_get_state(&kb_dbg);

        if (first_frame || changed || ui.scroll.animating) {
            ui_render_frame(&canvas, &ui, &kb_dbg);
            canvas.pushSprite(0, 0);
#if CONFIG_TUTORIAL_0083_PRESENT_WAIT_DMA
            display.waitDMA();
#endif
            first_frame = false;
        }

        vTaskDelay(pdMS_TO_TICKS(CONFIG_TUTORIAL_0083_FRAME_MS));
    }
}
