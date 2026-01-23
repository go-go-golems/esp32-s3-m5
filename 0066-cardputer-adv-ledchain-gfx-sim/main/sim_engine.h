#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "led_patterns.h"
#include "led_ws281x.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    led_pattern_cfg_t cfg;
    uint32_t frame_ms;

    uint16_t led_count;
    led_ws281x_color_order_t order;

    // Latest-frame snapshot (double-buffered), published by the engine task.
    uint8_t *frames[2];
    int active_frame;
    uint32_t frame_seq;
    uint32_t last_render_ms;
    uint32_t apply_seq_next;
    uint32_t apply_seq_applied;

    // Control queue (consumed by the engine task).
    QueueHandle_t q;
    TaskHandle_t task;

    // Protects cfg/frame_ms + snapshot metadata.
    SemaphoreHandle_t mu;
    StaticSemaphore_t mu_buf;
} sim_engine_t;

esp_err_t sim_engine_init(sim_engine_t *e, uint16_t led_count, uint32_t frame_ms);
void sim_engine_deinit(sim_engine_t *e);

void sim_engine_get_cfg(sim_engine_t *e, led_pattern_cfg_t *out);
void sim_engine_set_cfg(sim_engine_t *e, const led_pattern_cfg_t *cfg);

uint32_t sim_engine_get_frame_ms(sim_engine_t *e);
void sim_engine_set_frame_ms(sim_engine_t *e, uint32_t frame_ms);

uint16_t sim_engine_get_led_count(sim_engine_t *e);
led_ws281x_color_order_t sim_engine_get_order(sim_engine_t *e);

// Copies the latest published frame (wire-order pixels). out_pixels must have at least 3*led_count bytes.
esp_err_t sim_engine_copy_latest_pixels(sim_engine_t *e, uint8_t *out_pixels, size_t out_len);

#ifdef __cplusplus
}
#endif
