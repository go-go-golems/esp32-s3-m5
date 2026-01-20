/**
 * WS2811 LED Pattern Engine
 * Header file with structs and definitions
 * 
 * Target: ESP32, STM32, Arduino, etc.
 */

#ifndef LED_PATTERNS_H
#define LED_PATTERNS_H

#include <stdint.h>
#include <stdbool.h>

/*============================================================================
 * CONFIGURATION
 *============================================================================*/

#define LED_COUNT           50
#define DEFAULT_BRIGHTNESS  200
#define FRAME_RATE_MS       16      // ~60 FPS

/*============================================================================
 * BASIC TYPES
 *============================================================================*/

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb_t;

typedef struct {
    uint16_t h;     // 0-359
    uint8_t  s;     // 0-100
    uint8_t  l;     // 0-100
} hsl_t;

typedef enum {
    PATTERN_OFF,
    PATTERN_SOLID,
    PATTERN_RAINBOW,
    PATTERN_CHASE,
    PATTERN_BREATHING,
    PATTERN_SPARKLE,
    PATTERN_COUNT
} pattern_type_t;

typedef enum {
    DIR_FORWARD,
    DIR_REVERSE,
    DIR_BOUNCE
} direction_t;

typedef enum {
    CURVE_SINE,
    CURVE_LINEAR,
    CURVE_EASE_IN_OUT
} curve_t;

typedef enum {
    COLOR_MODE_FIXED,
    COLOR_MODE_RANDOM,
    COLOR_MODE_RAINBOW
} color_mode_t;

/*============================================================================
 * PATTERN PARAMETER STRUCTS
 *============================================================================*/

typedef struct {
    uint8_t  speed;         // 1-10
    uint8_t  saturation;    // 0-100
    uint8_t  spread;        // multiplier * 10 (10 = 1.0)
} rainbow_params_t;

typedef struct {
    uint8_t     speed;          // 1-10
    uint8_t     tail_length;    // 1-20
    rgb_t       color_fg;
    rgb_t       color_bg;
    direction_t direction;
    bool        fade_tail;
} chase_params_t;

typedef struct {
    uint8_t speed;          // 1-10
    rgb_t   color;
    uint8_t min_brightness; // 0-255
    uint8_t max_brightness; // 0-255
    curve_t curve;
} breathing_params_t;

typedef struct {
    uint8_t      speed;         // 1-10
    rgb_t        color;
    uint8_t      density;       // 0-100 (percentage)
    uint8_t      fade_speed;    // 1-100
    color_mode_t color_mode;
    rgb_t        background;
} sparkle_params_t;

/*============================================================================
 * UNIFIED PATTERN CONFIG
 *============================================================================*/

typedef union {
    rainbow_params_t   rainbow;
    chase_params_t     chase;
    breathing_params_t breathing;
    sparkle_params_t   sparkle;
} pattern_params_t;

typedef struct {
    pattern_type_t   type;
    pattern_params_t params;
    uint8_t          brightness;    // global brightness 0-255
} pattern_config_t;

/*============================================================================
 * PATTERN STATE (runtime)
 *============================================================================*/

typedef struct {
    uint32_t frame;             // frame counter
    uint32_t last_update_ms;    // timestamp
    int16_t  position;          // current position (for chase, etc)
    int8_t   direction;         // 1 or -1 for bounce
    float    phase;             // 0.0 - 1.0 for breathing, etc
    uint8_t  sparkle_brightness[LED_COUNT];  // per-LED sparkle state
} pattern_state_t;

/*============================================================================
 * LED STRIP CONTEXT
 *============================================================================*/

typedef struct {
    rgb_t            leds[LED_COUNT];
    pattern_config_t config;
    pattern_state_t  state;
    bool             running;
} led_strip_t;

/*============================================================================
 * FUNCTION PROTOTYPES
 *============================================================================*/

// Core
void     led_strip_init(led_strip_t* strip);
void     led_strip_update(led_strip_t* strip, uint32_t current_ms);
void     led_strip_set_pattern(led_strip_t* strip, pattern_config_t* config);
void     led_strip_clear(led_strip_t* strip);

// Pattern functions
void     pattern_rainbow(led_strip_t* strip);
void     pattern_chase(led_strip_t* strip);
void     pattern_breathing(led_strip_t* strip);
void     pattern_sparkle(led_strip_t* strip);

// Utilities
rgb_t    hsl_to_rgb(uint16_t h, uint8_t s, uint8_t l);
rgb_t    rgb_scale(rgb_t color, uint8_t scale);
rgb_t    rgb_blend(rgb_t a, rgb_t b, uint8_t blend);
uint8_t  ease_sine(uint8_t x);
uint8_t  random8(void);
uint16_t random16(void);

// Hardware abstraction (implement per platform)
void     hw_show_leds(rgb_t* leds, uint16_t count);
uint32_t hw_millis(void);

#endif // LED_PATTERNS_H
