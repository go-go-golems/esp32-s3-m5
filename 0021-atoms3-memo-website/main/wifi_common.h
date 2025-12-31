#pragma once

#include "esp_err.h"

// Shared WiFi prerequisites:
// - NVS init (with erase+retry on version mismatch)
// - esp_netif init
// - default event loop
// - esp_wifi init
esp_err_t wifi_common_init(void);

