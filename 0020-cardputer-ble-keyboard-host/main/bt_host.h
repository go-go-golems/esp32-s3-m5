#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t bda[6];
    uint8_t addr_type;
    int rssi;
    char name[32];
    uint32_t last_seen_ms;
} bt_le_device_t;

typedef struct {
    uint8_t bda[6];
    int rssi;
    uint32_t cod;
    char name[64];
    uint32_t last_seen_ms;
} bt_bredr_device_t;

void bt_host_init(void);

// Scan / discovery
void bt_host_scan_le_start(uint32_t seconds);
void bt_host_scan_le_stop(void);
void bt_host_devices_le_print(const char *filter_substr);
void bt_host_devices_le_clear(void);
void bt_host_devices_le_events_set_enabled(bool enabled);
bool bt_host_devices_le_events_get_enabled(void);
bool bt_host_le_device_get_by_index(int index, bt_le_device_t *out);
bool bt_host_le_device_get_by_bda(const uint8_t bda[6], bt_le_device_t *out);

void bt_host_scan_bredr_start(uint32_t seconds);
void bt_host_scan_bredr_stop(void);
void bt_host_devices_bredr_print(const char *filter_substr);
void bt_host_devices_bredr_clear(void);
void bt_host_devices_bredr_events_set_enabled(bool enabled);
bool bt_host_devices_bredr_events_get_enabled(void);
bool bt_host_bredr_device_get_by_index(int index, bt_bredr_device_t *out);
bool bt_host_bredr_device_get_by_bda(const uint8_t bda[6], bt_bredr_device_t *out);

// Connection / security
bool bt_host_le_connect(const uint8_t bda[6], uint8_t addr_type);
bool bt_host_bredr_connect(const uint8_t bda[6]);
void bt_host_disconnect(void);
// Initiate encryption/pairing for the given peer; if not connected yet, this
// will connect first and trigger encryption once the link is up.
bool bt_host_le_pair(const uint8_t bda[6], uint8_t addr_type);
bool bt_host_bredr_pair(const uint8_t bda[6]);
void bt_host_le_bonds_print(void);
void bt_host_bredr_bonds_print(void);
bool bt_host_le_unpair(const uint8_t bda[6]);
bool bt_host_bredr_unpair(const uint8_t bda[6]);

// Security prompt replies (optional; used when pairing requires interaction)
bool bt_host_le_sec_accept(const uint8_t bda[6], bool accept);
bool bt_host_le_passkey_reply(const uint8_t bda[6], bool accept, uint32_t passkey);
bool bt_host_le_confirm_reply(const uint8_t bda[6], bool accept);

bool bt_host_bredr_pin_reply(const uint8_t bda[6], bool accept, const char *pin);
bool bt_host_bredr_ssp_passkey_reply(const uint8_t bda[6], bool accept, uint32_t passkey);
bool bt_host_bredr_ssp_confirm_reply(const uint8_t bda[6], bool accept);

#ifdef __cplusplus
}
#endif
