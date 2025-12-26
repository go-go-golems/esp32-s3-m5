#pragma once

#include "esp_err.h"

// Start WiFi based on menuconfig settings (SoftAP / STA / AP+STA).
// Safe to call multiple times; second call returns ESP_OK.
esp_err_t wifi_app_start(void);

// Access the created netifs (may be nullptr depending on mode).
// These are owned by ESP-IDF; do not free.
struct esp_netif_obj;
typedef struct esp_netif_obj esp_netif_t;

esp_netif_t *wifi_app_get_ap_netif(void);
esp_netif_t *wifi_app_get_sta_netif(void);


