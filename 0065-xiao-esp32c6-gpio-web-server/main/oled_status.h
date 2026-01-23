#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Starts the OLED status display (best-effort).
// - Returns ESP_OK if the OLED feature is disabled or successfully started.
// - Returns an error if enabled but initialization failed.
esp_err_t oled_status_start(void);

#ifdef __cplusplus
}  // extern "C"
#endif

