#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#include "led_ws281x.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LED_PATTERN_OFF = 0,
    LED_PATTERN_RAINBOW,
    LED_PATTERN_CHASE,
    LED_PATTERN_BREATHING,
    LED_PATTERN_SPARKLE,
} led_pattern_type_t;

typedef struct {
    uint8_t speed;      // 1..255
    uint8_t saturation; // 0..100
    uint8_t spread_x10; // 10 => full 360° across strip
} led_rainbow_cfg_t;

typedef enum {
    LED_DIR_FORWARD = 0,
    LED_DIR_REVERSE,
    LED_DIR_BOUNCE,
} led_direction_t;

typedef struct {
    uint8_t speed;      // 1..255
    uint8_t tail_len;   // >= 1
    led_rgb8_t fg;
    led_rgb8_t bg;
    led_direction_t dir;
    bool fade_tail;
} led_chase_cfg_t;

typedef enum {
    LED_CURVE_SINE = 0,
    LED_CURVE_LINEAR,
    LED_CURVE_EASE_IN_OUT,
} led_curve_t;

typedef struct {
    uint8_t speed; // 1..255
    led_rgb8_t color;
    uint8_t min_bri; // 0..255
    uint8_t max_bri; // 0..255
    led_curve_t curve;
} led_breathing_cfg_t;

typedef enum {
    LED_SPARKLE_FIXED = 0,
    LED_SPARKLE_RANDOM,
    LED_SPARKLE_RAINBOW,
} led_sparkle_color_mode_t;

typedef struct {
    uint8_t speed;       // 1..255 (multiplier for fade/spawn dynamics, baseline is 10)
    led_rgb8_t color;    // used for FIXED mode
    uint8_t density_pct; // 0..100 (% chance per LED per frame)
    uint8_t fade_speed;  // 1..255 (brightness decrement per frame)
    led_sparkle_color_mode_t color_mode;
    led_rgb8_t background;
} led_sparkle_cfg_t;

typedef union {
    led_rainbow_cfg_t rainbow;
    led_chase_cfg_t chase;
    led_breathing_cfg_t breathing;
    led_sparkle_cfg_t sparkle;
} led_pattern_cfg_u;

typedef struct {
    led_pattern_type_t type;
    uint8_t global_brightness_pct; // 1..100, applied last
    led_pattern_cfg_u u;
} led_pattern_cfg_t;

typedef struct {
    uint32_t frame;
    uint32_t last_step_ms;
    int16_t chase_pos;
    int8_t chase_dir; // 1 or -1

    uint8_t *sparkle_bri;  // len = led_count
    uint16_t sparkle_len;  // == led_count
    uint32_t rng;
} led_pattern_state_t;

typedef struct {
    uint16_t led_count;
    led_pattern_cfg_t cfg;
    led_pattern_state_t st;
} led_patterns_t;

esp_err_t led_patterns_init(led_patterns_t *p, uint16_t led_count);
void led_patterns_deinit(led_patterns_t *p);

void led_patterns_set_cfg(led_patterns_t *p, const led_pattern_cfg_t *cfg);
void led_patterns_render_to_ws281x(led_patterns_t *p, uint32_t now_ms, led_ws281x_t *strip);

#ifdef __cplusplus
}
#endif
