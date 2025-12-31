/*
 * Ticket CLINTS-MEMO-WEBSITE: WiFi common init (ESP-IDF).
 */

#include "wifi_common.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

static const char *TAG = "atoms3_memo_wifi";

static bool s_inited = false;

esp_err_t wifi_common_init(void) {
    if (s_inited) return ESP_OK;

    // NVS is required by WiFi (for PHY calibration data, etc).
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "nvs_flash_init failed (%s), erasing NVS and retrying", esp_err_to_name(err));
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK) return err;

    // init netif/event loop (avoid failing if already created by other modules).
    err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err == ESP_ERR_WIFI_INIT_STATE) {
        // already initialized
        err = ESP_OK;
    }
    if (err != ESP_OK) return err;

#if !defined(CONFIG_CLINTS_MEMO_WIFI_QUIET_IDF_LOGS) || CONFIG_CLINTS_MEMO_WIFI_QUIET_IDF_LOGS
    esp_log_level_set("wifi", ESP_LOG_WARN);
    esp_log_level_set("pp", ESP_LOG_WARN);
    esp_log_level_set("esp_netif_lwip", ESP_LOG_WARN);
    esp_log_level_set("esp_netif_handlers", ESP_LOG_WARN);
#endif

    s_inited = true;
    return ESP_OK;
}
