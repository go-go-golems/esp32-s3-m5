#include "scene.h"
#include <string.h>
#include <esp_log.h>

static const char* TAG = "scene";

static SceneId s_current_scene = SCENE_TERRAIN;
static render_params_t s_params = {
    .camera_angle = 0.0f,
    .auto_rotate_speed = 0.0f,
    .contrast = 1.4f,
    .aperture = 0.97f,
    .pixel_size = 1,
    .encoder_step = 0.5235988f,  // 2π / 12 clicks per physical rotation
    .revision = 0,
    .paused = false,
    .wireframe = false,
    .debug_ui = true,
};

// Scene registry — implemented in scene_*.cpp files
extern scene_def_t scene_terrain_def;
extern scene_def_t scene_torus_def;
extern scene_def_t scene_ocean_def;
extern scene_def_t scene_planet_def;
extern scene_def_t scene_tunnel_def;

static scene_def_t* s_scenes[SCENE_COUNT] = {
    &scene_terrain_def,
    &scene_torus_def,
    &scene_ocean_def,
    &scene_planet_def,
    &scene_tunnel_def,
};

scene_def_t* scene_get(SceneId id) {
    if (id < 0 || id >= SCENE_COUNT) return s_scenes[0];
    return s_scenes[id];
}

scene_def_t* scene_current(void) {
    return s_scenes[s_current_scene];
}

void scene_set(SceneId id) {
    if (id < 0 || id >= SCENE_COUNT) return;
    s_current_scene = id;
    scene_def_t* s = scene_current();
    if (s->init) {
        s->init(s);
    }
    s_params.revision++;
    ESP_LOGI(TAG, "scene: %s", s->name);
}

SceneId scene_current_id(void) {
    return s_current_scene;
}

void scene_cycle_next(void) {
    s_current_scene = (SceneId)((s_current_scene + 1) % SCENE_COUNT);
    scene_def_t* s = scene_current();
    if (s->init) {
        s->init(s);
    }
    s_params.revision++;
    ESP_LOGI(TAG, "scene: %s", s->name);
}

render_params_t* render_params_get(void) {
    return &s_params;
}

void render_params_touch(void) {
    s_params.revision++;
}
