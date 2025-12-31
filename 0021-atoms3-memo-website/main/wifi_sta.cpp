/*
 * Ticket CLINTS-MEMO-WEBSITE: WiFi Station bring-up (ESP-IDF).
 */

#include "wifi_sta.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "sdkconfig.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"
#include "esp_wifi.h"

#include "wifi_common.h"

static const char *TAG = "atoms3_memo_wifi";

static bool s_started = false;
static esp_netif_t *s_sta_netif = nullptr;
static EventGroupHandle_t s_wifi_event_group = nullptr;
static int s_retry_count = 0;

static constexpr EventBits_t kWifiConnectedBit = BIT0;
static constexpr EventBits_t kWifiFailBit = BIT1;

static void log_sta_ip(void) {
    if (!s_sta_netif) return;
    esp_netif_ip_info_t ip = {};
    if (esp_netif_get_ip_info(s_sta_netif, &ip) != ESP_OK) return;
    ESP_LOGI(TAG, "STA IP: " IPSTR, IP2STR(&ip.ip));
    ESP_LOGI(TAG, "browse: http://" IPSTR "/", IP2STR(&ip.ip));
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    (void)arg;
    (void)event_data;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        (void)esp_wifi_connect();
        return;
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_count < CONFIG_CLINTS_MEMO_WIFI_STA_MAX_RETRY) {
            s_retry_count++;
            ESP_LOGW(TAG, "STA disconnected; retrying (%d/%d)", s_retry_count, CONFIG_CLINTS_MEMO_WIFI_STA_MAX_RETRY);
            (void)esp_wifi_connect();
        } else {
            ESP_LOGE(TAG, "STA failed to connect (retries exhausted)");
            xEventGroupSetBits(s_wifi_event_group, kWifiFailBit);
        }
        return;
    }

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        s_retry_count = 0;
        log_sta_ip();
        xEventGroupSetBits(s_wifi_event_group, kWifiConnectedBit);
        return;
    }
}

esp_err_t wifi_sta_start(void) {
    if (s_started) return ESP_OK;

    if (!CONFIG_CLINTS_MEMO_WIFI_STA_ENABLE) {
        return ESP_ERR_INVALID_STATE;
    }

    const char *ssid = CONFIG_CLINTS_MEMO_WIFI_STA_SSID;
    if (!ssid || ssid[0] == '\0') {
        ESP_LOGW(TAG, "STA enabled but SSID is empty; skipping STA start");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_ERROR_CHECK(wifi_common_init());

    if (!s_wifi_event_group) {
        s_wifi_event_group = xEventGroupCreate();
    }
    if (!s_wifi_event_group) return ESP_ERR_NO_MEM;

    if (!s_sta_netif) {
        s_sta_netif = esp_netif_create_default_wifi_sta();
        if (!s_sta_netif) return ESP_FAIL;
    }

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, nullptr));

    wifi_config_t sta_cfg = {};
    strncpy((char *)sta_cfg.sta.ssid, ssid, sizeof(sta_cfg.sta.ssid) - 1);
    const char *pass = CONFIG_CLINTS_MEMO_WIFI_STA_PASSWORD;
    if (pass && pass[0] != '\0') {
        strncpy((char *)sta_cfg.sta.password, pass, sizeof(sta_cfg.sta.password) - 1);
    }
    sta_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_LOGI(TAG, "starting STA: ssid=%s", ssid);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    const EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group,
        kWifiConnectedBit | kWifiFailBit,
        pdFALSE,
        pdFALSE,
        pdMS_TO_TICKS(CONFIG_CLINTS_MEMO_WIFI_STA_CONNECT_TIMEOUT_MS));

    if (bits & kWifiConnectedBit) {
        s_started = true;
        return ESP_OK;
    }

    if (bits & kWifiFailBit) {
        return ESP_FAIL;
    }

    ESP_LOGW(TAG, "STA connect timeout (%dms)", CONFIG_CLINTS_MEMO_WIFI_STA_CONNECT_TIMEOUT_MS);
    // Keep WiFi running; eventual connect will still be logged via IP_EVENT.
    s_started = true;
    return ESP_OK;
}
