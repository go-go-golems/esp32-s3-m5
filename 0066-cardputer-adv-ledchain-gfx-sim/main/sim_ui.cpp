#include "sim_ui.h"

#include <inttypes.h>
#include <stdint.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_timer.h"

#include "M5GFX.h"

#include "ui_kb.h"
#include "ui_overlay.h"

static const char *TAG = "sim_ui";

static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

static inline void unpack_rgb(const uint8_t *px3, led_ws281x_color_order_t order, uint8_t *r, uint8_t *g, uint8_t *b)
{
    if (order == LED_WS281X_ORDER_RGB) {
        *r = px3[0];
        *g = px3[1];
        *b = px3[2];
        return;
    }
    // GRB (default)
    *g = px3[0];
    *r = px3[1];
    *b = px3[2];
}

static void ui_task_main(void *arg)
{
    auto *engine = (sim_engine_t *)arg;

    m5gfx::M5GFX display;
    ESP_LOGI(TAG, "display init via M5GFX autodetect...");
    display.init();
    display.setColorDepth(16);

    const int w = (int)display.width();
    const int h = (int)display.height();
    ESP_LOGI(TAG, "display ready: %dx%d", w, h);

    M5Canvas canvas(&display);
#if CONFIG_TUTORIAL_0066_CANVAS_USE_PSRAM
    canvas.setPsram(true);
#else
    canvas.setPsram(false);
#endif
    canvas.setColorDepth(16);
    if (!canvas.createSprite(w, h)) {
        ESP_LOGE(TAG, "canvas createSprite failed (%dx%d)", w, h);
        vTaskDelete(NULL);
        return;
    }

    // Layout: 10x5 grid for 50 LEDs, with a small overlay row on top.
    static constexpr int kCols = 10;
    static constexpr int kRows = 5;
    static constexpr int kOverlayH = 16;
    const int grid_h = h - kOverlayH;
    const int cell_w = w / kCols;
    const int cell_h = grid_h / kRows;
    const int pad = 2;

    const size_t led_count = (size_t)sim_engine_get_led_count(engine);
    uint8_t frame[3 * 300] = {0};
    if (led_count > 300) {
        ESP_LOGE(TAG, "led_count too large for frame buffer: %u", (unsigned)led_count);
        vTaskDelete(NULL);
        return;
    }

    QueueHandle_t q = xQueueCreate(32, sizeof(ui_key_event_t));
    if (!q) {
        ESP_LOGE(TAG, "kb queue create failed");
        vTaskDelete(NULL);
        return;
    }
    ui_kb_start(q);

    ui_overlay_t ui = {};
    ui_overlay_init(&ui);

    for (;;) {
        // Keep a timestamp available for future UI features (repeat/idle timeouts).
        (void)(uint32_t)(esp_timer_get_time() / 1000);
        const uint32_t frame_ms = sim_engine_get_frame_ms(engine);

        (void)sim_engine_copy_latest_pixels(engine, frame, sizeof(frame));

        led_pattern_cfg_t cfg;
        sim_engine_get_cfg(engine, &cfg);

        canvas.fillScreen(TFT_BLACK);
        canvas.setTextSize(1, 1);
        canvas.setTextDatum(lgfx::textdatum_t::top_left);
        canvas.setTextColor(TFT_WHITE, TFT_BLACK);

        for (int i = 0; i < (int)led_count; i++) {
            const int col = i % kCols;
            const int row = i / kCols;
            if (row >= kRows) break;

            const int x0 = col * cell_w;
            const int y0 = kOverlayH + row * cell_h;

            uint8_t r = 0, g = 0, b = 0;
            unpack_rgb(&frame[i * 3], sim_engine_get_order(engine), &r, &g, &b);
            const uint16_t c565 = rgb565(r, g, b);

            canvas.fillRoundRect(x0 + pad, y0 + pad, cell_w - 2 * pad, cell_h - 2 * pad, 3, c565);
            canvas.drawRoundRect(x0 + pad, y0 + pad, cell_w - 2 * pad, cell_h - 2 * pad, 3, TFT_DARKGREY);
        }

        // Handle input after reading cfg (some actions apply immediately and should be reflected in overlay text).
        ui_key_event_t ev = {};
        while (xQueueReceive(q, &ev, 0) == pdTRUE) {
            ui_overlay_handle(&ui, engine, &ev);
        }
        sim_engine_get_cfg(engine, &cfg);

        ui_overlay_draw(&ui, &canvas, &cfg, (uint16_t)led_count, frame_ms);

        canvas.pushSprite(0, 0);
#if CONFIG_TUTORIAL_0066_PRESENT_WAIT_DMA
        display.waitDMA();
#endif

        vTaskDelay(pdMS_TO_TICKS(frame_ms));
    }
}

void sim_ui_start(sim_engine_t *engine)
{
    // M5GFX init can be stack-hungry; keep this comfortably above 4k to avoid stack corruption.
    xTaskCreate(&ui_task_main, "sim_ui", 8192, engine, 5, nullptr);
}
