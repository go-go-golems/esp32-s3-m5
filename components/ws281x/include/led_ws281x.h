#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "driver/rmt_tx.h"
#include "esp_err.h"

#include "ws281x_encoder.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LED_WS281X_ORDER_GRB = 0,
    LED_WS281X_ORDER_RGB = 1,
} led_ws281x_color_order_t;

typedef struct {
    int gpio_num;
    uint16_t led_count;
    led_ws281x_color_order_t order;

    uint32_t resolution_hz;
    uint32_t t0h_ns;
    uint32_t t0l_ns;
    uint32_t t1h_ns;
    uint32_t t1l_ns;
    uint32_t reset_us;
} led_ws281x_cfg_t;

typedef struct {
    led_ws281x_cfg_t cfg;
    rmt_channel_handle_t chan;
    rmt_encoder_handle_t encoder;
    uint8_t *pixels; // wire-order bytes: [3*led_count]
} led_ws281x_t;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} led_rgb8_t;

esp_err_t led_ws281x_init(led_ws281x_t *strip, const led_ws281x_cfg_t *cfg);
void led_ws281x_deinit(led_ws281x_t *strip);

void led_ws281x_clear(led_ws281x_t *strip);
esp_err_t led_ws281x_show(led_ws281x_t *strip);

void led_ws281x_set_pixel_rgb(led_ws281x_t *strip, uint16_t idx, led_rgb8_t rgb, uint8_t brightness_pct);

#ifdef __cplusplus
}
#endif

