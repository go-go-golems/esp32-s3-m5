#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "esp_err.h"
#include "esp_event.h"

// Application event base.
ESP_EVENT_DECLARE_BASE(HUB_EVT);

typedef enum {
    // Commands (sources: HTTP, scenes, tests)
    HUB_CMD_DEVICE_ADD = 1,
    HUB_CMD_DEVICE_REMOVE,
    HUB_CMD_DEVICE_INTERVIEW,
    HUB_CMD_DEVICE_SET,
    HUB_CMD_SCENE_TRIGGER,

    // Notifications (sources: registry/simulator)
    HUB_EVT_DEVICE_ADDED,
    HUB_EVT_DEVICE_REMOVED,
    HUB_EVT_DEVICE_INTERVIEWED,
    HUB_EVT_DEVICE_STATE,
    HUB_EVT_DEVICE_REPORT,
} hub_event_id_t;

typedef enum {
    HUB_DEVICE_PLUG = 1,
    HUB_DEVICE_BULB = 2,
    HUB_DEVICE_TEMP_SENSOR = 3,
} hub_device_type_t;

enum {
    HUB_CAP_ONOFF = 1u << 0,
    HUB_CAP_LEVEL = 1u << 1,
    HUB_CAP_POWER = 1u << 2,
    HUB_CAP_TEMPERATURE = 1u << 3,
};

typedef struct {
    uint32_t id;
    hub_device_type_t type;
    uint32_t caps;
    char name[32];

    bool on;
    uint8_t level; // 0..100
    float power_w;
    float temperature_c;
} hub_device_t;

typedef struct {
    uint32_t req_id;
    QueueHandle_t reply_q; // optional; used for HTTP request/response.
} hub_cmd_hdr_t;

typedef struct {
    esp_err_t status;
    uint32_t device_id;
    hub_device_t device; // meaningful when status==ESP_OK
} hub_reply_device_t;

typedef struct {
    esp_err_t status;
} hub_reply_status_t;

typedef struct {
    hub_cmd_hdr_t hdr;
    hub_device_type_t type;
    uint32_t caps;
    char name[32];
} hub_cmd_device_add_t;

typedef struct {
    hub_cmd_hdr_t hdr;
    uint32_t device_id;
    bool has_on;
    bool on;
    bool has_level;
    uint8_t level;
} hub_cmd_device_set_t;

typedef struct {
    hub_cmd_hdr_t hdr;
    uint32_t device_id;
} hub_cmd_device_interview_t;

typedef struct {
    hub_cmd_hdr_t hdr;
    uint32_t scene_id;
} hub_cmd_scene_trigger_t;

typedef struct {
    int64_t ts_us;
    uint32_t device_id;
    bool on;
    uint8_t level;
} hub_evt_device_state_t;

typedef struct {
    int64_t ts_us;
    uint32_t device_id;
    bool has_power;
    float power_w;
    bool has_temperature;
    float temperature_c;
} hub_evt_device_report_t;

