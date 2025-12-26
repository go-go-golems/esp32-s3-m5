#pragma once

#include "esp_err.h"

// Start WiFi in SoftAP mode using menuconfig defaults (Tutorial 0017).
// Safe to call multiple times; second call returns ESP_OK.
esp_err_t wifi_softap_start(void);


