#pragma once

#include "esp_err.h"

// Initialize display + backlight and create the offscreen canvas sprite.
esp_err_t display_app_init(void);

// Present the current offscreen canvas to the physical display.
void display_app_present(void);

// Render a PNG from a file path (FATFS/VFS) onto the canvas and present it.
esp_err_t display_app_png_from_file(const char *path);

// Small boot message helper.
void display_app_show_boot_screen(const char *line1, const char *line2);


