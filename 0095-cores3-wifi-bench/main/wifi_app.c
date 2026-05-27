/*
 * CoreS3 WiFi Benchmark: native WiFi bring-up.
 *
 * Same API as the Tab5's wifi_app.c but uses native esp_wifi
 * (not esp_wifi_remote / ESP-Hosted). The CoreS3's ESP32-S3 has
 * WiFi built into the same chip.
 */

#include "wifi_app.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"
#include "esp_wifi.h"
#include "lwip/inet.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "cores3_bench_wifi";

#define CORES3_WIFI_AP_SSID "CoreS3-Bench"
#define CORES3_WIFI_AP_PASSWORD "cores3bench"
#define CORES3_WIFI_AP_CHANNEL 1
#define CORES3_WIFI_AP_MAX_CONN 4
#define CORES3_WIFI_HOSTNAME "cores3-bench"
#define CORES3_WIFI_NVS_NAMESPACE "wifi"
#define CORES3_WIFI_NVS_KEY_SSID "ssid"
#define CORES3_WIFI_NVS_KEY_PASS "pass"
#define CORES3_WIFI_MAX_RETRY 10

static SemaphoreHandle_t s_mutex = NULL;
static esp_netif_t *s_ap_netif = NULL;
static esp_netif_t *s_sta_netif = NULL;
static bool s_started = false;
static bool s_wifi_inited = false;
static bool s_autoconnect = false;
static int s_retry_count = 0;

static char s_runtime_ssid[33] = {0};
static char s_runtime_pass[65] = {0};
static bool s_has_runtime_creds = false;
static bool s_has_saved_creds = false;

static wifi_app_status_t s_status = {
    .state = WIFI_APP_STATE_UNINIT,
    .ssid = {0},
    .has_saved_creds = false,
    .has_runtime_creds = false,
    .sta_ip4 = 0,
    .ap_ip4 = 0,
    .last_disconnect_reason = -1,
};

esp_err_t wifi_app_save_credentials(void);
esp_err_t wifi_app_connect(void);
esp_err_t wifi_app_disconnect(void);

static void lock_state(void) {
    if (s_mutex) xSemaphoreTake(s_mutex, portMAX_DELAY);
}
static void unlock_state(void) {
    if (s_mutex) xSemaphoreGive(s_mutex);
}

static void sync_status_locked(void) {
    strlcpy(s_status.ssid, s_runtime_ssid, sizeof(s_status.ssid));
    s_status.has_saved_creds = s_has_saved_creds;
    s_status.has_runtime_creds = s_has_runtime_creds;
}

static esp_err_t ensure_nvs(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS needs erase (%s), retrying", esp_err_to_name(err));
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err;
}

static void ensure_netif_and_event_loop(void) {
    esp_err_t err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) ESP_ERROR_CHECK(err);
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) ESP_ERROR_CHECK(err);
}

static esp_err_t nvs_open_wifi(nvs_handle_t *out) {
    if (!out) return ESP_ERR_INVALID_ARG;
    return nvs_open(CORES3_WIFI_NVS_NAMESPACE, NVS_READWRITE, out);
}

static esp_err_t save_current_credentials_to_nvs(void) {
    if (!s_has_runtime_creds || s_runtime_ssid[0] == '\0') return ESP_ERR_INVALID_STATE;
    nvs_handle_t nvs = 0;
    esp_err_t err = nvs_open_wifi(&nvs);
    if (err != ESP_OK) return err;
    err = nvs_set_str(nvs, CORES3_WIFI_NVS_KEY_SSID, s_runtime_ssid);
    if (err == ESP_OK) err = nvs_set_str(nvs, CORES3_WIFI_NVS_KEY_PASS, s_runtime_pass);
    if (err == ESP_OK) err = nvs_commit(nvs);
    nvs_close(nvs);
    return err;
}

static esp_err_t clear_saved_credentials_from_nvs(void) {
    nvs_handle_t nvs = 0;
    esp_err_t err = nvs_open_wifi(&nvs);
    if (err != ESP_OK) return err;
    (void)nvs_erase_key(nvs, CORES3_WIFI_NVS_KEY_SSID);
    (void)nvs_erase_key(nvs, CORES3_WIFI_NVS_KEY_PASS);
    err = nvs_commit(nvs);
    nvs_close(nvs);
    return err;
}

static esp_err_t load_saved_credentials(void) {
    nvs_handle_t nvs = 0;
    esp_err_t err = nvs_open_wifi(&nvs);
    if (err != ESP_OK) return err;
    char ssid[33] = {0};
    size_t ssid_len = sizeof(ssid);
    err = nvs_get_str(nvs, CORES3_WIFI_NVS_KEY_SSID, ssid, &ssid_len);
    if (err != ESP_OK || ssid[0] == '\0') { nvs_close(nvs); return ESP_ERR_NOT_FOUND; }
    char pass[65] = {0};
    size_t pass_len = sizeof(pass);
    const esp_err_t pass_err = nvs_get_str(nvs, CORES3_WIFI_NVS_KEY_PASS, pass, &pass_len);
    if (pass_err != ESP_OK) pass[0] = '\0';
    nvs_close(nvs);
    lock_state();
    strlcpy(s_runtime_ssid, ssid, sizeof(s_runtime_ssid));
    strlcpy(s_runtime_pass, pass, sizeof(s_runtime_pass));
    s_has_runtime_creds = true;
    s_has_saved_creds = true;
    sync_status_locked();
    unlock_state();
    return ESP_OK;
}

static void update_ap_ip_locked(void) {
    if (!s_ap_netif) return;
    esp_netif_ip_info_t ip = {0};
    if (esp_netif_get_ip_info(s_ap_netif, &ip) == ESP_OK)
        s_status.ap_ip4 = ntohl(ip.ip.addr);
}

static void update_sta_ip_locked(void) {
    if (!s_sta_netif) return;
    esp_netif_ip_info_t ip = {0};
    if (esp_netif_get_ip_info(s_sta_netif, &ip) == ESP_OK)
        s_status.sta_ip4 = ntohl(ip.ip.addr);
}

static void log_host_ip(const char *label, uint32_t ip4_host_order) {
    if (ip4_host_order == 0) return;
    ip4_addr_t ip = {.addr = htonl(ip4_host_order)};
    ESP_LOGI(TAG, "%s IP: " IPSTR, label, IP2STR(&ip));
}

static void log_host_urls(void) {
    if (s_status.ap_ip4 != 0) {
        ip4_addr_t ip = {.addr = htonl(s_status.ap_ip4)};
        ESP_LOGI(TAG, "SoftAP browse: http://" IPSTR "/", IP2STR(&ip));
    }
    if (s_status.sta_ip4 != 0) {
        ip4_addr_t ip = {.addr = htonl(s_status.sta_ip4)};
        ESP_LOGI(TAG, "STA browse: http://" IPSTR "/", IP2STR(&ip));
    }
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
    (void)arg;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_START) {
        lock_state(); update_ap_ip_locked(); unlock_state();
        log_host_urls();
        return;
    }
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        const wifi_event_ap_staconnected_t *e = event_data;
        if (e) ESP_LOGI(TAG, "AP client connected: %02x:%02x:%02x:%02x:%02x:%02x aid=%d",
                        e->mac[0],e->mac[1],e->mac[2],e->mac[3],e->mac[4],e->mac[5], e->aid);
        return;
    }
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        const wifi_event_ap_stadisconnected_t *e = event_data;
        if (e) ESP_LOGI(TAG, "AP client disconnected: %02x:%02x:%02x:%02x:%02x:%02x aid=%d",
                        e->mac[0],e->mac[1],e->mac[2],e->mac[3],e->mac[4],e->mac[5], e->aid);
        return;
    }
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        lock_state();
        s_status.state = WIFI_APP_STATE_IDLE;
        const bool do_connect = s_autoconnect && s_has_runtime_creds;
        unlock_state();
        if (do_connect) {
            ESP_LOGI(TAG, "STA start: connecting...");
            esp_err_t err = esp_wifi_connect();
            if (err == ESP_OK) { lock_state(); s_status.state = WIFI_APP_STATE_CONNECTING; unlock_state(); }
            else ESP_LOGW(TAG, "esp_wifi_connect failed: %s", esp_err_to_name(err));
        } else {
            ESP_LOGI(TAG, "STA start: idle (no saved credentials yet)");
        }
        return;
    }
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        lock_state(); s_status.state = WIFI_APP_STATE_CONNECTING; unlock_state();
        return;
    }
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        const wifi_event_sta_disconnected_t *disc = event_data;
        const int reason = disc ? (int)disc->reason : -1;
        lock_state();
        s_status.sta_ip4 = 0;
        s_status.last_disconnect_reason = reason;
        if (s_status.state != WIFI_APP_STATE_UNINIT) s_status.state = WIFI_APP_STATE_IDLE;
        const bool should_retry = s_autoconnect && s_has_runtime_creds && (s_retry_count < CORES3_WIFI_MAX_RETRY);
        unlock_state();
        ESP_LOGW(TAG, "STA disconnected (reason=%d)%s", reason, should_retry ? " -> retry" : "");
        if (should_retry) { s_retry_count++; esp_wifi_connect(); }
        return;
    }
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        lock_state();
        s_retry_count = 0;
        s_status.state = WIFI_APP_STATE_CONNECTED;
        update_sta_ip_locked();
        unlock_state();
        log_host_urls();
        return;
    }
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_LOST_IP) {
        lock_state();
        s_status.sta_ip4 = 0;
        if (s_status.state != WIFI_APP_STATE_UNINIT) s_status.state = WIFI_APP_STATE_CONNECTING;
        unlock_state();
        return;
    }
}

static esp_err_t apply_sta_config(void) {
    if (!s_wifi_inited) return ESP_ERR_INVALID_STATE;
    wifi_config_t sta_cfg = {0};
    lock_state();
    strlcpy((char *)sta_cfg.sta.ssid, s_runtime_ssid, sizeof(sta_cfg.sta.ssid));
    strlcpy((char *)sta_cfg.sta.password, s_runtime_pass, sizeof(sta_cfg.sta.password));
    unlock_state();
    sta_cfg.sta.threshold.authmode = WIFI_AUTH_OPEN;
    sta_cfg.sta.pmf_cfg.capable = true;
    sta_cfg.sta.pmf_cfg.required = false;
    return esp_wifi_set_config(WIFI_IF_STA, &sta_cfg);
}

esp_err_t wifi_app_start(void) {
    if (s_started) return ESP_OK;

    esp_err_t err = ensure_nvs();
    if (err != ESP_OK) return err;
    ensure_netif_and_event_loop();

    if (!s_mutex) { s_mutex = xSemaphoreCreateMutex(); if (!s_mutex) return ESP_ERR_NO_MEM; }

    /* Native WiFi init (NOT esp_wifi_remote) */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_LOST_IP, &wifi_event_handler, NULL));
    s_wifi_inited = true;

    lock_state();
    s_status.state = WIFI_APP_STATE_IDLE;
    s_status.last_disconnect_reason = -1;
    unlock_state();

    err = load_saved_credentials();
    if (err == ESP_OK) ESP_LOGI(TAG, "Loaded Wi-Fi credentials from NVS (ssid=%s)", s_runtime_ssid);

    lock_state(); s_autoconnect = s_has_runtime_creds; unlock_state();

    /* APSTA mode */
    s_ap_netif = esp_netif_create_default_wifi_ap();
    s_sta_netif = esp_netif_create_default_wifi_sta();
    (void)esp_netif_set_hostname(s_sta_netif, CORES3_WIFI_HOSTNAME);

    wifi_config_t ap_cfg = {0};
    strlcpy((char *)ap_cfg.ap.ssid, CORES3_WIFI_AP_SSID, sizeof(ap_cfg.ap.ssid));
    ap_cfg.ap.ssid_len = (uint8_t)strlen((const char *)ap_cfg.ap.ssid);
    strlcpy((char *)ap_cfg.ap.password, CORES3_WIFI_AP_PASSWORD, sizeof(ap_cfg.ap.password));
    ap_cfg.ap.channel = CORES3_WIFI_AP_CHANNEL;
    ap_cfg.ap.max_connection = CORES3_WIFI_AP_MAX_CONN;
    ap_cfg.ap.authmode = WIFI_AUTH_WPA2_PSK;
    ap_cfg.ap.pmf_cfg.capable = true;
    ap_cfg.ap.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_cfg));

    if (s_has_runtime_creds) {
        ESP_ERROR_CHECK(apply_sta_config());
        ESP_LOGI(TAG, "STA configured for ssid=%s", s_runtime_ssid);
    }

    ESP_ERROR_CHECK(esp_wifi_start());

    lock_state(); update_ap_ip_locked(); update_sta_ip_locked(); unlock_state();
    log_host_urls();

    s_started = true;
    return ESP_OK;
}

esp_err_t wifi_app_get_status(wifi_app_status_t *out) {
    if (!out) return ESP_ERR_INVALID_ARG;
    if (!s_mutex) return ESP_ERR_INVALID_STATE;
    lock_state(); *out = s_status; unlock_state();
    return ESP_OK;
}

esp_err_t wifi_app_set_credentials(const char *ssid, const char *password, bool save_to_nvs) {
    if (!ssid || ssid[0] == '\0') return ESP_ERR_INVALID_ARG;
    if (strlen(ssid) > 32 || (password && strlen(password) > 64)) return ESP_ERR_INVALID_SIZE;
    lock_state();
    strlcpy(s_runtime_ssid, ssid, sizeof(s_runtime_ssid));
    strlcpy(s_runtime_pass, password ? password : "", sizeof(s_runtime_pass));
    s_has_runtime_creds = true;
    sync_status_locked();
    unlock_state();
    if (save_to_nvs) {
        esp_err_t err = wifi_app_save_credentials();
        if (err != ESP_OK) return err;
        if (s_started) return wifi_app_connect();
    }
    if (s_wifi_inited) return apply_sta_config();
    return ESP_OK;
}

esp_err_t wifi_app_save_credentials(void) {
    esp_err_t err = save_current_credentials_to_nvs();
    if (err != ESP_OK) return err;
    lock_state(); s_has_saved_creds = true; sync_status_locked(); unlock_state();
    ESP_LOGI(TAG, "Saved Wi-Fi credentials to NVS (ssid=%s)", s_runtime_ssid);
    return ESP_OK;
}

esp_err_t wifi_app_clear_credentials(void) {
    esp_err_t err = clear_saved_credentials_from_nvs();
    lock_state();
    s_runtime_ssid[0] = '\0'; s_runtime_pass[0] = '\0';
    s_has_runtime_creds = false; s_has_saved_creds = false;
    s_autoconnect = false; s_retry_count = 0;
    s_status.sta_ip4 = 0; s_status.last_disconnect_reason = -1;
    s_status.state = WIFI_APP_STATE_IDLE;
    sync_status_locked(); unlock_state();
    if (s_wifi_inited) {
        (void)esp_wifi_disconnect();
        wifi_config_t sta_cfg = {0};
        (void)esp_wifi_set_config(WIFI_IF_STA, &sta_cfg);
    }
    return err;
}

esp_err_t wifi_app_connect(void) {
    if (!s_wifi_inited) return ESP_ERR_INVALID_STATE;
    lock_state();
    const bool have_creds = s_has_runtime_creds && s_runtime_ssid[0] != '\0';
    s_autoconnect = have_creds; s_retry_count = 0;
    unlock_state();
    if (!have_creds) return ESP_ERR_INVALID_STATE;
    esp_err_t err = apply_sta_config();
    if (err != ESP_OK) return err;
    err = esp_wifi_connect();
    if (err == ESP_OK) { lock_state(); s_status.state = WIFI_APP_STATE_CONNECTING; unlock_state(); }
    return err;
}

esp_err_t wifi_app_disconnect(void) {
    if (!s_wifi_inited) return ESP_ERR_INVALID_STATE;
    lock_state();
    s_autoconnect = false; s_retry_count = 0;
    s_status.sta_ip4 = 0; s_status.state = WIFI_APP_STATE_IDLE;
    unlock_state();
    return esp_wifi_disconnect();
}

esp_err_t wifi_app_scan(wifi_scan_entry_t *out, size_t max_out, size_t *out_n) {
    if (!out || !out_n || max_out == 0) return ESP_ERR_INVALID_ARG;
    if (!s_wifi_inited) return ESP_ERR_INVALID_STATE;
    wifi_scan_config_t scan_cfg = {
        .ssid = NULL, .bssid = NULL, .channel = 0,
        .show_hidden = true, .scan_type = WIFI_SCAN_TYPE_ACTIVE,
    };
    esp_err_t err = esp_wifi_scan_start(&scan_cfg, true);
    if (err != ESP_OK) return err;
    uint16_t ap_num = 0;
    err = esp_wifi_scan_get_ap_num(&ap_num);
    if (err != ESP_OK) return err;
    uint16_t want = ap_num;
    if (want > max_out) want = (uint16_t)max_out;
    if (want == 0) { *out_n = 0; return ESP_OK; }
    wifi_ap_record_t *records = calloc(want, sizeof(*records));
    if (!records) return ESP_ERR_NO_MEM;
    uint16_t got = want;
    err = esp_wifi_scan_get_ap_records(&got, records);
    if (err != ESP_OK) { free(records); return err; }
    for (uint16_t i = 0; i < got; i++) {
        memset(&out[i], 0, sizeof(out[i]));
        strlcpy(out[i].ssid, (const char *)records[i].ssid, sizeof(out[i].ssid));
        out[i].rssi = records[i].rssi;
        out[i].channel = records[i].primary;
        out[i].authmode = (uint8_t)records[i].authmode;
    }
    free(records);
    (void)esp_wifi_clear_ap_list();
    *out_n = (size_t)got;
    return ESP_OK;
}
