#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "led_patterns.h"
#include "led_ws281x.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    led_patterns_t patterns;
    led_pattern_cfg_t cfg;

    led_ws281x_t strip; // virtual: only cfg + pixels are used

    uint32_t frame_ms;
    SemaphoreHandle_t mu;
    StaticSemaphore_t mu_buf;
} sim_engine_t;

esp_err_t sim_engine_init(sim_engine_t *e, uint16_t led_count, uint32_t frame_ms);
void sim_engine_deinit(sim_engine_t *e);

void sim_engine_get_cfg(sim_engine_t *e, led_pattern_cfg_t *out);
void sim_engine_set_cfg(sim_engine_t *e, const led_pattern_cfg_t *cfg);

uint32_t sim_engine_get_frame_ms(sim_engine_t *e);
void sim_engine_set_frame_ms(sim_engine_t *e, uint32_t frame_ms);

// Renders one pattern step into the internal virtual strip and copies out the wire-order pixels.
// out_pixels must have at least 3*led_count bytes.
esp_err_t sim_engine_render(sim_engine_t *e, uint32_t now_ms, uint8_t *out_pixels, size_t out_len);

#ifdef __cplusplus
}
#endif

