#include "led_patterns.h"

#include <string.h>

#include "esp_check.h"
#include "esp_heap_caps.h"

static const char *TAG = "led_patterns";

static inline uint8_t clamp_u8(uint32_t v)
{
    if (v > 255) {
        return 255;
    }
    return (uint8_t)v;
}

static inline uint8_t scale_u8(uint8_t v, uint32_t pct)
{
    return (uint8_t)(((uint32_t)v * pct) / 100u);
}

static inline led_rgb8_t rgb_scale_u8(led_rgb8_t c, uint8_t scale)
{
    return (led_rgb8_t){
        .r = (uint8_t)(((uint32_t)c.r * scale) / 255u),
        .g = (uint8_t)(((uint32_t)c.g * scale) / 255u),
        .b = (uint8_t)(((uint32_t)c.b * scale) / 255u),
    };
}

static uint8_t ease_sine_u8(uint8_t x)
{
    // Full-cycle sine wave mapped to [0..255]. Generated from:
    // round((sin(2*pi*i/256)+1)/2*255), i=0..255
    static const uint8_t sine_u8[256] = {
        128, 131, 134, 137, 140, 143, 146, 149, 152, 155, 158, 162, 165, 167, 170, 173,
        176, 179, 182, 185, 188, 190, 193, 196, 198, 201, 203, 206, 208, 211, 213, 215,
        218, 220, 222, 224, 226, 228, 230, 232, 234, 235, 237, 238, 240, 241, 243, 244,
        245, 246, 248, 249, 250, 250, 251, 252, 253, 253, 254, 254, 254, 255, 255, 255,
        255, 255, 255, 255, 254, 254, 254, 253, 253, 252, 251, 250, 250, 249, 248, 246,
        245, 244, 243, 241, 240, 238, 237, 235, 234, 232, 230, 228, 226, 224, 222, 220,
        218, 215, 213, 211, 208, 206, 203, 201, 198, 196, 193, 190, 188, 185, 182, 179,
        176, 173, 170, 167, 165, 162, 158, 155, 152, 149, 146, 143, 140, 137, 134, 131,
        128, 124, 121, 118, 115, 112, 109, 106, 103, 100,  97,  93,  90,  88,  85,  82,
         79,  76,  73,  70,  67,  65,  62,  59,  57,  54,  52,  49,  47,  44,  42,  40,
         37,  35,  33,  31,  29,  27,  25,  23,  21,  20,  18,  17,  15,  14,  12,  11,
         10,   9,   7,   6,   5,   5,   4,   3,   2,   2,   1,   1,   1,   0,   0,   0,
          0,   0,   0,   0,   1,   1,   1,   2,   2,   3,   4,   5,   5,   6,   7,   9,
         10,  11,  12,  14,  15,  17,  18,  20,  21,  23,  25,  27,  29,  31,  33,  35,
         37,  40,  42,  44,  47,  49,  52,  54,  57,  59,  62,  65,  67,  70,  73,  76,
         79,  82,  85,  88,  90,  93,  97, 100, 103, 106, 109, 112, 115, 118, 121, 124,
    };
    return sine_u8[x];
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

static uint32_t rand32_next(led_patterns_t *p)
{
    // LCG: same constants as common libc rand implementations.
    p->st.rng = p->st.rng * 1103515245u + 12345u;
    return p->st.rng;
}

static void state_reset(led_patterns_t *p)
{
    p->st.frame = 0;
    p->st.last_step_ms = 0;
    p->st.chase_pos_q16 = 0;
    p->st.chase_dir = 1;
    p->st.sparkle_accum_q16 = 0;
    if (p->st.sparkle_bri && p->st.sparkle_len) {
        memset(p->st.sparkle_bri, 0, p->st.sparkle_len);
    }
}

esp_err_t led_patterns_init(led_patterns_t *p, uint16_t led_count)
{
    ESP_RETURN_ON_FALSE(p && led_count > 0, ESP_ERR_INVALID_ARG, TAG, "invalid arg");
    *p = (led_patterns_t){
        .led_count = led_count,
        .cfg = {
            .type = LED_PATTERN_RAINBOW,
            .global_brightness_pct = 25,
            .u.rainbow =
                {
                    .speed = 5,
                    .saturation = 100,
                    .spread_x10 = 10,
                },
        },
        .st =
            {
                .frame = 0,
                .last_step_ms = 0,
                .chase_pos_q16 = 0,
                .chase_dir = 1,
                .sparkle_bri = NULL,
                .sparkle_len = 0,
                .sparkle_accum_q16 = 0,
                .rng = 0x12345678u,
            },
    };

    p->st.sparkle_bri = (uint8_t *)heap_caps_malloc((size_t)led_count, MALLOC_CAP_DEFAULT);
    ESP_RETURN_ON_FALSE(p->st.sparkle_bri, ESP_ERR_NO_MEM, TAG, "malloc sparkle_bri failed");
    p->st.sparkle_len = led_count;
    memset(p->st.sparkle_bri, 0, (size_t)led_count);

    // Seed RNG differently per boot (best-effort). We avoid esp_random() to keep this module portable.
    p->st.rng ^= (uint32_t)(uintptr_t)p;
    p->st.rng ^= ((uint32_t)led_count << 16);

    return ESP_OK;
}

void led_patterns_deinit(led_patterns_t *p)
{
    if (!p) {
        return;
    }
    if (p->st.sparkle_bri) {
        free(p->st.sparkle_bri);
    }
    *p = (led_patterns_t){0};
}

void led_patterns_set_cfg(led_patterns_t *p, const led_pattern_cfg_t *cfg)
{
    if (!p || !cfg) {
        return;
    }
    p->cfg = *cfg;
    if (p->cfg.global_brightness_pct == 0) {
        p->cfg.global_brightness_pct = 1;
    }
    state_reset(p);
}

static void render_off(led_patterns_t *p, led_ws281x_t *strip)
{
    (void)p;
    led_ws281x_clear(strip);
}

static void render_rainbow(led_patterns_t *p, uint32_t now_ms, led_ws281x_t *strip)
{
    const led_rainbow_cfg_t *cfg = &p->cfg.u.rainbow;
    const uint32_t speed_rpm = (cfg->speed > 20) ? 20 : cfg->speed;
    const uint32_t sat = (cfg->saturation > 100) ? 100 : cfg->saturation;
    const uint32_t spread_x10 = cfg->spread_x10 ? cfg->spread_x10 : 10;

    // speed is rotations per minute (RPM). hue_offset is degrees [0..359].
    const uint32_t hue_offset = (speed_rpm == 0) ? 0 : (uint32_t)(((uint64_t)now_ms * speed_rpm * 360ULL) / 60000ULL) % 360;
    const uint32_t hue_range = (360 * spread_x10) / 10;

    for (uint16_t i = 0; i < p->led_count; i++) {
        const uint32_t hue = (hue_offset + ((uint32_t)i * hue_range) / p->led_count) % 360;
        uint32_t r = 0, g = 0, b = 0;
        hsv2rgb(hue, sat, 100, &r, &g, &b);
        led_ws281x_set_pixel_rgb(
            strip,
            i,
            (led_rgb8_t){
                .r = clamp_u8(r),
                .g = clamp_u8(g),
                .b = clamp_u8(b),
            },
            p->cfg.global_brightness_pct);
    }
}

static uint32_t chase_train_period(const led_chase_cfg_t *cfg)
{
    const uint32_t tail = cfg->tail_len ? cfg->tail_len : 1;
    const uint32_t gap = cfg->gap_len;
    const uint32_t period = tail + gap;
    return period ? period : 1;
}

static void chase_update_pos(led_patterns_t *p, uint32_t now_ms)
{
    led_chase_cfg_t *cfg = &p->cfg.u.chase;
    if (p->st.last_step_ms == 0) {
        p->st.last_step_ms = now_ms;
        return;
    }
    const uint32_t dt_ms = (uint32_t)(now_ms - p->st.last_step_ms);
    p->st.last_step_ms = now_ms;

    const uint32_t speed_lps = cfg->speed; // LEDs per second
    if (speed_lps == 0 || p->led_count == 0) {
        return;
    }

    // delta in Q16.16 (leds)
    const uint32_t delta_q16 = (uint32_t)(((uint64_t)speed_lps * (uint64_t)dt_ms * 65536ULL) / 1000ULL);

    if (cfg->dir == LED_DIR_BOUNCE) {
        const int64_t max_q16 = ((int64_t)(p->led_count - 1) << 16);
        int64_t pos = (int64_t)p->st.chase_pos_q16;
        pos += (int64_t)p->st.chase_dir * (int64_t)delta_q16;
        if (pos > max_q16) {
            pos = max_q16;
            p->st.chase_dir = -1;
        } else if (pos < 0) {
            pos = 0;
            p->st.chase_dir = 1;
        }
        p->st.chase_pos_q16 = (uint32_t)pos;
        return;
    }

    const uint32_t period_q16 = ((uint32_t)p->led_count) << 16;
    int64_t pos = (int64_t)p->st.chase_pos_q16;
    if (cfg->dir == LED_DIR_REVERSE) {
        pos -= (int64_t)delta_q16;
    } else {
        pos += (int64_t)delta_q16;
    }
    // Wrap to [0..period_q16)
    pos %= (int64_t)period_q16;
    if (pos < 0) {
        pos += (int64_t)period_q16;
    }
    p->st.chase_pos_q16 = (uint32_t)pos;
}

static void render_chase(led_patterns_t *p, uint32_t now_ms, led_ws281x_t *strip)
{
    led_chase_cfg_t *cfg = &p->cfg.u.chase;
    if (cfg->tail_len == 0) {
        cfg->tail_len = 1;
    }
    chase_update_pos(p, now_ms);

    const uint16_t head_base = (uint16_t)(p->st.chase_pos_q16 >> 16);
    const uint32_t train_period = chase_train_period(cfg);
    uint32_t trains = cfg->trains ? cfg->trains : 1;
    if (cfg->dir == LED_DIR_BOUNCE) {
        trains = 1;
    }
    if (train_period > 0) {
        const uint32_t max_trains = (uint32_t)p->led_count / train_period;
        if (max_trains == 0) {
            trains = 1;
        } else if (trains > max_trains) {
            trains = max_trains;
        }
    }
    if (trains == 0) {
        trains = 1;
    }

    for (uint16_t i = 0; i < p->led_count; i++) {
        uint16_t best_dist = 0xffffu;
        for (uint32_t t = 0; t < trains; t++) {
            const uint16_t head = (uint16_t)((head_base + (uint16_t)(t * train_period)) % p->led_count);
            uint16_t dist = 0;
            if (cfg->dir == LED_DIR_REVERSE) {
                dist = (uint16_t)((i + p->led_count - head) % p->led_count);
            } else {
                dist = (uint16_t)((head + p->led_count - i) % p->led_count);
            }
            if (dist < best_dist) {
                best_dist = dist;
                if (best_dist == 0) {
                    break;
                }
            }
        }

        if (best_dist < cfg->tail_len) {
            led_rgb8_t c = cfg->fg;
            if (cfg->fade_tail) {
                const uint8_t fade_u8 = (uint8_t)(255u - ((uint32_t)best_dist * 255u) / cfg->tail_len);
                c = rgb_scale_u8(c, fade_u8);
            }
            led_ws281x_set_pixel_rgb(strip, i, c, p->cfg.global_brightness_pct);
        } else {
            led_ws281x_set_pixel_rgb(strip, i, cfg->bg, p->cfg.global_brightness_pct);
        }
    }
}

static uint8_t curve_eval_u8(led_curve_t curve, uint8_t phase_u8)
{
    switch (curve) {
    case LED_CURVE_SINE:
        return ease_sine_u8(phase_u8);
    case LED_CURVE_LINEAR:
        if (phase_u8 < 128) {
            return (uint8_t)(phase_u8 * 2);
        }
        return (uint8_t)((255 - phase_u8) * 2);
    case LED_CURVE_EASE_IN_OUT:
        if (phase_u8 < 128) {
            return (uint8_t)(((uint16_t)phase_u8 * (uint16_t)phase_u8) >> 6);
        } else {
            const uint8_t t = (uint8_t)(255 - phase_u8);
            return (uint8_t)(255 - (((uint16_t)t * (uint16_t)t) >> 6));
        }
    default:
        return 128;
    }
}

static void render_breathing(led_patterns_t *p, uint32_t now_ms, led_ws281x_t *strip)
{
    led_breathing_cfg_t *cfg = &p->cfg.u.breathing;
    const uint8_t bpm = (cfg->speed > 20) ? 20 : cfg->speed;
    uint8_t wave_u8 = 255;
    if (bpm != 0) {
        // speed is breaths per minute. 1 => 60s period, 20 => 3s period.
        const uint32_t period_ms = 60000u / bpm;
        const uint8_t phase_u8 = (uint8_t)(((uint64_t)(now_ms % period_ms) * 256ULL) / period_ms);
        wave_u8 = curve_eval_u8(cfg->curve, phase_u8);
    }

    uint8_t min_b = cfg->min_bri;
    uint8_t max_b = cfg->max_bri;
    if (min_b > max_b) {
        const uint8_t t = min_b;
        min_b = max_b;
        max_b = t;
    }
    const uint8_t intensity = (uint8_t)(min_b + ((uint16_t)(max_b - min_b) * wave_u8) / 255u);
    const led_rgb8_t c = rgb_scale_u8(cfg->color, intensity);

    for (uint16_t i = 0; i < p->led_count; i++) {
        led_ws281x_set_pixel_rgb(strip, i, c, p->cfg.global_brightness_pct);
    }
}

static void render_sparkle(led_patterns_t *p, uint32_t now_ms, led_ws281x_t *strip)
{
    led_sparkle_cfg_t *cfg = &p->cfg.u.sparkle;
    if (cfg->fade_speed == 0) {
        cfg->fade_speed = 1;
    }
    if (cfg->density_pct > 100) {
        cfg->density_pct = 100;
    }

    // Fade existing sparkles.
    uint32_t dt_ms = 25;
    if (p->st.last_step_ms == 0) {
        p->st.last_step_ms = now_ms;
        dt_ms = 0;
    } else {
        dt_ms = (uint32_t)(now_ms - p->st.last_step_ms);
        p->st.last_step_ms = now_ms;
    }

    uint8_t fade_amount = 0;
    if (dt_ms) {
        uint32_t fade_amount_u32 = ((uint32_t)cfg->fade_speed * dt_ms + 24u) / 25u; // baseline ~25ms
        if (fade_amount_u32 < 1u) {
            fade_amount_u32 = 1u;
        } else if (fade_amount_u32 > 255u) {
            fade_amount_u32 = 255u;
        }
        fade_amount = (uint8_t)fade_amount_u32;
    }
    uint32_t active = 0;
    for (uint16_t i = 0; i < p->led_count; i++) {
        uint8_t v = p->st.sparkle_bri[i];
        v = (v > fade_amount) ? (uint8_t)(v - fade_amount) : 0;
        p->st.sparkle_bri[i] = v;
        if (v) {
            active++;
        }
    }

    // Spawn new sparkles (time-based rate + cap on concurrent sparkles).
    const uint32_t max_active = ((uint32_t)p->led_count * (uint32_t)cfg->density_pct) / 100u;
    if (dt_ms && cfg->speed && (max_active == 0 || active < max_active)) {
        const uint8_t speed = (cfg->speed > 20) ? 20 : cfg->speed;
        // speed 0..20 maps to 0.0..4.0 sparkles/sec (rate_x10 = speed*2).
        const uint32_t rate_x10 = (uint32_t)speed * 2u; // sparkles/sec * 10
        const uint32_t add_q16 = (uint32_t)(((uint64_t)rate_x10 * (uint64_t)dt_ms * 65536ULL) / 10000ULL);
        p->st.sparkle_accum_q16 += add_q16;
        uint32_t spawn_n = p->st.sparkle_accum_q16 >> 16;
        p->st.sparkle_accum_q16 &= 0xffffu;

        if (max_active > 0) {
            const uint32_t room = max_active - active;
            if (spawn_n > room) {
                spawn_n = room;
            }
        }

        uint32_t tries = 0;
        while (spawn_n && tries < (uint32_t)p->led_count * 2u) {
            const uint16_t idx = (uint16_t)(rand32_next(p) % p->led_count);
            if (p->st.sparkle_bri[idx] == 0) {
                p->st.sparkle_bri[idx] = 255;
                spawn_n--;
            }
            tries++;
        }
    }

    // Render.
    for (uint16_t i = 0; i < p->led_count; i++) {
        const uint8_t bri = p->st.sparkle_bri[i];
        if (bri == 0) {
            led_ws281x_set_pixel_rgb(strip, i, cfg->background, p->cfg.global_brightness_pct);
            continue;
        }

        led_rgb8_t sparkle_color = cfg->color;
        switch (cfg->color_mode) {
        case LED_SPARKLE_RANDOM: {
            const uint32_t hue = ((uint32_t)i * 37u + p->st.frame * 7u) % 360u;
            uint32_t r = 0, g = 0, b = 0;
            hsv2rgb(hue, 100, 100, &r, &g, &b);
            sparkle_color = (led_rgb8_t){.r = clamp_u8(r), .g = clamp_u8(g), .b = clamp_u8(b)};
            break;
        }
        case LED_SPARKLE_RAINBOW: {
            const uint32_t hue = ((uint32_t)i * 360u) / p->led_count;
            uint32_t r = 0, g = 0, b = 0;
            hsv2rgb(hue, 100, 100, &r, &g, &b);
            sparkle_color = (led_rgb8_t){.r = clamp_u8(r), .g = clamp_u8(g), .b = clamp_u8(b)};
            break;
        }
        case LED_SPARKLE_FIXED:
        default:
            break;
        }

        led_ws281x_set_pixel_rgb(strip, i, rgb_scale_u8(sparkle_color, bri), p->cfg.global_brightness_pct);
    }
}

void led_patterns_render_to_ws281x(led_patterns_t *p, uint32_t now_ms, led_ws281x_t *strip)
{
    if (!p || !strip) {
        return;
    }
    if (p->led_count != strip->cfg.led_count) {
        return;
    }

    switch (p->cfg.type) {
    case LED_PATTERN_OFF:
        render_off(p, strip);
        break;
    case LED_PATTERN_RAINBOW:
        render_rainbow(p, now_ms, strip);
        break;
    case LED_PATTERN_CHASE:
        render_chase(p, now_ms, strip);
        break;
    case LED_PATTERN_BREATHING:
        render_breathing(p, now_ms, strip);
        break;
    case LED_PATTERN_SPARKLE:
        render_sparkle(p, now_ms, strip);
        break;
    default:
        render_off(p, strip);
        break;
    }

    p->st.frame++;
}
