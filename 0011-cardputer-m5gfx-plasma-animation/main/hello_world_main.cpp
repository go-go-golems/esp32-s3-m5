/*
 * ESP32-S3 tutorial 0011:
 * Cardputer display bring-up + plasma animation using M5GFX (LovyanGFX) and the M5Canvas API.
 *
 * This chapter intentionally uses:
 * - M5GFX autodetect for Cardputer wiring (ST7789 on SPI3, offsets, rotation, backlight PWM)
 * - a sprite/canvas backbuffer
 * - an explicit waitDMA() after presents to avoid "flutter"/tearing from overlapping transfers
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

static const char *TAG = "cardputer_m5gfx_0011";

// Backwards compatibility: if the project was configured before Kconfig options existed,
// the CONFIG_ macros may be missing until you run `idf.py reconfigure` or `idf.py fullclean`.
#ifndef CONFIG_TUTORIAL_0011_FRAME_DELAY_MS
#define CONFIG_TUTORIAL_0011_FRAME_DELAY_MS 16
#endif
#ifndef CONFIG_TUTORIAL_0011_CANVAS_USE_PSRAM
#define CONFIG_TUTORIAL_0011_CANVAS_USE_PSRAM 0
#endif
#ifndef CONFIG_TUTORIAL_0011_PRESENT_WAIT_DMA
#define CONFIG_TUTORIAL_0011_PRESENT_WAIT_DMA 1
#endif
#ifndef CONFIG_TUTORIAL_0011_LOG_EVERY_N_FRAMES
#define CONFIG_TUTORIAL_0011_LOG_EVERY_N_FRAMES 60
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

    // Visual sanity test: show solid colors before starting animation work.
    ESP_LOGI(TAG, "visual test: solid colors (red/green/blue/white/black)");
    display.fillScreen(TFT_RED);
    vTaskDelay(pdMS_TO_TICKS(400));
    display.fillScreen(TFT_GREEN);
    vTaskDelay(pdMS_TO_TICKS(400));
    display.fillScreen(TFT_BLUE);
    vTaskDelay(pdMS_TO_TICKS(400));
    display.fillScreen(TFT_WHITE);
    vTaskDelay(pdMS_TO_TICKS(400));
    display.fillScreen(TFT_BLACK);
    vTaskDelay(pdMS_TO_TICKS(400));

    ESP_LOGI(TAG, "scaffold OK (plasma implementation comes next)");
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


