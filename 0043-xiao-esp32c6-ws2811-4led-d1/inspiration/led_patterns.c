/**
 * WS2811 LED Pattern Engine
 * Implementation with pattern algorithms
 */

#include "led_patterns.h"
#include <string.h>
#include <math.h>

/*============================================================================
 * LOOKUP TABLES
 *============================================================================*/

// Sine table for 0-255 input -> 0-255 output (quarter wave, mirrored)
static const uint8_t SINE_TABLE[64] = {
    0,   6,  12,  18,  25,  31,  37,  43,  49,  55,  61,  67,  73,  79,  85,  90,
   96, 101, 106, 112, 117, 122, 126, 131, 135, 140, 144, 148, 152, 155, 159, 162,
  165, 168, 171, 174, 176, 179, 181, 183, 185, 187, 188, 190, 191, 192, 193, 194,
  195, 196, 196, 197, 197, 197, 197, 197, 197, 197, 197, 197, 196, 196, 195, 194
};

/*============================================================================
 * UTILITY FUNCTIONS
 *============================================================================*/

static uint32_t _rand_state = 12345;

uint8_t random8(void) {
    _rand_state = _rand_state * 1103515245 + 12345;
    return (uint8_t)(_rand_state >> 16);
}

uint16_t random16(void) {
    return ((uint16_t)random8() << 8) | random8();
}

// Fast sine approximation: input 0-255, output 0-255
uint8_t ease_sine(uint8_t x) {
    uint8_t index = x >> 2;  // 0-63
    if (x < 64) return SINE_TABLE[index];
    if (x < 128) return SINE_TABLE[63 - (index - 16)];
    if (x < 192) return 255 - SINE_TABLE[index - 32];
    return 255 - SINE_TABLE[63 - (index - 48)];
}

// HSL to RGB conversion
rgb_t hsl_to_rgb(uint16_t h, uint8_t s, uint8_t l) {
    rgb_t result;
    
    if (s == 0) {
        uint8_t v = (l * 255) / 100;
        result.r = result.g = result.b = v;
        return result;
    }
    
    // Normalize to 0-255 range for calculation
    uint8_t s8 = (s * 255) / 100;
    uint8_t l8 = (l * 255) / 100;
    
    uint8_t c = ((255 - abs(2 * l8 - 255)) * s8) / 255;
    uint16_t h6 = (h * 6);  // h * 6, scaled
    uint8_t x = (c * (255 - abs((h6 / 60) % 512 - 255))) / 255;
    uint8_t m = l8 - c / 2;
    
    uint8_t r, g, b;
    uint8_t sector = h / 60;
    
    switch (sector % 6) {
        case 0: r = c; g = x; b = 0; break;
        case 1: r = x; g = c; b = 0; break;
        case 2: r = 0; g = c; b = x; break;
        case 3: r = 0; g = x; b = c; break;
        case 4: r = x; g = 0; b = c; break;
        default: r = c; g = 0; b = x; break;
    }
    
    result.r = r + m;
    result.g = g + m;
    result.b = b + m;
    return result;
}

// Scale RGB by brightness (0-255)
rgb_t rgb_scale(rgb_t color, uint8_t scale) {
    rgb_t result;
    result.r = ((uint16_t)color.r * scale) >> 8;
    result.g = ((uint16_t)color.g * scale) >> 8;
    result.b = ((uint16_t)color.b * scale) >> 8;
    return result;
}

// Blend two colors: blend=0 -> a, blend=255 -> b
rgb_t rgb_blend(rgb_t a, rgb_t b, uint8_t blend) {
    rgb_t result;
    uint8_t inv = 255 - blend;
    result.r = ((uint16_t)a.r * inv + (uint16_t)b.r * blend) >> 8;
    result.g = ((uint16_t)a.g * inv + (uint16_t)b.g * blend) >> 8;
    result.b = ((uint16_t)a.b * inv + (uint16_t)b.b * blend) >> 8;
    return result;
}

/*============================================================================
 * CORE FUNCTIONS
 *============================================================================*/

void led_strip_init(led_strip_t* strip) {
    memset(strip, 0, sizeof(led_strip_t));
    strip->config.brightness = DEFAULT_BRIGHTNESS;
    strip->running = false;
}

void led_strip_clear(led_strip_t* strip) {
    memset(strip->leds, 0, sizeof(strip->leds));
    hw_show_leds(strip->leds, LED_COUNT);
}

void led_strip_set_pattern(led_strip_t* strip, pattern_config_t* config) {
    memcpy(&strip->config, config, sizeof(pattern_config_t));
    memset(&strip->state, 0, sizeof(pattern_state_t));
    strip->state.direction = 1;
    strip->running = true;
}

void led_strip_update(led_strip_t* strip, uint32_t current_ms) {
    if (!strip->running) return;
    
    // Frame rate limiting
    if (current_ms - strip->state.last_update_ms < FRAME_RATE_MS) {
        return;
    }
    strip->state.last_update_ms = current_ms;
    strip->state.frame++;
    
    // Dispatch to pattern function
    switch (strip->config.type) {
        case PATTERN_RAINBOW:
            pattern_rainbow(strip);
            break;
        case PATTERN_CHASE:
            pattern_chase(strip);
            break;
        case PATTERN_BREATHING:
            pattern_breathing(strip);
            break;
        case PATTERN_SPARKLE:
            pattern_sparkle(strip);
            break;
        default:
            led_strip_clear(strip);
            return;
    }
    
    // Apply global brightness and output
    for (int i = 0; i < LED_COUNT; i++) {
        strip->leds[i] = rgb_scale(strip->leds[i], strip->config.brightness);
    }
    
    hw_show_leds(strip->leds, LED_COUNT);
}

/*============================================================================
 * PATTERN: RAINBOW
 *============================================================================*/

void pattern_rainbow(led_strip_t* strip) {
    rainbow_params_t* p = &strip->config.params.rainbow;
    uint32_t frame = strip->state.frame;
    
    // Calculate hue offset based on speed
    // speed 1-10 maps to rotation rate
    uint16_t hue_offset = (frame * p->speed * 2) % 360;
    
    // Calculate hue spread across strip
    // spread=10 means full 360° rainbow across all LEDs
    uint16_t hue_range = (360 * p->spread) / 10;
    
    for (int i = 0; i < LED_COUNT; i++) {
        // Calculate hue for this LED
        uint16_t hue = (hue_offset + (i * hue_range / LED_COUNT)) % 360;
        
        // Convert to RGB
        strip->leds[i] = hsl_to_rgb(hue, p->saturation, 50);
    }
}

/*============================================================================
 * PATTERN: CHASE
 *============================================================================*/

void pattern_chase(led_strip_t* strip) {
    chase_params_t* p = &strip->config.params.chase;
    pattern_state_t* s = &strip->state;
    
    // Update position based on speed
    // Update every N frames where N decreases with speed
    uint8_t update_interval = 11 - p->speed;  // speed 10 = every frame
    
    if (s->frame % update_interval == 0) {
        // Update position
        s->position += s->direction;
        
        // Handle direction modes
        if (p->direction == DIR_BOUNCE) {
            if (s->position >= LED_COUNT - 1) {
                s->position = LED_COUNT - 1;
                s->direction = -1;
            } else if (s->position <= 0) {
                s->position = 0;
                s->direction = 1;
            }
        } else if (p->direction == DIR_FORWARD) {
            if (s->position >= LED_COUNT) {
                s->position = -p->tail_length;
            }
        } else {  // DIR_REVERSE
            if (s->position < -p->tail_length) {
                s->position = LED_COUNT;
            }
        }
    }
    
    // Render LEDs
    for (int i = 0; i < LED_COUNT; i++) {
        // Distance from head position
        int16_t dist = s->position - i;
        if (p->direction == DIR_REVERSE) {
            dist = i - s->position;
        }
        
        if (dist >= 0 && dist < p->tail_length) {
            // LED is in tail
            if (p->fade_tail) {
                // Fade based on distance from head
                uint8_t fade = 255 - (dist * 255 / p->tail_length);
                strip->leds[i] = rgb_scale(p->color_fg, fade);
            } else {
                strip->leds[i] = p->color_fg;
            }
        } else {
            // Background
            strip->leds[i] = p->color_bg;
        }
    }
}

/*============================================================================
 * PATTERN: BREATHING
 *============================================================================*/

void pattern_breathing(led_strip_t* strip) {
    breathing_params_t* p = &strip->config.params.breathing;
    pattern_state_t* s = &strip->state;
    
    // Update phase based on speed
    // One full breath cycle = 256 steps
    // speed 1 = slow, speed 10 = fast
    float phase_increment = (float)p->speed * 0.02f;
    s->phase += phase_increment;
    if (s->phase >= 1.0f) {
        s->phase -= 1.0f;
    }
    
    // Calculate brightness based on curve type
    uint8_t phase_byte = (uint8_t)(s->phase * 255);
    uint8_t brightness_factor;
    
    switch (p->curve) {
        case CURVE_SINE:
            // Sine wave: smooth in and out
            brightness_factor = ease_sine(phase_byte);
            break;
            
        case CURVE_LINEAR:
            // Triangle wave
            if (phase_byte < 128) {
                brightness_factor = phase_byte * 2;
            } else {
                brightness_factor = (255 - phase_byte) * 2;
            }
            break;
            
        case CURVE_EASE_IN_OUT:
            // Quadratic ease in-out
            if (phase_byte < 128) {
                brightness_factor = (phase_byte * phase_byte) >> 6;
            } else {
                uint8_t t = 255 - phase_byte;
                brightness_factor = 255 - ((t * t) >> 6);
            }
            break;
            
        default:
            brightness_factor = 128;
    }
    
    // Map to min/max brightness range
    uint8_t brightness = p->min_brightness + 
        ((uint16_t)(p->max_brightness - p->min_brightness) * brightness_factor) / 255;
    
    // Apply to all LEDs
    rgb_t color = rgb_scale(p->color, brightness);
    for (int i = 0; i < LED_COUNT; i++) {
        strip->leds[i] = color;
    }
}

/*============================================================================
 * PATTERN: SPARKLE
 *============================================================================*/

void pattern_sparkle(led_strip_t* strip) {
    sparkle_params_t* p = &strip->config.params.sparkle;
    pattern_state_t* s = &strip->state;
    
    // Fade existing sparkles
    uint8_t fade_amount = p->fade_speed;
    for (int i = 0; i < LED_COUNT; i++) {
        if (s->sparkle_brightness[i] > fade_amount) {
            s->sparkle_brightness[i] -= fade_amount;
        } else {
            s->sparkle_brightness[i] = 0;
        }
    }
    
    // Spawn new sparkles based on density
    // density is percentage chance per LED per frame
    // Adjust by speed (higher speed = more frequent checks)
    uint8_t spawn_threshold = 255 - ((uint16_t)p->density * 255 / 100);
    
    for (int i = 0; i < LED_COUNT; i++) {
        if (random8() > spawn_threshold) {
            s->sparkle_brightness[i] = 255;  // Full brightness sparkle
        }
    }
    
    // Render LEDs
    for (int i = 0; i < LED_COUNT; i++) {
        if (s->sparkle_brightness[i] > 0) {
            rgb_t sparkle_color;
            
            switch (p->color_mode) {
                case COLOR_MODE_FIXED:
                    sparkle_color = p->color;
                    break;
                    
                case COLOR_MODE_RANDOM:
                    // Random color per sparkle (seeded by position + frame)
                    sparkle_color = hsl_to_rgb(
                        (i * 37 + s->frame * 7) % 360,  // pseudo-random hue
                        100,
                        50
                    );
                    break;
                    
                case COLOR_MODE_RAINBOW:
                    // Rainbow based on position
                    sparkle_color = hsl_to_rgb(
                        (i * 360 / LED_COUNT) % 360,
                        100,
                        50
                    );
                    break;
                    
                default:
                    sparkle_color = p->color;
            }
            
            // Apply sparkle brightness
            strip->leds[i] = rgb_scale(sparkle_color, s->sparkle_brightness[i]);
        } else {
            // Background color
            strip->leds[i] = p->background;
        }
    }
}

/*============================================================================
 * EXAMPLE USAGE / MAIN LOOP PSEUDOCODE
 *============================================================================*/

#ifdef EXAMPLE_MAIN

/*
 * Example main loop for Arduino/ESP32
 */

led_strip_t strip;

void setup() {
    // Initialize LED strip
    led_strip_init(&strip);
    
    // Configure rainbow pattern
    pattern_config_t config = {
        .type = PATTERN_RAINBOW,
        .brightness = 200,
        .params.rainbow = {
            .speed = 3,
            .saturation = 100,
            .spread = 10  // 1.0
        }
    };
    
    led_strip_set_pattern(&strip, &config);
}

void loop() {
    // Update pattern (handles frame rate internally)
    led_strip_update(&strip, hw_millis());
}

/*
 * Example: Switch to chase pattern
 */
void set_chase_mode() {
    pattern_config_t config = {
        .type = PATTERN_CHASE,
        .brightness = 255,
        .params.chase = {
            .speed = 5,
            .tail_length = 8,
            .color_fg = {0, 255, 255},    // Cyan
            .color_bg = {0, 0, 10},       // Dark blue
            .direction = DIR_BOUNCE,
            .fade_tail = true
        }
    };
    led_strip_set_pattern(&strip, &config);
}

/*
 * Example: Switch to breathing pattern
 */
void set_breathing_mode() {
    pattern_config_t config = {
        .type = PATTERN_BREATHING,
        .brightness = 255,
        .params.breathing = {
            .speed = 3,
            .color = {255, 0, 255},       // Magenta
            .min_brightness = 10,
            .max_brightness = 255,
            .curve = CURVE_SINE
        }
    };
    led_strip_set_pattern(&strip, &config);
}

/*
 * Example: Switch to sparkle pattern
 */
void set_sparkle_mode() {
    pattern_config_t config = {
        .type = PATTERN_SPARKLE,
        .brightness = 255,
        .params.sparkle = {
            .speed = 5,
            .color = {255, 255, 255},     // White
            .density = 8,                  // 8% chance per LED
            .fade_speed = 30,
            .color_mode = COLOR_MODE_RANDOM,
            .background = {5, 5, 5}
        }
    };
    led_strip_set_pattern(&strip, &config);
}

#endif // EXAMPLE_MAIN
