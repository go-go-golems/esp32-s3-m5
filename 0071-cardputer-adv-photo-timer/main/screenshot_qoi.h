#pragma once

#include <stddef.h>

#include <M5Unified.hpp>

void screenshot_qoi_to_usb_serial_jtag(m5gfx::M5GFX& display);
bool screenshot_qoi_to_usb_serial_jtag_ex(m5gfx::M5GFX& display, size_t* out_len);
