#pragma once

#include "esp_err.h"

// Starts WiFi for this firmware:
// - If STA is enabled and SSID is set: connect to that AP.
// - Otherwise: start SoftAP.
esp_err_t wifi_start(void);

