#pragma once

#include "esp_err.h"

esp_err_t storage_spiffs_init(void);

// Returns a pointer to a static string like "/spiffs/rec".
const char *storage_recordings_dir(void);

