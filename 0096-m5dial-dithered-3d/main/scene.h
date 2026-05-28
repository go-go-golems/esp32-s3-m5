#pragma once

#include <stdint.h>
#include <stdbool.h>

// Maximum geometry limits
#define SCENE_MAX_VERTICES   512
#define SCENE_MAX_TRIANGLES  1024

// Vertex: model-space position + vertex color
typedef struct {
    float x, y, z;          // position
    uint8_t cr, cg, cb;     // vertex color (0–255)
} scene_vertex_t;

// Triangle: three vertex indices
typedef struct {
    uint16_t v0, v1, v2;
} scene_tri_t;

// Scene definition
typedef struct {
    const char* name;
    const char* subtitle;
    float camera_distance;   // orbit radius
    float camera_height;     // height above target
    float target[3];         // look-at point

    // Dynamic vertex buffer (updated per-frame by update())
    scene_vertex_t* vertices;
    uint16_t vertex_count;
    scene_tri_t* triangles;
    uint16_t triangle_count;

    // Per-frame update (animate vertices, etc.)
    // time is seconds since boot
    void (*update)(void* ctx, float time);

    // Initialize scene (called once when scene is selected)
    void (*init)(void* ctx);

    // Fixed camera override (NULL for orbit mode)
    // If set, this function positions the camera directly
    void (*fixed_camera)(float* eye_x, float* eye_y, float* eye_z,
                         float* target_x, float* target_y, float* target_z,
                         float angle);
} scene_def_t;

// Scene enumeration
enum SceneId {
    SCENE_TERRAIN = 0,
    SCENE_TORUS,
    SCENE_OCEAN,
    SCENE_PLANET,
    SCENE_TUNNEL,
    SCENE_COUNT
};

// Get scene definition by ID
scene_def_t* scene_get(SceneId id);

// Get/set current scene
scene_def_t* scene_current(void);
void scene_set(SceneId id);
SceneId scene_current_id(void);
void scene_cycle_next(void);

typedef enum {
    RENDER_BACKEND_POSTER = 0,
    RENDER_BACKEND_PLANET3D = 1,
    RENDER_BACKEND_TERRAIN3D = 2,
} render_backend_t;

// Render parameters (externally modifiable)
typedef struct {
    float camera_angle;       // orbit angle in radians
    float auto_rotate_speed;  // rad/s (0 = manual only)
    float contrast;           // 0.6–2.5
    float aperture;           // 0.4–1.0 (circular mask radius)
    int pixel_size;           // 1–6 poster pixel block size
    render_backend_t backend; // poster scenes or experimental proper 3D path
    float encoder_step;       // radians per encoder delta
    uint32_t revision;        // incremented by console/state changes
    bool paused;
    bool wireframe;
    bool debug_ui;            // palette probes / diagnostic overlays
} render_params_t;

render_params_t* render_params_get(void);
void render_params_touch(void);
