#include <inttypes.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"

#include "led_patterns.h"
#include "led_console.h"
#include "led_task.h"
#include "led_ws281x.h"

static const char *TAG = "mo032_ws281x";

void app_main(void)
{
    ESP_LOGI(TAG, "boot: reset_reason=%d", (int)esp_reset_reason());

    led_ws281x_cfg_t cfg = {
        .gpio_num = CONFIG_MO032_WS281X_GPIO_NUM,
        .led_count = (uint16_t)CONFIG_MO032_WS281X_LED_COUNT,
        .order = LED_WS281X_ORDER_GRB,
        .resolution_hz = 10000000u,
        .t0h_ns = (uint32_t)CONFIG_MO032_WS281X_T0H_NS,
        .t0l_ns = (uint32_t)CONFIG_MO032_WS281X_T0L_NS,
        .t1h_ns = (uint32_t)CONFIG_MO032_WS281X_T1H_NS,
        .t1l_ns = (uint32_t)CONFIG_MO032_WS281X_T1L_NS,
        .reset_us = (uint32_t)CONFIG_MO032_WS281X_RESET_US,
    };

    ESP_LOGI(TAG, "config: gpio=%d leds=%u brightness=%u%%", cfg.gpio_num, (unsigned)cfg.led_count, (unsigned)CONFIG_MO032_WS281X_BRIGHTNESS_PCT);
    ESP_LOGI(TAG, "timing: T0H=%uns T0L=%uns T1H=%uns T1L=%uns reset=%uus res=%uHz", (unsigned)cfg.t0h_ns, (unsigned)cfg.t0l_ns, (unsigned)cfg.t1h_ns, (unsigned)cfg.t1l_ns, (unsigned)cfg.reset_us, (unsigned)cfg.resolution_hz);

    led_pattern_cfg_t pat_cfg = {
        .type = LED_PATTERN_RAINBOW,
        .global_brightness_pct = (uint8_t)CONFIG_MO032_WS281X_BRIGHTNESS_PCT,
        .u.rainbow =
            {
                .speed = 5,
                .saturation = 100,
                .spread_x10 = 10,
            },
    };

    const uint32_t frame_ms = (uint32_t)CONFIG_MO032_ANIM_FRAME_MS;
    ESP_ERROR_CHECK(led_task_start(&cfg, &pat_cfg, frame_ms));

    ESP_LOGI(TAG, "boot ok; led task running (type=rainbow)");
    led_console_start();

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
