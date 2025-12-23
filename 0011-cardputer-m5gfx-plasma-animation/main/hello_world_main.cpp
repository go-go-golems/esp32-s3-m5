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
#include <math.h>

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

static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

static inline uint16_t hsv_to_rgb565(uint8_t h, uint8_t s, uint8_t v) {
    if (s == 0) {
        return rgb565(v, v, v);
    }

    const uint8_t region = (uint8_t)(h / 43); // 0..5
    const uint8_t remainder = (uint8_t)((h - (region * 43)) * 6);

    const uint8_t p = (uint8_t)((v * (uint16_t)(255 - s)) >> 8);
    const uint8_t q = (uint8_t)((v * (uint16_t)(255 - ((s * (uint16_t)remainder) >> 8))) >> 8);
    const uint8_t t = (uint8_t)((v * (uint16_t)(255 - ((s * (uint16_t)(255 - remainder)) >> 8))) >> 8);

    uint8_t r = 0, g = 0, b = 0;
    switch (region) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
    }
    return rgb565(r, g, b);
}

static void build_plasma_tables(uint8_t sin8[256], uint16_t palette[256]) {
    constexpr float kTwoPi = 6.2831853071795864769f;
    for (int i = 0; i < 256; i++) {
        const float a = (float)i * kTwoPi / 256.0f;
        const float s = sinf(a);
        const int v = (int)((s + 1.0f) * 127.5f);
        sin8[i] = (uint8_t)((v < 0) ? 0 : (v > 255) ? 255 : v);
        palette[i] = hsv_to_rgb565((uint8_t)i, 255, 255);
    }
}

static inline void draw_plasma(uint16_t *dst565, int w, int h, uint32_t t,
                               const uint8_t sin8[256], const uint16_t palette[256]) {
    const uint8_t t1 = (uint8_t)(t & 0xFF);
    const uint8_t t2 = (uint8_t)((t >> 1) & 0xFF);
    const uint8_t t3 = (uint8_t)((t >> 2) & 0xFF);
    const uint8_t t4 = (uint8_t)((t * 3) & 0xFF);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            const uint8_t v1 = sin8[(uint8_t)(x * 3 + t1)];
            const uint8_t v2 = sin8[(uint8_t)(y * 4 + t2)];
            const uint8_t v3 = sin8[(uint8_t)((x * 2 + y * 2) + t3)];
            const uint8_t v4 = sin8[(uint8_t)((x - y) * 2 + t4)];
            const uint16_t sum = (uint16_t)v1 + (uint16_t)v2 + (uint16_t)v3 + (uint16_t)v4;
            const uint8_t idx = (uint8_t)(sum >> 2);
            dst565[y * w + x] = palette[idx];
        }
    }
}

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

    const int w = (int)display.width();
    const int h = (int)display.height();

    M5Canvas canvas(&display);
#if CONFIG_TUTORIAL_0011_CANVAS_USE_PSRAM
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

    uint8_t sin8[256] = {};
    uint16_t palette[256] = {};
    build_plasma_tables(sin8, palette);

    uint32_t t = 0;

#if CONFIG_TUTORIAL_0011_LOG_EVERY_N_FRAMES > 0
    uint32_t frame = 0;
    uint32_t last_tick = xTaskGetTickCount();
#endif

    ESP_LOGI(TAG, "starting plasma loop (frame_delay_ms=%d wait_dma=%d psram=%d)",
             CONFIG_TUTORIAL_0011_FRAME_DELAY_MS,
             (int)(0
#if CONFIG_TUTORIAL_0011_PRESENT_WAIT_DMA
                   + 1
#endif
                   ),
             (int)(0
#if CONFIG_TUTORIAL_0011_CANVAS_USE_PSRAM
                   + 1
#endif
                   ));

    while (true) {
        auto *dst = (uint16_t *)canvas.getBuffer();
        draw_plasma(dst, w, h, t, sin8, palette);

        canvas.pushSprite(0, 0);
#if CONFIG_TUTORIAL_0011_PRESENT_WAIT_DMA
        display.waitDMA();
#endif

        t += 2;
        if (CONFIG_TUTORIAL_0011_FRAME_DELAY_MS > 0) {
            vTaskDelay(pdMS_TO_TICKS(CONFIG_TUTORIAL_0011_FRAME_DELAY_MS));
        } else {
            taskYIELD();
        }

#if CONFIG_TUTORIAL_0011_LOG_EVERY_N_FRAMES > 0
        frame++;
        if ((frame % (uint32_t)CONFIG_TUTORIAL_0011_LOG_EVERY_N_FRAMES) == 0) {
            uint32_t now = xTaskGetTickCount();
            uint32_t dt_ms = (now - last_tick) * portTICK_PERIOD_MS;
            last_tick = now;
            ESP_LOGI(TAG, "heartbeat: frame=%" PRIu32 " dt=%" PRIu32 "ms free_heap=%" PRIu32,
                     frame, dt_ms, esp_get_free_heap_size());
        }
#endif
    }
}


