#include "gw_registry.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

#include "esp_log.h"
#include "esp_timer.h"

// Fixed-size, in-memory registry intended for bring-up and debugging.
// Not persisted; future work can back this with NVS and richer ZCL interview state.
#define GW_REGISTRY_MAX_DEVICES 32

static const char *TAG = "gw_reg_0031";

static gw_registry_device_t s_devs[GW_REGISTRY_MAX_DEVICES] = {0};
static portMUX_TYPE s_mu = portMUX_INITIALIZER_UNLOCKED;

static int find_by_short(uint16_t short_addr) {
    for (int i = 0; i < GW_REGISTRY_MAX_DEVICES; i++) {
        if (s_devs[i].in_use && s_devs[i].short_addr == short_addr) return i;
    }
    return -1;
}

static int find_by_ieee(const uint8_t ieee_addr[8]) {
    for (int i = 0; i < GW_REGISTRY_MAX_DEVICES; i++) {
        if (!s_devs[i].in_use) continue;
        if (memcmp(s_devs[i].ieee_addr, ieee_addr, 8) == 0) return i;
    }
    return -1;
}

static int alloc_slot(void) {
    for (int i = 0; i < GW_REGISTRY_MAX_DEVICES; i++) {
        if (!s_devs[i].in_use) return i;
    }
    return -1;
}

void gw_registry_on_device_announce(const gw_evt_zb_device_annce_t *ev) {
    if (!ev) return;

    const int64_t now_us = esp_timer_get_time();
    bool short_changed = false;
    uint16_t prev_short = 0;
    uint16_t new_short = ev->short_addr;
    uint8_t ieee_copy[8];
    memcpy(ieee_copy, ev->ieee_addr, sizeof(ieee_copy));

    portENTER_CRITICAL(&s_mu);
    int idx = find_by_short(ev->short_addr);
    if (idx < 0) idx = find_by_ieee(ev->ieee_addr);
    if (idx < 0) idx = alloc_slot();
    if (idx >= 0) {
        prev_short = s_devs[idx].short_addr;
        const bool had_prev = s_devs[idx].in_use;
        s_devs[idx].in_use = true;
        s_devs[idx].short_addr = ev->short_addr;
        memcpy(s_devs[idx].ieee_addr, ev->ieee_addr, 8);
        s_devs[idx].capability = ev->capability;
        s_devs[idx].seen_count++;
        s_devs[idx].last_seen_us = now_us;
        short_changed = had_prev && (prev_short != ev->short_addr);
    }
    portEXIT_CRITICAL(&s_mu);

    if (short_changed) {
        ESP_LOGI(TAG,
                 "device short changed: ieee=%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x 0x%04x -> 0x%04x",
                 ieee_copy[7],
                 ieee_copy[6],
                 ieee_copy[5],
                 ieee_copy[4],
                 ieee_copy[3],
                 ieee_copy[2],
                 ieee_copy[1],
                 ieee_copy[0],
                 (unsigned)prev_short,
                 (unsigned)new_short);
    }
}

size_t gw_registry_snapshot(gw_registry_device_t *out, size_t max_out) {
    if (!out || max_out == 0) return 0;

    size_t n = 0;
    portENTER_CRITICAL(&s_mu);
    for (int i = 0; i < GW_REGISTRY_MAX_DEVICES && n < max_out; i++) {
        if (!s_devs[i].in_use) continue;
        out[n++] = s_devs[i];
    }
    portEXIT_CRITICAL(&s_mu);
    return n;
}
