/*
 * Application event loop + command handlers for tutorial 0029.
 */

#include "hub_bus.h"

#include <inttypes.h>
#include <string.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "hub_http.h"
#include "hub_registry.h"
#include "hub_types.h"

static const char *TAG = "hub_bus_0029";

ESP_EVENT_DEFINE_BASE(HUB_EVT);

static esp_event_loop_handle_t s_loop = NULL;

esp_event_loop_handle_t hub_bus_get_loop(void) {
    return s_loop;
}

static void reply_status(const hub_cmd_hdr_t *hdr, esp_err_t status) {
    if (!hdr || !hdr->reply_q) {
        return;
    }
    hub_reply_status_t r = {.status = status};
    (void)xQueueSend(hdr->reply_q, &r, 0);
}

static void reply_device(const hub_cmd_hdr_t *hdr, esp_err_t status, const hub_device_t *device) {
    if (!hdr || !hdr->reply_q) {
        return;
    }
    hub_reply_device_t r = {.status = status, .device_id = device ? device->id : 0};
    if (device) {
        r.device = *device;
    } else {
        memset(&r.device, 0, sizeof(r.device));
    }
    (void)xQueueSend(hdr->reply_q, &r, 0);
}

static void on_cmd_device_add(void *arg, esp_event_base_t base, int32_t id, void *data) {
    (void)arg;
    (void)base;
    (void)id;
    const hub_cmd_device_add_t *cmd = (const hub_cmd_device_add_t *)data;
    if (!cmd) {
        return;
    }

    hub_device_t d = {0};
    d.type = cmd->type;
    d.caps = cmd->caps;
    strncpy(d.name, cmd->name, sizeof(d.name) - 1);

    // Defaults.
    d.on = false;
    d.level = 0;
    d.power_w = 0.0f;
    d.temperature_c = 22.0f;

    hub_device_t created = {0};
    esp_err_t err = hub_registry_add(&d, &created);
    if (err != ESP_OK) {
        reply_device(&cmd->hdr, err, NULL);
        return;
    }

    // Emit notification.
    (void)esp_event_post_to(s_loop, HUB_EVT, HUB_EVT_DEVICE_ADDED, &created, sizeof(created), 0);
    reply_device(&cmd->hdr, ESP_OK, &created);
}

static void on_cmd_device_set(void *arg, esp_event_base_t base, int32_t id, void *data) {
    (void)arg;
    (void)base;
    (void)id;
    const hub_cmd_device_set_t *cmd = (const hub_cmd_device_set_t *)data;
    if (!cmd) {
        return;
    }

    hub_device_t d = {0};
    esp_err_t err = hub_registry_get(cmd->device_id, &d);
    if (err != ESP_OK) {
        reply_status(&cmd->hdr, err);
        return;
    }

    bool changed = false;
    if (cmd->has_on) {
        if (d.on != cmd->on) {
            d.on = cmd->on;
            changed = true;
        }
        // If turning off a power-measuring device, power immediately drops.
        if (!d.on && (d.caps & HUB_CAP_POWER)) {
            d.power_w = 0.0f;
        }
    }
    if (cmd->has_level && (d.caps & HUB_CAP_LEVEL)) {
        const uint8_t lvl = (cmd->level > 100) ? 100 : cmd->level;
        if (d.level != lvl) {
            d.level = lvl;
            changed = true;
        }
    }

    if (changed) {
        (void)hub_registry_update(d.id, &d);

        hub_evt_device_state_t ev = {
            .ts_us = esp_timer_get_time(),
            .device_id = d.id,
            .on = d.on,
            .level = d.level,
        };
        (void)esp_event_post_to(s_loop, HUB_EVT, HUB_EVT_DEVICE_STATE, &ev, sizeof(ev), 0);
    }

    reply_status(&cmd->hdr, ESP_OK);
}

static void on_cmd_device_interview(void *arg, esp_event_base_t base, int32_t id, void *data) {
    (void)arg;
    (void)base;
    (void)id;
    const hub_cmd_device_interview_t *cmd = (const hub_cmd_device_interview_t *)data;
    if (!cmd) {
        return;
    }

    hub_device_t d = {0};
    esp_err_t err = hub_registry_get(cmd->device_id, &d);
    if (err != ESP_OK) {
        reply_status(&cmd->hdr, err);
        return;
    }

    // Fake an "interview": add typical caps based on type.
    switch (d.type) {
        case HUB_DEVICE_PLUG:
            d.caps |= HUB_CAP_ONOFF | HUB_CAP_POWER;
            break;
        case HUB_DEVICE_BULB:
            d.caps |= HUB_CAP_ONOFF | HUB_CAP_LEVEL;
            break;
        case HUB_DEVICE_TEMP_SENSOR:
            d.caps |= HUB_CAP_TEMPERATURE;
            break;
        default:
            break;
    }
    (void)hub_registry_update(d.id, &d);

    (void)esp_event_post_to(s_loop, HUB_EVT, HUB_EVT_DEVICE_INTERVIEWED, &d, sizeof(d), 0);
    reply_status(&cmd->hdr, ESP_OK);
}

static void scene_all_onoff(bool on) {
    hub_device_t snap[32];
    size_t n = 0;
    if (hub_registry_snapshot(snap, sizeof(snap) / sizeof(snap[0]), &n) != ESP_OK) {
        return;
    }
    for (size_t i = 0; i < n; i++) {
        if ((snap[i].caps & HUB_CAP_ONOFF) == 0) {
            continue;
        }
        hub_cmd_device_set_t cmd = {
            .hdr = {.req_id = 0, .reply_q = NULL},
            .device_id = snap[i].id,
            .has_on = true,
            .on = on,
            .has_level = false,
            .level = 0,
        };
        on_cmd_device_set(NULL, HUB_EVT, HUB_CMD_DEVICE_SET, &cmd);
    }
}

static void on_cmd_scene_trigger(void *arg, esp_event_base_t base, int32_t id, void *data) {
    (void)arg;
    (void)base;
    (void)id;
    const hub_cmd_scene_trigger_t *cmd = (const hub_cmd_scene_trigger_t *)data;
    if (!cmd) {
        return;
    }

    // Two built-in scenes:
    // - id=1: all_on
    // - id=2: all_off
    if (cmd->scene_id == 1) {
        scene_all_onoff(true);
        reply_status(&cmd->hdr, ESP_OK);
        return;
    }
    if (cmd->scene_id == 2) {
        scene_all_onoff(false);
        reply_status(&cmd->hdr, ESP_OK);
        return;
    }
    reply_status(&cmd->hdr, ESP_ERR_NOT_FOUND);
}

#if CONFIG_TUTORIAL_0029_ENABLE_WS_JSON
static const char *event_name(int32_t id) {
    switch (id) {
        case HUB_CMD_DEVICE_ADD: return "HUB_CMD_DEVICE_ADD";
        case HUB_CMD_DEVICE_REMOVE: return "HUB_CMD_DEVICE_REMOVE";
        case HUB_CMD_DEVICE_INTERVIEW: return "HUB_CMD_DEVICE_INTERVIEW";
        case HUB_CMD_DEVICE_SET: return "HUB_CMD_DEVICE_SET";
        case HUB_CMD_SCENE_TRIGGER: return "HUB_CMD_SCENE_TRIGGER";
        case HUB_EVT_DEVICE_ADDED: return "HUB_EVT_DEVICE_ADDED";
        case HUB_EVT_DEVICE_REMOVED: return "HUB_EVT_DEVICE_REMOVED";
        case HUB_EVT_DEVICE_INTERVIEWED: return "HUB_EVT_DEVICE_INTERVIEWED";
        case HUB_EVT_DEVICE_STATE: return "HUB_EVT_DEVICE_STATE";
        case HUB_EVT_DEVICE_REPORT: return "HUB_EVT_DEVICE_REPORT";
        default: return "UNKNOWN";
    }
}

// Best-effort: publish every bus event as one JSON line on the websocket event stream.
static void on_any_event_stream(void *arg, esp_event_base_t base, int32_t id, void *data) {
    (void)arg;
    (void)base;

    char buf[512];
    const int64_t ts_us = esp_timer_get_time();
    const char *name = event_name(id);

    // Minimal per-event payload projection.
    if (id == HUB_EVT_DEVICE_ADDED || id == HUB_EVT_DEVICE_INTERVIEWED) {
        const hub_device_t *d = (const hub_device_t *)data;
        if (!d) return;
        snprintf(buf, sizeof(buf),
                 "{\"ts_us\":%" PRIi64 ",\"name\":\"%s\",\"device\":{\"id\":%" PRIu32 ",\"type\":%d,\"caps\":%" PRIu32 ",\"name\":\"%s\",\"on\":%s,\"level\":%u}}",
                 ts_us, name, d->id, (int)d->type, d->caps, d->name, d->on ? "true" : "false", (unsigned)d->level);
        (void)hub_http_events_broadcast_json(buf);
        return;
    }
    if (id == HUB_EVT_DEVICE_STATE) {
        const hub_evt_device_state_t *ev = (const hub_evt_device_state_t *)data;
        if (!ev) return;
        snprintf(buf, sizeof(buf),
                 "{\"ts_us\":%" PRIi64 ",\"name\":\"%s\",\"device_id\":%" PRIu32 ",\"on\":%s,\"level\":%u}",
                 ts_us, name, ev->device_id, ev->on ? "true" : "false", (unsigned)ev->level);
        (void)hub_http_events_broadcast_json(buf);
        return;
    }
    if (id == HUB_EVT_DEVICE_REPORT) {
        const hub_evt_device_report_t *ev = (const hub_evt_device_report_t *)data;
        if (!ev) return;
        char pbuf[32] = "null";
        char tbuf[32] = "null";
        if (ev->has_power) {
            // Note: floats are printed as double by printf.
            snprintf(pbuf, sizeof(pbuf), "%.2f", (double)ev->power_w);
        }
        if (ev->has_temperature) {
            snprintf(tbuf, sizeof(tbuf), "%.2f", (double)ev->temperature_c);
        }
        snprintf(buf, sizeof(buf),
                 "{\"ts_us\":%" PRIi64 ",\"name\":\"%s\",\"device_id\":%" PRIu32 ",\"power_w\":%s,\"temperature_c\":%s}",
                 ts_us, name, ev->device_id, pbuf, tbuf);
        (void)hub_http_events_broadcast_json(buf);
        return;
    }

    snprintf(buf, sizeof(buf), "{\"ts_us\":%" PRIi64 ",\"name\":\"%s\"}", ts_us, name);
    (void)hub_http_events_broadcast_json(buf);
}
#endif

esp_err_t hub_bus_start(void) {
    if (s_loop) {
        return ESP_OK;
    }

    esp_event_loop_args_t args = {
        .queue_size = CONFIG_TUTORIAL_0029_HUB_EVENT_QUEUE_SIZE,
        .task_name = "hub_evt",
        .task_priority = 10,
        .task_stack_size = 4096,
        .task_core_id = 0,
    };

    ESP_LOGI(TAG, "creating hub event loop (queue=%d)", (int)args.queue_size);
    esp_err_t err = esp_event_loop_create(&args, &s_loop);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_event_loop_create failed: %s", esp_err_to_name(err));
        s_loop = NULL;
        return err;
    }

    ESP_ERROR_CHECK(esp_event_handler_register_with(s_loop, HUB_EVT, HUB_CMD_DEVICE_ADD, &on_cmd_device_add, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register_with(s_loop, HUB_EVT, HUB_CMD_DEVICE_SET, &on_cmd_device_set, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register_with(s_loop, HUB_EVT, HUB_CMD_DEVICE_INTERVIEW, &on_cmd_device_interview, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register_with(s_loop, HUB_EVT, HUB_CMD_SCENE_TRIGGER, &on_cmd_scene_trigger, NULL));

#if CONFIG_TUTORIAL_0029_ENABLE_WS_JSON
    // Always-on event stream publisher.
    ESP_ERROR_CHECK(esp_event_handler_register_with(s_loop, HUB_EVT, ESP_EVENT_ANY_ID, &on_any_event_stream, NULL));
#endif

    return ESP_OK;
}

void hub_bus_stop(void) {
    if (!s_loop) {
        return;
    }
    (void)esp_event_loop_delete(s_loop);
    s_loop = NULL;
}
