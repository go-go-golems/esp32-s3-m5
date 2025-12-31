/*
 * Ticket CLINTS-MEMO-WEBSITE: WiFi mode selection.
 */

#include "wifi.h"

#include "sdkconfig.h"

#include "esp_err.h"
#include "esp_log.h"

#include "wifi_softap.h"
#include "wifi_sta.h"

static const char *TAG = "atoms3_memo_wifi";

esp_err_t wifi_start(void) {
    if (CONFIG_CLINTS_MEMO_WIFI_STA_ENABLE && CONFIG_CLINTS_MEMO_WIFI_STA_SSID[0] != '\0') {
        const esp_err_t err = wifi_sta_start();
        if (err == ESP_OK) return ESP_OK;
        ESP_LOGW(TAG, "STA start failed (%s); falling back to SoftAP", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "STA not configured; starting SoftAP");
    }
    return wifi_softap_start();
}
