#include "terrain_poster.h"

#include <math.h>
#include <string.h>

#include "framebuffer.h"

namespace {

// 8×8 Bayer matrix, thresholds 0..63.  The matrix is locked to LCD pixels so
// manual rotation changes scene geometry without temporal noise or shimmer.
static const uint8_t kBayer8[8][8] = {
    { 0, 48, 12, 60, 3, 51, 15, 63 },
    { 32, 16, 44, 28, 35, 19, 47, 31 },
    { 8, 56, 4, 52, 11, 59, 7, 55 },
    { 40, 24, 36, 20, 43, 27, 39, 23 },
    { 2, 50, 14, 62, 1, 49, 13, 61 },
    { 34, 18, 46, 30, 33, 17, 45, 29 },
    { 10, 58, 6, 54, 9, 57, 5, 53 },
    { 42, 26, 38, 22, 41, 25, 37, 21 },
};

static inline int clamp_i(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static inline float clamp_f(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static inline int poster_pixel_size(const render_params_t* p) {
    if (!p) return 1;
    return clamp_i(p->pixel_size, 1, 6);
}

static inline int mask_radius(const render_params_t* p) {
    const float aperture = p ? clamp_f(p->aperture, 0.40f, 1.0f) : 0.97f;
    return clamp_i(static_cast<int>(119.0f * aperture), 48, 119);
}

static inline bool inside_round_lcd(int x, int y, const render_params_t* p) {
    const int r = mask_radius(p);
    const int dx = x - 120;
    const int dy = y - 120;
    return dx * dx + dy * dy <= r * r;
}

static inline int apply_contrast(int density, const render_params_t* p) {
    const float c = p ? clamp_f(p->contrast, 0.4f, 3.0f) : 1.4f;
    const float centered = (static_cast<float>(density) - 32.0f) * c + 32.0f;
    return clamp_i(static_cast<int>(centered), 0, 64);
}

static inline bool dither_on(int x, int y, int density, const render_params_t* p) {
    density = apply_contrast(density, p);
    if (density <= 0) return false;
    if (density >= 64) return true;
    const int px = poster_pixel_size(p);
    const int sx = (x / px) * px;
    const int sy = (y / px) * px;
    return density > kBayer8[sy & 7][sx & 7];
}

static inline void set_solid_if_visible(uint8_t* fb, int x, int y, uint8_t color, const render_params_t* p) {
    if (x >= 0 && x < FB_WIDTH && y >= 0 && y < FB_HEIGHT && inside_round_lcd(x, y, p)) {
        fb_set(fb, x, y, color);
    }
}

static inline void set_if_visible(uint8_t* fb, int x, int y, uint8_t color, const render_params_t* p) {
    if (x < 0 || x >= FB_WIDTH || y < 0 || y >= FB_HEIGHT || !inside_round_lcd(x, y, p)) return;

    const int px = poster_pixel_size(p);
    if (px <= 1) {
        fb_set(fb, x, y, color);
        return;
    }

    const int x0 = (x / px) * px;
    const int y0 = (y / px) * px;
    for (int yy = y0; yy < y0 + px && yy < FB_HEIGHT; ++yy) {
        for (int xx = x0; xx < x0 + px && xx < FB_WIDTH; ++xx) {
            if (inside_round_lcd(xx, yy, p)) fb_set(fb, xx, yy, color);
        }
    }
}

// ─── Small bitmap UI font ──────────────────────────────────

static const uint8_t GLYPH_A[7] = {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
static const uint8_t GLYPH_B[7] = {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E};
static const uint8_t GLYPH_C[7] = {0x0F, 0x10, 0x10, 0x10, 0x10, 0x10, 0x0F};
static const uint8_t GLYPH_D[7] = {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E};
static const uint8_t GLYPH_E[7] = {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F};
static const uint8_t GLYPH_I[7] = {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E};
static const uint8_t GLYPH_L[7] = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F};
static const uint8_t GLYPH_N[7] = {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11};
static const uint8_t GLYPH_O[7] = {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
static const uint8_t GLYPH_P[7] = {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10};
static const uint8_t GLYPH_R[7] = {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11};
static const uint8_t GLYPH_S[7] = {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E};
static const uint8_t GLYPH_T[7] = {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04};
static const uint8_t GLYPH_U[7] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
static const uint8_t GLYPH_0[7] = {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E};
static const uint8_t GLYPH_1[7] = {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E};
static const uint8_t GLYPH_2[7] = {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F};
static const uint8_t GLYPH_3[7] = {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E};
static const uint8_t GLYPH_4[7] = {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02};
static const uint8_t GLYPH_5[7] = {0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E};
static const uint8_t GLYPH_DASH[7] = {0, 0, 0, 0x0E, 0, 0, 0};

static const uint8_t* glyph_for(char c) {
    switch (c) {
        case 'A': return GLYPH_A;
        case 'B': return GLYPH_B;
        case 'C': return GLYPH_C;
        case 'D': return GLYPH_D;
        case 'E': return GLYPH_E;
        case 'I': return GLYPH_I;
        case 'L': return GLYPH_L;
        case 'N': return GLYPH_N;
        case 'O': return GLYPH_O;
        case 'P': return GLYPH_P;
        case 'R': return GLYPH_R;
        case 'S': return GLYPH_S;
        case 'T': return GLYPH_T;
        case 'U': return GLYPH_U;
        case '0': return GLYPH_0;
        case '1': return GLYPH_1;
        case '2': return GLYPH_2;
        case '3': return GLYPH_3;
        case '4': return GLYPH_4;
        case '5': return GLYPH_5;
        case '-': return GLYPH_DASH;
        default: return nullptr;
    }
}

static int text_width(const char* text, int scale) {
    int w = 0;
    for (const char* p = text; *p; ++p) {
        w += (*p == ' ') ? 4 * scale : 6 * scale;
    }
    return w > 0 ? w - scale : 0;
}

static void draw_text(uint8_t* fb, int x, int y, const char* text, int scale, uint8_t color,
                      const render_params_t* p) {
    int cursor = x;
    for (const char* ch = text; *ch; ++ch) {
        if (*ch == ' ') {
            cursor += 4 * scale;
            continue;
        }
        const uint8_t* g = glyph_for(*ch);
        if (!g) {
            cursor += 6 * scale;
            continue;
        }
        for (int gy = 0; gy < 7; ++gy) {
            for (int gx = 0; gx < 5; ++gx) {
                if (g[gy] & (1 << (4 - gx))) {
                    for (int yy = 0; yy < scale; ++yy) {
                        for (int xx = 0; xx < scale; ++xx) {
                            // Text/UI is intentionally solid and independent of
                            // scene pixel size/dither. Only the scene artwork is
                            // dithered/pixel-blocked.
                            set_solid_if_visible(fb, cursor + gx * scale + xx, y + gy * scale + yy, color, p);
                        }
                    }
                }
            }
        }
        cursor += 6 * scale;
    }
}

static void clear_rect_visible(uint8_t* fb, int x0, int y0, int w, int h, const render_params_t* p) {
    for (int y = y0; y < y0 + h; ++y) {
        if (y < 0 || y >= FB_HEIGHT) continue;
        for (int x = x0; x < x0 + w; ++x) {
            if (x < 0 || x >= FB_WIDTH) continue;
            if (inside_round_lcd(x, y, p)) fb_set(fb, x, y, COLOR_BLACK);
        }
    }
}

static void draw_common_ui(uint8_t* fb, const char* title, int scene_number, const render_params_t* p) {
    // Scene content is intentionally allowed to be graphic and dense, but text
    // and status marks need reserved quiet areas for readability on the real LCD.
    clear_rect_visible(fb, 50, 18, 140, 28, p);
    clear_rect_visible(fb, 70, 196, 100, 34, p);

    const int title_x = (FB_WIDTH - text_width(title, 2)) / 2;
    draw_text(fb, title_x, 25, title, 2, COLOR_WARM, p);

    const int y = 205;
    for (int i = 0; i < 5; ++i) {
        fb_fill_circle(fb, 98 + i * 11, y, 3, i == scene_number - 1 ? COLOR_WARM : COLOR_HIGH);
    }

    char code[] = "BIP-001";
    code[6] = static_cast<char>('0' + scene_number);
    draw_text(fb, 99, 216, code, 1, COLOR_HIGH, p);

    if (!p || p->debug_ui) {
        // Palette probes: keep while tuning so byte-order/palette bugs are obvious.
        fb_fill_rect(fb, 8, 8, 10, 10, COLOR_BLACK);
        fb_fill_rect(fb, 20, 8, 10, 10, COLOR_WARM);
        fb_fill_rect(fb, 32, 8, 10, 10, COLOR_COOL);
        fb_fill_rect(fb, 44, 8, 10, 10, COLOR_HIGH);
    }
}

static void draw_starfield(uint8_t* fb, float angle, const render_params_t* p) {
    for (int i = 0; i < 36; ++i) {
        const float a = i * 2.3999632f + angle * 0.06f;
        const float r = 18.0f + static_cast<float>((i * 37) % 94);
        const int x = 120 + static_cast<int>(cosf(a) * r);
        const int y = 120 + static_cast<int>(sinf(a) * r);
        if (((i * 17) & 3) == 0) {
            set_if_visible(fb, x, y, COLOR_WARM, p);
        } else {
            set_if_visible(fb, x, y, COLOR_HIGH, p);
        }
    }
}

// ─── TERRAIN ────────────────────────────────────────────────

static int ridge_y_for_x(int x, float angle) {
    const float xf = static_cast<float>(x);
    const float orbit = sinf(angle);
    const float orbit2 = cosf(angle * 0.7f);
    const float peak_x = 135.0f + orbit * 42.0f;
    const float shoulder_x = 70.0f + orbit * 22.0f;
    const float right_cut_x = 190.0f + orbit2 * 16.0f;

    const float peak = expf(-((xf - peak_x) * (xf - peak_x)) / (2.0f * 42.0f * 42.0f));
    const float shoulder = expf(-((xf - shoulder_x) * (xf - shoulder_x)) / (2.0f * 70.0f * 70.0f));
    const float right_cut = expf(-((xf - right_cut_x) * (xf - right_cut_x)) / (2.0f * 38.0f * 38.0f));
    const float wave = sinf(xf * 0.045f + angle * 2.0f) * 3.0f +
                       sinf(xf * 0.019f - angle * 1.3f) * 5.0f;
    const float y = 176.0f - peak * 54.0f - shoulder * 9.0f + right_cut * 8.0f + wave;
    return clamp_i(static_cast<int>(y), 104, 196);
}

static void draw_terrain(uint8_t* fb, const render_params_t* p) {
    const float angle = p ? p->camera_angle : 0.0f;
    for (int x = 0; x < FB_WIDTH; ++x) {
        const int ridge = ridge_y_for_x(x, angle);
        const int floor_y = 218;
        for (int y = ridge; y < FB_HEIGHT; ++y) {
            if (!inside_round_lcd(x, y, p)) continue;
            const int denom = floor_y - ridge;
            int depth = denom > 0 ? ((y - ridge) * 64) / denom : 64;
            depth = clamp_i(depth, 0, 64);
            int texture = ((x * 13 + y * 7) & 15) - 7;
            int density = 59 - (depth * 44) / 64 + texture / 2;
            if (y - ridge <= 2) density = 42 + texture;
            if (dither_on(x, y, clamp_i(density, 4, 62), p)) set_if_visible(fb, x, y, COLOR_COOL, p);
        }
    }
}

static void draw_sun(uint8_t* fb, const render_params_t* p) {
    const float angle = p ? p->camera_angle : 0.0f;
    const int cx = 184 + static_cast<int>(sinf(angle * 0.5f) * 10.0f);
    const int cy = 56 + static_cast<int>(cosf(angle * 0.5f) * 4.0f);
    const int halo_r = 29;
    const int core_r = 18;
    for (int y = cy - halo_r; y <= cy + halo_r; ++y) {
        if (y < 0 || y >= FB_HEIGHT) continue;
        for (int x = cx - halo_r; x <= cx + halo_r; ++x) {
            if (x < 0 || x >= FB_WIDTH || !inside_round_lcd(x, y, p)) continue;
            const int dx = x - cx;
            const int dy = y - cy;
            const int d2 = dx * dx + dy * dy;
            if (d2 > halo_r * halo_r) continue;
            int density = d2 <= core_r * core_r
                              ? 62
                              : static_cast<int>(48.0f - (sqrtf(static_cast<float>(d2)) - core_r) * 3.0f);
            if (dither_on(x, y, clamp_i(density, 0, 63), p)) set_if_visible(fb, x, y, COLOR_WARM, p);
        }
    }
}

static void render_terrain_scene(uint8_t* fb, const render_params_t* p) {
    fb_fill(fb, COLOR_BLACK);
    draw_sun(fb, p);
    draw_terrain(fb, p);
    draw_common_ui(fb, "TERRAIN", 1, p);
}

// ─── TOROID ────────────────────────────────────────────────

static void render_torus_scene(uint8_t* fb, const render_params_t* p) {
    fb_fill(fb, COLOR_BLACK);
    draw_starfield(fb, p ? p->camera_angle : 0.0f, p);

    const float angle = p ? p->camera_angle : 0.0f;
    const float ca = cosf(angle);
    const float sa = sinf(angle);
    const int cx = 120;
    const int cy = 124;

    for (int y = 42; y < 196; ++y) {
        for (int x = 34; x < 207; ++x) {
            if (!inside_round_lcd(x, y, p)) continue;
            const float dx = static_cast<float>(x - cx);
            const float dy = static_cast<float>(y - cy);
            const float xr = dx * ca + dy * sa;
            const float yr = -dx * sa + dy * ca;
            const float r = sqrtf((xr * xr) / (72.0f * 72.0f) + (yr * yr) / (45.0f * 45.0f));
            const float dist = fabsf(r - 1.0f);
            if (dist > 0.23f) continue;

            int density = static_cast<int>((0.23f - dist) * 230.0f);
            density += static_cast<int>((xr + 72.0f) * 0.10f);
            const uint8_t color = (yr < -6.0f || xr > 20.0f) ? COLOR_WARM : COLOR_COOL;
            if (dither_on(x, y, density, p)) set_if_visible(fb, x, y, color, p);
        }
    }

    // Dark center hole and a few bright rim pixels.
    for (int y = 76; y < 166; ++y) {
        for (int x = 72; x < 169; ++x) {
            const float dx = static_cast<float>(x - cx);
            const float dy = static_cast<float>(y - cy);
            const float hole = sqrtf((dx * dx) / (41.0f * 41.0f) + (dy * dy) / (25.0f * 25.0f));
            if (hole < 1.0f && inside_round_lcd(x, y, p)) set_if_visible(fb, x, y, COLOR_BLACK, p);
        }
    }
    draw_common_ui(fb, "TOROID", 2, p);
}

// ─── OCEAN ─────────────────────────────────────────────────

static void render_ocean_scene(uint8_t* fb, const render_params_t* p) {
    fb_fill(fb, COLOR_BLACK);
    const float angle = p ? p->camera_angle : 0.0f;
    const int horizon = 112 + static_cast<int>(sinf(angle * 0.8f) * 6.0f);

    // Low sun near the horizon.
    const int sx = 168 + static_cast<int>(cosf(angle * 0.45f) * 18.0f);
    const int sy = horizon - 18;
    for (int y = sy - 23; y <= sy + 23; ++y) {
        for (int x = sx - 23; x <= sx + 23; ++x) {
            if (!inside_round_lcd(x, y, p)) continue;
            const int dx = x - sx;
            const int dy = y - sy;
            const int d2 = dx * dx + dy * dy;
            if (d2 <= 23 * 23 && dither_on(x, y, 58 - static_cast<int>(sqrtf(static_cast<float>(d2)) * 1.3f), p)) {
                set_if_visible(fb, x, y, COLOR_WARM, p);
            }
        }
    }

    for (int y = horizon; y < FB_HEIGHT; ++y) {
        for (int x = 0; x < FB_WIDTH; ++x) {
            if (!inside_round_lcd(x, y, p)) continue;
            const float yy = static_cast<float>(y - horizon);
            const float wave = sinf(x * 0.13f + yy * 0.10f + angle * 2.2f) +
                               sinf(x * 0.035f - yy * 0.21f - angle * 1.4f);
            int density = 52 - static_cast<int>(yy * 0.12f) + static_cast<int>(wave * 9.0f);
            const bool foam = fabsf(wave) > 1.45f && ((y + x) & 3) == 0;
            if (foam && dither_on(x, y, 45, p)) set_if_visible(fb, x, y, COLOR_HIGH, p);
            else if (dither_on(x, y, density, p)) set_if_visible(fb, x, y, COLOR_COOL, p);
        }
    }

    draw_common_ui(fb, "OCEAN", 3, p);
}

// ─── PLANET ────────────────────────────────────────────────

static void render_planet_scene(uint8_t* fb, const render_params_t* p) {
    fb_fill(fb, COLOR_BLACK);
    const float angle = p ? p->camera_angle : 0.0f;
    draw_starfield(fb, angle, p);

    const int cx = 120;
    const int cy = 122;
    const int r = 58;
    const float lx = cosf(angle);
    const float ly = -0.45f;

    // Ring behind planet.
    for (int y = 78; y < 168; ++y) {
        for (int x = 35; x < 206; ++x) {
            if (!inside_round_lcd(x, y, p)) continue;
            const float dx = static_cast<float>(x - cx);
            const float dy = static_cast<float>(y - cy);
            const float rr = sqrtf((dx * dx) / (88.0f * 88.0f) + (dy * dy) / (19.0f * 19.0f));
            if (fabsf(rr - 1.0f) < 0.12f && dither_on(x, y, 38, p)) set_if_visible(fb, x, y, COLOR_WARM, p);
        }
    }

    for (int y = cy - r; y <= cy + r; ++y) {
        for (int x = cx - r; x <= cx + r; ++x) {
            if (!inside_round_lcd(x, y, p)) continue;
            const int dx = x - cx;
            const int dy = y - cy;
            const int d2 = dx * dx + dy * dy;
            if (d2 > r * r) continue;
            const float nx = static_cast<float>(dx) / r;
            const float ny = static_cast<float>(dy) / r;
            const float shade = nx * lx + ny * ly;
            const float bands = sinf(ny * 18.0f + angle * 1.5f) * 0.16f;
            int density = static_cast<int>(38.0f + shade * 30.0f + bands * 64.0f);
            const uint8_t color = shade > 0.20f ? COLOR_WARM : COLOR_COOL;
            if (dither_on(x, y, density, p)) set_if_visible(fb, x, y, color, p);
        }
    }

    // Ring front segment over lower planet.
    for (int y = 120; y < 170; ++y) {
        for (int x = 38; x < 203; ++x) {
            if (!inside_round_lcd(x, y, p)) continue;
            const float dx = static_cast<float>(x - cx);
            const float dy = static_cast<float>(y - cy);
            const float rr = sqrtf((dx * dx) / (88.0f * 88.0f) + (dy * dy) / (19.0f * 19.0f));
            if (fabsf(rr - 1.0f) < 0.09f && dither_on(x, y, 48, p)) set_if_visible(fb, x, y, COLOR_HIGH, p);
        }
    }

    draw_common_ui(fb, "PLANET", 4, p);
}

// ─── TUNNEL ────────────────────────────────────────────────

static void render_tunnel_scene(uint8_t* fb, const render_params_t* p) {
    fb_fill(fb, COLOR_BLACK);
    const float angle = p ? p->camera_angle : 0.0f;
    const float ca = cosf(angle);
    const float sa = sinf(angle);

    for (int y = 0; y < FB_HEIGHT; ++y) {
        for (int x = 0; x < FB_WIDTH; ++x) {
            if (!inside_round_lcd(x, y, p)) continue;
            const float dx = static_cast<float>(x - 120);
            const float dy = static_cast<float>(y - 122);
            const float xr = dx * ca + dy * sa;
            const float yr = -dx * sa + dy * ca;
            const float r = sqrtf(xr * xr + yr * yr);
            if (r < 12.0f || r > 98.0f) continue;
            const float a = atan2f(yr, xr);
            const float rings = fmodf(r * 0.145f - angle * 1.7f + 40.0f, 1.0f);
            const float spokes = fabsf(sinf(a * 6.0f + angle * 1.2f));
            int density = 0;
            uint8_t color = COLOR_COOL;
            if (rings < 0.16f) {
                density = 58 - static_cast<int>(r * 0.16f);
                color = COLOR_WARM;
            } else if (spokes > 0.88f) {
                density = 46 - static_cast<int>(r * 0.10f);
                color = COLOR_COOL;
            } else {
                density = 18 - static_cast<int>(r * 0.05f);
                color = COLOR_HIGH;
            }
            if (dither_on(x, y, density, p)) set_if_visible(fb, x, y, color, p);
        }
    }

    // Black vanishing point.
    fb_fill_circle(fb, 120, 122, 13, COLOR_BLACK);
    draw_common_ui(fb, "TUNNEL", 5, p);
}

}  // namespace

void poster_render_scene(uint8_t* fb, SceneId scene_id, const render_params_t* params) {
    switch (scene_id) {
        case SCENE_TERRAIN:
            render_terrain_scene(fb, params);
            break;
        case SCENE_TORUS:
            render_torus_scene(fb, params);
            break;
        case SCENE_OCEAN:
            render_ocean_scene(fb, params);
            break;
        case SCENE_PLANET:
            render_planet_scene(fb, params);
            break;
        case SCENE_TUNNEL:
            render_tunnel_scene(fb, params);
            break;
        default:
            render_terrain_scene(fb, params);
            break;
    }
}

void terrain_poster_render(uint8_t* fb, float camera_angle) {
    render_params_t params = *render_params_get();
    params.camera_angle = camera_angle;
    poster_render_scene(fb, SCENE_TERRAIN, &params);
}
