#include "led_ws281x.h"

#include <string.h>

#include "freertos/FreeRTOS.h"

#include "esp_check.h"
#include "esp_heap_caps.h"

static const char *TAG = "led_ws281x";

static inline uint8_t scale_u8(uint8_t v, uint32_t pct)
{
    return (uint8_t)(((uint32_t)v * pct) / 100u);
}

esp_err_t led_ws281x_init(led_ws281x_t *strip, const led_ws281x_cfg_t *cfg)
{
    ESP_RETURN_ON_FALSE(strip && cfg, ESP_ERR_INVALID_ARG, TAG, "invalid arg");
    ESP_RETURN_ON_FALSE(cfg->led_count > 0, ESP_ERR_INVALID_ARG, TAG, "led_count=0");
    ESP_RETURN_ON_FALSE(cfg->resolution_hz > 0, ESP_ERR_INVALID_ARG, TAG, "resolution=0");

    *strip = (led_ws281x_t){
        .cfg = *cfg,
        .chan = NULL,
        .encoder = NULL,
        .pixels = NULL,
    };

    strip->pixels = (uint8_t *)heap_caps_malloc((size_t)strip->cfg.led_count * 3, MALLOC_CAP_DEFAULT);
    ESP_RETURN_ON_FALSE(strip->pixels, ESP_ERR_NO_MEM, TAG, "malloc pixels failed");
    memset(strip->pixels, 0, (size_t)strip->cfg.led_count * 3);

    rmt_tx_channel_config_t tx_cfg = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = strip->cfg.gpio_num,
        .mem_block_symbols = 64,
        .resolution_hz = strip->cfg.resolution_hz,
        .trans_queue_depth = 4,
    };
    ESP_RETURN_ON_ERROR(rmt_new_tx_channel(&tx_cfg, &strip->chan), TAG, "rmt_new_tx_channel failed");

    ws281x_encoder_config_t enc_cfg = {
        .resolution_hz = strip->cfg.resolution_hz,
        .t0h_ns = strip->cfg.t0h_ns,
        .t0l_ns = strip->cfg.t0l_ns,
        .t1h_ns = strip->cfg.t1h_ns,
        .t1l_ns = strip->cfg.t1l_ns,
        .reset_us = strip->cfg.reset_us,
    };
    ESP_RETURN_ON_ERROR(rmt_new_ws281x_encoder(&enc_cfg, &strip->encoder), TAG, "rmt_new_ws281x_encoder failed");

    ESP_RETURN_ON_ERROR(rmt_enable(strip->chan), TAG, "rmt_enable failed");
    return ESP_OK;
}

void led_ws281x_deinit(led_ws281x_t *strip)
{
    if (!strip) {
        return;
    }
    if (strip->chan) {
        (void)rmt_disable(strip->chan);
        (void)rmt_del_channel(strip->chan);
        strip->chan = NULL;
    }
    if (strip->encoder) {
        (void)rmt_del_encoder(strip->encoder);
        strip->encoder = NULL;
    }
    if (strip->pixels) {
        free(strip->pixels);
        strip->pixels = NULL;
    }
}

void led_ws281x_clear(led_ws281x_t *strip)
{
    if (!strip || !strip->pixels) {
        return;
    }
    memset(strip->pixels, 0, (size_t)strip->cfg.led_count * 3);
}

esp_err_t led_ws281x_show(led_ws281x_t *strip)
{
    ESP_RETURN_ON_FALSE(strip && strip->chan && strip->encoder && strip->pixels, ESP_ERR_INVALID_STATE, TAG, "not init");

    rmt_transmit_config_t tx_cfg = {
        .loop_count = 0,
    };
    const size_t nbytes = (size_t)strip->cfg.led_count * 3;
    esp_err_t err = rmt_transmit(strip->chan, strip->encoder, strip->pixels, nbytes, &tx_cfg);
    if (err != ESP_OK) {
        return err;
    }
    return rmt_tx_wait_all_done(strip->chan, portMAX_DELAY);
}

void led_ws281x_set_pixel_rgb(led_ws281x_t *strip, uint16_t idx, led_rgb8_t rgb, uint8_t brightness_pct)
{
    if (!strip || !strip->pixels || idx >= strip->cfg.led_count) {
        return;
    }

    const uint8_t r = scale_u8(rgb.r, brightness_pct);
    const uint8_t g = scale_u8(rgb.g, brightness_pct);
    const uint8_t b = scale_u8(rgb.b, brightness_pct);

    const size_t o = (size_t)idx * 3;
    switch (strip->cfg.order) {
    case LED_WS281X_ORDER_RGB:
        strip->pixels[o + 0] = r;
        strip->pixels[o + 1] = g;
        strip->pixels[o + 2] = b;
        break;
    case LED_WS281X_ORDER_GRB:
    default:
        strip->pixels[o + 0] = g;
        strip->pixels[o + 1] = r;
        strip->pixels[o + 2] = b;
        break;
    }
}
