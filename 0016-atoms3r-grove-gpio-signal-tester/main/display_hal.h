#pragma once

#include "M5GFX.h"

// Display bring-up for AtomS3R GC9107 via M5GFX/LovyanGFX.
bool display_init_m5gfx(void);

// Access the initialized display instance (valid after display_init_m5gfx()).
m5gfx::M5GFX &display_get(void);

// Present an offscreen canvas to the physical display (optionally using DMA).
void display_present_canvas(M5Canvas &canvas);



