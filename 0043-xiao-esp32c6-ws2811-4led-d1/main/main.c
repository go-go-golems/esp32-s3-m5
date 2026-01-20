#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "driver/rmt_tx.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ws281x_encoder.h"

static const char *TAG = "mo031_ws2811";

#define RMT_RESOLUTION_HZ 10000000u // 10MHz => 0.1us ticks

static inline uint8_t scale_u8(uint8_t v, uint32_t pct)
{
    return (uint8_t)(((uint32_t)v * pct) / 100u);
}

static void set_pixel_grb(uint8_t *buf, uint32_t idx, uint8_t r, uint8_t g, uint8_t b, uint32_t brightness_pct)
{
    buf[idx * 3 + 0] = scale_u8(g, brightness_pct);
    buf[idx * 3 + 1] = scale_u8(r, brightness_pct);
    buf[idx * 3 + 2] = scale_u8(b, brightness_pct);
}

static void clear_pixels(uint8_t *buf, uint32_t count)
{
    memset(buf, 0, count * 3);
}

void app_main(void)
{
    const uint32_t led_count = (uint32_t)CONFIG_MO031_WS281X_LED_COUNT;
    const uint32_t brightness_pct = (uint32_t)CONFIG_MO031_WS281X_BRIGHTNESS_PCT;
    const int gpio_num = CONFIG_MO031_WS281X_GPIO_NUM;

    ESP_LOGI(TAG, "boot: reset_reason=%d", (int)esp_reset_reason());
    ESP_LOGI(TAG, "config: gpio=%d leds=%u brightness=%u%%", gpio_num, (unsigned)led_count, (unsigned)brightness_pct);
    ESP_LOGI(
        TAG,
        "timing: T0H=%dns T0L=%dns T1H=%dns T1L=%dns reset=%dus @ %uHz",
        (int)CONFIG_MO031_WS281X_T0H_NS,
        (int)CONFIG_MO031_WS281X_T0L_NS,
        (int)CONFIG_MO031_WS281X_T1H_NS,
        (int)CONFIG_MO031_WS281X_T1L_NS,
        (int)CONFIG_MO031_WS281X_RESET_US,
        (unsigned)RMT_RESOLUTION_HZ);

    ESP_LOGI(TAG, "Create RMT TX channel");
    rmt_channel_handle_t chan = NULL;
    rmt_tx_channel_config_t tx_cfg = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = gpio_num,
        .mem_block_symbols = 64,
        .resolution_hz = RMT_RESOLUTION_HZ,
        .trans_queue_depth = 4,
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_cfg, &chan));

    ESP_LOGI(TAG, "Install WS281x encoder");
    rmt_encoder_handle_t encoder = NULL;
    ws281x_encoder_config_t enc_cfg = {
        .resolution_hz = RMT_RESOLUTION_HZ,
        .t0h_ns = (uint32_t)CONFIG_MO031_WS281X_T0H_NS,
        .t0l_ns = (uint32_t)CONFIG_MO031_WS281X_T0L_NS,
        .t1h_ns = (uint32_t)CONFIG_MO031_WS281X_T1H_NS,
        .t1l_ns = (uint32_t)CONFIG_MO031_WS281X_T1L_NS,
        .reset_us = (uint32_t)CONFIG_MO031_WS281X_RESET_US,
    };
    ESP_ERROR_CHECK(rmt_new_ws281x_encoder(&enc_cfg, &encoder));

    ESP_LOGI(TAG, "Enable RMT channel");
    ESP_ERROR_CHECK(rmt_enable(chan));

    uint8_t *pixels = (uint8_t *)heap_caps_malloc(led_count * 3, MALLOC_CAP_DEFAULT);
    if (!pixels) {
        ESP_LOGE(TAG, "heap_caps_malloc(%u) failed", (unsigned)(led_count * 3));
        return;
    }
    clear_pixels(pixels, led_count);

    ESP_LOGI(TAG, "Start pattern loop");
    rmt_transmit_config_t tx_x = {
        .loop_count = 0,
    };

    uint32_t pos = 0;
    while (true) {
        clear_pixels(pixels, led_count);

        // Moving "one-hot" pixel with rotating color to confirm indexing.
        const uint32_t i = pos % led_count;
        const uint32_t phase = (pos / led_count) % 4;
        uint8_t r = 0, g = 0, b = 0;
        switch (phase) {
        case 0:
            r = 255;
            break;
        case 1:
            g = 255;
            break;
        case 2:
            b = 255;
            break;
        default:
            r = g = b = 255;
            break;
        }
        set_pixel_grb(pixels, i, r, g, b, brightness_pct);

        ESP_ERROR_CHECK(rmt_transmit(chan, encoder, pixels, led_count * 3, &tx_x));
        ESP_ERROR_CHECK(rmt_tx_wait_all_done(chan, portMAX_DELAY));

        pos++;
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}
