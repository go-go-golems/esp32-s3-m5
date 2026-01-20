#include <inttypes.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"

#include "led_patterns.h"
#include "led_ws281x.h"

static const char *TAG = "mo032_ws281x";

void app_main(void)
{
    ESP_LOGI(TAG, "boot: reset_reason=%d", (int)esp_reset_reason());

    led_ws281x_t strip = {};
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

    ESP_ERROR_CHECK(led_ws281x_init(&strip, &cfg));

    led_patterns_t pat = {};
    ESP_ERROR_CHECK(led_patterns_init(&pat, cfg.led_count));

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
    led_patterns_set_cfg(&pat, &pat_cfg);

    ESP_LOGI(TAG, "boot ok; starting pattern loop (type=rainbow)");

    const uint32_t frame_ms = (uint32_t)CONFIG_MO032_ANIM_FRAME_MS;
    int64_t last_log_us = 0;
    for (;;) {
        const int64_t now_us = esp_timer_get_time();
        const uint32_t now_ms = (uint32_t)(now_us / 1000);

        led_patterns_render_to_ws281x(&pat, now_ms, &strip);
        ESP_ERROR_CHECK(led_ws281x_show(&strip));

        if (now_us - last_log_us >= 1000000) {
            ESP_LOGI(TAG, "loop: uptime=%" PRIi64 "ms", now_us / 1000);
            last_log_us = now_us;
        }
        vTaskDelay(pdMS_TO_TICKS(frame_ms));
    }
}
