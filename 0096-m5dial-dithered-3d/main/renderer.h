#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "framebuffer.h"
#include "palette.h"
#include "scene.h"

// Initialize renderer (allocates working buffers)
bool renderer_init(void);

// Render one frame of the current scene into the 2-bit framebuffer
// Returns frame time in microseconds
uint64_t renderer_render_frame(uint8_t* fb, const palette_t* pal,
                               const scene_def_t* scene,
                               const render_params_t* params);

// Get render stats
typedef struct {
    uint32_t triangles_submitted;
    uint32_t triangles_drawn;
    uint32_t pixels_written;
    uint64_t frame_time_us;
} render_stats_t;

const render_stats_t* renderer_stats(void);
void renderer_stats_record(uint32_t triangles_submitted,
                           uint32_t triangles_drawn,
                           uint32_t pixels_written,
                           uint64_t frame_time_us);
