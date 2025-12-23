/*
 * ESP32-S3 tutorial 0012:
 * Cardputer "typewriter": keyboard input -> on-screen text UI using M5GFX (LovyanGFX).
 *
 * This chapter intentionally uses:
 * - M5GFX autodetect for Cardputer wiring (ST7789 on SPI3, offsets, rotation, backlight PWM)
 * - a sprite/canvas backbuffer for full-screen redraws
 * - an optional waitDMA() after presents to avoid tearing from overlapping transfers
 */

#include <stdint.h>
#include <inttypes.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"

#include "M5GFX.h"

static const char *TAG = "cardputer_typewriter_0012";

// Backwards compatibility: if the project was configured before Kconfig options existed,
// the CONFIG_ macros may be missing until you run `idf.py reconfigure` or `idf.py fullclean`.
#ifndef CONFIG_TUTORIAL_0012_PRESENT_WAIT_DMA
#define CONFIG_TUTORIAL_0012_PRESENT_WAIT_DMA 1
#endif
#ifndef CONFIG_TUTORIAL_0012_CANVAS_USE_PSRAM
#define CONFIG_TUTORIAL_0012_CANVAS_USE_PSRAM 0
#endif

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "boot; free_heap=%" PRIu32 " dma_free=%" PRIu32,
             esp_get_free_heap_size(),
             (uint32_t)heap_caps_get_free_size(MALLOC_CAP_DMA));

    m5gfx::M5GFX display;
    ESP_LOGI(TAG, "bringing up M5GFX display via autodetect...");
    display.init();
    display.setColorDepth(16);

    ESP_LOGI(TAG, "display ready: width=%d height=%d", (int)display.width(), (int)display.height());

    const int w = (int)display.width();
    const int h = (int)display.height();

    M5Canvas canvas(&display);
#if CONFIG_TUTORIAL_0012_CANVAS_USE_PSRAM
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

    ESP_LOGI(TAG, "canvas ok: %u bytes; free_heap=%" PRIu32 " dma_free=%" PRIu32,
             (unsigned)canvas.bufferLength(),
             esp_get_free_heap_size(),
             (uint32_t)heap_caps_get_free_size(MALLOC_CAP_DMA));

    canvas.fillScreen(TFT_BLACK);
    canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    canvas.setCursor(0, 0);
    canvas.printf("Cardputer Typewriter (0012)\n\n");
    canvas.printf("Next: keyboard scan + text buffer\n");
    canvas.printf("Refs: 0007 (keyboard), 0011 (display)\n");
    canvas.pushSprite(0, 0);
#if CONFIG_TUTORIAL_0012_PRESENT_WAIT_DMA
    display.waitDMA();
#endif

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}



