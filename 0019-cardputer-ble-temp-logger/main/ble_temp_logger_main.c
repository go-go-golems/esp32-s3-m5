/*
 * ESP32-S3 tutorial 0019:
 * Cardputer BLE temperature logger (mock sensor).
 *
 * GATT:
 * - Service UUID: 0xFFF0
 * - Temp characteristic: 0xFFF1 (READ + NOTIFY) value=int16_le (centi-degC)
 * - Control characteristic: 0xFFF2 (WRITE) value=uint16_le notify_period_ms (0 disables loop)
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_common_api.h"
#include "esp_gatts_api.h"
#include "esp_log.h"
#include "esp_system.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_heap_caps.h"
#include "nvs_flash.h"

static const char *TAG = "ble_temp";

#define DEV_NAME "CP_TEMP_LOGGER"

#define PROFILE_APP_ID 0
#define GATTS_NUM_HANDLES 8

#define EXT_ADV_HANDLE 0
#define NUM_EXT_ADV_SET 1
#define EXT_ADV_DURATION 0
#define EXT_ADV_MAX_EVENTS 0

#define UUID_PRIMARY_SERVICE 0x2800
#define UUID_CHAR_DECL 0x2803
#define UUID_CCCD 0x2902

#define UUID_SVC_TEMP_LOGGER 0xFFF0
#define UUID_CHAR_TEMPERATURE 0xFFF1
#define UUID_CHAR_CONTROL 0xFFF2

static uint16_t s_handle_table[GATTS_NUM_HANDLES] = {0};

enum {
    IDX_SVC,

    IDX_CHAR_TEMP_DECL,
    IDX_CHAR_TEMP_VALUE,
    IDX_CHAR_TEMP_CCCD,

    IDX_CHAR_CTRL_DECL,
    IDX_CHAR_CTRL_VALUE,

    IDX_NB,
};

static esp_gatt_if_t s_gatts_if = ESP_GATT_IF_NONE;
static uint16_t s_conn_id = 0;
static bool s_connected = false;
static bool s_notify_enabled = false;

static TaskHandle_t s_notify_task = NULL;
static uint32_t s_period_ms = 1000;

static bool s_ext_adv_params_set = false;
static bool s_ext_adv_data_set = false;

typedef struct {
    uint32_t connects;
    uint32_t disconnects;
    uint32_t notifies_sent;
    uint32_t notifies_fail;
    uint32_t ctrl_writes;
    uint32_t cccd_writes;
} counters_t;

static counters_t s_ctr = {0};

static void log_stats(const char *reason) {
    const size_t free8 = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    const size_t min_free8 = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
    const size_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);

    ESP_LOGI(TAG,
             "stats reason=%s conn=%d notify=%d period_ms=%" PRIu32
             " connects=%" PRIu32 " disconnects=%" PRIu32
             " notifies_ok=%" PRIu32 " notifies_fail=%" PRIu32
             " ctrl_writes=%" PRIu32 " cccd_writes=%" PRIu32
             " heap_free=%u heap_min=%u heap_largest=%u",
             reason ? reason : "?",
             s_connected ? 1 : 0,
             s_notify_enabled ? 1 : 0,
             s_period_ms,
             s_ctr.connects,
             s_ctr.disconnects,
             s_ctr.notifies_sent,
             s_ctr.notifies_fail,
             s_ctr.ctrl_writes,
             s_ctr.cccd_writes,
             (unsigned)free8,
             (unsigned)min_free8,
             (unsigned)largest);
}

static int16_t mock_temp_centi(uint32_t seq) {
    // Deterministic ramp: 20.00C .. 29.99C repeating.
    const int32_t base = 2000;
    const int32_t span = 1000;
    int32_t v = base + (int32_t)(seq % (uint32_t)span);
    return (int16_t)v;
}

static void set_temp_attr(int16_t centi_c) {
    uint8_t v[2] = {(uint8_t)(centi_c & 0xFF), (uint8_t)((centi_c >> 8) & 0xFF)};
    esp_err_t err = esp_ble_gatts_set_attr_value(s_handle_table[IDX_CHAR_TEMP_VALUE], sizeof(v), v);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "set_attr_value(temp) failed: %s", esp_err_to_name(err));
    }
}

static esp_err_t send_temp_notify(int16_t centi_c) {
    if (!s_connected || !s_notify_enabled || s_gatts_if == ESP_GATT_IF_NONE) {
        return ESP_OK;
    }

    uint8_t v[2] = {(uint8_t)(centi_c & 0xFF), (uint8_t)((centi_c >> 8) & 0xFF)};
    return esp_ble_gatts_send_indicate(s_gatts_if,
                                      s_conn_id,
                                      s_handle_table[IDX_CHAR_TEMP_VALUE],
                                      sizeof(v),
                                      v,
                                      false /* need_confirm */);
}

static void notify_task(void *arg) {
    (void)arg;

    uint32_t seq = 0;
    uint32_t ticks = 0;

    while (true) {
        const uint32_t period_ms = s_period_ms;

        int16_t t = mock_temp_centi(seq++);
        set_temp_attr(t);

        if (s_connected && s_notify_enabled && period_ms != 0) {
            esp_err_t err = send_temp_notify(t);
            if (err == ESP_OK) {
                s_ctr.notifies_sent++;
            } else {
                s_ctr.notifies_fail++;
                ESP_LOGW(TAG, "notify failed: %s", esp_err_to_name(err));
            }
        }

        // Periodic health dump.
        if ((ticks++ % 10) == 0) {
            log_stats("periodic");
        }

        if (period_ms == 0) {
            // "Disabled" mode: sleep until a control write wakes us up.
            (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            continue;
        }

        // Sleep, but allow control writes to wake us early and apply new period immediately.
        (void)ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(period_ms));
    }
}

// Extended advertising raw payload (AD structures).
// - Flags (general discoverable, BR/EDR not supported)
// - Complete list of 16-bit service UUIDs: 0xFFF0
// - Complete local name: "CP_TEMP_LOGGER"
static const uint8_t s_ext_adv_raw_data[] = {
    0x02, ESP_BLE_AD_TYPE_FLAG, 0x06,
    0x03, ESP_BLE_AD_TYPE_16SRV_CMPL, 0xF0, 0xFF,
    0x0F, ESP_BLE_AD_TYPE_NAME_CMPL,
    'C','P','_','T','E','M','P','_','L','O','G','G','E','R',
};

static esp_ble_gap_ext_adv_t s_ext_adv[1] = {
    [0] = {EXT_ADV_HANDLE, EXT_ADV_DURATION, EXT_ADV_MAX_EVENTS},
};

static esp_ble_gap_ext_adv_params_t s_ext_adv_params = {
    .type = ESP_BLE_GAP_SET_EXT_ADV_PROP_CONNECTABLE,
    .interval_min = 0x20,
    .interval_max = 0x40,
    .channel_map = ADV_CHNL_ALL,
    .filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
    .primary_phy = ESP_BLE_GAP_PHY_1M,
    .max_skip = 0,
    .secondary_phy = ESP_BLE_GAP_PHY_1M,
    .sid = 0,
    .scan_req_notif = false,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .tx_power = EXT_ADV_TX_PWR_NO_PREFERENCE,
};

static void start_advertising(void) {
    // Only start once params + data have been configured (callback-driven).
    if (!s_ext_adv_params_set || !s_ext_adv_data_set) {
        ESP_LOGI(TAG, "adv: not ready (params=%d data=%d), setting params", s_ext_adv_params_set ? 1 : 0, s_ext_adv_data_set ? 1 : 0);
        esp_err_t e = esp_ble_gap_ext_adv_set_params(EXT_ADV_HANDLE, &s_ext_adv_params);
        if (e != ESP_OK) {
            ESP_LOGW(TAG, "ext_adv_set_params failed: %s", esp_err_to_name(e));
        }
        return;
    }

    esp_err_t err = esp_ble_gap_ext_adv_start(NUM_EXT_ADV_SET, &s_ext_adv[0]);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "ext adv start failed: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "ext adv start requested");
    }
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    switch (event) {
    case ESP_GAP_BLE_EXT_ADV_SET_PARAMS_COMPLETE_EVT: {
        s_ext_adv_params_set = (param->ext_adv_set_params.status == ESP_BT_STATUS_SUCCESS);
        ESP_LOGI(TAG, "ext adv params set: status=%u instance=%u",
                 (unsigned)param->ext_adv_set_params.status,
                 (unsigned)param->ext_adv_set_params.instance);
        if (s_ext_adv_params_set) {
            esp_err_t e = esp_ble_gap_config_ext_adv_data_raw(EXT_ADV_HANDLE, sizeof(s_ext_adv_raw_data), s_ext_adv_raw_data);
            if (e != ESP_OK) {
                ESP_LOGW(TAG, "config_ext_adv_data_raw failed: %s", esp_err_to_name(e));
            }
        }
        break;
    }
    case ESP_GAP_BLE_EXT_ADV_DATA_SET_COMPLETE_EVT: {
        s_ext_adv_data_set = (param->ext_adv_data_set.status == ESP_BT_STATUS_SUCCESS);
        ESP_LOGI(TAG, "ext adv data set: status=%u instance=%u",
                 (unsigned)param->ext_adv_data_set.status,
                 (unsigned)param->ext_adv_data_set.instance);
        start_advertising();
        break;
    }
    case ESP_GAP_BLE_EXT_ADV_START_COMPLETE_EVT:
        ESP_LOGI(TAG, "ext adv start complete: status=%u inst_num=%u",
                 (unsigned)param->ext_adv_start.status,
                 (unsigned)param->ext_adv_start.instance_num);
        break;
    case ESP_GAP_BLE_EXT_ADV_STOP_COMPLETE_EVT:
        ESP_LOGI(TAG, "ext adv stop complete: status=%u inst_num=%u",
                 (unsigned)param->ext_adv_stop.status,
                 (unsigned)param->ext_adv_stop.instance_num);
        break;
    default:
        break;
    }
}

static void handle_write_cccd(const esp_ble_gatts_cb_param_t *param) {
    if (!param->write.is_prep && param->write.len == 2) {
        uint16_t v = (uint16_t)(param->write.value[1] << 8) | param->write.value[0];
        bool enabled = (v == 0x0001);
        s_notify_enabled = enabled;
        s_ctr.cccd_writes++;
        ESP_LOGI(TAG, "cccd write: 0x%04x => notify=%d", (unsigned)v, enabled ? 1 : 0);
        log_stats("cccd");
    } else {
        ESP_LOGW(TAG, "cccd write invalid: len=%d is_prep=%d", param->write.len, param->write.is_prep ? 1 : 0);
    }
}

static void handle_write_control(const esp_ble_gatts_cb_param_t *param) {
    if (!param->write.is_prep && param->write.len == 2) {
        uint16_t period_ms = (uint16_t)(param->write.value[1] << 8) | param->write.value[0];
        s_period_ms = (uint32_t)period_ms;
        s_ctr.ctrl_writes++;

        ESP_LOGI(TAG, "control write: period_ms=%u", (unsigned)period_ms);
        if (s_notify_task != NULL) {
            xTaskNotifyGive(s_notify_task);
        }
        log_stats("control");
    } else {
        ESP_LOGW(TAG, "control write invalid: len=%d is_prep=%d", param->write.len, param->write.is_prep ? 1 : 0);
    }
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    switch (event) {
    case ESP_GATTS_REG_EVT: {
        ESP_LOGI(TAG, "GATTS_REG_EVT status=%d app_id=%d", param->reg.status, param->reg.app_id);
        s_gatts_if = gatts_if;

        esp_err_t err = esp_ble_gap_set_device_name(DEV_NAME);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "set_device_name failed: %s", esp_err_to_name(err));
        }

        // BLE 5.0: use extended advertising APIs (callback-driven sequencing).
        s_ext_adv_params_set = false;
        s_ext_adv_data_set = false;
        err = esp_ble_gap_ext_adv_set_params(EXT_ADV_HANDLE, &s_ext_adv_params);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "ext_adv_set_params failed: %s", esp_err_to_name(err));
        }

        // Create the GATT attribute table.
        static const uint16_t primary_service_uuid = UUID_PRIMARY_SERVICE;
        static const uint16_t char_decl_uuid = UUID_CHAR_DECL;
        static const uint16_t cccd_uuid = UUID_CCCD;

        static const uint8_t char_prop_read_notify =
            ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
        static const uint8_t char_prop_write =
            ESP_GATT_CHAR_PROP_BIT_WRITE;

        static const uint16_t svc_uuid = UUID_SVC_TEMP_LOGGER;
        static const uint16_t temp_uuid = UUID_CHAR_TEMPERATURE;
        static const uint16_t ctrl_uuid = UUID_CHAR_CONTROL;

        static uint8_t temp_value[2] = {0x00, 0x00};
        static uint8_t ctrl_value[2] = {0xE8, 0x03}; // 1000ms
        static uint8_t cccd_value[2] = {0x00, 0x00};

        static const esp_gatts_attr_db_t gatt_db[IDX_NB] = {
            // Service Declaration
            [IDX_SVC] =
                {
                    {ESP_GATT_AUTO_RSP},
                    {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
                     sizeof(uint16_t), sizeof(svc_uuid), (uint8_t *)&svc_uuid},
                },

            // Temp Characteristic Declaration
            [IDX_CHAR_TEMP_DECL] =
                {
                    {ESP_GATT_AUTO_RSP},
                    {ESP_UUID_LEN_16, (uint8_t *)&char_decl_uuid, ESP_GATT_PERM_READ,
                     sizeof(uint8_t), sizeof(uint8_t), (uint8_t *)&char_prop_read_notify},
                },

            // Temp Characteristic Value
            [IDX_CHAR_TEMP_VALUE] =
                {
                    {ESP_GATT_AUTO_RSP},
                    {ESP_UUID_LEN_16, (uint8_t *)&temp_uuid, ESP_GATT_PERM_READ,
                     sizeof(temp_value), sizeof(temp_value), temp_value},
                },

            // Temp CCCD
            [IDX_CHAR_TEMP_CCCD] =
                {
                    {ESP_GATT_AUTO_RSP},
                    {ESP_UUID_LEN_16, (uint8_t *)&cccd_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                     sizeof(cccd_value), sizeof(cccd_value), cccd_value},
                },

            // Control Characteristic Declaration
            [IDX_CHAR_CTRL_DECL] =
                {
                    {ESP_GATT_AUTO_RSP},
                    {ESP_UUID_LEN_16, (uint8_t *)&char_decl_uuid, ESP_GATT_PERM_READ,
                     sizeof(uint8_t), sizeof(uint8_t), (uint8_t *)&char_prop_write},
                },

            // Control Characteristic Value (write-only)
            [IDX_CHAR_CTRL_VALUE] =
                {
                    {ESP_GATT_AUTO_RSP},
                    {ESP_UUID_LEN_16, (uint8_t *)&ctrl_uuid, ESP_GATT_PERM_WRITE,
                     sizeof(ctrl_value), sizeof(ctrl_value), ctrl_value},
                },
        };

        err = esp_ble_gatts_create_attr_tab(gatt_db, gatts_if, IDX_NB, 0);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "create_attr_tab failed: %s", esp_err_to_name(err));
        }
        break;
    }
    case ESP_GATTS_CREAT_ATTR_TAB_EVT: {
        ESP_LOGI(TAG, "CREATE_ATTR_TAB_EVT status=%d num=%d", param->add_attr_tab.status, param->add_attr_tab.num_handle);
        if (param->add_attr_tab.status != ESP_GATT_OK) {
            ESP_LOGE(TAG, "create attr table failed, status=0x%x", param->add_attr_tab.status);
            break;
        }
        if (param->add_attr_tab.num_handle != IDX_NB) {
            ESP_LOGE(TAG, "create attr table unexpected num_handle=%d", param->add_attr_tab.num_handle);
            break;
        }

        memcpy(s_handle_table, param->add_attr_tab.handles, sizeof(uint16_t) * IDX_NB);
        ESP_LOGI(TAG, "service handle=0x%04x temp=0x%04x cccd=0x%04x ctrl=0x%04x",
                 (unsigned)s_handle_table[IDX_SVC],
                 (unsigned)s_handle_table[IDX_CHAR_TEMP_VALUE],
                 (unsigned)s_handle_table[IDX_CHAR_TEMP_CCCD],
                 (unsigned)s_handle_table[IDX_CHAR_CTRL_VALUE]);

        esp_err_t err = esp_ble_gatts_start_service(s_handle_table[IDX_SVC]);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "start_service failed: %s", esp_err_to_name(err));
        } else {
            ESP_LOGI(TAG, "service started");
        }
        break;
    }
    case ESP_GATTS_CONNECT_EVT: {
        s_connected = true;
        s_conn_id = param->connect.conn_id;
        s_ctr.connects++;
        ESP_LOGI(TAG,
                 "connect: conn_id=%d addr=%02x:%02x:%02x:%02x:%02x:%02x",
                 param->connect.conn_id,
                 param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
                 param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
        log_stats("connect");
        break;
    }
    case ESP_GATTS_DISCONNECT_EVT: {
        s_connected = false;
        s_notify_enabled = false;
        s_ctr.disconnects++;
        ESP_LOGI(TAG, "disconnect: reason=0x%02x", (unsigned)param->disconnect.reason);
        log_stats("disconnect");
        start_advertising();
        break;
    }
    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(TAG, "mtu: %d", param->mtu.mtu);
        break;
    case ESP_GATTS_WRITE_EVT: {
        // Auto response is enabled, but we still observe writes to update internal state.
        const uint16_t h = param->write.handle;
        ESP_LOGI(TAG, "write: handle=0x%04x len=%d is_prep=%d", (unsigned)h, param->write.len, param->write.is_prep ? 1 : 0);

        if (h == s_handle_table[IDX_CHAR_TEMP_CCCD]) {
            handle_write_cccd(param);
        } else if (h == s_handle_table[IDX_CHAR_CTRL_VALUE]) {
            handle_write_control(param);
        }
        break;
    }
    case ESP_GATTS_CONF_EVT:
        if (param->conf.status != ESP_GATT_OK) {
            ESP_LOGW(TAG, "conf evt status=0x%x", (unsigned)param->conf.status);
        }
        break;
    default:
        break;
    }
}

static void ble_init(void) {
    esp_err_t err;

    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // BLE-only: release Classic BT memory to save heap.
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    err = esp_bt_controller_init(&bt_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "bt_controller_init failed: %s", esp_err_to_name(err));
        return;
    }
    err = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "bt_controller_enable failed: %s", esp_err_to_name(err));
        return;
    }

    err = esp_bluedroid_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "bluedroid_init failed: %s", esp_err_to_name(err));
        return;
    }
    err = esp_bluedroid_enable();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "bluedroid_enable failed: %s", esp_err_to_name(err));
        return;
    }

    ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_event_handler));
    ESP_ERROR_CHECK(esp_ble_gatts_register_callback(gatts_event_handler));
    ESP_ERROR_CHECK(esp_ble_gatts_app_register(PROFILE_APP_ID));
}

void app_main(void) {
    ESP_LOGI(TAG, "boot: %s", DEV_NAME);

    // Initial control defaults.
    s_period_ms = 1000;
    set_temp_attr(mock_temp_centi(0));

    ble_init();

    xTaskCreate(notify_task, "temp_notify", 4096, NULL, 10, &s_notify_task);

    log_stats("boot");
}


