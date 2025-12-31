#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_gattc_api.h"

#ifdef __cplusplus
extern "C" {
#endif

void hid_host_init(void);
bool hid_host_open(const uint8_t bda[6], uint8_t addr_type);
void hid_host_close(void);

// Glue: forward GATTC events so the HID host stack can function.
void hid_host_forward_gattc_event(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);

#ifdef __cplusplus
}
#endif
