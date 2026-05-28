#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "framebuffer.h"
#include "scene.h"

#define R3D_W 80
#define R3D_H 80
#define R3D_PIXEL_SCALE 3
#define R3D_Z_BITS 16

typedef struct {
    uint16_t logical_w;
    uint16_t logical_h;
    uint8_t pixel_scale;
    uint8_t z_bits;
    uint16_t sphere_vertices;
    uint16_t sphere_triangles;
    uint32_t triangles_submitted;
    uint32_t triangles_drawn;
    uint32_t planet_pixels;
    uint32_t ring_pixels;
    uint32_t moon_pixels;
    uint32_t zbuffer_bytes;
    uint32_t colorbuffer_bytes;
    uint64_t render_time_us;
} renderer3d_stats_t;

bool renderer3d_init(void);
uint64_t renderer3d_render_planet(uint8_t* fb, const render_params_t* params);
const renderer3d_stats_t* renderer3d_stats(void);
