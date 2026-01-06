#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

esp_err_t hub_wifi_start(void);

typedef enum {
    WIFI_STA_STATE_UNINIT = 0,
    WIFI_STA_STATE_IDLE,
    WIFI_STA_STATE_CONNECTING,
    WIFI_STA_STATE_CONNECTED,
} wifi_sta_state_t;

typedef struct {
    wifi_sta_state_t state;
    char ssid[33];
    bool has_saved_creds;
    bool has_runtime_creds;
    uint32_t ip4; // host order, 0 if none
    int last_disconnect_reason; // wifi_err_reason_t or -1
} wifi_sta_status_t;

esp_err_t hub_wifi_get_status(wifi_sta_status_t *out);
esp_err_t hub_wifi_set_credentials(const char *ssid, const char *password, bool save_to_nvs);
esp_err_t hub_wifi_clear_credentials(void);
esp_err_t hub_wifi_connect(void);
esp_err_t hub_wifi_disconnect(void);

typedef struct {
    char ssid[33];
    int rssi;
    uint8_t channel;
    uint8_t authmode; // wifi_auth_mode_t
} wifi_sta_scan_entry_t;

esp_err_t hub_wifi_scan(wifi_sta_scan_entry_t *out, size_t max_out, size_t *out_n);
