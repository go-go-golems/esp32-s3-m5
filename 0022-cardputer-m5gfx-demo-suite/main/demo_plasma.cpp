#include "demo_plasma.h"

#include <math.h>

namespace {

static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

static inline uint16_t hsv_to_rgb565(uint8_t h, uint8_t s, uint8_t v) {
    if (s == 0) {
        return rgb565(v, v, v);
    }

    const uint8_t region = (uint8_t)(h / 43); // 0..5
    const uint8_t remainder = (uint8_t)((h - (region * 43)) * 6);

    const uint8_t p = (uint8_t)((v * (uint16_t)(255 - s)) >> 8);
    const uint8_t q = (uint8_t)((v * (uint16_t)(255 - ((s * (uint16_t)remainder) >> 8))) >> 8);
    const uint8_t t = (uint8_t)((v * (uint16_t)(255 - ((s * (uint16_t)(255 - remainder)) >> 8))) >> 8);

    uint8_t r = 0, g = 0, b = 0;
    switch (region) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
    }
    return rgb565(r, g, b);
}

static void build_tables(uint8_t sin8[256], uint16_t palette[256]) {
    constexpr float kTwoPi = 6.2831853071795864769f;
    for (int i = 0; i < 256; i++) {
        const float a = (float)i * kTwoPi / 256.0f;
        const float s = sinf(a);
        const int v = (int)((s + 1.0f) * 127.5f);
        sin8[i] = (uint8_t)((v < 0) ? 0 : (v > 255) ? 255 : v);
        palette[i] = hsv_to_rgb565((uint8_t)i, 255, 255);
    }
}

static inline void draw_plasma(uint16_t *dst565, int w, int h, uint32_t t, const uint8_t sin8[256],
                               const uint16_t palette[256]) {
    const uint8_t t1 = (uint8_t)(t & 0xFF);
    const uint8_t t2 = (uint8_t)((t >> 1) & 0xFF);
    const uint8_t t3 = (uint8_t)((t >> 2) & 0xFF);
    const uint8_t t4 = (uint8_t)((t * 3) & 0xFF);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            const uint8_t v1 = sin8[(uint8_t)(x * 3 + t1)];
            const uint8_t v2 = sin8[(uint8_t)(y * 4 + t2)];
            const uint8_t v3 = sin8[(uint8_t)((x * 2 + y * 2) + t3)];
            const uint8_t v4 = sin8[(uint8_t)((x - y) * 2 + t4)];
            const uint16_t sum = (uint16_t)v1 + (uint16_t)v2 + (uint16_t)v3 + (uint16_t)v4;
            const uint8_t idx = (uint8_t)(sum >> 2);
            dst565[y * w + x] = palette[idx];
        }
    }
}

} // namespace

void plasma_init(PlasmaState &s) {
    build_tables(s.sin8, s.palette);
    s.t = 0;
    s.inited = true;
}

void plasma_render(M5Canvas &body, PlasmaState &s) {
    if (!s.inited) {
        plasma_init(s);
    }

    body.setTextSize(1, 1);
    body.setTextDatum(lgfx::textdatum_t::top_left);

    auto *dst = (uint16_t *)body.getBuffer();
    draw_plasma(dst, body.width(), body.height(), s.t, s.sin8, s.palette);
    s.t += 2;
}
