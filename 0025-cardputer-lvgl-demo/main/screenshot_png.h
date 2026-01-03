#pragma once

#include <stddef.h>

#include "M5GFX.h"

// Writes a framed PNG over USB-Serial/JTAG:
//   PNG_BEGIN <len>\n<raw png bytes>\nPNG_END\n
void screenshot_png_to_usb_serial_jtag(m5gfx::M5GFX &display);

// Same as screenshot_png_to_usb_serial_jtag(), but returns success/failure and the PNG length.
bool screenshot_png_to_usb_serial_jtag_ex(m5gfx::M5GFX &display, size_t *out_len);

// Captures a screenshot and saves it to MicroSD (under /sd/shots/; 8.3 names).
// Returns success/failure and optionally the PNG length + absolute path written.
bool screenshot_png_save_to_sd_ex(m5gfx::M5GFX &display, char *out_path, size_t out_path_cap, size_t *out_len);
