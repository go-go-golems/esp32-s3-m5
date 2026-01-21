#pragma once

#include "esp_err.h"
#include "esp_netif.h"

esp_err_t wifi_app_start(void);
esp_netif_t* wifi_app_get_ap_netif(void);

