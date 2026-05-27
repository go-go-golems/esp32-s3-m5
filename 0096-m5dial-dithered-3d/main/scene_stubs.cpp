#include "scene.h"

// Stub scenes — will be implemented in Phase 4.
// For now, provide minimal definitions so the project compiles.

static scene_vertex_t s_stub_verts[3];
static scene_tri_t s_stub_tris[1];

static void stub_init(void* ctx) {
    scene_def_t* s = static_cast<scene_def_t*>(ctx);
    s->vertices = s_stub_verts;
    s->vertex_count = 0;
    s->triangles = s_stub_tris;
    s->triangle_count = 0;
}

scene_def_t scene_torus_def = {
    .name = "TOROID",
    .subtitle = "geo.002",
    .camera_distance = 9.0f,
    .camera_height = 0.0f,
    .target = {0, 0, 0},
    .vertices = nullptr,
    .vertex_count = 0,
    .triangles = nullptr,
    .triangle_count = 0,
    .update = nullptr,
    .init = stub_init,
    .fixed_camera = nullptr,
};

scene_def_t scene_ocean_def = {
    .name = "OCEAN",
    .subtitle = "tide.003",
    .camera_distance = 10.0f,
    .camera_height = 2.4f,
    .target = {0, 0.5f, 0},
    .vertices = nullptr,
    .vertex_count = 0,
    .triangles = nullptr,
    .triangle_count = 0,
    .update = nullptr,
    .init = stub_init,
    .fixed_camera = nullptr,
};

scene_def_t scene_planet_def = {
    .name = "PLANET",
    .subtitle = "sat.004",
    .camera_distance = 9.0f,
    .camera_height = 0.5f,
    .target = {0, 0, 0},
    .vertices = nullptr,
    .vertex_count = 0,
    .triangles = nullptr,
    .triangle_count = 0,
    .update = nullptr,
    .init = stub_init,
    .fixed_camera = nullptr,
};

scene_def_t scene_tunnel_def = {
    .name = "TUNNEL",
    .subtitle = "tube.005",
    .camera_distance = 0.1f,
    .camera_height = 0.0f,
    .target = {0, 0, -10},
    .vertices = nullptr,
    .vertex_count = 0,
    .triangles = nullptr,
    .triangle_count = 0,
    .update = nullptr,
    .init = stub_init,
    .fixed_camera = nullptr,
};
