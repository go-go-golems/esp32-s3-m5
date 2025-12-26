/*
 * Tutorial 0017: WiFi bring-up (SoftAP / STA DHCP / AP+STA).
 */

#include "wifi_app.h"

#include <string.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

static const char *TAG = "atoms3r_web_ui_0017";

static bool s_started = false;
static esp_netif_t *s_ap_netif = nullptr;
static esp_netif_t *s_sta_netif = nullptr;

static EventGroupHandle_t s_ev = nullptr;
static const EventBits_t BIT_STA_GOT_IP = (1 << 0);

esp_netif_t *wifi_app_get_ap_netif(void) {
    return s_ap_netif;
}

esp_netif_t *wifi_app_get_sta_netif(void) {
    return s_sta_netif;
}

static esp_err_t ensure_nvs(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "nvs_flash_init failed (%s), erasing NVS and retrying", esp_err_to_name(err));
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err;
}

static void ensure_netif_and_event_loop(void) {
    esp_err_t err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(err);
    }
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(err);
    }
}

static void log_netif_ip(const char *label, esp_netif_t *netif) {
    if (!netif) return;
    esp_netif_ip_info_t ip = {};
    if (esp_netif_get_ip_info(netif, &ip) != ESP_OK) return;
    ESP_LOGI(TAG, "%s IP: " IPSTR, label, IP2STR(&ip.ip));
}

static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data) {
    (void)arg;
    (void)event_data;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "STA start: connecting...");
        esp_wifi_connect();
        return;
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "STA disconnected; retrying...");
        esp_wifi_connect();
        return;
    }

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI(TAG, "STA got DHCP lease");
        if (s_ev) {
            xEventGroupSetBits(s_ev, BIT_STA_GOT_IP);
        }
        log_netif_ip("STA", s_sta_netif);
        return;
    }
}

static void setup_softap_config(wifi_config_t *out_cfg) {
    wifi_config_t ap_cfg = {};
    const char *ssid = CONFIG_TUTORIAL_0017_WIFI_SOFTAP_SSID;
    const char *pass = CONFIG_TUTORIAL_0017_WIFI_SOFTAP_PASSWORD;

    memset(ap_cfg.ap.ssid, 0, sizeof(ap_cfg.ap.ssid));
    strncpy((char *)ap_cfg.ap.ssid, ssid, sizeof(ap_cfg.ap.ssid) - 1);
    ap_cfg.ap.ssid_len = (uint8_t)strlen((const char *)ap_cfg.ap.ssid);

    memset(ap_cfg.ap.password, 0, sizeof(ap_cfg.ap.password));
    strncpy((char *)ap_cfg.ap.password, pass, sizeof(ap_cfg.ap.password) - 1);
    ap_cfg.ap.channel = (uint8_t)CONFIG_TUTORIAL_0017_WIFI_SOFTAP_CHANNEL;
    ap_cfg.ap.max_connection = (uint8_t)CONFIG_TUTORIAL_0017_WIFI_SOFTAP_MAX_CONN;

    if (!pass || pass[0] == '\0') {
        ap_cfg.ap.authmode = WIFI_AUTH_OPEN;
    } else {
        ap_cfg.ap.authmode = WIFI_AUTH_WPA2_PSK;
    }

    *out_cfg = ap_cfg;
}

#if CONFIG_TUTORIAL_0017_WIFI_MODE_STA || CONFIG_TUTORIAL_0017_WIFI_MODE_APSTA
static void setup_sta_config(wifi_config_t *out_cfg) {
    wifi_config_t sta_cfg = {};
    const char *ssid = CONFIG_TUTORIAL_0017_WIFI_STA_SSID;
    const char *pass = CONFIG_TUTORIAL_0017_WIFI_STA_PASSWORD;

    memset(sta_cfg.sta.ssid, 0, sizeof(sta_cfg.sta.ssid));
    strncpy((char *)sta_cfg.sta.ssid, ssid, sizeof(sta_cfg.sta.ssid) - 1);

    memset(sta_cfg.sta.password, 0, sizeof(sta_cfg.sta.password));
    strncpy((char *)sta_cfg.sta.password, pass, sizeof(sta_cfg.sta.password) - 1);

    sta_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    sta_cfg.sta.pmf_cfg.capable = true;
    sta_cfg.sta.pmf_cfg.required = false;

    *out_cfg = sta_cfg;
}
#endif

static void start_softap(void) {
    if (!s_ap_netif) {
        s_ap_netif = esp_netif_create_default_wifi_ap();
    }

    wifi_config_t ap_cfg = {};
    setup_softap_config(&ap_cfg);

    ESP_LOGI(TAG, "starting SoftAP: ssid=%s channel=%d max_conn=%d",
             CONFIG_TUTORIAL_0017_WIFI_SOFTAP_SSID,
             CONFIG_TUTORIAL_0017_WIFI_SOFTAP_CHANNEL,
             CONFIG_TUTORIAL_0017_WIFI_SOFTAP_MAX_CONN);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_cfg));
    log_netif_ip("SoftAP", s_ap_netif);
    ESP_LOGI(TAG, "browse (SoftAP): http://" IPSTR "/", 192, 168, 4, 1);
}

#if CONFIG_TUTORIAL_0017_WIFI_MODE_STA || CONFIG_TUTORIAL_0017_WIFI_MODE_APSTA
static esp_err_t wait_for_sta_ip(void) {
    if (!s_ev) return ESP_FAIL;
    const TickType_t to = pdMS_TO_TICKS(CONFIG_TUTORIAL_0017_WIFI_STA_CONNECT_TIMEOUT_MS);
    const EventBits_t bits = xEventGroupWaitBits(s_ev, BIT_STA_GOT_IP, pdFALSE, pdFALSE, to);
    return (bits & BIT_STA_GOT_IP) ? ESP_OK : ESP_ERR_TIMEOUT;
}
#endif

esp_err_t wifi_app_start(void) {
    if (s_started) return ESP_OK;

    esp_err_t err = ensure_nvs();
    if (err != ESP_OK) return err;

    ensure_netif_and_event_loop();

    if (!s_ev) {
        s_ev = xEventGroupCreate();
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register STA/IP event handlers (safe even if STA isn't used).
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, nullptr));

#if CONFIG_TUTORIAL_0017_WIFI_MODE_SOFTAP
    s_ap_netif = esp_netif_create_default_wifi_ap();
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

    wifi_config_t ap_cfg = {};
    setup_softap_config(&ap_cfg);
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_cfg));

    ESP_ERROR_CHECK(esp_wifi_start());
    start_softap();

#elif CONFIG_TUTORIAL_0017_WIFI_MODE_STA
    s_sta_netif = esp_netif_create_default_wifi_sta();
    if (CONFIG_TUTORIAL_0017_WIFI_STA_HOSTNAME[0] != '\0') {
        (void)esp_netif_set_hostname(s_sta_netif, CONFIG_TUTORIAL_0017_WIFI_STA_HOSTNAME);
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    wifi_config_t sta_cfg = {};
    setup_sta_config(&sta_cfg);
    ESP_LOGI(TAG, "starting STA (DHCP): ssid=%s", CONFIG_TUTORIAL_0017_WIFI_STA_SSID);
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_cfg));

    ESP_ERROR_CHECK(esp_wifi_start());
    err = wait_for_sta_ip();
    if (err == ESP_OK) {
        // IP already logged by handler.
    } else {
        ESP_LOGW(TAG, "STA did not get IP within %d ms", CONFIG_TUTORIAL_0017_WIFI_STA_CONNECT_TIMEOUT_MS);
#if CONFIG_TUTORIAL_0017_WIFI_STA_FALLBACK_TO_SOFTAP
        ESP_LOGW(TAG, "falling back to SoftAP");
        s_ap_netif = esp_netif_create_default_wifi_ap();
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
        wifi_config_t ap_cfg = {};
        setup_softap_config(&ap_cfg);
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_cfg));
        start_softap();
#endif
    }

#elif CONFIG_TUTORIAL_0017_WIFI_MODE_APSTA
    s_ap_netif = esp_netif_create_default_wifi_ap();
    s_sta_netif = esp_netif_create_default_wifi_sta();
    if (CONFIG_TUTORIAL_0017_WIFI_STA_HOSTNAME[0] != '\0') {
        (void)esp_netif_set_hostname(s_sta_netif, CONFIG_TUTORIAL_0017_WIFI_STA_HOSTNAME);
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    wifi_config_t ap_cfg = {};
    wifi_config_t sta_cfg = {};
    setup_softap_config(&ap_cfg);
    setup_sta_config(&sta_cfg);
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_cfg));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_cfg));

    ESP_ERROR_CHECK(esp_wifi_start());
    start_softap();
    ESP_LOGI(TAG, "starting STA (DHCP): ssid=%s", CONFIG_TUTORIAL_0017_WIFI_STA_SSID);
    (void)wait_for_sta_ip();

#else
#error "No WiFi mode selected"
#endif

    s_started = true;
    return ESP_OK;
}


