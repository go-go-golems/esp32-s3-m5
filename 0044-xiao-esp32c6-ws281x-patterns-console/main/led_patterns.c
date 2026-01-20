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
    static const uint8_t sine_table[64] = {
        0,   6,  12,  18,  25,  31,  37,  43,  49,  55,  61,  67,  73,  79,  85,  90,
        96, 101, 106, 112, 117, 122, 126, 131, 135, 140, 144, 148, 152, 155, 159, 162,
        165, 168, 171, 174, 176, 179, 181, 183, 185, 187, 188, 190, 191, 192, 193, 194,
        195, 196, 196, 197, 197, 197, 197, 197, 197, 197, 197, 197, 196, 196, 195, 194,
    };

    const uint8_t index = x >> 2; // 0-63
    if (x < 64) {
        return sine_table[index];
    }
    if (x < 128) {
        return sine_table[63 - (index - 16)];
    }
    if (x < 192) {
        return (uint8_t)(255 - sine_table[index - 32]);
    }
    return (uint8_t)(255 - sine_table[63 - (index - 48)]);
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

static uint8_t rand8(led_patterns_t *p)
{
    return (uint8_t)(rand32_next(p) >> 16);
}

static void state_reset(led_patterns_t *p)
{
    p->st.frame = 0;
    p->st.last_step_ms = 0;
    p->st.chase_pos = 0;
    p->st.chase_dir = 1;
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
                .chase_pos = 0,
                .chase_dir = 1,
                .sparkle_bri = NULL,
                .sparkle_len = 0,
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
    const uint32_t speed = cfg->speed ? cfg->speed : 1;
    const uint32_t sat = (cfg->saturation > 100) ? 100 : cfg->saturation;
    const uint32_t spread_x10 = cfg->spread_x10 ? cfg->spread_x10 : 10;

    const uint32_t hue_offset = (uint32_t)(((uint64_t)now_ms * speed * 360ULL) / 10000ULL) % 360;
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

static void chase_step_update(led_patterns_t *p, uint32_t now_ms)
{
    led_chase_cfg_t *cfg = &p->cfg.u.chase;
    const uint8_t speed = cfg->speed ? cfg->speed : 1;
    const uint32_t step_ms = 200u / speed; // speed 1 => 200ms, speed 10 => 20ms

    if (p->st.last_step_ms == 0) {
        p->st.last_step_ms = now_ms;
        return;
    }
    if ((uint32_t)(now_ms - p->st.last_step_ms) < step_ms) {
        return;
    }
    p->st.last_step_ms = now_ms;

    p->st.chase_pos += p->st.chase_dir;

    const int16_t tail = (int16_t)(cfg->tail_len ? cfg->tail_len : 1);
    switch (cfg->dir) {
    case LED_DIR_BOUNCE:
        if (p->st.chase_pos >= (int16_t)(p->led_count - 1)) {
            p->st.chase_pos = (int16_t)(p->led_count - 1);
            p->st.chase_dir = -1;
        } else if (p->st.chase_pos <= 0) {
            p->st.chase_pos = 0;
            p->st.chase_dir = 1;
        }
        break;
    case LED_DIR_REVERSE:
        if (p->st.chase_pos < -tail) {
            p->st.chase_pos = (int16_t)p->led_count;
        }
        break;
    case LED_DIR_FORWARD:
    default:
        if (p->st.chase_pos >= (int16_t)p->led_count) {
            p->st.chase_pos = -tail;
        }
        break;
    }
}

static void render_chase(led_patterns_t *p, uint32_t now_ms, led_ws281x_t *strip)
{
    led_chase_cfg_t *cfg = &p->cfg.u.chase;
    if (cfg->tail_len == 0) {
        cfg->tail_len = 1;
    }
    chase_step_update(p, now_ms);

    for (uint16_t i = 0; i < p->led_count; i++) {
        int16_t dist = p->st.chase_pos - (int16_t)i;
        if (cfg->dir == LED_DIR_REVERSE) {
            dist = (int16_t)i - p->st.chase_pos;
        }
        if (dist >= 0 && dist < (int16_t)cfg->tail_len) {
            led_rgb8_t c = cfg->fg;
            if (cfg->fade_tail) {
                const uint8_t fade_u8 = (uint8_t)(255u - ((uint32_t)dist * 255u) / cfg->tail_len);
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
    const uint8_t speed = cfg->speed ? cfg->speed : 1;

    uint32_t period_ms = 2500u / speed;
    if (period_ms < 200) {
        period_ms = 200;
    }

    const uint8_t phase_u8 = (uint8_t)(((uint64_t)(now_ms % period_ms) * 256ULL) / period_ms);
    const uint8_t wave_u8 = curve_eval_u8(cfg->curve, phase_u8);

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
    (void)now_ms;
    led_sparkle_cfg_t *cfg = &p->cfg.u.sparkle;
    if (cfg->fade_speed == 0) {
        cfg->fade_speed = 1;
    }
    if (cfg->density_pct > 100) {
        cfg->density_pct = 100;
    }

    // Fade existing sparkles.
    const uint8_t fade_amount = cfg->fade_speed;
    for (uint16_t i = 0; i < p->led_count; i++) {
        uint8_t v = p->st.sparkle_bri[i];
        p->st.sparkle_bri[i] = (v > fade_amount) ? (uint8_t)(v - fade_amount) : 0;
    }

    // Spawn new sparkles.
    const uint8_t spawn_threshold = (uint8_t)(255 - ((uint16_t)cfg->density_pct * 255u) / 100u);
    for (uint16_t i = 0; i < p->led_count; i++) {
        if (rand8(p) > spawn_threshold) {
            p->st.sparkle_bri[i] = 255;
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

