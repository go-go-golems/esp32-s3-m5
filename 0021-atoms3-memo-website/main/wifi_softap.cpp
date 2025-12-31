/*
 * Ticket CLINTS-MEMO-WEBSITE: WiFi SoftAP bring-up (ESP-IDF).
 */

#include "wifi_softap.h"

#include <string.h>

#include "sdkconfig.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

static const char *TAG = "atoms3_memo_website_0021";

static bool s_started = false;
static esp_netif_t *s_ap_netif = nullptr;

static void log_ap_ip(void) {
    if (!s_ap_netif) return;
    esp_netif_ip_info_t ip = {};
    if (esp_netif_get_ip_info(s_ap_netif, &ip) != ESP_OK) {
        return;
    }
    ESP_LOGI(TAG, "SoftAP IP: " IPSTR, IP2STR(&ip.ip));
}

esp_err_t wifi_softap_start(void) {
    if (s_started) return ESP_OK;

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

    s_ap_netif = esp_netif_create_default_wifi_ap();
    if (!s_ap_netif) return ESP_FAIL;

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t ap_cfg = {};
    const char *ssid = CONFIG_CLINTS_MEMO_WIFI_SOFTAP_SSID;
    const char *pass = CONFIG_CLINTS_MEMO_WIFI_SOFTAP_PASSWORD;

    memset(ap_cfg.ap.ssid, 0, sizeof(ap_cfg.ap.ssid));
    strncpy((char *)ap_cfg.ap.ssid, ssid, sizeof(ap_cfg.ap.ssid) - 1);
    ap_cfg.ap.ssid_len = (uint8_t)strlen((const char *)ap_cfg.ap.ssid);

    memset(ap_cfg.ap.password, 0, sizeof(ap_cfg.ap.password));
    strncpy((char *)ap_cfg.ap.password, pass, sizeof(ap_cfg.ap.password) - 1);

    ap_cfg.ap.channel = (uint8_t)CONFIG_CLINTS_MEMO_WIFI_SOFTAP_CHANNEL;
    ap_cfg.ap.max_connection = (uint8_t)CONFIG_CLINTS_MEMO_WIFI_SOFTAP_MAX_CONN;

    // Auth: open if password is empty, else WPA2-PSK.
    if (!pass || pass[0] == '\0') {
        ap_cfg.ap.authmode = WIFI_AUTH_OPEN;
    } else {
        ap_cfg.ap.authmode = WIFI_AUTH_WPA2_PSK;
    }

    ESP_LOGI(TAG, "starting SoftAP: ssid=%s channel=%d max_conn=%d auth=%s",
             ssid,
             CONFIG_CLINTS_MEMO_WIFI_SOFTAP_CHANNEL,
             CONFIG_CLINTS_MEMO_WIFI_SOFTAP_MAX_CONN,
             (ap_cfg.ap.authmode == WIFI_AUTH_OPEN) ? "OPEN" : "WPA2_PSK");

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    log_ap_ip();
    ESP_LOGI(TAG, "browse: http://" IPSTR "/", 192, 168, 4, 1);

    s_started = true;
    return ESP_OK;
}


