#ifndef DISPLAY_APP_H
#define DISPLAY_APP_H

#include "esp_err.h"

/**
 * Initialize the Tab5 display and render the M5 logo.
 *
 * High-level sequence:
 *   1. Initialize shared I2C.
 *   2. Initialize PI4IOE board I/O expanders.
 *   3. Reset touch/LCD-related board control lines.
 *   4. Start the display through the Tab5 BSP wrapper.
 *   5. Rotate to landscape drawing coordinates.
 *   6. Turn on the backlight.
 *   7. Create a centered LVGL image with the M5 logo asset.
 */
esp_err_t display_app_init(void);

#endif // DISPLAY_APP_H
