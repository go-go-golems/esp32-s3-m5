#pragma once

#include <stddef.h>

#include "M5GFX.h"

// Writes a framed PNG over USB-Serial/JTAG:
//   PNG_BEGIN <len>\n<raw png bytes>\nPNG_END\n
void screenshot_png_to_usb_serial_jtag(m5gfx::M5GFX &display);

// Same as screenshot_png_to_usb_serial_jtag(), but returns success/failure and the PNG length.
bool screenshot_png_to_usb_serial_jtag_ex(m5gfx::M5GFX &display, size_t *out_len);
