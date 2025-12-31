#include "ble_host.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_err.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"

#include "hid_host.h"

static const char *TAG = "ble_host";

// Keep the "device registry" small and bounded.
#define MAX_DEVICES 20

static ble_host_device_t s_devices[MAX_DEVICES];
static int s_num_devices = 0;

static uint32_t s_pending_scan_seconds = 0;
static bool s_scanning = false;
static bool s_auto_accept_sec_req = true;
static bool s_auto_confirm_nc = true;

static bool s_pending_connect = false;
static uint8_t s_pending_connect_bda[6] = {0};
static uint8_t s_pending_connect_addr_type = BLE_ADDR_TYPE_PUBLIC;

static uint32_t now_ms(void) {
    return (uint32_t)(esp_timer_get_time() / 1000);
}

static bool bda_equal(const uint8_t a[6], const uint8_t b[6]) {
    return memcmp(a, b, 6) == 0;
}

static void bda_to_str(const uint8_t bda[6], char out[18]) {
    snprintf(out, 18, "%02x:%02x:%02x:%02x:%02x:%02x", bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
}

static const char *gatt_status_to_str(esp_gatt_status_t status) {
    switch (status) {
    case ESP_GATT_OK:
        return "ESP_GATT_OK";
    case ESP_GATT_ERROR:
        return "ESP_GATT_ERROR";
    case ESP_GATT_INVALID_HANDLE:
        return "ESP_GATT_INVALID_HANDLE";
    case ESP_GATT_READ_NOT_PERMIT:
        return "ESP_GATT_READ_NOT_PERMIT";
    case ESP_GATT_WRITE_NOT_PERMIT:
        return "ESP_GATT_WRITE_NOT_PERMIT";
    case ESP_GATT_INVALID_PDU:
        return "ESP_GATT_INVALID_PDU";
    case ESP_GATT_INSUF_AUTHENTICATION:
        return "ESP_GATT_INSUF_AUTHENTICATION";
    case ESP_GATT_REQ_NOT_SUPPORTED:
        return "ESP_GATT_REQ_NOT_SUPPORTED";
    case ESP_GATT_INVALID_OFFSET:
        return "ESP_GATT_INVALID_OFFSET";
    case ESP_GATT_INSUF_AUTHORIZATION:
        return "ESP_GATT_INSUF_AUTHORIZATION";
    case ESP_GATT_PREPARE_Q_FULL:
        return "ESP_GATT_PREPARE_Q_FULL";
    case ESP_GATT_NOT_FOUND:
        return "ESP_GATT_NOT_FOUND";
    case ESP_GATT_NOT_LONG:
        return "ESP_GATT_NOT_LONG";
    case ESP_GATT_INSUF_KEY_SIZE:
        return "ESP_GATT_INSUF_KEY_SIZE";
    case ESP_GATT_INVALID_ATTR_LEN:
        return "ESP_GATT_INVALID_ATTR_LEN";
    case ESP_GATT_ERR_UNLIKELY:
        return "ESP_GATT_ERR_UNLIKELY";
    case ESP_GATT_INSUF_ENCRYPTION:
        return "ESP_GATT_INSUF_ENCRYPTION";
    case ESP_GATT_UNSUPPORT_GRP_TYPE:
        return "ESP_GATT_UNSUPPORT_GRP_TYPE";
    case ESP_GATT_INSUF_RESOURCE:
        return "ESP_GATT_INSUF_RESOURCE";
    case ESP_GATT_NO_RESOURCES:
        return "ESP_GATT_NO_RESOURCES";
    case ESP_GATT_INTERNAL_ERROR:
        return "ESP_GATT_INTERNAL_ERROR";
    case ESP_GATT_WRONG_STATE:
        return "ESP_GATT_WRONG_STATE";
    case ESP_GATT_DB_FULL:
        return "ESP_GATT_DB_FULL";
    case ESP_GATT_BUSY:
        return "ESP_GATT_BUSY";
    case ESP_GATT_STACK_RSP:
        return "ESP_GATT_STACK_RSP";
    case ESP_GATT_APP_RSP:
        return "ESP_GATT_APP_RSP";
    case ESP_GATT_UNKNOWN_ERROR:
        return "ESP_GATT_UNKNOWN_ERROR";
    default:
        return "ESP_GATT_STATUS_UNKNOWN";
    }
}

static const char *gatt_conn_reason_to_str(esp_gatt_conn_reason_t reason) {
    switch (reason) {
    case ESP_GATT_CONN_UNKNOWN:
        return "ESP_GATT_CONN_UNKNOWN";
    case ESP_GATT_CONN_L2C_FAILURE:
        return "ESP_GATT_CONN_L2C_FAILURE";
    case ESP_GATT_CONN_TIMEOUT:
        return "ESP_GATT_CONN_TIMEOUT";
    case ESP_GATT_CONN_TERMINATE_PEER_USER:
        return "ESP_GATT_CONN_TERMINATE_PEER_USER";
    case ESP_GATT_CONN_TERMINATE_LOCAL_HOST:
        return "ESP_GATT_CONN_TERMINATE_LOCAL_HOST";
    case ESP_GATT_CONN_FAIL_ESTABLISH:
        return "ESP_GATT_CONN_FAIL_ESTABLISH";
    case ESP_GATT_CONN_LMP_TIMEOUT:
        return "ESP_GATT_CONN_LMP_TIMEOUT";
    case ESP_GATT_CONN_CONN_CANCEL:
        return "ESP_GATT_CONN_CONN_CANCEL";
    case ESP_GATT_CONN_NONE:
        return "ESP_GATT_CONN_NONE";
    default:
        return "ESP_GATT_CONN_REASON_UNKNOWN";
    }
}

static void update_device_registry(const uint8_t bda[6], uint8_t addr_type, int rssi, const char *name) {
    int idx = -1;
    for (int i = 0; i < s_num_devices; i++) {
        if (bda_equal(s_devices[i].bda, bda)) {
            idx = i;
            break;
        }
    }
    if (idx < 0) {
        if (s_num_devices >= MAX_DEVICES) {
            // Simple eviction: overwrite oldest by last_seen_ms.
            uint32_t oldest = s_devices[0].last_seen_ms;
            idx = 0;
            for (int i = 1; i < s_num_devices; i++) {
                if (s_devices[i].last_seen_ms < oldest) {
                    oldest = s_devices[i].last_seen_ms;
                    idx = i;
                }
            }
        } else {
            idx = s_num_devices++;
        }
        memset(&s_devices[idx], 0, sizeof(s_devices[idx]));
        memcpy(s_devices[idx].bda, bda, 6);
    }

    s_devices[idx].addr_type = addr_type;
    s_devices[idx].rssi = rssi;
    s_devices[idx].last_seen_ms = now_ms();
    if (name && *name) {
        snprintf(s_devices[idx].name, sizeof(s_devices[idx].name), "%s", name);
    }
}

static void handle_scan_result(const struct ble_scan_result_evt_param *scan_rst) {
    char name_buf[32] = {0};

    uint8_t name_len = 0;
    uint8_t *name = esp_ble_resolve_adv_data_by_type((uint8_t *)scan_rst->ble_adv,
                                                    (uint16_t)scan_rst->adv_data_len,
                                                    ESP_BLE_AD_TYPE_NAME_CMPL,
                                                    &name_len);
    if (!name) {
        name = esp_ble_resolve_adv_data_by_type((uint8_t *)scan_rst->ble_adv,
                                               (uint16_t)scan_rst->adv_data_len,
                                               ESP_BLE_AD_TYPE_NAME_SHORT,
                                               &name_len);
    }
    if (name && name_len > 0) {
        if (name_len >= sizeof(name_buf)) name_len = sizeof(name_buf) - 1;
        memcpy(name_buf, name, name_len);
        name_buf[name_len] = '\0';
    }

    update_device_registry(scan_rst->bda,
                           (uint8_t)scan_rst->ble_addr_type,
                           (int)scan_rst->rssi,
                           name_buf[0] ? name_buf : NULL);
}

static esp_ble_scan_params_t s_scan_params = {
    .scan_type = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval = 0x50,
    .scan_window = 0x30,
    .scan_duplicate = BLE_SCAN_DUPLICATE_ENABLE,
};

static void gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    switch (event) {
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
        if (s_pending_scan_seconds != 0) {
            esp_err_t err = esp_ble_gap_start_scanning(s_pending_scan_seconds);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "start_scanning failed: %s", esp_err_to_name(err));
            } else {
                s_scanning = true;
            }
            s_pending_scan_seconds = 0;
        }
        break;
    }

    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
        switch (param->scan_rst.search_evt) {
        case ESP_GAP_SEARCH_INQ_RES_EVT:
            handle_scan_result(&param->scan_rst);
            break;
        case ESP_GAP_SEARCH_INQ_CMPL_EVT:
            s_scanning = false;
            ESP_LOGI(TAG, "scan complete: num_resps=%u", (unsigned)param->scan_rst.num_resps);
            break;
        default:
            break;
        }
        break;
    }

    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
        ESP_LOGI(TAG, "scan start: status=%u", (unsigned)param->scan_start_cmpl.status);
        break;

    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        s_scanning = false;
        ESP_LOGI(TAG, "scan stop: status=%u", (unsigned)param->scan_stop_cmpl.status);
        if (s_pending_connect) {
            char a[18] = {0};
            bda_to_str(s_pending_connect_bda, a);
            ESP_LOGI(TAG, "connect pending: starting open now (addr=%s type=%s)",
                     a,
                     (s_pending_connect_addr_type == BLE_ADDR_TYPE_PUBLIC) ? "pub" : "rand");
            const bool ok = hid_host_open(s_pending_connect_bda, s_pending_connect_addr_type);
            if (!ok) {
                ESP_LOGE(TAG, "connect pending: hid_host_open failed");
            }
            s_pending_connect = false;
        }
        break;

    case ESP_GAP_BLE_AUTH_CMPL_EVT:
        if (param->ble_security.auth_cmpl.success) {
            ESP_LOGI(TAG, "auth complete: success");
        } else {
            ESP_LOGW(TAG, "auth complete: fail_reason=%u", (unsigned)param->ble_security.auth_cmpl.fail_reason);
        }
        break;

    case ESP_GAP_BLE_PASSKEY_NOTIF_EVT:
        ESP_LOGI(TAG,
                 "passkey notif: addr=%02x:%02x:%02x:%02x:%02x:%02x passkey=%u",
                 param->ble_security.key_notif.bd_addr[0],
                 param->ble_security.key_notif.bd_addr[1],
                 param->ble_security.key_notif.bd_addr[2],
                 param->ble_security.key_notif.bd_addr[3],
                 param->ble_security.key_notif.bd_addr[4],
                 param->ble_security.key_notif.bd_addr[5],
                 (unsigned)param->ble_security.key_notif.passkey);
        break;

    case ESP_GAP_BLE_PASSKEY_REQ_EVT:
        ESP_LOGI(TAG,
                 "passkey req: addr=%02x:%02x:%02x:%02x:%02x:%02x (use: passkey <addr> <6digits>)",
                 param->ble_security.ble_req.bd_addr[0],
                 param->ble_security.ble_req.bd_addr[1],
                 param->ble_security.ble_req.bd_addr[2],
                 param->ble_security.ble_req.bd_addr[3],
                 param->ble_security.ble_req.bd_addr[4],
                 param->ble_security.ble_req.bd_addr[5]);
        break;

    case ESP_GAP_BLE_NC_REQ_EVT:
        ESP_LOGI(TAG,
                 "numeric compare req: addr=%02x:%02x:%02x:%02x:%02x:%02x passkey=%u",
                 param->ble_security.key_notif.bd_addr[0],
                 param->ble_security.key_notif.bd_addr[1],
                 param->ble_security.key_notif.bd_addr[2],
                 param->ble_security.key_notif.bd_addr[3],
                 param->ble_security.key_notif.bd_addr[4],
                 param->ble_security.key_notif.bd_addr[5],
                 (unsigned)param->ble_security.key_notif.passkey);
        if (s_auto_confirm_nc) {
            (void)esp_ble_confirm_reply(param->ble_security.key_notif.bd_addr, true);
        }
        break;

    case ESP_GAP_BLE_SEC_REQ_EVT:
        ESP_LOGI(TAG,
                 "sec req: addr=%02x:%02x:%02x:%02x:%02x:%02x",
                 param->ble_security.ble_req.bd_addr[0],
                 param->ble_security.ble_req.bd_addr[1],
                 param->ble_security.ble_req.bd_addr[2],
                 param->ble_security.ble_req.bd_addr[3],
                 param->ble_security.ble_req.bd_addr[4],
                 param->ble_security.ble_req.bd_addr[5]);
        if (s_auto_accept_sec_req) {
            (void)esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
        }
        break;

    default:
        break;
    }
}

static void gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) {
    // Critical: forward GATTC events so the HID host can function.
    hid_host_forward_gattc_event((int)event, (int)gattc_if, (void *)param);

    switch (event) {
    case ESP_GATTC_CONNECT_EVT:
        ESP_LOGI(TAG, "gattc connect: conn_id=%u", (unsigned)param->connect.conn_id);
        break;
    case ESP_GATTC_OPEN_EVT: {
        char a[18] = {0};
        bda_to_str(param->open.remote_bda, a);
        ESP_LOGI(TAG,
                 "gattc open: status=0x%02x (%s) conn_id=%u mtu=%u addr=%s",
                 (unsigned)param->open.status,
                 gatt_status_to_str(param->open.status),
                 (unsigned)param->open.conn_id,
                 (unsigned)param->open.mtu,
                 a);
        break;
    }
    case ESP_GATTC_DISCONNECT_EVT:
        ESP_LOGI(TAG,
                 "gattc disconnect: reason=0x%04x (%s) conn_id=%u",
                 (unsigned)param->disconnect.reason,
                 gatt_conn_reason_to_str(param->disconnect.reason),
                 (unsigned)param->disconnect.conn_id);
        break;
    case ESP_GATTC_CLOSE_EVT:
        ESP_LOGI(TAG,
                 "gattc close: status=0x%02x (%s) reason=0x%04x (%s) conn_id=%u",
                 (unsigned)param->close.status,
                 gatt_status_to_str(param->close.status),
                 (unsigned)param->close.reason,
                 gatt_conn_reason_to_str(param->close.reason),
                 (unsigned)param->close.conn_id);
        break;
    default:
        break;
    }
}

static void set_security_defaults(void) {
    // Prefer bonding, but keep it permissive for bring-up (Just Works possible).
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_BOND;
    esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t key_size = 16;

    ESP_ERROR_CHECK(esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, 1));
    ESP_ERROR_CHECK(esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, 1));
    ESP_ERROR_CHECK(esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, 1));
    ESP_ERROR_CHECK(esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, 1));
    ESP_ERROR_CHECK(esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, 1));
}

void ble_host_init(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_cb));
    ESP_ERROR_CHECK(esp_ble_gattc_register_callback(gattc_cb));

    set_security_defaults();

    ESP_LOGI(TAG, "initialized");
}

void ble_host_scan_start(uint32_t seconds) {
    if (seconds == 0) seconds = 30;

    s_pending_scan_seconds = seconds;
    esp_err_t err = esp_ble_gap_set_scan_params(&s_scan_params);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "set_scan_params failed: %s", esp_err_to_name(err));
        s_pending_scan_seconds = 0;
    }
}

void ble_host_scan_stop(void) {
    if (!s_scanning) {
        ESP_LOGI(TAG, "scan already stopped");
        return;
    }
    esp_err_t err = esp_ble_gap_stop_scanning();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "stop_scanning failed: %s", esp_err_to_name(err));
        // If we were stopping scan in order to connect, we may never receive
        // ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT; fail "open" forward here.
        s_scanning = false;
        if (s_pending_connect) {
            char a[18] = {0};
            bda_to_str(s_pending_connect_bda, a);
            ESP_LOGI(TAG, "connect pending: starting open now after stop_scanning error (addr=%s type=%s)",
                     a,
                     (s_pending_connect_addr_type == BLE_ADDR_TYPE_PUBLIC) ? "pub" : "rand");
            const bool ok = hid_host_open(s_pending_connect_bda, s_pending_connect_addr_type);
            if (!ok) {
                ESP_LOGE(TAG, "connect pending: hid_host_open failed");
            }
            s_pending_connect = false;
        }
    }
}

void ble_host_devices_print(void) {
    printf("idx  addr              type rssi  age_ms  name\n");
    const uint32_t now = now_ms();
    for (int i = 0; i < s_num_devices; i++) {
        char a[18] = {0};
        bda_to_str(s_devices[i].bda, a);
        const uint32_t age = now - s_devices[i].last_seen_ms;
        printf("%-4d %-17s %-4s %-5d %-7" PRIu32 " %s\n",
               i,
               a,
               (s_devices[i].addr_type == BLE_ADDR_TYPE_PUBLIC) ? "pub" : "rand",
               s_devices[i].rssi,
               age,
               s_devices[i].name[0] ? s_devices[i].name : "(no name)");
    }
}

bool ble_host_device_get_by_index(int index, ble_host_device_t *out) {
    if (!out) return false;
    if (index < 0 || index >= s_num_devices) return false;
    *out = s_devices[index];
    return true;
}

bool ble_host_connect(const uint8_t bda[6], uint8_t addr_type) {
    if (!bda) return false;
    if (s_scanning) {
        memcpy(s_pending_connect_bda, bda, sizeof(s_pending_connect_bda));
        s_pending_connect_addr_type = addr_type;
        s_pending_connect = true;
        ble_host_scan_stop();
        return true;
    }
    return hid_host_open(bda, addr_type);
}

void ble_host_disconnect(void) {
    hid_host_close();
}

bool ble_host_pair(const uint8_t bda[6]) {
    if (!bda) return false;
    esp_err_t err = esp_ble_set_encryption((uint8_t *)bda, ESP_BLE_SEC_ENCRYPT_NO_MITM);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "set_encryption failed: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

void ble_host_bonds_print(void) {
    int n = esp_ble_get_bond_device_num();
    if (n <= 0) {
        printf("bonds: (none)\n");
        return;
    }

    esp_ble_bond_dev_t list[16];
    if (n > (int)(sizeof(list) / sizeof(list[0]))) n = (int)(sizeof(list) / sizeof(list[0]));
    int want = n;
    esp_err_t err = esp_ble_get_bond_device_list(&want, list);
    if (err != ESP_OK) {
        printf("bonds: error: %s\n", esp_err_to_name(err));
        return;
    }

    printf("bonds (%d):\n", want);
    for (int i = 0; i < want; i++) {
        char a[18] = {0};
        bda_to_str(list[i].bd_addr, a);
        printf("  %d: %s\n", i, a);
    }
}

bool ble_host_unpair(const uint8_t bda[6]) {
    if (!bda) return false;
    esp_err_t err = esp_ble_remove_bond_device((uint8_t *)bda);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "remove_bond_device failed: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

bool ble_host_sec_accept(const uint8_t bda[6], bool accept) {
    if (!bda) return false;
    esp_err_t err = esp_ble_gap_security_rsp((uint8_t *)bda, accept);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "security_rsp failed: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

bool ble_host_passkey_reply(const uint8_t bda[6], bool accept, uint32_t passkey) {
    if (!bda) return false;
    esp_err_t err = esp_ble_passkey_reply((uint8_t *)bda, accept, passkey);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "passkey_reply failed: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

bool ble_host_confirm_reply(const uint8_t bda[6], bool accept) {
    if (!bda) return false;
    esp_err_t err = esp_ble_confirm_reply((uint8_t *)bda, accept);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "confirm_reply failed: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}
