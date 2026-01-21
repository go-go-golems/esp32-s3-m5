#include <stdint.h>

#include "sdkconfig.h"

#include "esp_idf_version.h"
#include "esp_log.h"
#include "esp_system.h"

#include "http_server.h"
#include "wifi_console.h"
#include "wifi_mgr.h"

#include "led_patterns.h"
#include "led_console.h"
#include "led_task.h"
#include "led_ws281x.h"

static const char *TAG = "mo034_0046";

static void on_wifi_got_ip(uint32_t ip4_host_order, void *ctx)
{
    (void)ip4_host_order;
    (void)ctx;
    (void)http_server_start();
}

void app_main(void)
{
    ESP_LOGI(TAG, "boot: ESP-IDF %s", esp_get_idf_version());
    ESP_LOGI(TAG, "reset_reason=%d", (int)esp_reset_reason());

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

    wifi_mgr_set_on_got_ip_cb(on_wifi_got_ip, NULL);
    ESP_ERROR_CHECK(wifi_mgr_start());

    // Console is intentionally started after Wi-Fi stack bring-up, so the first prompt
    // appears quickly and early Wi-Fi logs are visible.
    const wifi_console_config_t wifi_console_cfg = {
        .prompt = "c6> ",
        .register_extra = led_console_register_commands,
    };
    wifi_console_start(&wifi_console_cfg);
}
