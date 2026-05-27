#pragma once

#include <stdint.h>

#include "scene.h"

// Fast poster-style scene renderers matching the JSX reference aesthetic:
// screen-locked ordered dithering, four-color palettes, circular mask, and
// large graphic silhouettes that are cheap enough for no-PSRAM M5Dial hardware.
void poster_render_scene(uint8_t* fb, SceneId scene_id, const render_params_t* params);

// Backward-compatible helper for the initial TERRAIN-only path.
void terrain_poster_render(uint8_t* fb, float camera_angle);
