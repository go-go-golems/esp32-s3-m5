#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WIFI_APP_STATE_UNINIT = 0,
    WIFI_APP_STATE_IDLE,
    WIFI_APP_STATE_CONNECTING,
    WIFI_APP_STATE_CONNECTED,
} wifi_app_state_t;

typedef struct {
    wifi_app_state_t state;
    char ssid[33];
    bool has_saved_creds;
    bool has_runtime_creds;
    uint32_t sta_ip4; // host order; 0 if none
    uint32_t ap_ip4;  // host order; 0 if none
    int last_disconnect_reason;
} wifi_app_status_t;

typedef struct {
    char ssid[33];
    int rssi;
    uint8_t channel;
    uint8_t authmode; // wifi_auth_mode_t
} wifi_scan_entry_t;

esp_err_t wifi_app_start(void);
esp_err_t wifi_app_get_status(wifi_app_status_t *out);
esp_err_t wifi_app_set_credentials(const char *ssid, const char *password, bool save_to_nvs);
esp_err_t wifi_app_save_credentials(void);
esp_err_t wifi_app_clear_credentials(void);
esp_err_t wifi_app_connect(void);
esp_err_t wifi_app_disconnect(void);
esp_err_t wifi_app_scan(wifi_scan_entry_t *out, size_t max_out, size_t *out_n);

#ifdef __cplusplus
}
#endif
