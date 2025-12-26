#pragma once

#include <stddef.h>

#include "esp_err.h"

// Mount the tutorial storage FATFS partition at CONFIG_TUTORIAL_0017_STORAGE_MOUNT_PATH.
esp_err_t storage_fatfs_mount(void);

// Ensure the graphics directory exists (CONFIG_TUTORIAL_0017_STORAGE_GRAPHICS_DIR).
esp_err_t storage_fatfs_ensure_graphics_dir(void);

// Validate a filename used for /api/graphics/<name>:
// - no "/" or "\\" characters
// - no ".."
// - length > 0 and <= max_len
bool storage_fatfs_is_valid_filename(const char *name, size_t max_len);


