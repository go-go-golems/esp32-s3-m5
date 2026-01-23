#include "sim_engine.h"

#include <string.h>

#include "esp_check.h"
#include "esp_heap_caps.h"

static const char *TAG = "sim_engine";

esp_err_t sim_engine_init(sim_engine_t *e, uint16_t led_count, uint32_t frame_ms)
{
    ESP_RETURN_ON_FALSE(e && led_count > 0, ESP_ERR_INVALID_ARG, TAG, "invalid arg");
    memset(e, 0, sizeof(*e));

    e->mu = xSemaphoreCreateMutexStatic(&e->mu_buf);
    ESP_RETURN_ON_FALSE(e->mu, ESP_ERR_NO_MEM, TAG, "mutex init failed");

    e->frame_ms = (frame_ms == 0) ? 1 : frame_ms;

    e->strip = (led_ws281x_t){
        .cfg =
            {
                .gpio_num = -1,
                .led_count = led_count,
                .order = LED_WS281X_ORDER_GRB,
                .resolution_hz = 0,
                .t0h_ns = 0,
                .t0l_ns = 0,
                .t1h_ns = 0,
                .t1l_ns = 0,
                .reset_us = 0,
            },
        .chan = nullptr,
        .encoder = nullptr,
        .pixels = nullptr,
    };

    e->strip.pixels = (uint8_t *)heap_caps_malloc((size_t)led_count * 3, MALLOC_CAP_DEFAULT);
    ESP_RETURN_ON_FALSE(e->strip.pixels, ESP_ERR_NO_MEM, TAG, "malloc pixels failed");
    memset(e->strip.pixels, 0, (size_t)led_count * 3);

    ESP_RETURN_ON_ERROR(led_patterns_init(&e->patterns, led_count), TAG, "patterns init failed");

    e->cfg = (led_pattern_cfg_t){
        .type = LED_PATTERN_RAINBOW,
        .global_brightness_pct = 25,
        .u =
            {
                .rainbow =
                    {
                        .speed = 5,
                        .saturation = 100,
                        .spread_x10 = 10,
                    },
            },
    };
    led_patterns_set_cfg(&e->patterns, &e->cfg);

    return ESP_OK;
}

void sim_engine_deinit(sim_engine_t *e)
{
    if (!e) {
        return;
    }

    if (e->mu) {
        (void)xSemaphoreTake(e->mu, portMAX_DELAY);
    }

    led_patterns_deinit(&e->patterns);

    if (e->strip.pixels) {
        free(e->strip.pixels);
        e->strip.pixels = nullptr;
    }

    if (e->mu) {
        xSemaphoreGive(e->mu);
    }

    *e = (sim_engine_t){0};
}

void sim_engine_get_cfg(sim_engine_t *e, led_pattern_cfg_t *out)
{
    if (!e || !out || !e->mu) {
        return;
    }
    if (xSemaphoreTake(e->mu, portMAX_DELAY) != pdTRUE) {
        return;
    }
    *out = e->cfg;
    xSemaphoreGive(e->mu);
}

void sim_engine_set_cfg(sim_engine_t *e, const led_pattern_cfg_t *cfg)
{
    if (!e || !cfg || !e->mu) {
        return;
    }
    if (xSemaphoreTake(e->mu, portMAX_DELAY) != pdTRUE) {
        return;
    }
    e->cfg = *cfg;
    led_patterns_set_cfg(&e->patterns, &e->cfg);
    xSemaphoreGive(e->mu);
}

uint32_t sim_engine_get_frame_ms(sim_engine_t *e)
{
    if (!e || !e->mu) {
        return 16;
    }
    if (xSemaphoreTake(e->mu, portMAX_DELAY) != pdTRUE) {
        return 16;
    }
    const uint32_t v = e->frame_ms;
    xSemaphoreGive(e->mu);
    return v;
}

void sim_engine_set_frame_ms(sim_engine_t *e, uint32_t frame_ms)
{
    if (!e || !e->mu) {
        return;
    }
    if (frame_ms == 0) {
        frame_ms = 1;
    }
    if (xSemaphoreTake(e->mu, portMAX_DELAY) != pdTRUE) {
        return;
    }
    e->frame_ms = frame_ms;
    xSemaphoreGive(e->mu);
}

esp_err_t sim_engine_render(sim_engine_t *e, uint32_t now_ms, uint8_t *out_pixels, size_t out_len)
{
    ESP_RETURN_ON_FALSE(e && e->mu && out_pixels, ESP_ERR_INVALID_ARG, TAG, "invalid arg");
    ESP_RETURN_ON_FALSE(e->strip.pixels, ESP_ERR_INVALID_STATE, TAG, "pixels null");
    ESP_RETURN_ON_FALSE(out_len >= (size_t)e->strip.cfg.led_count * 3, ESP_ERR_INVALID_SIZE, TAG, "short out");

    if (xSemaphoreTake(e->mu, portMAX_DELAY) != pdTRUE) {
        return ESP_FAIL;
    }

    led_patterns_render_to_ws281x(&e->patterns, now_ms, &e->strip);
    memcpy(out_pixels, e->strip.pixels, (size_t)e->strip.cfg.led_count * 3);

    xSemaphoreGive(e->mu);
    return ESP_OK;
}

