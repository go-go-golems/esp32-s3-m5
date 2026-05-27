#include "scene.h"
#include <math.h>
#include <string.h>
#include <esp_log.h>

static const char* TAG = "scene_terrain";

// Terrain smoke-test grid.  The first renderer is triangle-major and not yet
// scanline-bucketed, so keep the mesh intentionally small until the optimized
// scanline-major pass lands.
#define TERRAIN_GRID 8
#define TERRAIN_VERTS (TERRAIN_GRID * TERRAIN_GRID)  // 400
#define TERRAIN_TRIS  (2 * (TERRAIN_GRID - 1) * (TERRAIN_GRID - 1))  // 722

static scene_vertex_t s_terrain_verts[TERRAIN_VERTS];
static scene_tri_t s_terrain_tris[TERRAIN_TRIS];

// Simple 2D noise (same formula as m5dial.jsx)
static float noise2d(float x, float y) {
    return sinf(x * 0.45f + 1.2f) * cosf(y * 0.31f + 0.4f) * 0.55f +
           sinf(x * 1.1f + 0.5f) * cosf(y * 0.78f + 1.3f) * 0.30f +
           sinf(x * 2.3f + 2.1f) * cosf(y * 1.7f + 0.2f) * 0.15f;
}

static void terrain_init(void* ctx) {
    scene_def_t* s = (scene_def_t*)ctx;
    s->vertices = s_terrain_verts;
    s->triangles = s_terrain_tris;
    s->vertex_count = TERRAIN_VERTS;
    s->triangle_count = TERRAIN_TRIS;

    // Build height grid centered at origin. The original JSX plane is 40×40,
    // but this smoke-test renderer uses a tighter 14-unit terrain patch so it
    // sits comfortably inside the 240×240 physical display while we tune the
    // camera and rasterizer.
    const float extent = 14.0f;
    const float step = extent / (TERRAIN_GRID - 1);

    for (int gy = 0; gy < TERRAIN_GRID; gy++) {
        for (int gx = 0; gx < TERRAIN_GRID; gx++) {
            int i = gy * TERRAIN_GRID + gx;
            float x = (gx - TERRAIN_GRID / 2) * step;
            float z = (gy - TERRAIN_GRID / 2) * step;

            // Height from noise
            float h = noise2d(x * 0.4f, z * 0.4f) * 4.5f;

            // Push down center valley
            float distC = sqrtf(x * x + z * z);
            h += (1.0f - fminf(1.0f, distC / 8.0f)) * -1.0f;

            s_terrain_verts[i].x = x;
            s_terrain_verts[i].y = h;
            s_terrain_verts[i].z = z;

            // Color: blue gradient (dark valleys → bright peaks)
            float t = (h + 2.0f) / 6.0f;
            if (t < 0) t = 0;
            if (t > 1) t = 1;
            s_terrain_verts[i].cr = (uint8_t)(10 + t * 38);     // 0.04 + t*0.15
            s_terrain_verts[i].cg = (uint8_t)(15 + t * 51);     // 0.06 + t*0.20
            s_terrain_verts[i].cb = (uint8_t)(64 + t * 166);     // 0.25 + t*0.65
        }
    }

    // Build triangle indices (CCW winding)
    int ti = 0;
    for (int gy = 0; gy < TERRAIN_GRID - 1; gy++) {
        for (int gx = 0; gx < TERRAIN_GRID - 1; gx++) {
            int tl = gy * TERRAIN_GRID + gx;
            int tr = tl + 1;
            int bl = tl + TERRAIN_GRID;
            int br = bl + 1;

            s_terrain_tris[ti++] = { (uint16_t)tl, (uint16_t)bl, (uint16_t)tr };
            s_terrain_tris[ti++] = { (uint16_t)tr, (uint16_t)bl, (uint16_t)br };
        }
    }
}

scene_def_t scene_terrain_def = {
    .name = "TERRAIN",
    .subtitle = "alp.001",
    .camera_distance = 11.0f,
    .camera_height = 3.2f,
    .target = { 0.0f, 1.5f, 0.0f },
    .vertices = s_terrain_verts,
    .vertex_count = 0,
    .triangles = s_terrain_tris,
    .triangle_count = 0,
    .update = nullptr,
    .init = terrain_init,
    .fixed_camera = nullptr,
};
