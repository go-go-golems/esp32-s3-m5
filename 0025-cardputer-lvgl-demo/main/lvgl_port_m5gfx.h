#pragma once

#include <stdint.h>

#include "M5GFX.h"

// LVGL headers are C; keep the include in a stable place for this project.
extern "C" {
#include "lvgl.h"
}

struct LvglM5gfxConfig {
    int buffer_lines = 40;
    bool double_buffer = true;
    bool swap_bytes = false;
    int tick_ms = 2;
};

// Initializes LVGL and registers a display driver that flushes to M5GFX.
// Returns true on success; false if buffers cannot be allocated.
bool lvgl_port_m5gfx_init(m5gfx::M5GFX &display, const LvglM5gfxConfig &cfg);

