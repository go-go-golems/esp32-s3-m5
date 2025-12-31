#pragma once

#include "esp_err.h"

// Start WiFi in STA mode and try to connect to CONFIG_CLINTS_MEMO_WIFI_STA_*.
// Safe to call multiple times; second call returns ESP_OK.
esp_err_t wifi_sta_start(void);

