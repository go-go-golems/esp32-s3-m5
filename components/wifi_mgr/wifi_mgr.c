/*
 * wifi_mgr: Wi-Fi STA manager (console-friendly).
 *
 * Derived from:
 * - esp32-s3-m5/0046-xiao-esp32c6-led-patterns-webui/main/wifi_mgr.c
 * - esp32-s3-m5/0048-cardputer-js-web/main/wifi_mgr.c
 *
 * Goal: provide a small, self-contained STA manager with:
 * - runtime credentials (set via esp_console)
 * - optional persistence to NVS
 * - simple "status" snapshot for /api/status and console UX
 * - autoconnect retry loop (bounded)
 */
 
#include "wifi_mgr.h"

#include <stdlib.h>
#include <string.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"
#include "esp_wifi.h"
#include "lwip/inet.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "wifi_mgr";

static SemaphoreHandle_t s_mu = NULL;

static int s_retry = 0;
static bool s_started = false;
static bool s_autoconnect = false;

static esp_netif_t *s_sta_netif = NULL;
static bool s_wifi_inited = false;

static char s_runtime_ssid[33] = {0};
static char s_runtime_pass[65] = {0};
static bool s_has_runtime = false;
static bool s_has_saved = false;

static wifi_mgr_status_t s_status = {
    .state = WIFI_MGR_STATE_UNINIT,
    .ssid = {0},
    .has_saved_creds = false,
    .has_runtime_creds = false,
    .ip4 = 0,
    .last_disconnect_reason = -1,
};

static wifi_mgr_on_got_ip_cb_t s_on_got_ip_cb = NULL;
static void *s_on_got_ip_ctx = NULL;

static void lock_mu(void)
{
    if (s_mu) xSemaphoreTake(s_mu, portMAX_DELAY);
}

static void unlock_mu(void)
{
    if (s_mu) xSemaphoreGive(s_mu);
}

static bool creds_present_unsafe(void)
{
    return s_runtime_ssid[0] != '\0';
}

static esp_err_t ensure_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "nvs init failed (%s), erasing...", esp_err_to_name(err));
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err;
}

static void ensure_netif_and_loop(void)
{
    esp_err_t err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(err);
    }
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(err);
    }
}

static esp_err_t nvs_open_wifi(nvs_handle_t *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    return nvs_open("wifi", NVS_READWRITE, out);
}

static esp_err_t load_saved_credentials(void)
{
    nvs_handle_t nvs = 0;
    esp_err_t err = nvs_open_wifi(&nvs);
    if (err != ESP_OK) return err;

    char ssid[33] = {0};
    size_t ssid_len = sizeof(ssid);
    err = nvs_get_str(nvs, "ssid", ssid, &ssid_len);
    if (err != ESP_OK || ssid[0] == '\0') {
        nvs_close(nvs);
        return ESP_ERR_NOT_FOUND;
    }

    char pass[65] = {0};
    size_t pass_len = sizeof(pass);
    const esp_err_t pass_err = nvs_get_str(nvs, "pass", pass, &pass_len);
    if (pass_err != ESP_OK) {
        pass[0] = '\0';
    }
    nvs_close(nvs);

    lock_mu();
    strlcpy(s_runtime_ssid, ssid, sizeof(s_runtime_ssid));
    strlcpy(s_runtime_pass, pass, sizeof(s_runtime_pass));
    s_has_runtime = true;
    s_has_saved = true;
    strlcpy(s_status.ssid, s_runtime_ssid, sizeof(s_status.ssid));
    s_status.has_runtime_creds = true;
    s_status.has_saved_creds = true;
    unlock_mu();

    return ESP_OK;
}

static esp_err_t save_credentials_to_nvs(const char *ssid, const char *password)
{
    nvs_handle_t nvs = 0;
    esp_err_t err = nvs_open_wifi(&nvs);
    if (err != ESP_OK) return err;
    err = nvs_set_str(nvs, "ssid", ssid ? ssid : "");
    if (err == ESP_OK) err = nvs_set_str(nvs, "pass", password ? password : "");
    if (err == ESP_OK) err = nvs_commit(nvs);
    nvs_close(nvs);
    return err;
}

static esp_err_t clear_credentials_in_nvs(void)
{
    nvs_handle_t nvs = 0;
    esp_err_t err = nvs_open_wifi(&nvs);
    if (err != ESP_OK) return err;
    (void)nvs_erase_key(nvs, "ssid");
    (void)nvs_erase_key(nvs, "pass");
    err = nvs_commit(nvs);
    nvs_close(nvs);
    return err;
}

static void status_set_ip_from_netif(esp_netif_t *sta)
{
    if (!sta) return;
    esp_netif_ip_info_t ip = {};
    if (esp_netif_get_ip_info(sta, &ip) != ESP_OK) return;
    s_status.ip4 = ntohl(ip.ip.addr);
}

static void status_log_ip(uint32_t ip4_host_order)
{
    if (ip4_host_order == 0) return;
    ip4_addr_t ip = {.addr = htonl(ip4_host_order)};
    ESP_LOGI(TAG, "STA IP: " IPSTR, IP2STR(&ip));
    ESP_LOGI(TAG, "browse: http://" IPSTR "/", IP2STR(&ip));
}

static esp_err_t apply_runtime_config(void)
{
    if (!s_wifi_inited) return ESP_ERR_INVALID_STATE;

    wifi_config_t sta_cfg = {0};
    lock_mu();
    strlcpy((char *)sta_cfg.sta.ssid, s_runtime_ssid, sizeof(sta_cfg.sta.ssid));
    strlcpy((char *)sta_cfg.sta.password, s_runtime_pass, sizeof(sta_cfg.sta.password));
    unlock_mu();

    sta_cfg.sta.threshold.authmode = WIFI_AUTH_OPEN;
    sta_cfg.sta.pmf_cfg.capable = true;
    sta_cfg.sta.pmf_cfg.required = false;
    return esp_wifi_set_config(WIFI_IF_STA, &sta_cfg);
}

static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        lock_mu();
        s_started = true;
        if (s_status.state == WIFI_MGR_STATE_UNINIT) {
            s_status.state = WIFI_MGR_STATE_IDLE;
        }
        const bool do_connect = s_autoconnect && creds_present_unsafe();
        unlock_mu();

        if (do_connect) {
            ESP_LOGI(TAG, "STA start: autoconnect...");
            const esp_err_t err = esp_wifi_connect();
            if (err == ESP_OK) {
                lock_mu();
                s_status.state = WIFI_MGR_STATE_CONNECTING;
                unlock_mu();
            } else {
                ESP_LOGW(TAG, "autoconnect failed: %s", esp_err_to_name(err));
            }
        } else {
            ESP_LOGI(TAG, "STA start: idle (no credentials yet)");
        }
        return;
    }

    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_CONNECTED) {
        lock_mu();
        if (s_status.state != WIFI_MGR_STATE_CONNECTED) {
            s_status.state = WIFI_MGR_STATE_CONNECTING;
        }
        unlock_mu();
        return;
    }

    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        const wifi_event_sta_disconnected_t *disc = (const wifi_event_sta_disconnected_t *)data;
        const int reason = disc ? (int)disc->reason : -1;

        lock_mu();
        s_status.state = WIFI_MGR_STATE_IDLE;
        s_status.ip4 = 0;
        s_status.last_disconnect_reason = reason;
        const bool do_retry =
            s_autoconnect && creds_present_unsafe() && (s_retry < CONFIG_WIFI_MGR_MAX_RETRY);
        unlock_mu();

        ESP_LOGW(TAG, "STA disconnected (reason=%d)%s", reason, do_retry ? " -> retry" : "");
        if (do_retry) {
            lock_mu();
            s_retry++;
            unlock_mu();
            esp_wifi_connect();
            return;
        }

        if (s_autoconnect && creds_present_unsafe()) {
            ESP_LOGE(TAG, "max retries reached (%d)", CONFIG_WIFI_MGR_MAX_RETRY);
        }
        return;
    }

    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        const ip_event_got_ip_t *got = (const ip_event_got_ip_t *)data;
        const uint32_t ip4 = got ? ntohl(got->ip_info.ip.addr) : 0;

        lock_mu();
        s_retry = 0;
        s_status.state = WIFI_MGR_STATE_CONNECTED;
        s_status.ip4 = ip4;
        unlock_mu();

        status_log_ip(ip4);

        const wifi_mgr_on_got_ip_cb_t cb = s_on_got_ip_cb;
        void *cb_ctx = s_on_got_ip_ctx;
        if (cb) cb(ip4, cb_ctx);
        return;
    }

    if (base == IP_EVENT && id == IP_EVENT_STA_LOST_IP) {
        lock_mu();
        s_status.ip4 = 0;
        unlock_mu();
        return;
    }
}

void wifi_mgr_set_on_got_ip_cb(wifi_mgr_on_got_ip_cb_t cb, void *ctx)
{
    lock_mu();
    s_on_got_ip_cb = cb;
    s_on_got_ip_ctx = ctx;
    unlock_mu();
}

esp_err_t wifi_mgr_start(void)
{
    if (!s_mu) s_mu = xSemaphoreCreateMutex();
    if (!s_mu) return ESP_ERR_NO_MEM;

    lock_mu();
    if (s_wifi_inited) {
        unlock_mu();
        return ESP_OK;
    }
    unlock_mu();

    ESP_ERROR_CHECK(ensure_nvs());
    ensure_netif_and_loop();

    if (!s_sta_netif) {
        s_sta_netif = esp_netif_create_default_wifi_sta();
    }

    if (!s_wifi_inited) {
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        s_wifi_inited = true;

        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, s_sta_netif));
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, s_sta_netif));
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_LOST_IP, &wifi_event_handler, s_sta_netif));
    }

    (void)load_saved_credentials();

    lock_mu();
    s_autoconnect = CONFIG_WIFI_MGR_AUTOCONNECT_ON_BOOT && s_has_runtime;
    unlock_mu();

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    status_set_ip_from_netif(s_sta_netif);

    if (!s_started) {
        ESP_ERROR_CHECK(esp_wifi_start());
    }

    lock_mu();
    ESP_LOGI(TAG, "Wi-Fi ready: %s (ssid=%s)", s_autoconnect ? "autoconnect enabled" : "autoconnect disabled", s_runtime_ssid);
    unlock_mu();

    return ESP_OK;
}

esp_err_t wifi_mgr_get_status(wifi_mgr_status_t *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    lock_mu();
    *out = s_status;
    unlock_mu();
    return ESP_OK;
}

esp_err_t wifi_mgr_set_credentials(const char *ssid, const char *password, bool save_to_nvs)
{
    if (!ssid || ssid[0] == '\0') return ESP_ERR_INVALID_ARG;
    if (!password) password = "";

    lock_mu();
    strlcpy(s_runtime_ssid, ssid, sizeof(s_runtime_ssid));
    strlcpy(s_runtime_pass, password, sizeof(s_runtime_pass));
    s_has_runtime = true;
    s_has_saved = s_has_saved && save_to_nvs;
    strlcpy(s_status.ssid, s_runtime_ssid, sizeof(s_status.ssid));
    s_status.has_runtime_creds = true;
    s_status.has_saved_creds = s_has_saved;
    unlock_mu();

    esp_err_t err = ESP_OK;
    if (save_to_nvs) {
        err = save_credentials_to_nvs(ssid, password);
        if (err == ESP_OK) {
            lock_mu();
            s_has_saved = true;
            s_status.has_saved_creds = true;
            unlock_mu();
        }
    }

    if (s_wifi_inited) {
        (void)apply_runtime_config();
    }
    return err;
}

esp_err_t wifi_mgr_clear_credentials(void)
{
    lock_mu();
    s_runtime_ssid[0] = '\0';
    s_runtime_pass[0] = '\0';
    s_has_runtime = false;
    s_has_saved = false;
    s_autoconnect = false;
    s_retry = 0;
    s_status.ssid[0] = '\0';
    s_status.has_runtime_creds = false;
    s_status.has_saved_creds = false;
    unlock_mu();

    if (s_wifi_inited) {
        (void)esp_wifi_disconnect();
        wifi_config_t sta_cfg = {0};
        (void)esp_wifi_set_config(WIFI_IF_STA, &sta_cfg);
    }
    return clear_credentials_in_nvs();
}

esp_err_t wifi_mgr_connect(void)
{
    if (!s_wifi_inited) return ESP_ERR_INVALID_STATE;
    lock_mu();
    const bool have_creds = creds_present_unsafe();
    if (have_creds) s_autoconnect = true;
    s_retry = 0;
    unlock_mu();

    if (!have_creds) return ESP_ERR_INVALID_ARG;
    esp_err_t err = apply_runtime_config();
    if (err != ESP_OK) return err;
    err = esp_wifi_connect();
    if (err == ESP_OK) {
        lock_mu();
        s_status.state = WIFI_MGR_STATE_CONNECTING;
        unlock_mu();
    }
    return err;
}

esp_err_t wifi_mgr_disconnect(void)
{
    if (!s_wifi_inited) return ESP_ERR_INVALID_STATE;
    lock_mu();
    s_autoconnect = false;
    unlock_mu();
    return esp_wifi_disconnect();
}

esp_err_t wifi_mgr_scan(wifi_mgr_scan_entry_t *out, size_t max_out, size_t *out_n)
{
    if (out_n) *out_n = 0;
    if (!out || max_out == 0 || !out_n) return ESP_ERR_INVALID_ARG;
    if (!s_wifi_inited) return ESP_ERR_INVALID_STATE;

    wifi_scan_config_t scan_cfg = {
        .ssid = 0,
        .bssid = 0,
        .channel = 0,
        .show_hidden = true,
    };
    esp_err_t err = esp_wifi_scan_start(&scan_cfg, true /* block */);
    if (err != ESP_OK) return err;

    uint16_t ap_num = 0;
    err = esp_wifi_scan_get_ap_num(&ap_num);
    if (err != ESP_OK) return err;

    const size_t want = (ap_num < max_out) ? ap_num : max_out;
    if (want == 0) {
        *out_n = 0;
        return ESP_OK;
    }

    wifi_ap_record_t *recs = (wifi_ap_record_t *)calloc(want, sizeof(*recs));
    if (!recs) return ESP_ERR_NO_MEM;

    uint16_t n = (uint16_t)want;
    err = esp_wifi_scan_get_ap_records(&n, recs);
    if (err != ESP_OK) {
        free(recs);
        return err;
    }

    size_t out_i = 0;
    for (uint16_t i = 0; i < n && out_i < max_out; i++) {
        wifi_mgr_scan_entry_t e = {0};
        strlcpy(e.ssid, (const char *)recs[i].ssid, sizeof(e.ssid));
        e.rssi = recs[i].rssi;
        e.channel = recs[i].primary;
        e.authmode = (uint8_t)recs[i].authmode;
        out[out_i++] = e;
    }
    free(recs);

    *out_n = out_i;
    (void)esp_wifi_clear_ap_list();
    return ESP_OK;
}
