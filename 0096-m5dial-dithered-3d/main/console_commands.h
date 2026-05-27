#pragma once

#include <stdint.h>

// Provide the current 2-bit framebuffer so console commands such as dumpfb can
// export screenshots over USB Serial/JTAG.
void console_commands_set_framebuffer(const uint8_t* framebuffer);

// Register all esp_console commands for the dithered 3D viewer
void console_commands_register(void);
