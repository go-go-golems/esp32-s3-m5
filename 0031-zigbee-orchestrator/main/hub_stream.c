/*
 * Protobuf (nanopb) event stream bridge for tutorial 0029.
 *
 * Goal: publish hub bus traffic over WebSocket as protobuf binary frames without
 * doing heavy work (encode/malloc/send) inside the hub event loop task.
 */

#include "hub_stream.h"

#include <string.h>

#include "sdkconfig.h"

#if CONFIG_TUTORIAL_0029_ENABLE_WS_PB

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "pb_encode.h"

#include "hub_http.h"
#include "hub_pb.h"
#include "hub_types.h"

static const char *TAG = "hub_stream_0029";

typedef union {
    hub_device_t device;
    hub_cmd_device_add_t cmd_add;
    hub_cmd_device_set_t cmd_set;
    hub_cmd_device_interview_t cmd_interview;
    hub_cmd_scene_trigger_t cmd_scene;
    hub_evt_device_state_t st;
    hub_evt_device_report_t rep;
} hub_stream_payload_u;

typedef struct {
    int32_t id;
    hub_stream_payload_u payload;
} hub_stream_item_t;

static QueueHandle_t s_q = NULL;
static TaskHandle_t s_task = NULL;

static uint32_t s_drops = 0;
static uint32_t s_enc_fail = 0;
static uint32_t s_send_fail = 0;

static size_t payload_size_for_id(int32_t id) {
    switch (id) {
        case HUB_CMD_DEVICE_ADD:
            return sizeof(hub_cmd_device_add_t);
        case HUB_CMD_DEVICE_SET:
            return sizeof(hub_cmd_device_set_t);
        case HUB_CMD_DEVICE_INTERVIEW:
            return sizeof(hub_cmd_device_interview_t);
        case HUB_CMD_SCENE_TRIGGER:
            return sizeof(hub_cmd_scene_trigger_t);
        case HUB_EVT_DEVICE_ADDED:
        case HUB_EVT_DEVICE_INTERVIEWED:
            return sizeof(hub_device_t);
        case HUB_EVT_DEVICE_STATE:
            return sizeof(hub_evt_device_state_t);
        case HUB_EVT_DEVICE_REPORT:
            return sizeof(hub_evt_device_report_t);
        default:
            return 0;
    }
}

static void on_any_event_enqueue(void *arg, esp_event_base_t base, int32_t id, void *data) {
    (void)arg;
    (void)base;

    if (!s_q) return;
    if (hub_http_events_client_count() == 0) return;

    const size_t sz = payload_size_for_id(id);
    if (sz == 0) return;
    if (!data) return;

    hub_stream_item_t it = {.id = id};
    memset(&it.payload, 0, sizeof(it.payload));
    memcpy(&it.payload, data, sz);

    if (xQueueSend(s_q, &it, 0) != pdTRUE) {
        s_drops++;
    }
}

static void stream_task(void *arg) {
    (void)arg;

    hub_stream_item_t it = {0};
    uint8_t buf[512];

    while (true) {
        if (xQueueReceive(s_q, &it, portMAX_DELAY) != pdTRUE) continue;
        if (hub_http_events_client_count() == 0) continue;

        hub_v1_HubEvent ev = hub_v1_HubEvent_init_zero;
        if (!hub_pb_build_event(it.id, &it.payload, &ev)) {
            continue;
        }

        size_t need = 0;
        if (!pb_get_encoded_size(&need, hub_v1_HubEvent_fields, &ev)) {
            s_enc_fail++;
            continue;
        }
        if (need > sizeof(buf)) {
            s_enc_fail++;
            continue;
        }

        pb_ostream_t s = pb_ostream_from_buffer(buf, sizeof(buf));
        if (!pb_encode(&s, hub_v1_HubEvent_fields, &ev)) {
            s_enc_fail++;
            continue;
        }

        esp_err_t err = hub_http_events_broadcast_pb(buf, (size_t)s.bytes_written);
        if (err != ESP_OK) {
            s_send_fail++;
        }
    }
}

esp_err_t hub_stream_start(esp_event_loop_handle_t loop) {
    if (!loop) return ESP_ERR_INVALID_ARG;
    if (s_task) return ESP_OK;

    if (!s_q) {
        s_q = xQueueCreate(64, sizeof(hub_stream_item_t));
        if (!s_q) return ESP_ERR_NO_MEM;
    }

    BaseType_t ok = xTaskCreate(stream_task, "hub_stream", 4096, NULL, 6, &s_task);
    if (ok != pdPASS) {
        s_task = NULL;
        return ESP_FAIL;
    }

    esp_err_t err = esp_event_handler_register_with(loop, HUB_EVT, ESP_EVENT_ANY_ID, &on_any_event_enqueue, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_event_handler_register_with failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "protobuf WS stream bridge started");
    return ESP_OK;
}

void hub_stream_get_stats(uint32_t *out_enqueue_drops, uint32_t *out_encode_failures, uint32_t *out_send_failures) {
    if (out_enqueue_drops) *out_enqueue_drops = s_drops;
    if (out_encode_failures) *out_encode_failures = s_enc_fail;
    if (out_send_failures) *out_send_failures = s_send_fail;
}

#else

esp_err_t hub_stream_start(esp_event_loop_handle_t loop) {
    if (!loop) return ESP_ERR_INVALID_ARG;
    (void)loop;
    return ESP_OK;
}

void hub_stream_get_stats(uint32_t *out_enqueue_drops, uint32_t *out_encode_failures, uint32_t *out_send_failures) {
    if (out_enqueue_drops) *out_enqueue_drops = 0;
    if (out_encode_failures) *out_encode_failures = 0;
    if (out_send_failures) *out_send_failures = 0;
}

#endif
