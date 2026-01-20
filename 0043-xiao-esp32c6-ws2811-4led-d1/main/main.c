#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "driver/rmt_tx.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ws281x_encoder.h"

static const char *TAG = "mo031_ws2811";

#define RMT_RESOLUTION_HZ 10000000u // 10MHz => 0.1us ticks

static inline uint8_t scale_u8(uint8_t v, uint32_t pct)
{
    return (uint8_t)(((uint32_t)v * pct) / 100u);
}

static uint8_t ease_sine_u8(uint8_t x)
{
    // Quarter-wave 0..63 table, mirrored to 0..255.
    static const uint8_t sine_table[64] = {
        0,   6,  12,  18,  25,  31,  37,  43,  49,  55,  61,  67,  73,  79,  85,  90,
        96, 101, 106, 112, 117, 122, 126, 131, 135, 140, 144, 148, 152, 155, 159, 162,
        165, 168, 171, 174, 176, 179, 181, 183, 185, 187, 188, 190, 191, 192, 193, 194,
        195, 196, 196, 197, 197, 197, 197, 197, 197, 197, 197, 197, 196, 196, 195, 194,
    };

    const uint8_t index = x >> 2; // 0-63
    if (x < 64) {
        return sine_table[index];
    }
    if (x < 128) {
        return sine_table[63 - (index - 16)];
    }
    if (x < 192) {
        return (uint8_t)(255 - sine_table[index - 32]);
    }
    return (uint8_t)(255 - sine_table[63 - (index - 48)]);
}

static void hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b)
{
    h %= 360;
    const uint32_t rgb_max = (uint32_t)(v * 2.55f);
    const uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

    const uint32_t i = h / 60;
    const uint32_t diff = h % 60;
    const uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i) {
    case 0:
        *r = rgb_max;
        *g = rgb_min + rgb_adj;
        *b = rgb_min;
        break;
    case 1:
        *r = rgb_max - rgb_adj;
        *g = rgb_max;
        *b = rgb_min;
        break;
    case 2:
        *r = rgb_min;
        *g = rgb_max;
        *b = rgb_min + rgb_adj;
        break;
    case 3:
        *r = rgb_min;
        *g = rgb_max - rgb_adj;
        *b = rgb_max;
        break;
    case 4:
        *r = rgb_min + rgb_adj;
        *g = rgb_min;
        *b = rgb_max;
        break;
    default:
        *r = rgb_max;
        *g = rgb_min;
        *b = rgb_max - rgb_adj;
        break;
    }
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
    const uint32_t intensity_min_pct = (uint32_t)CONFIG_MO031_INTENSITY_MIN_PCT;
    const uint32_t intensity_period_ms = (uint32_t)CONFIG_MO031_INTENSITY_PERIOD_MS;
    const uint32_t frame_ms = (uint32_t)CONFIG_MO031_ANIM_FRAME_MS;
    const int gpio_num = CONFIG_MO031_WS281X_GPIO_NUM;
    const uint32_t rainbow_speed = (uint32_t)CONFIG_MO031_RAINBOW_SPEED;
    const uint32_t rainbow_saturation = (uint32_t)CONFIG_MO031_RAINBOW_SATURATION;
    const uint32_t rainbow_spread_x10 = (uint32_t)CONFIG_MO031_RAINBOW_SPREAD_X10;

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
    ESP_LOGI(
        TAG,
        "rainbow: speed=%u sat=%u spread_x10=%u",
        (unsigned)rainbow_speed,
        (unsigned)rainbow_saturation,
        (unsigned)rainbow_spread_x10);
    ESP_LOGI(
        TAG,
        "anim: frame_ms=%u intensity_min=%u%% intensity_max=%u%% intensity_period_ms=%u",
        (unsigned)frame_ms,
        (unsigned)intensity_min_pct,
        (unsigned)brightness_pct,
        (unsigned)intensity_period_ms);

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
    int64_t last_progress_log_us = 0;
    while (true) {
        const int64_t now_us = esp_timer_get_time();
        const uint32_t now_ms = (uint32_t)(now_us / 1000);

        // Ported from inspiration/led_patterns.c: hue_offset + hue spread across strip.
        // Make hue offset time-based for smoother motion independent of frame rate.
        const uint32_t hue_offset = (uint32_t)(((uint64_t)now_ms * rainbow_speed * 360ULL) / 10000ULL) % 360;
        const uint32_t hue_range = (360 * rainbow_spread_x10) / 10;

        // Smooth intensity modulation between [min,max].
        uint32_t min_pct = intensity_min_pct;
        uint32_t max_pct = brightness_pct;
        if (min_pct > max_pct) {
            uint32_t tmp = min_pct;
            min_pct = max_pct;
            max_pct = tmp;
        }
        const uint8_t phase_u8 = (uint8_t)(((uint64_t)(now_ms % intensity_period_ms) * 256ULL) / intensity_period_ms);
        const uint8_t wave = ease_sine_u8(phase_u8);
        const uint32_t dyn_pct = min_pct + ((max_pct - min_pct) * (uint32_t)wave) / 255u;

        for (uint32_t i = 0; i < led_count; i++) {
            const uint32_t hue = (hue_offset + (i * hue_range / led_count)) % 360;
            uint32_t r = 0, g = 0, b = 0;
            hsv2rgb(hue, rainbow_saturation, 100, &r, &g, &b);
            set_pixel_grb(pixels, i, (uint8_t)r, (uint8_t)g, (uint8_t)b, dyn_pct);
        }

        ESP_ERROR_CHECK(rmt_transmit(chan, encoder, pixels, led_count * 3, &tx_x));
        ESP_ERROR_CHECK(rmt_tx_wait_all_done(chan, portMAX_DELAY));

        if (now_us - last_progress_log_us >= 1000000) {
            ESP_LOGI(TAG, "loop: frame=%u hue_offset=%u intensity=%u%%", (unsigned)pos, (unsigned)hue_offset, (unsigned)dyn_pct);
            last_progress_log_us = now_us;
        }

        pos++;
        vTaskDelay(pdMS_TO_TICKS(frame_ms));
    }
}
