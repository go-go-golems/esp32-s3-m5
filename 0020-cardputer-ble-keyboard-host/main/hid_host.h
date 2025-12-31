#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void hid_host_init(void);
bool hid_host_open(const uint8_t bda[6], uint8_t addr_type);
void hid_host_close(void);

// Glue: forward GATTC events so the HID host stack can function.
void hid_host_forward_gattc_event(int event, int gattc_if, void *param);

#ifdef __cplusplus
}
#endif
