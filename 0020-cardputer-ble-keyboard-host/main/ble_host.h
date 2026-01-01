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
} ble_host_device_t;

void ble_host_init(void);

// Scan / discovery
void ble_host_scan_start(uint32_t seconds);
void ble_host_scan_stop(void);
void ble_host_devices_print(const char *filter_substr);
void ble_host_devices_clear(void);
void ble_host_devices_events_set_enabled(bool enabled);
bool ble_host_devices_events_get_enabled(void);
bool ble_host_device_get_by_index(int index, ble_host_device_t *out);
bool ble_host_device_get_by_bda(const uint8_t bda[6], ble_host_device_t *out);

// Connection / security
bool ble_host_connect(const uint8_t bda[6], uint8_t addr_type);
void ble_host_disconnect(void);
// Initiate encryption/pairing for the given peer; if not connected yet, this
// will connect first and trigger encryption once the link is up.
bool ble_host_pair(const uint8_t bda[6], uint8_t addr_type);
void ble_host_bonds_print(void);
bool ble_host_unpair(const uint8_t bda[6]);

// Security prompt replies (optional; used when pairing requires interaction)
bool ble_host_sec_accept(const uint8_t bda[6], bool accept);
bool ble_host_passkey_reply(const uint8_t bda[6], bool accept, uint32_t passkey);
bool ble_host_confirm_reply(const uint8_t bda[6], bool accept);

#ifdef __cplusplus
}
#endif
