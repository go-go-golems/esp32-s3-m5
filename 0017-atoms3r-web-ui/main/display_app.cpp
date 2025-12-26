/*
 * Tutorial 0017: Display app wrapper (canvas + present + PNG rendering).
 */

#include "display_app.h"

#include <inttypes.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_heap_caps.h"
#include "esp_log.h"

#include "M5GFX.h"

#include "backlight.h"
#include "display_hal.h"

static const char *TAG = "atoms3r_web_ui_0017";

static bool s_inited = false;

static const int s_w = CONFIG_TUTORIAL_0017_LCD_HRES;
static const int s_h = CONFIG_TUTORIAL_0017_LCD_VRES;

static M5Canvas s_canvas(&display_get());

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

esp_err_t display_app_init(void) {
    if (s_inited) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "boot; free_heap=%" PRIu32 " dma_free=%" PRIu32,
             esp_get_free_heap_size(),
             (uint32_t)heap_caps_get_free_size(MALLOC_CAP_DMA));

    backlight_prepare_for_init();

    ESP_LOGI(TAG, "bringing up M5GFX display...");
    if (!display_init_m5gfx()) {
        ESP_LOGE(TAG, "display init failed");
        return ESP_FAIL;
    }

    backlight_enable_after_init();

    ESP_LOGI(TAG, "visual test: solid colors (red/green/blue/white/black)");
    visual_test_solid_colors();

#if CONFIG_TUTORIAL_0017_CANVAS_USE_PSRAM
    s_canvas.setPsram(true);
#else
    s_canvas.setPsram(false);
#endif
    s_canvas.setColorDepth(16);
    void *buf = s_canvas.createSprite(s_w, s_h);
    if (!buf) {
        ESP_LOGE(TAG, "canvas createSprite failed (%dx%d)", s_w, s_h);
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "canvas ok: %u bytes; free_heap=%" PRIu32 " dma_free=%" PRIu32,
             (unsigned)s_canvas.bufferLength(),
             esp_get_free_heap_size(),
             (uint32_t)heap_caps_get_free_size(MALLOC_CAP_DMA));

    s_inited = true;
    return ESP_OK;
}

void display_app_present(void) {
    display_present_canvas(s_canvas);
}

void display_app_show_boot_screen(const char *line1, const char *line2) {
    s_canvas.fillScreen(TFT_BLACK);
    s_canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    s_canvas.setCursor(0, 0);
    if (line1) s_canvas.printf("%s\n", line1);
    if (line2) s_canvas.printf("%s\n", line2);
    display_app_present();
}

esp_err_t display_app_png_from_file(const char *path) {
    if (!s_inited) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!path || !path[0]) {
        return ESP_ERR_INVALID_ARG;
    }

    s_canvas.fillScreen(TFT_BLACK);
    const bool ok = s_canvas.drawPngFile(path, 0, 0);
    if (!ok) {
        ESP_LOGE(TAG, "drawPngFile failed: %s", path);
        return ESP_FAIL;
    }
    display_app_present();

    // Release any scratch memory held by the PNG decoder.
    display_get().releasePngMemory();
    return ESP_OK;
}


