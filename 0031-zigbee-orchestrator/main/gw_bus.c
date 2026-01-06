#include "gw_bus.h"

#include <inttypes.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "gw_zb.h"
#include "gw_registry.h"

static const char *TAG = "gw_bus_0031";

ESP_EVENT_DEFINE_BASE(GW_EVT);

static esp_event_loop_handle_t s_loop = NULL;

static portMUX_TYPE s_mon_mu = portMUX_INITIALIZER_UNLOCKED;
static bool s_monitor_enabled = false;
static uint64_t s_monitor_dropped = 0;
static TickType_t s_mon_window_start = 0;
static uint32_t s_mon_window_lines = 0;

static void on_cmd_permit_join(void *arg, esp_event_base_t base, int32_t id, void *data) {
    (void)arg;
    (void)base;
    (void)id;
    const gw_cmd_permit_join_t *cmd = (const gw_cmd_permit_join_t *)data;
    if (!cmd) return;

    if (!gw_zb_started()) {
        ESP_LOGW(TAG, "permit_join rejected: zigbee not started req_id=%" PRIu64, cmd->req_id);
        const gw_evt_cmd_result_t out = {.req_id = cmd->req_id, .status = ESP_ERR_INVALID_STATE};
        (void)esp_event_post_to(s_loop, GW_EVT, GW_EVT_CMD_RESULT, &out, sizeof(out), 0);
        return;
    }

    const esp_err_t err = gw_zb_request_permit_join(cmd->seconds, cmd->req_id, 0);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "permit_join enqueue failed: %s req_id=%" PRIu64, esp_err_to_name(err), cmd->req_id);
        const gw_evt_cmd_result_t out = {.req_id = cmd->req_id, .status = err};
        (void)esp_event_post_to(s_loop, GW_EVT, GW_EVT_CMD_RESULT, &out, sizeof(out), 0);
        return;
    }
}

static void on_cmd_onoff(void *arg, esp_event_base_t base, int32_t id, void *data) {
    (void)arg;
    (void)base;
    (void)id;
    const gw_cmd_onoff_t *cmd = (const gw_cmd_onoff_t *)data;
    if (!cmd) return;

    if (!gw_zb_started()) {
        ESP_LOGW(TAG, "onoff rejected: zigbee not started req_id=%" PRIu64, cmd->req_id);
        const gw_evt_cmd_result_t out = {.req_id = cmd->req_id, .status = ESP_ERR_INVALID_STATE};
        (void)esp_event_post_to(s_loop, GW_EVT, GW_EVT_CMD_RESULT, &out, sizeof(out), 0);
        return;
    }

    const esp_err_t err = gw_zb_request_onoff(cmd->short_addr, cmd->dst_ep, cmd->cmd_id, cmd->req_id, 0);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "onoff enqueue failed: %s req_id=%" PRIu64, esp_err_to_name(err), cmd->req_id);
        const gw_evt_cmd_result_t out = {.req_id = cmd->req_id, .status = err};
        (void)esp_event_post_to(s_loop, GW_EVT, GW_EVT_CMD_RESULT, &out, sizeof(out), 0);
        return;
    }
}

static void on_evt_zb_device_annce(void *arg, esp_event_base_t base, int32_t id, void *data) {
    (void)arg;
    (void)base;
    (void)id;
    const gw_evt_zb_device_annce_t *ev = (const gw_evt_zb_device_annce_t *)data;
    if (!ev) return;
    gw_registry_on_device_announce(ev);
}

static void on_any_event_monitor(void *arg, esp_event_base_t base, int32_t id, void *data) {
    (void)arg;
    (void)base;

    bool enabled = false;
    portENTER_CRITICAL(&s_mon_mu);
    enabled = s_monitor_enabled;
    portEXIT_CRITICAL(&s_mon_mu);
    if (!enabled) return;

    // Rate-limit monitor output to keep the REPL usable.
    const TickType_t now = xTaskGetTickCount();
    bool should_drop = false;
    portENTER_CRITICAL(&s_mon_mu);
    if ((now - s_mon_window_start) >= pdMS_TO_TICKS(1000)) {
        s_mon_window_start = now;
        s_mon_window_lines = 0;
    }
    if (s_mon_window_lines >= CONFIG_GW_MONITOR_MAX_LINES_PER_SEC) {
        s_monitor_dropped++;
        should_drop = true;
    } else {
        s_mon_window_lines++;
    }
    portEXIT_CRITICAL(&s_mon_mu);
    if (should_drop) return;

    const char *name = gw_event_id_to_str(id);
    if (id == GW_CMD_PERMIT_JOIN && data) {
        const gw_cmd_permit_join_t *cmd = (const gw_cmd_permit_join_t *)data;
        printf("[gw] %s (%" PRIi32 ") req_id=%" PRIu64 " seconds=%u\n",
               name,
               id,
               cmd->req_id,
               (unsigned)cmd->seconds);
    } else if (id == GW_CMD_ONOFF && data) {
        const gw_cmd_onoff_t *cmd = (const gw_cmd_onoff_t *)data;
        printf("[gw] %s (%" PRIi32 ") req_id=%" PRIu64 " short=0x%04x ep=%u cmd=%u\n",
               name,
               id,
               cmd->req_id,
               (unsigned)cmd->short_addr,
               (unsigned)cmd->dst_ep,
               (unsigned)cmd->cmd_id);
    } else if (id == GW_EVT_CMD_RESULT && data) {
        const gw_evt_cmd_result_t *ev = (const gw_evt_cmd_result_t *)data;
        printf("[gw] %s (%" PRIi32 ") req_id=%" PRIu64 " status=%" PRIi32 "\n", name, id, ev->req_id, ev->status);
    } else if (id == GW_EVT_ZB_PERMIT_JOIN_STATUS && data) {
        const gw_evt_zb_permit_join_status_t *ev = (const gw_evt_zb_permit_join_status_t *)data;
        printf("[gw] %s (%" PRIi32 ") seconds=%u\n", name, id, (unsigned)ev->seconds);
    } else if (id == GW_EVT_ZB_DEVICE_ANNCE && data) {
        const gw_evt_zb_device_annce_t *ev = (const gw_evt_zb_device_annce_t *)data;
        printf("[gw] %s (%" PRIi32 ") short=0x%04x ieee=%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x cap=0x%02x\n",
               name,
               id,
               (unsigned)ev->short_addr,
               ev->ieee_addr[7],
               ev->ieee_addr[6],
               ev->ieee_addr[5],
               ev->ieee_addr[4],
               ev->ieee_addr[3],
               ev->ieee_addr[2],
               ev->ieee_addr[1],
               ev->ieee_addr[0],
               (unsigned)ev->capability);
    } else {
        printf("[gw] %s (%" PRIi32 ")\n", name, id);
    }
}

esp_err_t gw_bus_start(void) {
    if (s_loop) return ESP_OK;

    esp_event_loop_args_t args = {
        .queue_size = CONFIG_GW_EVENT_QUEUE_SIZE,
        .task_name = "gw_evt",
        .task_priority = 10,
        .task_stack_size = 6144,
        .task_core_id = 0,
    };

    esp_err_t err = esp_event_loop_create(&args, &s_loop);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_event_loop_create failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_ERROR_CHECK(esp_event_handler_register_with(s_loop, GW_EVT, GW_CMD_PERMIT_JOIN, &on_cmd_permit_join, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register_with(s_loop, GW_EVT, GW_CMD_ONOFF, &on_cmd_onoff, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register_with(s_loop, GW_EVT, GW_EVT_ZB_DEVICE_ANNCE, &on_evt_zb_device_annce, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register_with(s_loop, GW_EVT, ESP_EVENT_ANY_ID, &on_any_event_monitor, NULL));

    ESP_LOGI(TAG, "gw bus started (queue=%d)", CONFIG_GW_EVENT_QUEUE_SIZE);
    return ESP_OK;
}

esp_event_loop_handle_t gw_bus_get_loop(void) {
    return s_loop;
}

esp_err_t gw_bus_post_permit_join(uint16_t seconds, uint64_t req_id, TickType_t timeout) {
    if (!s_loop) return ESP_ERR_INVALID_STATE;
    const gw_cmd_permit_join_t cmd = {.req_id = req_id, .seconds = seconds};
    return esp_event_post_to(s_loop, GW_EVT, GW_CMD_PERMIT_JOIN, &cmd, sizeof(cmd), timeout);
}

esp_err_t gw_bus_post_onoff(uint16_t short_addr, uint8_t dst_ep, uint8_t cmd_id, uint64_t req_id, TickType_t timeout) {
    if (!s_loop) return ESP_ERR_INVALID_STATE;
    const gw_cmd_onoff_t cmd = {.req_id = req_id, .short_addr = short_addr, .dst_ep = dst_ep, .cmd_id = cmd_id};
    return esp_event_post_to(s_loop, GW_EVT, GW_CMD_ONOFF, &cmd, sizeof(cmd), timeout);
}

void gw_bus_set_monitor_enabled(bool enabled) {
    portENTER_CRITICAL(&s_mon_mu);
    s_monitor_enabled = enabled;
    portEXIT_CRITICAL(&s_mon_mu);
}

bool gw_bus_monitor_enabled(void) {
    bool enabled = false;
    portENTER_CRITICAL(&s_mon_mu);
    enabled = s_monitor_enabled;
    portEXIT_CRITICAL(&s_mon_mu);
    return enabled;
}

uint64_t gw_bus_monitor_dropped(void) {
    uint64_t v = 0;
    portENTER_CRITICAL(&s_mon_mu);
    v = s_monitor_dropped;
    portEXIT_CRITICAL(&s_mon_mu);
    return v;
}

const char *gw_event_id_to_str(int32_t id) {
    switch (id) {
        case GW_CMD_PERMIT_JOIN:
            return "GW_CMD_PERMIT_JOIN";
        case GW_CMD_ONOFF:
            return "GW_CMD_ONOFF";
        case GW_EVT_CMD_RESULT:
            return "GW_EVT_CMD_RESULT";
        case GW_EVT_ZB_PERMIT_JOIN_STATUS:
            return "GW_EVT_ZB_PERMIT_JOIN_STATUS";
        case GW_EVT_ZB_DEVICE_ANNCE:
            return "GW_EVT_ZB_DEVICE_ANNCE";
        default:
            return "GW_EVT_UNKNOWN";
    }
}
