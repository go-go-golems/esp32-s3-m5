#include "gw_zb.h"

#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "nvs_flash.h"
#include "nvs.h"

#include "esp_check.h"
#include "esp_log.h"

#include "esp_host_zb_api.h"
#include "esp_zigbee_core.h"
#include "zcl/esp_zigbee_zcl_command.h"
#include "zb_config_platform.h"

#include "gw_bus.h"
#include "gw_types.h"

static const char *TAG = "gw_zb_0031";

// Force-link the application signal handler object from `libmain.a` so the
// zb_host component can always resolve it (static archive extraction order can
// otherwise drop it).
extern void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct);
static void *volatile s_force_link_app_signal_handler __attribute__((used)) = (void *)esp_zb_app_signal_handler;

// ZNSP protocol IDs/status codes (mirrors the host example's private header).
enum {
    ZNSP_NETWORK_PERMIT_JOINING = 0x0005,
    ZNSP_NETWORK_PRIMARY_CHANNEL_SET = 0x0010,
    ZNSP_NETWORK_CHANNEL_SET = 0x0014,
    ZNSP_ZCL_ENDPOINT_ADD = 0x0100,
    ZNSP_ZCL_WRITE = 0x0107,
};
typedef enum {
    ZNSP_STATUS_SUCCESS = 0x00,
    ZNSP_STATUS_ERR_FATAL = 0x01,
    ZNSP_STATUS_BAD_ARGUMENT = 0x02,
    ZNSP_STATUS_ERR_NO_MEM = 0x03,
} znsp_status_t;

typedef enum {
    GW_ZB_CMD_PERMIT_JOIN = 1,
    GW_ZB_CMD_ONOFF = 2,
} gw_zb_cmd_kind_t;

typedef struct {
    gw_zb_cmd_kind_t kind;
    uint64_t req_id;
    uint16_t seconds;
    uint16_t short_addr;
    uint8_t dst_ep;
    uint8_t cmd_id;
} gw_zb_cmd_t;

static QueueHandle_t s_cmd_q = NULL;
static TaskHandle_t s_stack_task = NULL;
static TaskHandle_t s_cmd_task = NULL;
static bool s_started = false;
static SemaphoreHandle_t s_znsp_mu = NULL;
static uint32_t s_primary_channel_mask = 0;

static const char *NVS_NS = "gw_zb";
static const char *NVS_KEY_PRIMARY_CH_MASK = "primary_ch";

static esp_err_t map_znsp_status_to_err(uint8_t status);

static esp_err_t ensure_nvs_ready(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "nvs init: %s -> erasing", esp_err_to_name(err));
        ESP_RETURN_ON_ERROR(nvs_flash_erase(), TAG, "nvs erase failed");
        err = nvs_flash_init();
    }
    return err;
}

static esp_err_t load_primary_channel_mask_from_nvs(uint32_t *out_mask) {
    if (!out_mask) return ESP_ERR_INVALID_ARG;
    *out_mask = 0;

    nvs_handle_t h = 0;
    esp_err_t err = nvs_open(NVS_NS, NVS_READONLY, &h);
    if (err != ESP_OK) return err;

    uint32_t v = 0;
    err = nvs_get_u32(h, NVS_KEY_PRIMARY_CH_MASK, &v);
    nvs_close(h);
    if (err == ESP_OK) {
        *out_mask = v;
    }
    return err;
}

static esp_err_t store_primary_channel_mask_to_nvs(uint32_t mask) {
    nvs_handle_t h = 0;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;

    err = nvs_set_u32(h, NVS_KEY_PRIMARY_CH_MASK, mask);
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    return err;
}

uint32_t gw_zb_get_primary_channel_mask(void) {
    return s_primary_channel_mask;
}

esp_err_t gw_zb_set_primary_channel_mask(uint32_t mask, bool persist, bool apply_now, TickType_t timeout) {
    // Basic validation: allow 0 ("don't care"), otherwise must contain at least one Zigbee channel bit.
    if (mask != 0) {
        const uint32_t valid_bits = 0x07FFF800U; // channels 11..26
        if ((mask & valid_bits) == 0) return ESP_ERR_INVALID_ARG;
    }

    s_primary_channel_mask = mask;
    if (persist) {
        const esp_err_t err = store_primary_channel_mask_to_nvs(mask);
        if (err != ESP_OK) return err;
    }

    if (!apply_now) return ESP_OK;

    // Set both the general channel mask and the primary channel set; different
    // Zigbee SDK paths consult different masks during formation/steering.
    uint8_t out = 0xFF;
    uint16_t outlen = sizeof(out);
    const esp_err_t txerr1 = gw_zb_znsp_request(ZNSP_NETWORK_CHANNEL_SET, &mask, sizeof(mask), &out, &outlen, timeout);
    const esp_err_t st1 = (txerr1 == ESP_OK) ? map_znsp_status_to_err(out) : txerr1;

    out = 0xFF;
    outlen = sizeof(out);
    const esp_err_t txerr2 = gw_zb_znsp_request(ZNSP_NETWORK_PRIMARY_CHANNEL_SET, &mask, sizeof(mask), &out, &outlen, timeout);
    const esp_err_t st2 = (txerr2 == ESP_OK) ? map_znsp_status_to_err(out) : txerr2;

    return (st1 != ESP_OK) ? st1 : st2;
}

static esp_err_t gw_zb_post_cmd_result(uint64_t req_id, esp_err_t status) {
    esp_event_loop_handle_t loop = gw_bus_get_loop();
    if (!loop) return ESP_ERR_INVALID_STATE;
    const gw_evt_cmd_result_t out = {.req_id = req_id, .status = (int32_t)status};
    return esp_event_post_to(loop, GW_EVT, GW_EVT_CMD_RESULT, &out, sizeof(out), 0);
}

static esp_err_t map_znsp_status_to_err(uint8_t status) {
    switch ((znsp_status_t)status) {
        case ZNSP_STATUS_SUCCESS:
            return ESP_OK;
        case ZNSP_STATUS_BAD_ARGUMENT:
            return ESP_ERR_INVALID_ARG;
        case ZNSP_STATUS_ERR_NO_MEM:
            return ESP_ERR_NO_MEM;
        case ZNSP_STATUS_ERR_FATAL:
        default:
            return ESP_FAIL;
    }
}

esp_err_t gw_zb_znsp_request(uint16_t id,
                             const void *buffer,
                             uint16_t len,
                             void *output,
                             uint16_t *outlen,
                             TickType_t timeout) {
    if (!s_started || !s_znsp_mu) return ESP_ERR_INVALID_STATE;
    if (!output || !outlen) return ESP_ERR_INVALID_ARG;

    if (xSemaphoreTake(s_znsp_mu, timeout) != pdTRUE) return ESP_ERR_TIMEOUT;
    const esp_err_t err = esp_host_zb_output(id, buffer, len, output, outlen);
    xSemaphoreGive(s_znsp_mu);
    return err;
}

static void gw_zb_cmd_task_main(void *arg) {
    (void)arg;
    while (true) {
        gw_zb_cmd_t cmd = {0};
        if (xQueueReceive(s_cmd_q, &cmd, portMAX_DELAY) != pdTRUE) continue;

        switch (cmd.kind) {
            case GW_ZB_CMD_PERMIT_JOIN: {
                const uint8_t sec8 = (cmd.seconds > 255) ? 255 : (uint8_t)cmd.seconds;
                uint8_t out = 0xFF;
                uint16_t outlen = sizeof(out);

                ESP_LOGI(TAG, "permit_join request seconds=%u req_id=%" PRIu64, (unsigned)sec8, cmd.req_id);
                const esp_err_t txerr = gw_zb_znsp_request(ZNSP_NETWORK_PERMIT_JOINING,
                                                          &sec8,
                                                          sizeof(sec8),
                                                          &out,
                                                          &outlen,
                                                          pdMS_TO_TICKS(3000));

                const esp_err_t status = (txerr == ESP_OK) ? map_znsp_status_to_err(out) : txerr;
                ESP_LOGI(TAG, "permit_join response status=0x%02x (%s) req_id=%" PRIu64,
                         (unsigned)out,
                         esp_err_to_name(status),
                         cmd.req_id);
                (void)gw_zb_post_cmd_result(cmd.req_id, status);
                break;
            }
            case GW_ZB_CMD_ONOFF: {
                uint8_t out = 0xFF;
                uint16_t outlen = sizeof(out);

                typedef struct __attribute__((packed)) {
                    esp_zb_zcl_basic_cmd_t zcl_basic_cmd;
                    uint8_t address_mode;
                    uint16_t profile_id;
                    uint16_t cluster_id;
                    uint16_t custom_cmd_id;
                    uint8_t direction;
                    uint8_t type;
                    uint16_t size;
                } gw_ncp_zcl_write_t;

                const gw_ncp_zcl_write_t payload = {
                    .zcl_basic_cmd =
                        {
                            .dst_addr_u = {.addr_short = cmd.short_addr},
                            .dst_endpoint = cmd.dst_ep,
                            .src_endpoint = 1,
                        },
                    .address_mode = 0x02, // ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT
                    .profile_id = 0x0104, // HA profile
                    .cluster_id = 0x0006, // OnOff
                    .custom_cmd_id = cmd.cmd_id,
                    .direction = 0x00, // ESP_ZB_ZCL_CMD_DIRECTION_TO_SRV
                    .type = 0x00,      // ESP_ZB_ZCL_ATTR_TYPE_NULL
                    .size = 0,
                };

                ESP_LOGI(TAG,
                         "onoff request short=0x%04x ep=%u cmd=%u req_id=%" PRIu64,
                         (unsigned)cmd.short_addr,
                         (unsigned)cmd.dst_ep,
                         (unsigned)cmd.cmd_id,
                         cmd.req_id);
                const esp_err_t txerr = gw_zb_znsp_request(ZNSP_ZCL_WRITE,
                                                          &payload,
                                                          sizeof(payload),
                                                          &out,
                                                          &outlen,
                                                          pdMS_TO_TICKS(3000));
                const esp_err_t status = (txerr == ESP_OK) ? map_znsp_status_to_err(out) : txerr;
                ESP_LOGI(TAG,
                         "onoff response status=0x%02x (%s) req_id=%" PRIu64,
                         (unsigned)out,
                         esp_err_to_name(status),
                         cmd.req_id);
                (void)gw_zb_post_cmd_result(cmd.req_id, status);
                break;
            }
            default:
                break;
        }
    }
}

static void gw_zb_stack_task_main(void *arg) {
    (void)arg;

    // The H2 ROM prints a boot banner on the same pins used for the NCP UART bus.
    // Delay host bring-up so we can flush those bytes at bus init and start from
    // a clean SLIP framing boundary.
    vTaskDelay(pdMS_TO_TICKS(1500));

    esp_zb_platform_config_t config = {
        .radio_config = {.radio_mode = RADIO_MODE_UART_NCP},
        .host_config = {.host_mode = HOST_CONNECTION_MODE_UART},
    };

    ESP_ERROR_CHECK(esp_zb_platform_config(&config));

    // Apply a persisted primary channel mask (if set). This must be done before
    // network formation to reliably influence the chosen channel.
    uint32_t persisted_mask = 0;
    if (load_primary_channel_mask_from_nvs(&persisted_mask) == ESP_OK) {
        s_primary_channel_mask = persisted_mask;
    }
    if (s_primary_channel_mask) {
        const TickType_t to = pdMS_TO_TICKS(3000);
        esp_err_t st = gw_zb_set_primary_channel_mask(s_primary_channel_mask, false, true, to);
        ESP_LOGI(TAG, "channel mask applied: 0x%08" PRIx32 " (%s)", s_primary_channel_mask, esp_err_to_name(st));
    } else {
        ESP_LOGI(TAG, "primary channel mask: default (not set)");
    }

    // Minimal coordinator config. (The full HA macros are not used in 0031.)
    esp_zb_cfg_t zb_cfg = {
        .esp_zb_role = ESP_ZB_DEVICE_TYPE_COORDINATOR,
        .install_code_policy = false,
        .nwk_cfg = {.zczr_cfg = {.max_children = 16}},
    };

    ESP_LOGI(TAG, "zb init/start (host+NCP)");
    esp_zb_init(&zb_cfg);
    (void)esp_zb_start(false);

    ESP_LOGI(TAG, "zb notify loop start");
    esp_zb_stack_main_loop();
}

esp_err_t gw_zb_start(void) {
    if (s_started) return ESP_OK;

    ESP_RETURN_ON_ERROR(ensure_nvs_ready(), TAG, "nvs not ready");

    s_znsp_mu = xSemaphoreCreateMutex();
    if (!s_znsp_mu) return ESP_ERR_NO_MEM;

    s_cmd_q = xQueueCreate(8, sizeof(gw_zb_cmd_t));
    if (!s_cmd_q) return ESP_ERR_NO_MEM;

    if (xTaskCreate(gw_zb_cmd_task_main, "gw_zb_cmd", 4096, NULL, 9, &s_cmd_task) != pdTRUE) {
        return ESP_FAIL;
    }

    if (xTaskCreate(gw_zb_stack_task_main, "gw_zb_stack", 6144, NULL, 8, &s_stack_task) != pdTRUE) {
        return ESP_FAIL;
    }

    s_started = true;
    return ESP_OK;
}

bool gw_zb_started(void) {
    return s_started;
}

esp_err_t gw_zb_request_permit_join(uint16_t seconds, uint64_t req_id, TickType_t timeout) {
    if (!s_started || !s_cmd_q) return ESP_ERR_INVALID_STATE;

    const gw_zb_cmd_t cmd = {.kind = GW_ZB_CMD_PERMIT_JOIN, .req_id = req_id, .seconds = seconds};
    return (xQueueSend(s_cmd_q, &cmd, timeout) == pdTRUE) ? ESP_OK : ESP_ERR_TIMEOUT;
}

esp_err_t gw_zb_request_onoff(uint16_t short_addr, uint8_t dst_ep, uint8_t cmd_id, uint64_t req_id, TickType_t timeout) {
    if (!s_started || !s_cmd_q) return ESP_ERR_INVALID_STATE;
    if (dst_ep == 0) return ESP_ERR_INVALID_ARG;
    if (cmd_id > 2) return ESP_ERR_INVALID_ARG;

    const gw_zb_cmd_t cmd = {
        .kind = GW_ZB_CMD_ONOFF,
        .req_id = req_id,
        .short_addr = short_addr,
        .dst_ep = dst_ep,
        .cmd_id = cmd_id,
    };
    return (xQueueSend(s_cmd_q, &cmd, timeout) == pdTRUE) ? ESP_OK : ESP_ERR_TIMEOUT;
}
