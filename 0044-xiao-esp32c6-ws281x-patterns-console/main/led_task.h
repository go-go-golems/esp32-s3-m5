#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#include "led_patterns.h"
#include "led_ws281x.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LED_MSG_SET_PATTERN_CFG = 1,
    LED_MSG_SET_PATTERN_TYPE,
    LED_MSG_SET_GLOBAL_BRIGHTNESS_PCT,
    LED_MSG_SET_FRAME_MS,

    LED_MSG_SET_RAINBOW,
    LED_MSG_SET_CHASE,
    LED_MSG_SET_BREATHING,
    LED_MSG_SET_SPARKLE,

    LED_MSG_WS_SET_CFG,   // stage driver config (partial update)
    LED_MSG_WS_APPLY_CFG, // apply staged config (reinit)

    LED_MSG_PAUSE,
    LED_MSG_RESUME,
    LED_MSG_CLEAR,
} led_msg_type_t;

typedef enum {
    LED_WS_CFG_GPIO = (1u << 0),
    LED_WS_CFG_LED_COUNT = (1u << 1),
    LED_WS_CFG_ORDER = (1u << 2),
    LED_WS_CFG_TIMING = (1u << 3),     // t0/t1 timing fields
    LED_WS_CFG_RESET_US = (1u << 4),
    LED_WS_CFG_RESOLUTION_HZ = (1u << 5),
} led_ws_cfg_mask_t;

typedef struct {
    uint32_t set_mask;
    led_ws281x_cfg_t cfg;
} led_ws281x_cfg_update_t;

typedef struct {
    bool running;
    bool paused;
    uint32_t frame_ms;

    led_ws281x_cfg_t ws_cfg;
    led_pattern_cfg_t pat_cfg;
} led_status_t;

typedef struct {
    led_msg_type_t type;
    union {
        led_pattern_cfg_t pattern;
        led_pattern_type_t pattern_type;
        uint8_t brightness_pct;
        uint32_t frame_ms;
        led_rainbow_cfg_t rainbow;
        led_chase_cfg_t chase;
        led_breathing_cfg_t breathing;
        led_sparkle_cfg_t sparkle;
        led_ws281x_cfg_update_t ws_update;
    } u;
} led_msg_t;

esp_err_t led_task_start(const led_ws281x_cfg_t *ws_cfg, const led_pattern_cfg_t *pat_cfg, uint32_t frame_ms);
esp_err_t led_task_send(const led_msg_t *msg, uint32_t timeout_ms);
void led_task_get_status(led_status_t *out); // snapshot (best-effort)

#ifdef __cplusplus
}
#endif
