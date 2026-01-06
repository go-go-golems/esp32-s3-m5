#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_event.h"

// Application event base.
ESP_EVENT_DECLARE_BASE(GW_EVT);

typedef enum {
    // Commands (from console/HTTP)
    GW_CMD_PERMIT_JOIN = 1,
    GW_CMD_ONOFF = 2,

    // Notifications
    GW_EVT_CMD_RESULT = 100,

    // Zigbee notifications (from host+NCP stack)
    GW_EVT_ZB_PERMIT_JOIN_STATUS = 200,
    GW_EVT_ZB_DEVICE_ANNCE = 201,
} gw_event_id_t;

typedef struct {
    uint64_t req_id;
    uint16_t seconds;
} gw_cmd_permit_join_t;

typedef struct {
    uint64_t req_id;
    uint16_t short_addr;
    uint8_t dst_ep;
    uint8_t cmd_id; // 0=off, 1=on, 2=toggle
} gw_cmd_onoff_t;

typedef struct {
    uint64_t req_id;
    int32_t status; // esp_err_t or app-defined
} gw_evt_cmd_result_t;

typedef struct {
    uint8_t seconds;
} gw_evt_zb_permit_join_status_t;

typedef struct {
    uint16_t short_addr;
    uint8_t ieee_addr[8];
    uint8_t capability;
} gw_evt_zb_device_annce_t;
