/*
 * nanopb/protobuf capture + encoding utilities for tutorial 0029.
 *
 * This is intentionally embedded-only and WS-agnostic:
 * - keeps internal esp_event payloads as fixed-size C structs (hub_types.h)
 * - encodes a protobuf envelope at the boundary (validated via console hex dump)
 */

#include "hub_pb.h"

#include <inttypes.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_timer.h"

#include "pb_encode.h"

#include "hub_events.pb.h"
#include "hub_types.h"

static const char *TAG = "hub_pb_0029";

static SemaphoreHandle_t s_mu = NULL;
static bool s_capture_enabled = false;
static bool s_last_valid = false;
static hub_v1_HubEvent s_last = hub_v1_HubEvent_init_zero;

static void lock(void) {
    if (s_mu) xSemaphoreTake(s_mu, portMAX_DELAY);
}

static void unlock(void) {
    if (s_mu) xSemaphoreGive(s_mu);
}

static hub_v1_DeviceType to_pb_device_type(hub_device_type_t t) {
    return (hub_v1_DeviceType)t;
}

static hub_v1_EventId to_pb_event_id(int32_t id) {
    return (hub_v1_EventId)id;
}

void hub_pb_fill_device(hub_v1_Device *out, const hub_device_t *d) {
    if (!out || !d) return;
    memset(out, 0, sizeof(*out));
    out->id = d->id;
    out->type = to_pb_device_type(d->type);
    out->caps = d->caps;
    strlcpy(out->name, d->name, sizeof(out->name));
    out->on = d->on;
    out->level = (uint32_t)d->level;
    out->power_w = d->power_w;
    out->temperature_c = d->temperature_c;
}

bool hub_pb_build_event(int32_t id, const void *data, hub_v1_HubEvent *out) {
    if (!out) return false;

    hub_v1_HubEvent ev = hub_v1_HubEvent_init_zero;
    ev.schema_version = 1;
    ev.id = to_pb_event_id(id);
    ev.ts_us = esp_timer_get_time();

    if (id == HUB_EVT_DEVICE_ADDED || id == HUB_EVT_DEVICE_INTERVIEWED) {
        const hub_device_t *d = (const hub_device_t *)data;
        if (!d) return false;
        ev.which_payload = hub_v1_HubEvent_device_tag;
        hub_pb_fill_device(&ev.payload.device, d);
    } else if (id == HUB_EVT_DEVICE_STATE) {
        const hub_evt_device_state_t *st = (const hub_evt_device_state_t *)data;
        if (!st) return false;
        ev.ts_us = st->ts_us;
        ev.which_payload = hub_v1_HubEvent_device_state_tag;
        ev.payload.device_state.ts_us = st->ts_us;
        ev.payload.device_state.device_id = st->device_id;
        ev.payload.device_state.on = st->on;
        ev.payload.device_state.level = (uint32_t)st->level;
    } else if (id == HUB_EVT_DEVICE_REPORT) {
        const hub_evt_device_report_t *rep = (const hub_evt_device_report_t *)data;
        if (!rep) return false;
        ev.ts_us = rep->ts_us;
        ev.which_payload = hub_v1_HubEvent_device_report_tag;
        ev.payload.device_report.ts_us = rep->ts_us;
        ev.payload.device_report.device_id = rep->device_id;
        ev.payload.device_report.has_power = rep->has_power;
        ev.payload.device_report.power_w = rep->power_w;
        ev.payload.device_report.has_temperature = rep->has_temperature;
        ev.payload.device_report.temperature_c = rep->temperature_c;
    } else if (id == HUB_CMD_DEVICE_ADD) {
        const hub_cmd_device_add_t *cmd = (const hub_cmd_device_add_t *)data;
        if (!cmd) return false;
        ev.which_payload = hub_v1_HubEvent_cmd_device_add_tag;
        ev.payload.cmd_device_add.req_id = (uint64_t)cmd->hdr.req_id;
        ev.payload.cmd_device_add.type = to_pb_device_type(cmd->type);
        ev.payload.cmd_device_add.caps = cmd->caps;
        strlcpy(ev.payload.cmd_device_add.name, cmd->name, sizeof(ev.payload.cmd_device_add.name));
    } else if (id == HUB_CMD_DEVICE_SET) {
        const hub_cmd_device_set_t *cmd = (const hub_cmd_device_set_t *)data;
        if (!cmd) return false;
        ev.which_payload = hub_v1_HubEvent_cmd_device_set_tag;
        ev.payload.cmd_device_set.req_id = (uint64_t)cmd->hdr.req_id;
        ev.payload.cmd_device_set.device_id = cmd->device_id;
        ev.payload.cmd_device_set.has_on = cmd->has_on;
        ev.payload.cmd_device_set.on = cmd->on;
        ev.payload.cmd_device_set.has_level = cmd->has_level;
        ev.payload.cmd_device_set.level = (uint32_t)cmd->level;
    } else if (id == HUB_CMD_DEVICE_INTERVIEW) {
        const hub_cmd_device_interview_t *cmd = (const hub_cmd_device_interview_t *)data;
        if (!cmd) return false;
        ev.which_payload = hub_v1_HubEvent_cmd_device_interview_tag;
        ev.payload.cmd_device_interview.req_id = (uint64_t)cmd->hdr.req_id;
        ev.payload.cmd_device_interview.device_id = cmd->device_id;
    } else if (id == HUB_CMD_SCENE_TRIGGER) {
        const hub_cmd_scene_trigger_t *cmd = (const hub_cmd_scene_trigger_t *)data;
        if (!cmd) return false;
        ev.which_payload = hub_v1_HubEvent_cmd_scene_trigger_tag;
        ev.payload.cmd_scene_trigger.req_id = (uint64_t)cmd->hdr.req_id;
        ev.payload.cmd_scene_trigger.scene_id = cmd->scene_id;
    } else {
        return false;
    }

    *out = ev;
    return true;
}

static void on_hub_event_capture(void *arg, esp_event_base_t base, int32_t id, void *data) {
    (void)arg;
    (void)base;

    if (!s_capture_enabled) return;

    hub_v1_HubEvent ev = hub_v1_HubEvent_init_zero;
    if (!hub_pb_build_event(id, data, &ev)) return;

    lock();
    s_last = ev;
    s_last_valid = true;
    unlock();
}

esp_err_t hub_pb_register(esp_event_loop_handle_t loop) {
    if (!loop) return ESP_ERR_INVALID_ARG;
    if (!s_mu) {
        s_mu = xSemaphoreCreateMutex();
        if (!s_mu) return ESP_ERR_NO_MEM;
    }
    esp_err_t err = esp_event_handler_register_with(loop, HUB_EVT, ESP_EVENT_ANY_ID, &on_hub_event_capture, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_event_handler_register_with failed: %s", esp_err_to_name(err));
    }
    return err;
}

void hub_pb_set_capture(bool enabled) {
    lock();
    s_capture_enabled = enabled;
    unlock();
}

void hub_pb_get_status(bool *out_capture_enabled, bool *out_have_last) {
    if (out_capture_enabled) *out_capture_enabled = false;
    if (out_have_last) *out_have_last = false;

    lock();
    if (out_capture_enabled) *out_capture_enabled = s_capture_enabled;
    if (out_have_last) *out_have_last = s_last_valid;
    unlock();
}

esp_err_t hub_pb_encode_last(uint8_t *buf, size_t cap, size_t *out_len) {
    if (!buf || cap == 0 || !out_len) return ESP_ERR_INVALID_ARG;
    *out_len = 0;

    if (!s_mu) return ESP_ERR_INVALID_STATE;

    hub_v1_HubEvent ev = hub_v1_HubEvent_init_zero;
    lock();
    if (!s_last_valid) {
        unlock();
        return ESP_ERR_INVALID_STATE;
    }
    ev = s_last;
    unlock();

    size_t need = 0;
    if (!pb_get_encoded_size(&need, hub_v1_HubEvent_fields, &ev)) {
        return ESP_FAIL;
    }
    if (need > cap) {
        return ESP_ERR_NO_MEM;
    }

    pb_ostream_t s = pb_ostream_from_buffer(buf, cap);
    if (!pb_encode(&s, hub_v1_HubEvent_fields, &ev)) {
        ESP_LOGW(TAG, "encode failed: %s", PB_GET_ERROR(&s));
        return ESP_FAIL;
    }

    *out_len = (size_t)s.bytes_written;
    return ESP_OK;
}
