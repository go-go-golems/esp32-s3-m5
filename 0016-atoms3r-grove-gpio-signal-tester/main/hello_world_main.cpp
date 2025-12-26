/*
 * ESP32-S3 tutorial 0016:
 * AtomS3R GROVE GPIO signal tester (GPIO1/GPIO2) with:
 * - esp_console control plane over USB Serial/JTAG
 * - LCD status UI
 * - GPIO TX patterns and GPIO RX edge counting
 */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"

#include "M5GFX.h"

#include "backlight.h"
#include "console_repl.h"
#include "control_plane.h"
#include "display_hal.h"
#include "lcd_ui.h"
#include "signal_state.h"

static const char *TAG = "atoms3r_gpio_sig_tester";

extern "C" void app_main(void) {
    const int w = CONFIG_TUTORIAL_0016_LCD_HRES;
    const int h = CONFIG_TUTORIAL_0016_LCD_VRES;

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

    M5Canvas canvas(&display_get());
#if CONFIG_TUTORIAL_0016_CANVAS_USE_PSRAM
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

    // Control plane wiring.
    QueueHandle_t ctrl_q = ctrl_init();
    if (!ctrl_q) {
        return;
    }

    tester_state_init();
    lcd_ui_start(&canvas);
    console_start();

    ESP_LOGI(TAG, "ready: type 'help' at the USB Serial/JTAG console");

    while (true) {
        CtrlEvent ev = {};
        if (xQueueReceive(ctrl_q, &ev, portMAX_DELAY) == pdTRUE) {
            tester_state_apply_event(ev);
        }
    }
}



