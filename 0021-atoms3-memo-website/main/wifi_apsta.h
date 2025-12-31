#pragma once

#include "esp_err.h"

// Start WiFi in AP+STA mode:
// - Connect STA to CONFIG_CLINTS_MEMO_WIFI_STA_*
// - Keep SoftAP enabled as a recovery/debug backdoor
esp_err_t wifi_apsta_start(void);

