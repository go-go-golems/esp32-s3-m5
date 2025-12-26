/*
 * ESP32-S3 tutorial 0017:
 * AtomS3R Web UI (graphics upload + WebSocket terminal).
 *
 * Step 0: scaffold + prove display/backlight bring-up works (matches existing tutorials).
 */

#include <inttypes.h>
#include <stdint.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_heap_caps.h"
#include "esp_log.h"

#include "M5GFX.h"

#include "backlight.h"
#include "display_hal.h"

static const char *TAG = "atoms3r_web_ui_0017";

static void visual_test_solid_colors(void) {
    auto &display = display_get();
    display.fillScreen(TFT_RED);
    vTaskDelay(pdMS_TO_TICKS(120));
    display.fillScreen(TFT_GREEN);
    vTaskDelay(pdMS_TO_TICKS(120));
    display.fillScreen(TFT_BLUE);
    vTaskDelay(pdMS_TO_TICKS(120));
    display.fillScreen(TFT_WHITE);
    vTaskDelay(pdMS_TO_TICKS(120));
    display.fillScreen(TFT_BLACK);
    vTaskDelay(pdMS_TO_TICKS(120));
}

extern "C" void app_main(void) {
    const int w = CONFIG_TUTORIAL_0017_LCD_HRES;
    const int h = CONFIG_TUTORIAL_0017_LCD_VRES;

    ESP_LOGI(TAG, "boot; free_heap=%" PRIu32 " dma_free=%" PRIu32,
             esp_get_free_heap_size(),
             (uint32_t)heap_caps_get_free_size(MALLOC_CAP_DMA));

    backlight_prepare_for_init();

    ESP_LOGI(TAG, "bringing up M5GFX display...");
    if (!display_init_m5gfx()) {
        ESP_LOGE(TAG, "display init failed, aborting");
        return;
    }

    backlight_enable_after_init();

    ESP_LOGI(TAG, "visual test: solid colors (red/green/blue/white/black)");
    visual_test_solid_colors();

    M5Canvas canvas(&display_get());
#if CONFIG_TUTORIAL_0017_CANVAS_USE_PSRAM
    canvas.setPsram(true);
#else
    canvas.setPsram(false);
#endif
    canvas.setColorDepth(16);
    void *buf = canvas.createSprite(w, h);
    if (!buf) {
        ESP_LOGE(TAG, "canvas createSprite failed (%dx%d)", w, h);
        return;
    }

    canvas.fillScreen(TFT_BLACK);
    canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    canvas.setCursor(0, 0);
    canvas.printf("AtomS3R Web UI\n");
    canvas.printf("Tutorial 0017\n");
    canvas.printf("\n");
    canvas.printf("Next: WiFi+HTTP\n");
    display_present_canvas(canvas);

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


