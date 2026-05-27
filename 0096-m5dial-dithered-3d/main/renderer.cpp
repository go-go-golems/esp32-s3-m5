#include "renderer.h"
#include "fixedpoint.h"
#include "trig_lut.h"
#include <string.h>
#include <math.h>
#include <esp_log.h>
#include <esp_timer.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "renderer";

// Bayer 4×4 threshold matrix (values 0–15)
static const uint8_t BAYER4X4[4][4] = {
    {  0,  8,  2, 10 },
    { 12,  4, 14,  6 },
    {  3, 11,  1,  9 },
    { 15,  7, 13,  5 },
};

// Scanline Z-buffer (16-bit depth) and projected vertices.
// Keep projected vertices static: SCENE_MAX_VERTICES * projected_vertex_t is
// larger than the app task stack on ESP32-S3.
static int16_t s_zbuf[FB_WIDTH];

typedef struct {
    float sx, sy;    // screen coordinates (pixels)
    float depth;     // camera-space Z (for sorting / Z-buffer)
    float lum;       // luminance 0–1
    uint8_t chroma;  // 0=warm, 1=cool, 2=white/high, 3=neutral
} projected_vertex_t;

static projected_vertex_t s_proj[SCENE_MAX_VERTICES];
static bool s_visible[SCENE_MAX_VERTICES];

// Render stats
static render_stats_t s_stats = {};

bool renderer_init(void) {
    ESP_LOGI(TAG, "renderer initialized");
    return true;
}

// Project a world-space vertex to screen coordinates
// Returns true if vertex is in front of camera
static bool project_vertex(const scene_vertex_t* v,
                           float cam_angle_idx,  // trig LUT index
                           const scene_def_t* scene,
                           projected_vertex_t* out) {
    // Camera orbit: eye position
    float sin_a = trig_sin_f(trig_idx_to_float(cam_angle_idx));
    float cos_a = trig_cos_f(trig_idx_to_float(cam_angle_idx));

    float eye_x = scene->target[0] + scene->camera_distance * sin_a;
    float eye_y = scene->target[1] + scene->camera_height;
    float eye_z = scene->target[2] + scene->camera_distance * cos_a;

    // View vector: eye → target
    float fwd_x = scene->target[0] - eye_x;
    float fwd_y = scene->target[1] - eye_y;
    float fwd_z = scene->target[2] - eye_z;
    float fwd_len = sqrtf(fwd_x * fwd_x + fwd_y * fwd_y + fwd_z * fwd_z);
    if (fwd_len < 0.001f) return false;
    fwd_x /= fwd_len; fwd_y /= fwd_len; fwd_z /= fwd_len;

    // Right = world_up × forward (world up = 0,1,0)
    float right_x = fwd_z;   // 0*0 - 1*fwd_z... actually cross(0,1,0, fwd_x,fwd_y,fwd_z) = (fwd_z, 0, -fwd_x)
    float right_y = 0;
    float right_z = -fwd_x;
    float right_len = sqrtf(right_x * right_x + right_z * right_z);
    if (right_len < 0.001f) return false;
    right_x /= right_len; right_z /= right_len;

    // Up = forward × right
    float up_x = fwd_y * right_z - fwd_z * right_y;
    float up_y = fwd_z * right_x - fwd_x * right_z;
    float up_z = fwd_x * right_y - fwd_y * right_x;

    // Transform vertex to camera space
    float dx = v->x - eye_x;
    float dy = v->y - eye_y;
    float dz = v->z - eye_z;

    float view_x = dx * right_x + dy * right_y + dz * right_z;
    float view_y = dx * up_x + dy * up_y + dz * up_z;
    float view_z = dx * fwd_x + dy * fwd_y + dz * fwd_z;

    // Near clip
    if (view_z < 0.1f) return false;

    // Perspective projection: 50° FOV
    const float focal = 2.145f;  // 1/tan(25°)
    const float scale = 120.0f;  // half of 240

    out->sx = (view_x * focal / view_z) * scale + 120.0f;
    out->sy = -(view_y * focal / view_z) * scale + 120.0f;
    out->depth = view_z;

    // Compute luminance and chroma from vertex color
    float r = v->cr / 255.0f;
    float g = v->cg / 255.0f;
    float b = v->cb / 255.0f;
    float lum = (r + g + b) / 3.0f;

    // Classify chroma (same logic as GLSL DITHER_FS)
    uint8_t chroma;
    bool is_white = (r > 0.55f && g > 0.55f && b > 0.55f);
    if (is_white && lum > 0.5f) {
        chroma = 2;  // white/high
    } else if (r > b + 0.05f) {
        chroma = 0;  // warm
    } else if (b > r + 0.05f) {
        chroma = 1;  // cool
    } else {
        chroma = 3;  // neutral → lean cool
    }

    out->lum = lum;
    out->chroma = chroma;
    return true;
}

// Rasterize one triangle with scanline Z-buffer + inline dithering
static void rasterize_triangle(const projected_vertex_t* v0,
                               const projected_vertex_t* v1,
                               const projected_vertex_t* v2,
                               uint8_t* fb,
                               float contrast,
                               int mask_radius_sq) {
    s_stats.triangles_submitted++;

    // Compute bounding box in screen space (integer pixels)
    int min_y = (int)v0->sy;
    int max_y = (int)v0->sy;
    if ((int)v1->sy < min_y) min_y = (int)v1->sy;
    if ((int)v1->sy > max_y) max_y = (int)v1->sy;
    if ((int)v2->sy < min_y) min_y = (int)v2->sy;
    if ((int)v2->sy > max_y) max_y = (int)v2->sy;

    // Clip to screen
    if (min_y < 0) min_y = 0;
    if (max_y >= FB_HEIGHT) max_y = FB_HEIGHT - 1;
    if (min_y > max_y) return;

    // For each scanline in the triangle's vertical extent
    for (int y = min_y; y <= max_y; y++) {
        // First smoke-test version uses a scanline-local Z buffer per triangle.
        // Full inter-triangle occlusion will be restored when the renderer is
        // reorganized into a true scanline-major pass.
        for (int zi = 0; zi < FB_WIDTH; zi++) {
            s_zbuf[zi] = 0x7FFF;
        }

        // Find x-intercepts of triangle edges with this scanline
        // Using edge function / half-space rasterization
        const projected_vertex_t* verts[3] = { v0, v1, v2 };

        // Find the leftmost and rightmost x on this scanline
        // by solving for each edge's intersection at y+0.5
        float fy = y + 0.5f;
        int x_left = FB_WIDTH;
        int x_right = -1;

        for (int e = 0; e < 3; e++) {
            const projected_vertex_t* va = verts[e];
            const projected_vertex_t* vb = verts[(e + 1) % 3];

            // Does this edge cross the scanline?
            if ((va->sy < fy) == (vb->sy < fy)) continue;

            float t = (fy - va->sy) / (vb->sy - va->sy);
            float ix = va->sx + t * (vb->sx - va->sx);

            int x = (int)ix;
            if (x < x_left) x_left = x;
            if (x > x_right) x_right = x;
        }

        // Clip to screen
        if (x_left < 0) x_left = 0;
        if (x_right >= FB_WIDTH) x_right = FB_WIDTH - 1;
        if (x_left > x_right) continue;

        // For each pixel in the span, compute barycentric coords
        // and interpolate depth + luminance
        float dx02 = v0->sx - v2->sx, dy02 = v0->sy - v2->sy;
        float dx12 = v1->sx - v2->sx, dy12 = v1->sy - v2->sy;
        float area = dx02 * dy12 - dx12 * dy02;
        if (fabsf(area) < 0.001f) continue;
        float inv_area = 1.0f / area;

        for (int x = x_left; x <= x_right; x++) {
            float fx = x + 0.5f;

            // Barycentric coordinates
            float dx_p2 = fx - v2->sx, dy_p2 = fy - v2->sy;
            float w0 = (dx_p2 * dy12 - dx12 * dy_p2) * inv_area;

            float dx_p0 = fx - v0->sx, dy_p0 = fy - v0->sy;
            float w1 = (dx02 * dy_p0 - dx_p0 * dy02) * inv_area;

            float w2 = 1.0f - w0 - w1;

            // Skip pixels outside the triangle
            if (w0 < 0 || w1 < 0 || w2 < 0) continue;

            // Interpolated depth
            float depth = w0 * v0->depth + w1 * v1->depth + w2 * v2->depth;
            int16_t z_val = (int16_t)(depth * 256.0f);

            // Z-buffer test
            if (z_val >= s_zbuf[x]) continue;
            s_zbuf[x] = z_val;

            // Interpolated luminance and chroma
            float lum = w0 * v0->lum + w1 * v1->lum + w2 * v2->lum;

            // Dominant chroma (pick the chroma of the vertex with highest weight)
            uint8_t chroma;
            if (w0 >= w1 && w0 >= w2) chroma = v0->chroma;
            else if (w1 >= w2) chroma = v1->chroma;
            else chroma = v2->chroma;

            // Contrast enhancement (S-curve-ish)
            lum = (lum - 0.5f) * contrast + 0.5f;
            if (lum < 0) lum = 0;
            if (lum > 1) lum = 1;

            // Bayer dither + 4-color quantization
            uint8_t lum4 = (uint8_t)(lum * 15.0f);  // 0–15
            uint8_t bayer = BAYER4X4[y & 3][x & 3];
            uint8_t pixel_color = COLOR_BLACK;

            if (chroma == 2) {
                // White/high
                if (lum4 > bayer) pixel_color = COLOR_HIGH;
            } else if (chroma == 0) {
                // Warm
                if (lum4 > bayer) pixel_color = COLOR_WARM;
            } else {
                // Cool or neutral → lean cool
                if (lum4 > bayer) pixel_color = COLOR_COOL;
            }

            // Circular mask
            int dx_m = x - 120;
            int dy_m = y - 120;
            if (dx_m * dx_m + dy_m * dy_m > mask_radius_sq) {
                pixel_color = COLOR_BLACK;
            }

            fb_set(fb, x, y, pixel_color);
            s_stats.pixels_written++;
        }

        s_stats.triangles_drawn++;
    }
}

uint64_t renderer_render_frame(uint8_t* fb, const palette_t* pal,
                               const scene_def_t* scene,
                               const render_params_t* params) {
    uint64_t start_us = esp_timer_get_time();
    s_stats = {};

    // Convert camera angle to trig LUT index
    int angle_idx = trig_angle_to_idx(params->camera_angle);

    // Compute circular mask radius squared
    int mask_r = (int)(params->aperture * 120.0f);
    int mask_radius_sq = mask_r * mask_r;

    // Project all vertices
    for (int i = 0; i < scene->vertex_count && i < SCENE_MAX_VERTICES; i++) {
        s_visible[i] = project_vertex(&scene->vertices[i], angle_idx, scene, &s_proj[i]);
    }

    // Render each triangle
    for (int t = 0; t < scene->triangle_count && t < SCENE_MAX_TRIANGLES; t++) {
        if ((t & 0x0F) == 0) {
            taskYIELD();
        }
        int i0 = scene->triangles[t].v0;
        int i1 = scene->triangles[t].v1;
        int i2 = scene->triangles[t].v2;

        if (!s_visible[i0] || !s_visible[i1] || !s_visible[i2]) continue;

        rasterize_triangle(&s_proj[i0], &s_proj[i1], &s_proj[i2],
                          fb, params->contrast, mask_radius_sq);
    }

    // Draw wireframe overlay if enabled
    if (params->wireframe) {
        for (int t = 0; t < scene->triangle_count && t < SCENE_MAX_TRIANGLES; t++) {
            if ((t & 0x0F) == 0) {
                taskYIELD();
            }
            int i0 = scene->triangles[t].v0;
            int i1 = scene->triangles[t].v1;
            int i2 = scene->triangles[t].v2;
            if (!s_visible[i0] || !s_visible[i1] || !s_visible[i2]) continue;

            // Draw edges as white pixels.
            int sx[3], sy[3];
            sx[0] = (int)s_proj[i0].sx; sy[0] = (int)s_proj[i0].sy;
            sx[1] = (int)s_proj[i1].sx; sy[1] = (int)s_proj[i1].sy;
            sx[2] = (int)s_proj[i2].sx; sy[2] = (int)s_proj[i2].sy;

            // Simple line drawing for edges
            for (int e = 0; e < 3; e++) {
                int x0 = sx[e], y0 = sy[e];
                int x1 = sx[(e+1)%3], y1 = sy[(e+1)%3];
                // Bresenham's line algorithm
                int dx = abs(x1 - x0), dy = abs(y1 - y0);
                int sx_s = x0 < x1 ? 1 : -1;
                int sy_s = y0 < y1 ? 1 : -1;
                int err = dx - dy;
                while (1) {
                    if (x0 >= 0 && x0 < FB_WIDTH && y0 >= 0 && y0 < FB_HEIGHT) {
                        int ddx = x0 - 120, ddy = y0 - 120;
                        if (ddx*ddx + ddy*ddy <= mask_radius_sq) {
                            fb_set(fb, x0, y0, COLOR_HIGH);
                        }
                    }
                    if (x0 == x1 && y0 == y1) break;
                    int e2 = 2 * err;
                    if (e2 > -dy) { err -= dy; x0 += sx_s; }
                    if (e2 < dx) { err += dx; y0 += sy_s; }
                }
            }
        }
    }

    uint64_t end_us = esp_timer_get_time();
    s_stats.frame_time_us = end_us - start_us;
    return s_stats.frame_time_us;
}

const render_stats_t* renderer_stats(void) {
    return &s_stats;
}

void renderer_stats_record(uint32_t triangles_submitted,
                           uint32_t triangles_drawn,
                           uint32_t pixels_written,
                           uint64_t frame_time_us) {
    s_stats.triangles_submitted = triangles_submitted;
    s_stats.triangles_drawn = triangles_drawn;
    s_stats.pixels_written = pixels_written;
    s_stats.frame_time_us = frame_time_us;
}
