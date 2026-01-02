#pragma once

#include <stddef.h>

#include "M5GFX.h"

// Writes a framed PNG over USB-Serial/JTAG:
//   PNG_BEGIN <len>\n<raw png bytes>\nPNG_END\n
void screenshot_png_to_usb_serial_jtag(m5gfx::M5GFX &display);

