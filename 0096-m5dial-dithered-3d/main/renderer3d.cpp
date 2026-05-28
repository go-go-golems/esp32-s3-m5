#include "renderer3d.h"

#include <math.h>
#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {

static const char* TAG = "renderer3d";

constexpr int kLatSteps = 18;
constexpr int kLonSteps = 28;
constexpr int kMaxVertices = (kLatSteps + 1) * kLonSteps;
constexpr int kMaxTriangles = (kLatSteps * kLonSteps * 2);
constexpr float kPlanetRadius = 2.6f;
constexpr float kNear = 3.0f;
constexpr float kFar = 16.0f;
constexpr uint16_t kZMax = 0xFFFF;

static const uint8_t kBayer4[4][4] = {
    { 0, 8, 2, 10 },
    { 12, 4, 14, 6 },
    { 3, 11, 1, 9 },
    { 15, 7, 13, 5 },
};

struct Vertex {
    float x;
    float y;
    float z;
    float r;
    float g;
    float b;
};

struct Tri {
    uint16_t a;
    uint16_t b;
    uint16_t c;
};

struct Projected {
    float x;
    float y;
    float z;
    float r;
    float g;
    float b;
    bool visible;
};

struct Basis {
    float eye[3];
    float right[3];
    float up[3];
    float fwd[3];
};

static Vertex s_vertices[kMaxVertices];
static Tri s_tris[kMaxTriangles];
static Projected s_projected[kMaxVertices];
static uint16_t s_zbuf[R3D_W * R3D_H];
static uint8_t s_colorbuf[R3D_W * R3D_H];
static uint16_t s_vertex_count = 0;
static uint16_t s_tri_count = 0;
static bool s_ready = false;
static renderer3d_stats_t s_stats = {};

static inline float clamp_f(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static inline int clamp_i(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static float noise2(float x, float y) {
    return sinf(x * 1.37f + y * 2.11f) * 0.55f
         + sinf(x * 3.91f - y * 1.73f) * 0.30f
         + cosf(x * 2.47f + y * 0.83f) * 0.15f;
}

static void build_sphere(void) {
    s_vertex_count = 0;
    for (int iy = 0; iy <= kLatSteps; ++iy) {
        const float v = static_cast<float>(iy) / static_cast<float>(kLatSteps);
        const float theta = -static_cast<float>(M_PI) * 0.5f + v * static_cast<float>(M_PI);
        const float yy = sinf(theta);
        const float ring_r = cosf(theta);
        for (int ix = 0; ix < kLonSteps; ++ix) {
            const float u = static_cast<float>(ix) / static_cast<float>(kLonSteps);
            const float phi = u * 2.0f * static_cast<float>(M_PI);
            const float xx = cosf(phi) * ring_r;
            const float zz = sinf(phi) * ring_r;

            const float n = noise2(xx * 2.0f + zz * 0.7f, yy * 2.0f) * 0.16f
                          + noise2(zz * 1.2f, yy * 1.5f) * 0.07f;
            const float scale = kPlanetRadius * (1.0f + n * 0.5f);
            const float px = xx * scale;
            const float py = yy * scale;
            const float pz = zz * scale;

            const float lat = py / kPlanetRadius;
            const float heat = fmaxf(0.0f, lat);
            const float cold = fmaxf(0.0f, -lat);
            const float speckle = sinf(px * 5.0f) * cosf(pz * 5.0f) * 0.5f + 0.5f;
            const float rr = heat * (0.6f + speckle * 0.5f) + fmaxf(0.0f, n) * 0.5f;
            const float bb = cold * (0.6f + speckle * 0.5f) + fmaxf(0.0f, -n) * 0.3f;

            s_vertices[s_vertex_count++] = Vertex{px, py, pz, fminf(1.0f, rr), 0.0f, fminf(1.0f, bb)};
        }
    }

    s_tri_count = 0;
    for (int iy = 0; iy < kLatSteps; ++iy) {
        for (int ix = 0; ix < kLonSteps; ++ix) {
            const uint16_t a = iy * kLonSteps + ix;
            const uint16_t b = iy * kLonSteps + ((ix + 1) % kLonSteps);
            const uint16_t c = (iy + 1) * kLonSteps + ix;
            const uint16_t d = (iy + 1) * kLonSteps + ((ix + 1) % kLonSteps);
            if (iy != 0) s_tris[s_tri_count++] = Tri{a, c, b};
            if (iy != kLatSteps - 1) s_tris[s_tri_count++] = Tri{b, c, d};
        }
    }
}

static Basis camera_basis(float angle) {
    Basis b = {};
    b.eye[0] = sinf(angle) * 9.0f;
    b.eye[1] = 0.5f;
    b.eye[2] = cosf(angle) * 9.0f;

    float fx = -b.eye[0];
    float fy = -b.eye[1];
    float fz = -b.eye[2];
    float fl = sqrtf(fx * fx + fy * fy + fz * fz);
    if (fl < 0.001f) fl = 1.0f;
    b.fwd[0] = fx / fl;
    b.fwd[1] = fy / fl;
    b.fwd[2] = fz / fl;

    b.right[0] = b.fwd[2];
    b.right[1] = 0.0f;
    b.right[2] = -b.fwd[0];
    float rl = sqrtf(b.right[0] * b.right[0] + b.right[2] * b.right[2]);
    if (rl < 0.001f) rl = 1.0f;
    b.right[0] /= rl;
    b.right[2] /= rl;

    b.up[0] = b.fwd[1] * b.right[2] - b.fwd[2] * b.right[1];
    b.up[1] = b.fwd[2] * b.right[0] - b.fwd[0] * b.right[2];
    b.up[2] = b.fwd[0] * b.right[1] - b.fwd[1] * b.right[0];
    return b;
}

static Projected project_vertex(const Vertex& v, const Basis& basis, float model_angle) {
    const float cm = cosf(model_angle);
    const float sm = sinf(model_angle);
    const float wx = v.x * cm + v.z * sm;
    const float wy = v.y;
    const float wz = -v.x * sm + v.z * cm;

    const float dx = wx - basis.eye[0];
    const float dy = wy - basis.eye[1];
    const float dz = wz - basis.eye[2];
    const float vx = dx * basis.right[0] + dy * basis.right[1] + dz * basis.right[2];
    const float vy = dx * basis.up[0] + dy * basis.up[1] + dz * basis.up[2];
    const float vz = dx * basis.fwd[0] + dy * basis.fwd[1] + dz * basis.fwd[2];

    Projected p = {};
    if (vz <= 0.1f) {
        p.visible = false;
        return p;
    }

    constexpr float focal = 2.145f;
    p.x = (vx * focal / vz) * (R3D_W * 0.5f) + R3D_W * 0.5f;
    p.y = -(vy * focal / vz) * (R3D_H * 0.5f) + R3D_H * 0.5f;
    p.z = vz;
    p.r = v.r;
    p.g = v.g;
    p.b = v.b;
    p.visible = true;
    return p;
}

static uint8_t quantize(float r, float g, float b, int x, int y, float contrast) {
    r = clamp_f((r - 0.5f) * contrast + 0.5f, 0.0f, 1.0f);
    g = clamp_f((g - 0.5f) * contrast + 0.5f, 0.0f, 1.0f);
    b = clamp_f((b - 0.5f) * contrast + 0.5f, 0.0f, 1.0f);
    const float t = static_cast<float>(kBayer4[y & 3][x & 3]) / 16.0f;
    const float lum = (r + g + b) / 3.0f;
    if (r > 0.55f && g > 0.55f && b > 0.55f && lum > t) return COLOR_HIGH;
    if (r > b + 0.05f) return r > t ? COLOR_WARM : COLOR_BLACK;
    if (b > r + 0.05f) return b > t ? COLOR_COOL : COLOR_BLACK;
    if (lum > 0.25f && lum > t) return COLOR_COOL;
    return COLOR_BLACK;
}

static inline float edge(const Projected& a, const Projected& b, float px, float py) {
    return (b.x - a.x) * (py - a.y) - (b.y - a.y) * (px - a.x);
}

static void rasterize_sphere(const render_params_t* params) {
    const float contrast = params ? clamp_f(params->contrast, 0.4f, 3.0f) : 1.4f;
    for (uint16_t ti = 0; ti < s_tri_count; ++ti) {
        if ((ti & 0x1F) == 0) taskYIELD();
        const Tri& tri = s_tris[ti];
        const Projected& a = s_projected[tri.a];
        const Projected& b = s_projected[tri.b];
        const Projected& c = s_projected[tri.c];
        s_stats.triangles_submitted++;
        if (!a.visible || !b.visible || !c.visible) continue;

        const float area = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
        if (area >= -0.01f || fabsf(area) < 0.01f) continue;
        const float inv_area = 1.0f / area;
        const int minx = clamp_i(static_cast<int>(floorf(fminf(a.x, fminf(b.x, c.x)))), 0, R3D_W - 1);
        const int maxx = clamp_i(static_cast<int>(ceilf(fmaxf(a.x, fmaxf(b.x, c.x)))), 0, R3D_W - 1);
        const int miny = clamp_i(static_cast<int>(floorf(fminf(a.y, fminf(b.y, c.y)))), 0, R3D_H - 1);
        const int maxy = clamp_i(static_cast<int>(ceilf(fmaxf(a.y, fmaxf(b.y, c.y)))), 0, R3D_H - 1);

        bool drew_triangle = false;
        for (int y = miny; y <= maxy; ++y) {
            for (int x = minx; x <= maxx; ++x) {
                const float px = x + 0.5f;
                const float py = y + 0.5f;
                float w0 = edge(b, c, px, py) * inv_area;
                float w1 = edge(c, a, px, py) * inv_area;
                float w2 = edge(a, b, px, py) * inv_area;
                if (w0 < 0.0f || w1 < 0.0f || w2 < 0.0f) continue;

                const float z = w0 * a.z + w1 * b.z + w2 * c.z;
                const float zn = clamp_f((z - kNear) / (kFar - kNear), 0.0f, 1.0f);
                const uint16_t zi = static_cast<uint16_t>(zn * kZMax + 0.5f);
                const int idx = y * R3D_W + x;
                if (zi >= s_zbuf[idx]) continue;
                s_zbuf[idx] = zi;

                const float rr = w0 * a.r + w1 * b.r + w2 * c.r;
                const float gg = w0 * a.g + w1 * b.g + w2 * c.g;
                const float bb = w0 * a.b + w1 * b.b + w2 * c.b;
                s_colorbuf[idx] = quantize(rr, gg, bb, x, y, contrast);
                s_stats.planet_pixels++;
                drew_triangle = true;
            }
        }
        if (drew_triangle) s_stats.triangles_drawn++;
    }
}

static uint32_t draw_reference_ring(bool front, float angle) {
    const float cx = R3D_W * 0.5f;
    const float cy = R3D_H * 0.52f;
    const float rx = R3D_W * 0.39f;
    const float ry = R3D_H * 0.085f;
    const float thickness = fmaxf(1.2f, R3D_W * 0.010f);
    const float ca = cosf(angle);
    const float sa = sinf(angle);
    uint32_t drawn = 0;

    for (int y = 0; y < R3D_H; ++y) {
        for (int x = 0; x < R3D_W; ++x) {
            const float dx = x + 0.5f - cx;
            const float dy = y + 0.5f - cy;
            const float xr = dx * ca + dy * sa;
            const float yr = -dx * sa + dy * ca;
            const bool is_front = yr >= 0.0f;
            if (is_front != front) continue;
            const float rr = sqrtf((xr * xr) / (rx * rx) + (yr * yr) / (ry * ry));
            const float dist = fabsf(rr - 1.0f) * fminf(rx, ry);
            if (dist > thickness) continue;
            const uint8_t density = dist < thickness * 0.55f ? 15 : 10;
            if (density > kBayer4[y & 3][x & 3]) {
                s_colorbuf[y * R3D_W + x] = front ? COLOR_HIGH : COLOR_COOL;
                drawn++;
            }
        }
    }
    return drawn;
}

static uint32_t draw_reference_moon(float camera_angle, float time_angle) {
    Vertex moon = {
        cosf(time_angle * 0.6f) * 5.2f,
        sinf(time_angle * 0.4f) * 0.6f,
        sinf(time_angle * 0.6f) * 5.2f,
        1.0f, 1.0f, 1.0f,
    };
    const Basis basis = camera_basis(camera_angle);
    const Projected p = project_vertex(moon, basis, 0.0f);
    if (!p.visible) return 0;

    const float radius = fmaxf(1.0f, R3D_W * 0.035f / fmaxf(0.5f, p.z / 9.0f));
    uint32_t drawn = 0;
    const int y0 = clamp_i(static_cast<int>(p.y - radius - 1.0f), 0, R3D_H - 1);
    const int y1 = clamp_i(static_cast<int>(p.y + radius + 2.0f), 0, R3D_H - 1);
    const int x0 = clamp_i(static_cast<int>(p.x - radius - 1.0f), 0, R3D_W - 1);
    const int x1 = clamp_i(static_cast<int>(p.x + radius + 2.0f), 0, R3D_W - 1);
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            const float dx = x + 0.5f - p.x;
            const float dy = y + 0.5f - p.y;
            if (dx * dx + dy * dy <= radius * radius && 14 > kBayer4[y & 3][x & 3]) {
                s_colorbuf[y * R3D_W + x] = COLOR_HIGH;
                drawn++;
            }
        }
    }
    return drawn;
}

static inline bool inside_mask(int x, int y, const render_params_t* params) {
    const float aperture = params ? clamp_f(params->aperture, 0.40f, 1.0f) : 0.97f;
    const int r = clamp_i(static_cast<int>(116.0f * aperture), 46, 116);
    const int dx = x - 120;
    const int dy = y - 120;
    return dx * dx + dy * dy <= r * r;
}

static void expand_to_framebuffer(uint8_t* fb, const render_params_t* params) {
    fb_fill(fb, COLOR_BLACK);
    for (int ly = 0; ly < R3D_H; ++ly) {
        for (int lx = 0; lx < R3D_W; ++lx) {
            const uint8_t c = s_colorbuf[ly * R3D_W + lx];
            if (c == COLOR_BLACK) continue;
            const int x0 = lx * R3D_PIXEL_SCALE;
            const int y0 = ly * R3D_PIXEL_SCALE;
            for (int yy = 0; yy < R3D_PIXEL_SCALE; ++yy) {
                for (int xx = 0; xx < R3D_PIXEL_SCALE; ++xx) {
                    const int px = x0 + xx;
                    const int py = y0 + yy;
                    if (inside_mask(px, py, params)) fb_set(fb, px, py, c);
                }
            }
        }
    }
}

static const uint8_t GLYPH_A[7] = {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
static const uint8_t GLYPH_E[7] = {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F};
static const uint8_t GLYPH_L[7] = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F};
static const uint8_t GLYPH_N[7] = {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11};
static const uint8_t GLYPH_P[7] = {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10};
static const uint8_t GLYPH_T[7] = {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04};

static const uint8_t* glyph_for(char c) {
    switch (c) {
        case 'A': return GLYPH_A;
        case 'E': return GLYPH_E;
        case 'L': return GLYPH_L;
        case 'N': return GLYPH_N;
        case 'P': return GLYPH_P;
        case 'T': return GLYPH_T;
        default: return nullptr;
    }
}

static void draw_title(uint8_t* fb, const render_params_t* params) {
    const char* text = "PLANET";
    constexpr int scale = 2;
    int x = 84;
    constexpr int y = 26;
    for (const char* ch = text; *ch; ++ch) {
        const uint8_t* glyph = glyph_for(*ch);
        if (!glyph) {
            x += 4 * scale;
            continue;
        }
        for (int gy = 0; gy < 7; ++gy) {
            for (int gx = 0; gx < 5; ++gx) {
                if ((glyph[gy] & (1 << (4 - gx))) == 0) continue;
                for (int yy = 0; yy < scale; ++yy) {
                    for (int xx = 0; xx < scale; ++xx) {
                        const int px = x + gx * scale + xx;
                        const int py = y + gy * scale + yy;
                        if (px >= 0 && px < FB_WIDTH && py >= 0 && py < FB_HEIGHT && inside_mask(px, py, params)) {
                            fb_set(fb, px, py, COLOR_WARM);
                        }
                    }
                }
            }
        }
        x += 6 * scale;
    }
}

} // namespace

bool renderer3d_init(void) {
    build_sphere();
    s_ready = true;
    ESP_LOGI(TAG, "proper 3D planet renderer initialized: %u vertices, %u triangles, z=%u bytes, color=%u bytes",
             s_vertex_count,
             s_tri_count,
             (unsigned)sizeof(s_zbuf),
             (unsigned)sizeof(s_colorbuf));
    return true;
}

uint64_t renderer3d_render_planet(uint8_t* fb, const render_params_t* params) {
    if (!s_ready) renderer3d_init();

    const uint64_t start_us = esp_timer_get_time();
    memset(s_colorbuf, COLOR_BLACK, sizeof(s_colorbuf));
    for (int i = 0; i < R3D_W * R3D_H; ++i) s_zbuf[i] = kZMax;

    s_stats = {};
    s_stats.logical_w = R3D_W;
    s_stats.logical_h = R3D_H;
    s_stats.pixel_scale = R3D_PIXEL_SCALE;
    s_stats.z_bits = R3D_Z_BITS;
    s_stats.sphere_vertices = s_vertex_count;
    s_stats.sphere_triangles = s_tri_count;
    s_stats.zbuffer_bytes = sizeof(s_zbuf);
    s_stats.colorbuffer_bytes = sizeof(s_colorbuf);

    const float camera_angle = params ? params->camera_angle : 0.0f;
    const float planet_angle = camera_angle * 0.35f;
    const float ring_angle = camera_angle * 0.10f;
    const Basis basis = camera_basis(camera_angle);

    for (uint16_t i = 0; i < s_vertex_count; ++i) {
        s_projected[i] = project_vertex(s_vertices[i], basis, planet_angle);
    }

    const uint32_t ring_back = draw_reference_ring(false, ring_angle);
    rasterize_sphere(params);
    const uint32_t ring_front = draw_reference_ring(true, ring_angle);
    s_stats.ring_pixels = ring_back + ring_front;
    s_stats.moon_pixels = draw_reference_moon(camera_angle, planet_angle);

    expand_to_framebuffer(fb, params);
    draw_title(fb, params);

    const uint64_t end_us = esp_timer_get_time();
    s_stats.render_time_us = end_us - start_us;
    return s_stats.render_time_us;
}

const renderer3d_stats_t* renderer3d_stats(void) {
    return &s_stats;
}
