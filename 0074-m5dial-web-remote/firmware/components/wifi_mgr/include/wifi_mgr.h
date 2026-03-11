#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WIFI_MGR_STATE_UNINIT = 0,
    WIFI_MGR_STATE_IDLE = 1,
    WIFI_MGR_STATE_CONNECTING = 2,
    WIFI_MGR_STATE_CONNECTED = 3,
} wifi_mgr_state_t;

typedef struct {
    wifi_mgr_state_t state;
    char ssid[33];
    bool has_saved_creds;
    bool has_runtime_creds;
    uint32_t ip4;  // host order
    int last_disconnect_reason;
} wifi_mgr_status_t;

typedef struct {
    char ssid[33];
    int8_t rssi;
    uint8_t channel;
    uint8_t authmode;
} wifi_mgr_scan_entry_t;

typedef void (*wifi_mgr_on_got_ip_cb_t)(uint32_t ip4_host_order, void *ctx);

esp_err_t wifi_mgr_start(void);

esp_err_t wifi_mgr_get_status(wifi_mgr_status_t *out);

esp_err_t wifi_mgr_set_credentials(const char *ssid, const char *password, bool save_to_nvs);
esp_err_t wifi_mgr_clear_credentials(void);

esp_err_t wifi_mgr_connect(void);
esp_err_t wifi_mgr_disconnect(void);

esp_err_t wifi_mgr_scan(wifi_mgr_scan_entry_t *out, size_t max_out, size_t *out_n);

void wifi_mgr_set_on_got_ip_cb(wifi_mgr_on_got_ip_cb_t cb, void *ctx);

#ifdef __cplusplus
}  // extern "C"
#endif

