/*
 * Ticket CLINTS-MEMO-WEBSITE: AP+STA mode (debug-friendly).
 */

#include "wifi_apsta.h"

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
static esp_netif_t *s_ap_netif = nullptr;
static esp_netif_t *s_sta_netif = nullptr;

static EventGroupHandle_t s_wifi_event_group = nullptr;
static int s_retry_count = 0;

static constexpr EventBits_t kWifiConnectedBit = BIT0;
static constexpr EventBits_t kWifiFailBit = BIT1;

static void log_ap_ip(void) {
    // Default AP IP is typically 192.168.4.1 even before DHCP is in play.
    ESP_LOGI(TAG, "SoftAP browse: http://" IPSTR "/", 192, 168, 4, 1);
}

static void log_sta_ip(void) {
    if (!s_sta_netif) return;
    esp_netif_ip_info_t ip = {};
    if (esp_netif_get_ip_info(s_sta_netif, &ip) != ESP_OK) return;
    ESP_LOGI(TAG, "STA IP: " IPSTR, IP2STR(&ip.ip));
    ESP_LOGI(TAG, "STA browse: http://" IPSTR "/", IP2STR(&ip.ip));
}

static const char *wifi_disc_reason_str(uint8_t reason) {
    switch (reason) {
    case WIFI_REASON_AUTH_EXPIRE: return "AUTH_EXPIRE";
    case WIFI_REASON_AUTH_LEAVE: return "AUTH_LEAVE";
    case WIFI_REASON_ASSOC_EXPIRE: return "ASSOC_EXPIRE";
    case WIFI_REASON_ASSOC_TOOMANY: return "ASSOC_TOOMANY";
    case WIFI_REASON_NOT_AUTHED: return "NOT_AUTHED";
    case WIFI_REASON_NOT_ASSOCED: return "NOT_ASSOCED";
    case WIFI_REASON_ASSOC_LEAVE: return "ASSOC_LEAVE";
    case WIFI_REASON_ASSOC_NOT_AUTHED: return "ASSOC_NOT_AUTHED";
    case WIFI_REASON_DISASSOC_PWRCAP_BAD: return "DISASSOC_PWRCAP_BAD";
    case WIFI_REASON_DISASSOC_SUPCHAN_BAD: return "DISASSOC_SUPCHAN_BAD";
    case WIFI_REASON_BSS_TRANSITION_DISASSOC: return "BSS_TRANSITION_DISASSOC";
    case WIFI_REASON_IE_INVALID: return "IE_INVALID";
    case WIFI_REASON_MIC_FAILURE: return "MIC_FAILURE";
    case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT: return "4WAY_HANDSHAKE_TIMEOUT";
    case WIFI_REASON_GROUP_KEY_UPDATE_TIMEOUT: return "GROUP_KEY_UPDATE_TIMEOUT";
    case WIFI_REASON_IE_IN_4WAY_DIFFERS: return "IE_IN_4WAY_DIFFERS";
    case WIFI_REASON_GROUP_CIPHER_INVALID: return "GROUP_CIPHER_INVALID";
    case WIFI_REASON_PAIRWISE_CIPHER_INVALID: return "PAIRWISE_CIPHER_INVALID";
    case WIFI_REASON_AKMP_INVALID: return "AKMP_INVALID";
    case WIFI_REASON_UNSUPP_RSN_IE_VERSION: return "UNSUPP_RSN_IE_VERSION";
    case WIFI_REASON_INVALID_RSN_IE_CAP: return "INVALID_RSN_IE_CAP";
    case WIFI_REASON_802_1X_AUTH_FAILED: return "802_1X_AUTH_FAILED";
    case WIFI_REASON_CIPHER_SUITE_REJECTED: return "CIPHER_SUITE_REJECTED";
    case WIFI_REASON_BEACON_TIMEOUT: return "BEACON_TIMEOUT";
    case WIFI_REASON_NO_AP_FOUND: return "NO_AP_FOUND";
    case WIFI_REASON_AUTH_FAIL: return "AUTH_FAIL";
    case WIFI_REASON_ASSOC_FAIL: return "ASSOC_FAIL";
    case WIFI_REASON_HANDSHAKE_TIMEOUT: return "HANDSHAKE_TIMEOUT";
    default: return "UNKNOWN";
    }
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    (void)arg;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        (void)esp_wifi_connect();
        return;
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        const wifi_event_sta_disconnected_t *disc = (const wifi_event_sta_disconnected_t *)event_data;
        const uint8_t reason = disc ? disc->reason : 0;
        ESP_LOGW(TAG, "STA disconnected: reason=%u (%s)", (unsigned)reason, wifi_disc_reason_str(reason));

        if (s_retry_count < CONFIG_CLINTS_MEMO_WIFI_STA_MAX_RETRY) {
            s_retry_count++;
            ESP_LOGW(TAG, "STA retrying (%d/%d)", s_retry_count, CONFIG_CLINTS_MEMO_WIFI_STA_MAX_RETRY);
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

esp_err_t wifi_apsta_start(void) {
    if (s_started) return ESP_OK;

    const char *ssid = CONFIG_CLINTS_MEMO_WIFI_STA_SSID;
    if (!ssid || ssid[0] == '\0') return ESP_ERR_INVALID_ARG;

    ESP_ERROR_CHECK(wifi_common_init());

    if (!s_wifi_event_group) {
        s_wifi_event_group = xEventGroupCreate();
    }
    if (!s_wifi_event_group) return ESP_ERR_NO_MEM;

    if (!s_ap_netif) {
        s_ap_netif = esp_netif_create_default_wifi_ap();
        if (!s_ap_netif) return ESP_FAIL;
    }
    if (!s_sta_netif) {
        s_sta_netif = esp_netif_create_default_wifi_sta();
        if (!s_sta_netif) return ESP_FAIL;
    }

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, nullptr));

    wifi_config_t ap_cfg = {};
    const char *ap_ssid = CONFIG_CLINTS_MEMO_WIFI_SOFTAP_SSID;
    const char *ap_pass = CONFIG_CLINTS_MEMO_WIFI_SOFTAP_PASSWORD;

    strncpy((char *)ap_cfg.ap.ssid, ap_ssid, sizeof(ap_cfg.ap.ssid) - 1);
    ap_cfg.ap.ssid_len = (uint8_t)strlen((const char *)ap_cfg.ap.ssid);
    strncpy((char *)ap_cfg.ap.password, ap_pass, sizeof(ap_cfg.ap.password) - 1);
    ap_cfg.ap.channel = (uint8_t)CONFIG_CLINTS_MEMO_WIFI_SOFTAP_CHANNEL;
    ap_cfg.ap.max_connection = (uint8_t)CONFIG_CLINTS_MEMO_WIFI_SOFTAP_MAX_CONN;
    ap_cfg.ap.authmode = (ap_pass && ap_pass[0] != '\0') ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;

    wifi_config_t sta_cfg = {};
    strncpy((char *)sta_cfg.sta.ssid, ssid, sizeof(sta_cfg.sta.ssid) - 1);
    const char *pass = CONFIG_CLINTS_MEMO_WIFI_STA_PASSWORD;
    if (pass && pass[0] != '\0') {
        strncpy((char *)sta_cfg.sta.password, pass, sizeof(sta_cfg.sta.password) - 1);
    }
    sta_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_LOGI(TAG, "starting AP+STA: ap_ssid=%s sta_ssid=%s", ap_ssid, ssid);
    log_ap_ip();

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_cfg));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    const EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group,
        kWifiConnectedBit | kWifiFailBit,
        pdFALSE,
        pdFALSE,
        pdMS_TO_TICKS(CONFIG_CLINTS_MEMO_WIFI_STA_CONNECT_TIMEOUT_MS));

    s_started = true;

    if (bits & kWifiConnectedBit) return ESP_OK;
    if (bits & kWifiFailBit) return ESP_FAIL;

    ESP_LOGW(TAG, "STA connect timeout (%dms); SoftAP remains available", CONFIG_CLINTS_MEMO_WIFI_STA_CONNECT_TIMEOUT_MS);
    return ESP_OK;
}

