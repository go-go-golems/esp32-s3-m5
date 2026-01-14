#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_wifi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CAM_WIFI_STATE_UNINIT = 0,
    CAM_WIFI_STATE_IDLE,
    CAM_WIFI_STATE_CONNECTING,
    CAM_WIFI_STATE_CONNECTED,
} cam_wifi_state_t;

typedef struct {
    cam_wifi_state_t state;
    char ssid[33];
    bool has_saved_creds;
    bool has_runtime_creds;
    uint32_t ip4;
    int last_disconnect_reason;
} cam_wifi_status_t;

typedef struct {
    char ssid[33];
    int rssi;
    uint8_t channel;
    wifi_auth_mode_t authmode;
} cam_wifi_scan_entry_t;

esp_err_t cam_wifi_start(void);
esp_err_t cam_wifi_get_status(cam_wifi_status_t *out);
esp_err_t cam_wifi_set_credentials(const char *ssid, const char *password, bool save_to_nvs);
esp_err_t cam_wifi_clear_credentials(void);
esp_err_t cam_wifi_connect(void);
esp_err_t cam_wifi_disconnect(void);
esp_err_t cam_wifi_scan(cam_wifi_scan_entry_t *entries, size_t max_entries, size_t *out_count);
bool cam_wifi_is_connected(void);

#ifdef __cplusplus
}
#endif
