#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "gw_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool in_use;
    uint16_t short_addr;
    uint8_t ieee_addr[8];
    uint8_t capability;
    uint32_t seen_count;
    int64_t last_seen_us;
} gw_registry_device_t;

void gw_registry_on_device_announce(const gw_evt_zb_device_annce_t *ev);
size_t gw_registry_snapshot(gw_registry_device_t *out, size_t max_out);

#ifdef __cplusplus
}
#endif
