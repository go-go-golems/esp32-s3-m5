#include <inttypes.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"

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

    // Minimal bring-up frame: one red pixel so wiring/order is obvious.
    led_ws281x_clear(&strip);
    led_ws281x_set_pixel_rgb(&strip, 0, (led_rgb8_t){.r = 255, .g = 0, .b = 0}, (uint8_t)CONFIG_MO032_WS281X_BRIGHTNESS_PCT);
    ESP_ERROR_CHECK(led_ws281x_show(&strip));

    ESP_LOGI(TAG, "boot ok; running placeholder loop (uptime logging)");
    int64_t last_log_us = 0;
    for (;;) {
        const int64_t now_us = esp_timer_get_time();
        if (now_us - last_log_us >= 1000000) {
            ESP_LOGI(TAG, "uptime=%" PRIi64 "ms", now_us / 1000);
            last_log_us = now_us;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
