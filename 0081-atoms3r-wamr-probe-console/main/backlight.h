#pragma once

#include <stdint.h>

// Backlight sequencing helpers (optional: GPIO gate + I2C brightness device).
void backlight_prepare_for_init(void);
void backlight_enable_after_init(void);

// Best-effort: sets brightness when the I2C brightness device is enabled.
void backlight_set_brightness(uint8_t brightness);


