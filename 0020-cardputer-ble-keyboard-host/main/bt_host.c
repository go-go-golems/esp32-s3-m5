#include "bt_host.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "freertos/FreeRTOS.h"

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_err.h"
#include "esp_gap_bt_api.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"

#include "bt_decode.h"
#include "hid_host.h"

static const char *TAG = "bt_host";

#if defined(CONFIG_SOC_BT_CLASSIC_SUPPORTED) && CONFIG_SOC_BT_CLASSIC_SUPPORTED
#define BT_HOST_HAS_BREDR 1
#else
#define BT_HOST_HAS_BREDR 0
#endif

// Keep the "device registry" bounded; this is the list shown by `devices ...`.
// (Scan results can be much larger; we keep the most recently seen devices.)
#define MAX_LE_DEVICES 100
#define MAX_BREDR_DEVICES 100
#define DEVICE_STALE_MS 30000

static bt_le_device_t s_le_devices[MAX_LE_DEVICES];
static int s_le_num_devices = 0;

static uint32_t s_le_pending_scan_seconds = 0;
static bool s_le_scanning = false;
static bool s_le_auto_accept_sec_req = true;
static bool s_le_auto_confirm_nc = true;
static bool s_le_device_events_enabled = true;
static uint32_t s_le_last_prune_ms = 0;

static bool s_le_pending_connect = false;
static uint8_t s_le_pending_connect_bda[6] = {0};
static uint8_t s_le_pending_connect_addr_type = BLE_ADDR_TYPE_PUBLIC;

static bool s_le_connected = false;
static uint8_t s_le_connected_bda[6] = {0};

static bool s_le_pending_pair = false;
static uint8_t s_le_pending_pair_bda[6] = {0};

#if BT_HOST_HAS_BREDR
static bt_bredr_device_t s_bredr_devices[MAX_BREDR_DEVICES];
static int s_bredr_num_devices = 0;

static bool s_bredr_scanning = false;
static bool s_bredr_device_events_enabled = true;
static uint32_t s_bredr_last_prune_ms = 0;
static bool s_bredr_auto_confirm_ssp = true;

static bool s_bredr_pending_connect = false;
static uint8_t s_bredr_pending_connect_bda[6] = {0};
#endif

static uint32_t now_ms(void) {
    return (uint32_t)(esp_timer_get_time() / 1000);
}

static bool bda_equal(const uint8_t a[6], const uint8_t b[6]) {
    return memcmp(a, b, 6) == 0;
}

static void bda_to_str(const uint8_t bda[6], char out[18]) {
    snprintf(out, 18, "%02x:%02x:%02x:%02x:%02x:%02x", bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
}

static void bda_to_str_dash(const uint8_t bda[6], char out[18]) {
    snprintf(out, 18, "%02X-%02X-%02X-%02X-%02X-%02X", bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
}

static void clear_le_connection_state(void) {
    s_le_connected = false;
    memset(s_le_connected_bda, 0, sizeof(s_le_connected_bda));
}

static bool start_encryption_for_peer(const uint8_t bda[6]) {
    if (!bda) return false;
    esp_err_t err = esp_ble_set_encryption((uint8_t *)bda, ESP_BLE_SEC_ENCRYPT_NO_MITM);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "set_encryption failed: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

static void print_le_device_event_new(const uint8_t bda[6], const char *name) {
    if (!s_le_device_events_enabled) return;
    char addr[18] = {0};
    bda_to_str(bda, addr);
    if (name && *name) {
        printf("[NEW] LE Device %s %s\n", addr, name);
    } else {
        char dash[18] = {0};
        bda_to_str_dash(bda, dash);
        printf("[NEW] LE Device %s %s\n", addr, dash);
    }
}

static void print_le_device_event_del(const uint8_t bda[6], const char *name) {
    if (!s_le_device_events_enabled) return;
    char addr[18] = {0};
    bda_to_str(bda, addr);
    if (name && *name) {
        printf("[DEL] LE Device %s %s\n", addr, name);
    } else {
        char dash[18] = {0};
        bda_to_str_dash(bda, dash);
        printf("[DEL] LE Device %s %s\n", addr, dash);
    }
}

static void print_le_device_event_chg_rssi(const uint8_t bda[6], int rssi) {
    if (!s_le_device_events_enabled) return;
    char addr[18] = {0};
    bda_to_str(bda, addr);
    printf("[CHG] LE Device %s RSSI: 0x%08" PRIx32 " (%d)\n", addr, (uint32_t)rssi, rssi);
}

static void print_le_device_event_chg_name(const uint8_t bda[6], const char *name) {
    if (!s_le_device_events_enabled) return;
    if (!name || !*name) return;
    char addr[18] = {0};
    bda_to_str(bda, addr);
    printf("[CHG] LE Device %s Name: %s\n", addr, name);
}

static void remove_le_device_at(int idx) {
    if (idx < 0 || idx >= s_le_num_devices) return;
    print_le_device_event_del(s_le_devices[idx].bda, s_le_devices[idx].name);
    for (int i = idx + 1; i < s_le_num_devices; i++) {
        s_le_devices[i - 1] = s_le_devices[i];
    }
    s_le_num_devices--;
    if (s_le_num_devices < 0) s_le_num_devices = 0;
}

static void prune_stale_le_devices(uint32_t now) {
    if (now - s_le_last_prune_ms < 1000) return;
    s_le_last_prune_ms = now;

    // Remove devices that haven't been seen in a while.
    for (int i = s_le_num_devices - 1; i >= 0; i--) {
        const uint32_t age = now - s_le_devices[i].last_seen_ms;
        if (age > DEVICE_STALE_MS) {
            remove_le_device_at(i);
        }
    }
}

static void update_le_device_registry(const uint8_t bda[6], uint8_t addr_type, int rssi, const char *name) {
    int idx = -1;
    for (int i = 0; i < s_le_num_devices; i++) {
        if (bda_equal(s_le_devices[i].bda, bda)) {
            idx = i;
            break;
        }
    }
    const bool is_new = (idx < 0);
    int prev_rssi = 0;
    char prev_name[32] = {0};
    if (!is_new) {
        prev_rssi = s_le_devices[idx].rssi;
        snprintf(prev_name, sizeof(prev_name), "%s", s_le_devices[idx].name);
    }

    if (is_new) {
        if (s_le_num_devices >= MAX_LE_DEVICES) {
            // Simple eviction: overwrite oldest by last_seen_ms.
            uint32_t oldest = s_le_devices[0].last_seen_ms;
            idx = 0;
            for (int i = 1; i < s_le_num_devices; i++) {
                if (s_le_devices[i].last_seen_ms < oldest) {
                    oldest = s_le_devices[i].last_seen_ms;
                    idx = i;
                }
            }
            // Overwrite implies eviction (a DEL + NEW pair).
            print_le_device_event_del(s_le_devices[idx].bda, s_le_devices[idx].name);
        } else {
            idx = s_le_num_devices++;
        }
        memset(&s_le_devices[idx], 0, sizeof(s_le_devices[idx]));
        memcpy(s_le_devices[idx].bda, bda, 6);
    }

    s_le_devices[idx].addr_type = addr_type;
    s_le_devices[idx].rssi = rssi;
    s_le_devices[idx].last_seen_ms = now_ms();
    if (name && *name) {
        snprintf(s_le_devices[idx].name, sizeof(s_le_devices[idx].name), "%s", name);
    }

    if (is_new && idx < s_le_num_devices) {
        print_le_device_event_new(bda, s_le_devices[idx].name);
        return;
    }

    // Change events for existing devices.
    if (!is_new) {
        if (rssi != prev_rssi) {
            print_le_device_event_chg_rssi(bda, rssi);
        }
        if (name && *name && strncmp(prev_name, s_le_devices[idx].name, sizeof(s_le_devices[idx].name)) != 0) {
            print_le_device_event_chg_name(bda, s_le_devices[idx].name);
        }
    }
}

#if BT_HOST_HAS_BREDR
static void print_bredr_device_event_new(const uint8_t bda[6], const char *name) {
    if (!s_bredr_device_events_enabled) return;
    char addr[18] = {0};
    bda_to_str(bda, addr);
    if (name && *name) {
        printf("[NEW] BR Device %s %s\n", addr, name);
    } else {
        char dash[18] = {0};
        bda_to_str_dash(bda, dash);
        printf("[NEW] BR Device %s %s\n", addr, dash);
    }
}

static void print_bredr_device_event_del(const uint8_t bda[6], const char *name) {
    if (!s_bredr_device_events_enabled) return;
    char addr[18] = {0};
    bda_to_str(bda, addr);
    if (name && *name) {
        printf("[DEL] BR Device %s %s\n", addr, name);
    } else {
        char dash[18] = {0};
        bda_to_str_dash(bda, dash);
        printf("[DEL] BR Device %s %s\n", addr, dash);
    }
}

static void print_bredr_device_event_chg_rssi(const uint8_t bda[6], int rssi) {
    if (!s_bredr_device_events_enabled) return;
    char addr[18] = {0};
    bda_to_str(bda, addr);
    printf("[CHG] BR Device %s RSSI: 0x%08" PRIx32 " (%d)\n", addr, (uint32_t)rssi, rssi);
}

static void print_bredr_device_event_chg_name(const uint8_t bda[6], const char *name) {
    if (!s_bredr_device_events_enabled) return;
    if (!name || !*name) return;
    char addr[18] = {0};
    bda_to_str(bda, addr);
    printf("[CHG] BR Device %s Name: %s\n", addr, name);
}

static bool cod_likely_keyboard(uint32_t cod) {
    const uint32_t major = esp_bt_gap_get_cod_major_dev(cod);
    if (major != ESP_BT_COD_MAJOR_DEV_PERIPHERAL) return false;
    const uint32_t minor = esp_bt_gap_get_cod_minor_dev(cod);
    const uint32_t kbd_bits = (ESP_BT_COD_MINOR_PERIPHERAL_KEYBOARD >> ESP_BT_COD_MINOR_DEV_BIT_OFFSET);
    const uint32_t combo_bits = (ESP_BT_COD_MINOR_PERIPHERAL_COMBO >> ESP_BT_COD_MINOR_DEV_BIT_OFFSET);
    return ((minor & kbd_bits) == kbd_bits) || ((minor & combo_bits) == combo_bits);
}

static void remove_bredr_device_at(int idx) {
    if (idx < 0 || idx >= s_bredr_num_devices) return;
    print_bredr_device_event_del(s_bredr_devices[idx].bda, s_bredr_devices[idx].name);
    for (int i = idx + 1; i < s_bredr_num_devices; i++) {
        s_bredr_devices[i - 1] = s_bredr_devices[i];
    }
    s_bredr_num_devices--;
    if (s_bredr_num_devices < 0) s_bredr_num_devices = 0;
}

static void prune_stale_bredr_devices(uint32_t now) {
    if (now - s_bredr_last_prune_ms < 1000) return;
    s_bredr_last_prune_ms = now;

    for (int i = s_bredr_num_devices - 1; i >= 0; i--) {
        const uint32_t age = now - s_bredr_devices[i].last_seen_ms;
        if (age > DEVICE_STALE_MS) {
            remove_bredr_device_at(i);
        }
    }
}

static void update_bredr_device_registry(const uint8_t bda[6], int rssi, uint32_t cod, const char *name) {
    int idx = -1;
    for (int i = 0; i < s_bredr_num_devices; i++) {
        if (bda_equal(s_bredr_devices[i].bda, bda)) {
            idx = i;
            break;
        }
    }
    const bool is_new = (idx < 0);

    int prev_rssi = 0;
    char prev_name[64] = {0};
    if (!is_new) {
        prev_rssi = s_bredr_devices[idx].rssi;
        snprintf(prev_name, sizeof(prev_name), "%s", s_bredr_devices[idx].name);
    }

    if (is_new) {
        if (s_bredr_num_devices >= MAX_BREDR_DEVICES) {
            uint32_t oldest = s_bredr_devices[0].last_seen_ms;
            idx = 0;
            for (int i = 1; i < s_bredr_num_devices; i++) {
                if (s_bredr_devices[i].last_seen_ms < oldest) {
                    oldest = s_bredr_devices[i].last_seen_ms;
                    idx = i;
                }
            }
            print_bredr_device_event_del(s_bredr_devices[idx].bda, s_bredr_devices[idx].name);
        } else {
            idx = s_bredr_num_devices++;
        }
        memset(&s_bredr_devices[idx], 0, sizeof(s_bredr_devices[idx]));
        memcpy(s_bredr_devices[idx].bda, bda, 6);
    }

    s_bredr_devices[idx].rssi = rssi;
    s_bredr_devices[idx].cod = cod;
    s_bredr_devices[idx].last_seen_ms = now_ms();
    if (name && *name) {
        snprintf(s_bredr_devices[idx].name, sizeof(s_bredr_devices[idx].name), "%s", name);
    }

    if (is_new && idx < s_bredr_num_devices) {
        print_bredr_device_event_new(bda, s_bredr_devices[idx].name);
        return;
    }

    if (!is_new) {
        if (rssi != prev_rssi) {
            print_bredr_device_event_chg_rssi(bda, rssi);
        }
        if (name && *name && strncmp(prev_name, s_bredr_devices[idx].name, sizeof(s_bredr_devices[idx].name)) != 0) {
            print_bredr_device_event_chg_name(bda, s_bredr_devices[idx].name);
        }
    }
}
#endif

static void handle_scan_result(const struct ble_scan_result_evt_param *scan_rst) {
    char name_buf[32] = {0};

    uint8_t name_len = 0;
    const uint16_t eir_len = (uint16_t)scan_rst->adv_data_len + (uint16_t)scan_rst->scan_rsp_len;
    uint8_t *name = esp_ble_resolve_adv_data_by_type((uint8_t *)scan_rst->ble_adv,
                                                    eir_len,
                                                    ESP_BLE_AD_TYPE_NAME_CMPL,
                                                    &name_len);
    if (!name) {
        name = esp_ble_resolve_adv_data_by_type((uint8_t *)scan_rst->ble_adv,
                                               eir_len,
                                               ESP_BLE_AD_TYPE_NAME_SHORT,
                                               &name_len);
    }
    if (name && name_len > 0) {
        if (name_len >= sizeof(name_buf)) name_len = sizeof(name_buf) - 1;
        memcpy(name_buf, name, name_len);
        name_buf[name_len] = '\0';
    }

    update_le_device_registry(scan_rst->bda,
                           (uint8_t)scan_rst->ble_addr_type,
                           (int)scan_rst->rssi,
                           name_buf[0] ? name_buf : NULL);

    prune_stale_le_devices(now_ms());
}

static esp_ble_scan_params_t s_scan_params = {
    .scan_type = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval = 0x50,
    .scan_window = 0x30,
    .scan_duplicate = BLE_SCAN_DUPLICATE_ENABLE,
};

static void le_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    switch (event) {
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
        if (s_le_pending_scan_seconds != 0) {
            esp_err_t err = esp_ble_gap_start_scanning(s_le_pending_scan_seconds);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "start_scanning failed: %s", esp_err_to_name(err));
            } else {
                s_le_scanning = true;
            }
            s_le_pending_scan_seconds = 0;
        }
        break;
    }

    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
        switch (param->scan_rst.search_evt) {
        case ESP_GAP_SEARCH_INQ_RES_EVT:
            handle_scan_result(&param->scan_rst);
            break;
        case ESP_GAP_SEARCH_INQ_CMPL_EVT:
            s_le_scanning = false;
            ESP_LOGI(TAG, "scan complete: num_resps=%u", (unsigned)param->scan_rst.num_resps);
            prune_stale_le_devices(now_ms());
            break;
        default:
            break;
        }
        break;
    }

    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
        ESP_LOGI(TAG,
                 "scan start: status=0x%04x (%s) (%s)",
                 (unsigned)param->scan_start_cmpl.status,
                 bt_decode_bt_status_name(param->scan_start_cmpl.status),
                 bt_decode_bt_status_desc(param->scan_start_cmpl.status));
        break;

    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        s_le_scanning = false;
        ESP_LOGI(TAG,
                 "scan stop: status=0x%04x (%s) (%s)",
                 (unsigned)param->scan_stop_cmpl.status,
                 bt_decode_bt_status_name(param->scan_stop_cmpl.status),
                 bt_decode_bt_status_desc(param->scan_stop_cmpl.status));
        prune_stale_le_devices(now_ms());
        if (s_le_pending_connect) {
            char a[18] = {0};
            bda_to_str(s_le_pending_connect_bda, a);
            ESP_LOGI(TAG, "connect pending: starting open now (addr=%s type=%s)",
                     a,
                     (s_le_pending_connect_addr_type == BLE_ADDR_TYPE_PUBLIC) ? "pub" : "rand");
            const bool ok = hid_host_open(HID_HOST_TRANSPORT_LE, s_le_pending_connect_bda, s_le_pending_connect_addr_type);
            if (!ok) {
                ESP_LOGE(TAG, "connect pending: hid_host_open failed");
            }
            s_le_pending_connect = false;
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
        if (s_le_auto_confirm_nc) {
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
        if (s_le_auto_accept_sec_req) {
            (void)esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
        }
        break;

    default:
        break;
    }
}

#if BT_HOST_HAS_BREDR
static void handle_bredr_discovery_result(const esp_bt_gap_cb_param_t *param) {
    if (!param) return;

    const uint8_t *bda = param->disc_res.bda;
    int rssi = 0;
    bool has_rssi = false;
    uint32_t cod = 0;
    bool has_cod = false;
    char name_buf[64] = {0};

    for (int i = 0; i < param->disc_res.num_prop; i++) {
        const esp_bt_gap_dev_prop_t *prop = &param->disc_res.prop[i];
        if (!prop || !prop->val || prop->len <= 0) continue;

        switch (prop->type) {
        case ESP_BT_GAP_DEV_PROP_RSSI:
            rssi = (int)(*(int8_t *)prop->val);
            has_rssi = true;
            break;
        case ESP_BT_GAP_DEV_PROP_COD:
            cod = *(uint32_t *)prop->val;
            has_cod = true;
            break;
        case ESP_BT_GAP_DEV_PROP_BDNAME: {
            int n = prop->len;
            if (n >= (int)sizeof(name_buf)) n = (int)sizeof(name_buf) - 1;
            memcpy(name_buf, prop->val, n);
            name_buf[n] = '\0';
            break;
        }
        case ESP_BT_GAP_DEV_PROP_EIR: {
            uint8_t eir_len = 0;
            uint8_t *eir = (uint8_t *)prop->val;
            uint8_t *name = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_CMPL_LOCAL_NAME, &eir_len);
            if (!name) {
                name = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_SHORT_LOCAL_NAME, &eir_len);
            }
            if (name && eir_len > 0) {
                if (eir_len >= sizeof(name_buf)) eir_len = sizeof(name_buf) - 1;
                memcpy(name_buf, name, eir_len);
                name_buf[eir_len] = '\0';
            }
            break;
        }
        default:
            break;
        }
    }

    update_bredr_device_registry(bda,
                                 has_rssi ? rssi : 0,
                                 has_cod ? cod : 0,
                                 name_buf[0] ? name_buf : NULL);
}

static void bredr_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
    switch (event) {
    case ESP_BT_GAP_DISC_STATE_CHANGED_EVT: {
        const esp_bt_gap_discovery_state_t st = param ? param->disc_st_chg.state : ESP_BT_GAP_DISCOVERY_STOPPED;
        s_bredr_scanning = (st == ESP_BT_GAP_DISCOVERY_STARTED);
        ESP_LOGI(TAG, "bredr scan: %s", s_bredr_scanning ? "started" : "stopped");

        prune_stale_bredr_devices(now_ms());

        if (!s_bredr_scanning && s_bredr_pending_connect) {
            char a[18] = {0};
            bda_to_str(s_bredr_pending_connect_bda, a);
            ESP_LOGI(TAG, "bredr connect pending: starting open now (addr=%s)", a);
            const bool ok = hid_host_open(HID_HOST_TRANSPORT_BR_EDR, s_bredr_pending_connect_bda, 0);
            if (!ok) {
                ESP_LOGE(TAG, "bredr connect pending: hid_host_open failed");
            }
            s_bredr_pending_connect = false;
        }
        break;
    }

    case ESP_BT_GAP_DISC_RES_EVT:
        handle_bredr_discovery_result(param);
        prune_stale_bredr_devices(now_ms());
        break;

    case ESP_BT_GAP_PIN_REQ_EVT:
        if (!param) break;
        ESP_LOGI(TAG,
                 "bredr pin req: addr=%02x:%02x:%02x:%02x:%02x:%02x min_16_digit=%d (use: bt-pin <addr> <pin>)",
                 param->pin_req.bda[0],
                 param->pin_req.bda[1],
                 param->pin_req.bda[2],
                 param->pin_req.bda[3],
                 param->pin_req.bda[4],
                 param->pin_req.bda[5],
                 (int)param->pin_req.min_16_digit);
        break;

    case ESP_BT_GAP_CFM_REQ_EVT:
        if (!param) break;
        ESP_LOGI(TAG,
                 "bredr numeric compare req: addr=%02x:%02x:%02x:%02x:%02x:%02x passkey=%" PRIu32 " (use: bt-confirm <addr> yes|no)",
                 param->cfm_req.bda[0],
                 param->cfm_req.bda[1],
                 param->cfm_req.bda[2],
                 param->cfm_req.bda[3],
                 param->cfm_req.bda[4],
                 param->cfm_req.bda[5],
                 (uint32_t)param->cfm_req.num_val);
        if (s_bredr_auto_confirm_ssp) {
            (void)esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
        }
        break;

    case ESP_BT_GAP_KEY_REQ_EVT:
        if (!param) break;
        ESP_LOGI(TAG,
                 "bredr passkey req: addr=%02x:%02x:%02x:%02x:%02x:%02x (use: bt-passkey <addr> <6digits>)",
                 param->key_req.bda[0],
                 param->key_req.bda[1],
                 param->key_req.bda[2],
                 param->key_req.bda[3],
                 param->key_req.bda[4],
                 param->key_req.bda[5]);
        break;

    case ESP_BT_GAP_KEY_NOTIF_EVT:
        if (!param) break;
        ESP_LOGI(TAG,
                 "bredr passkey notif: addr=%02x:%02x:%02x:%02x:%02x:%02x passkey=%" PRIu32,
                 param->key_notif.bda[0],
                 param->key_notif.bda[1],
                 param->key_notif.bda[2],
                 param->key_notif.bda[3],
                 param->key_notif.bda[4],
                 param->key_notif.bda[5],
                 (uint32_t)param->key_notif.passkey);
        break;

    case ESP_BT_GAP_AUTH_CMPL_EVT:
        if (!param) break;
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(TAG, "bredr auth complete: success name=%s", (const char *)param->auth_cmpl.device_name);
        } else {
            ESP_LOGW(TAG,
                     "bredr auth complete: status=0x%02x (%s) (%s) name=%s",
                     (unsigned)param->auth_cmpl.stat,
                     bt_decode_bt_status_name(param->auth_cmpl.stat),
                     bt_decode_bt_status_desc(param->auth_cmpl.stat),
                     (const char *)param->auth_cmpl.device_name);
        }
        break;

    case ESP_BT_GAP_REMOVE_BOND_DEV_COMPLETE_EVT:
        if (!param) break;
        ESP_LOGI(TAG,
                 "remove bond complete: status=0x%02x (%s) (%s)",
                 (unsigned)param->remove_bond_dev_cmpl.status,
                 bt_decode_bt_status_name(param->remove_bond_dev_cmpl.status),
                 bt_decode_bt_status_desc(param->remove_bond_dev_cmpl.status));
        break;

    default:
        break;
    }
}
#endif

static void gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) {
    // Critical: forward GATTC events so the HID host can function.
    hid_host_forward_gattc_event((int)event, (int)gattc_if, (void *)param);

    switch (event) {
    case ESP_GATTC_CONNECT_EVT:
        if (param) {
            s_le_connected = true;
            memcpy(s_le_connected_bda, param->connect.remote_bda, sizeof(s_le_connected_bda));
        }
        ESP_LOGI(TAG, "gattc connect: conn_id=%u", (unsigned)param->connect.conn_id);
        break;
    case ESP_GATTC_OPEN_EVT: {
        char a[18] = {0};
        bda_to_str(param->open.remote_bda, a);
        ESP_LOGI(TAG,
                 "gattc open: status=0x%02x (%s) (%s) conn_id=%u mtu=%u addr=%s",
                 (unsigned)param->open.status,
                 bt_decode_gatt_status_name(param->open.status),
                 bt_decode_gatt_status_desc(param->open.status),
                 (unsigned)param->open.conn_id,
                 (unsigned)param->open.mtu,
                 a);

        if (param->open.status == ESP_GATT_OK) {
            s_le_connected = true;
            memcpy(s_le_connected_bda, param->open.remote_bda, sizeof(s_le_connected_bda));
        }

        if (s_le_pending_pair && bda_equal(s_le_pending_pair_bda, param->open.remote_bda)) {
            if (param->open.status != ESP_GATT_OK) {
                ESP_LOGW(TAG, "pair pending: open failed; not starting encryption");
            } else {
                ESP_LOGI(TAG, "pair pending: starting encryption now");
                (void)start_encryption_for_peer(param->open.remote_bda);
            }
            s_le_pending_pair = false;
        }
        break;
    }
    case ESP_GATTC_DISCONNECT_EVT:
        ESP_LOGI(TAG,
                 "gattc disconnect: reason=0x%04x (%s) (%s) conn_id=%u",
                 (unsigned)param->disconnect.reason,
                 bt_decode_gatt_conn_reason_name(param->disconnect.reason),
                 bt_decode_gatt_conn_reason_desc(param->disconnect.reason),
                 (unsigned)param->disconnect.conn_id);
        clear_le_connection_state();
        s_le_pending_pair = false;
        break;
    case ESP_GATTC_CLOSE_EVT:
        ESP_LOGI(TAG,
                 "gattc close: status=0x%02x (%s) (%s) reason=0x%04x (%s) (%s) conn_id=%u",
                 (unsigned)param->close.status,
                 bt_decode_gatt_status_name(param->close.status),
                 bt_decode_gatt_status_desc(param->close.status),
                 (unsigned)param->close.reason,
                 bt_decode_gatt_conn_reason_name(param->close.reason),
                 bt_decode_gatt_conn_reason_desc(param->close.reason),
                 (unsigned)param->close.conn_id);
        clear_le_connection_state();
        s_le_pending_pair = false;
        break;
    default:
        break;
    }
}

static void set_le_security_defaults(void) {
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

static void set_bredr_security_defaults(void) {
#if BT_HOST_HAS_BREDR
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_NONE;
    ESP_ERROR_CHECK(esp_bt_gap_set_security_param(ESP_BT_SP_IOCAP_MODE, &iocap, sizeof(iocap)));

    // Use variable PIN: request will surface via ESP_BT_GAP_PIN_REQ_EVT.
    esp_bt_pin_code_t pin_code;
    memset(pin_code, 0, sizeof(pin_code));
    ESP_ERROR_CHECK(esp_bt_gap_set_pin(ESP_BT_PIN_TYPE_VARIABLE, 0, pin_code));
#else
    // BR/EDR is not supported on ESP32-S3 targets.
#endif
}

void bt_host_init(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

#if !BT_HOST_HAS_BREDR
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
#endif

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
#if BT_HOST_HAS_BREDR
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BTDM));
#else
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
#endif
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    ESP_ERROR_CHECK(esp_ble_gap_register_callback(le_gap_cb));
    ESP_ERROR_CHECK(esp_ble_gattc_register_callback(gattc_cb));

#if BT_HOST_HAS_BREDR
    ESP_ERROR_CHECK(esp_bt_gap_register_callback(bredr_gap_cb));
    ESP_ERROR_CHECK(esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_NON_DISCOVERABLE));
#endif
    set_le_security_defaults();
    set_bredr_security_defaults();

#if BT_HOST_HAS_BREDR
    ESP_LOGI(TAG, "initialized (BTDM)");
#else
    ESP_LOGI(TAG, "initialized (BLE-only)");
#endif
}

void bt_host_scan_le_start(uint32_t seconds) {
    if (seconds == 0) seconds = 30;

    // Avoid running both discovery mechanisms at the same time.
    bt_host_scan_bredr_stop();

    s_le_pending_scan_seconds = seconds;
    esp_err_t err = esp_ble_gap_set_scan_params(&s_scan_params);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "set_scan_params failed: %s", esp_err_to_name(err));
        s_le_pending_scan_seconds = 0;
    }
}

void bt_host_scan_le_stop(void) {
    if (!s_le_scanning) {
        ESP_LOGI(TAG, "scan already stopped");
        return;
    }
    esp_err_t err = esp_ble_gap_stop_scanning();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "stop_scanning failed: %s", esp_err_to_name(err));
        // If we were stopping scan in order to connect, we may never receive
        // ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT; fail "open" forward here.
        s_le_scanning = false;
        if (s_le_pending_connect) {
            char a[18] = {0};
            bda_to_str(s_le_pending_connect_bda, a);
            ESP_LOGI(TAG, "connect pending: starting open now after stop_scanning error (addr=%s type=%s)",
                     a,
                     (s_le_pending_connect_addr_type == BLE_ADDR_TYPE_PUBLIC) ? "pub" : "rand");
            const bool ok = hid_host_open(HID_HOST_TRANSPORT_LE, s_le_pending_connect_bda, s_le_pending_connect_addr_type);
            if (!ok) {
                ESP_LOGE(TAG, "connect pending: hid_host_open failed");
            }
            s_le_pending_connect = false;
        }
    }
}

static bool strcasestr_simple(const char *haystack, const char *needle) {
    if (!needle || !*needle) return true;
    if (!haystack) return false;
    const size_t nlen = strlen(needle);
    for (const char *p = haystack; *p; p++) {
        size_t i = 0;
        for (; i < nlen; i++) {
            const unsigned char hc = (unsigned char)p[i];
            if (!hc) break;
            const unsigned char nc = (unsigned char)needle[i];
            if ((unsigned char)tolower(hc) != (unsigned char)tolower(nc)) break;
        }
        if (i == nlen) return true;
    }
    return false;
}

void bt_host_devices_le_print(const char *filter_substr) {
    const char *filter = (filter_substr && *filter_substr) ? filter_substr : NULL;
    printf("devices le: %d/%d (most recently seen)%s%s\n",
           s_le_num_devices,
           MAX_LE_DEVICES,
           filter ? " filter=" : "",
           filter ? filter : "");
    printf("idx  addr              type rssi  age_ms  name\n");
    const uint32_t now = now_ms();
    for (int i = 0; i < s_le_num_devices; i++) {
        char a[18] = {0};
        bda_to_str(s_le_devices[i].bda, a);
        const uint32_t age = now - s_le_devices[i].last_seen_ms;
        if (filter) {
            if (!strcasestr_simple(a, filter) && !strcasestr_simple(s_le_devices[i].name, filter)) {
                continue;
            }
        }
        printf("%-4d %-17s %-4s %-5d %-7" PRIu32 " %s\n",
               i,
               a,
               (s_le_devices[i].addr_type == BLE_ADDR_TYPE_PUBLIC) ? "pub" : "rand",
               s_le_devices[i].rssi,
               age,
               s_le_devices[i].name[0] ? s_le_devices[i].name : "(no name)");
    }
}

void bt_host_devices_le_clear(void) {
    for (int i = s_le_num_devices - 1; i >= 0; i--) {
        remove_le_device_at(i);
    }
    memset(s_le_devices, 0, sizeof(s_le_devices));
    s_le_num_devices = 0;
    s_le_last_prune_ms = 0;
}

void bt_host_devices_le_events_set_enabled(bool enabled) {
    s_le_device_events_enabled = enabled;
}

bool bt_host_devices_le_events_get_enabled(void) {
    return s_le_device_events_enabled;
}

bool bt_host_le_device_get_by_index(int index, bt_le_device_t *out) {
    if (!out) return false;
    if (index < 0 || index >= s_le_num_devices) return false;
    *out = s_le_devices[index];
    return true;
}

bool bt_host_le_device_get_by_bda(const uint8_t bda[6], bt_le_device_t *out) {
    if (!bda || !out) return false;
    for (int i = 0; i < s_le_num_devices; i++) {
        if (bda_equal(s_le_devices[i].bda, bda)) {
            *out = s_le_devices[i];
            return true;
        }
    }
    return false;
}

bool bt_host_le_connect(const uint8_t bda[6], uint8_t addr_type) {
    if (!bda) return false;
    if (s_le_scanning) {
        memcpy(s_le_pending_connect_bda, bda, sizeof(s_le_pending_connect_bda));
        s_le_pending_connect_addr_type = addr_type;
        s_le_pending_connect = true;
        bt_host_scan_le_stop();
        return true;
    }
    return hid_host_open(HID_HOST_TRANSPORT_LE, bda, addr_type);
}

void bt_host_disconnect(void) {
    hid_host_close();
}

bool bt_host_le_pair(const uint8_t bda[6], uint8_t addr_type) {
    if (!bda) return false;

    if (s_le_connected) {
        if (!bda_equal(s_le_connected_bda, bda)) {
            char want[18] = {0};
            char have[18] = {0};
            bda_to_str(bda, want);
            bda_to_str(s_le_connected_bda, have);
            ESP_LOGE(TAG, "pair: already connected to %s; disconnect first (wanted %s)", have, want);
            return false;
        }
        return start_encryption_for_peer(bda);
    }

    s_le_pending_pair = true;
    memcpy(s_le_pending_pair_bda, bda, sizeof(s_le_pending_pair_bda));
    return bt_host_le_connect(bda, addr_type);
}

void bt_host_le_bonds_print(void) {
    int n = esp_ble_get_bond_device_num();
    if (n <= 0) {
        printf("bonds le: (none)\n");
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

    printf("bonds le (%d):\n", want);
    for (int i = 0; i < want; i++) {
        char a[18] = {0};
        bda_to_str(list[i].bd_addr, a);
        printf("  %d: %s\n", i, a);
    }
}

bool bt_host_le_unpair(const uint8_t bda[6]) {
    if (!bda) return false;
    esp_err_t err = esp_ble_remove_bond_device((uint8_t *)bda);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "remove_bond_device failed: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

bool bt_host_le_sec_accept(const uint8_t bda[6], bool accept) {
    if (!bda) return false;
    esp_err_t err = esp_ble_gap_security_rsp((uint8_t *)bda, accept);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "security_rsp failed: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

bool bt_host_le_passkey_reply(const uint8_t bda[6], bool accept, uint32_t passkey) {
    if (!bda) return false;
    esp_err_t err = esp_ble_passkey_reply((uint8_t *)bda, accept, passkey);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "passkey_reply failed: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

bool bt_host_le_confirm_reply(const uint8_t bda[6], bool accept) {
    if (!bda) return false;
    esp_err_t err = esp_ble_confirm_reply((uint8_t *)bda, accept);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "confirm_reply failed: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

#if BT_HOST_HAS_BREDR
static uint8_t seconds_to_inq_len(uint32_t seconds) {
    if (seconds == 0) seconds = 10;
    // inq_len uses 1.28s units.
    uint32_t inq_len = (seconds * 100 + 127) / 128;
    if (inq_len < ESP_BT_GAP_MIN_INQ_LEN) inq_len = ESP_BT_GAP_MIN_INQ_LEN;
    if (inq_len > ESP_BT_GAP_MAX_INQ_LEN) inq_len = ESP_BT_GAP_MAX_INQ_LEN;
    return (uint8_t)inq_len;
}

void bt_host_scan_bredr_start(uint32_t seconds) {
    // Avoid running both discovery mechanisms at the same time.
    bt_host_scan_le_stop();

    const uint8_t inq_len = seconds_to_inq_len(seconds);
    esp_err_t err = esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, inq_len, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "bredr start_discovery failed: %s", esp_err_to_name(err));
        return;
    }
    // The state callback will set s_bredr_scanning.
}

void bt_host_scan_bredr_stop(void) {
    if (!s_bredr_scanning) {
        ESP_LOGI(TAG, "bredr scan already stopped");
        return;
    }
    esp_err_t err = esp_bt_gap_cancel_discovery();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "bredr cancel_discovery failed: %s", esp_err_to_name(err));
        s_bredr_scanning = false;
        if (s_bredr_pending_connect) {
            char a[18] = {0};
            bda_to_str(s_bredr_pending_connect_bda, a);
            ESP_LOGI(TAG, "bredr connect pending: starting open now after cancel_discovery error (addr=%s)", a);
            const bool ok = hid_host_open(HID_HOST_TRANSPORT_BR_EDR, s_bredr_pending_connect_bda, 0);
            if (!ok) {
                ESP_LOGE(TAG, "bredr connect pending: hid_host_open failed");
            }
            s_bredr_pending_connect = false;
        }
    }
}

void bt_host_devices_bredr_print(const char *filter_substr) {
    const char *filter = (filter_substr && *filter_substr) ? filter_substr : NULL;
    printf("devices bredr: %d/%d (most recently seen)%s%s\n",
           s_bredr_num_devices,
           MAX_BREDR_DEVICES,
           filter ? " filter=" : "",
           filter ? filter : "");
    printf("idx  addr              rssi  age_ms  cod       kbd  name\n");
    const uint32_t now = now_ms();
    for (int i = 0; i < s_bredr_num_devices; i++) {
        char a[18] = {0};
        bda_to_str(s_bredr_devices[i].bda, a);
        const uint32_t age = now - s_bredr_devices[i].last_seen_ms;
        if (filter) {
            if (!strcasestr_simple(a, filter) && !strcasestr_simple(s_bredr_devices[i].name, filter)) {
                continue;
            }
        }
        printf("%-4d %-17s %-5d %-7" PRIu32 " 0x%06" PRIx32 " %-4s %s\n",
               i,
               a,
               s_bredr_devices[i].rssi,
               age,
               (uint32_t)(s_bredr_devices[i].cod & 0x00FFFFFFu),
               cod_likely_keyboard(s_bredr_devices[i].cod) ? "yes" : "no",
               s_bredr_devices[i].name[0] ? s_bredr_devices[i].name : "(no name)");
    }
}

void bt_host_devices_bredr_clear(void) {
    for (int i = s_bredr_num_devices - 1; i >= 0; i--) {
        remove_bredr_device_at(i);
    }
    memset(s_bredr_devices, 0, sizeof(s_bredr_devices));
    s_bredr_num_devices = 0;
    s_bredr_last_prune_ms = 0;
}

void bt_host_devices_bredr_events_set_enabled(bool enabled) {
    s_bredr_device_events_enabled = enabled;
}

bool bt_host_devices_bredr_events_get_enabled(void) {
    return s_bredr_device_events_enabled;
}

bool bt_host_bredr_device_get_by_index(int index, bt_bredr_device_t *out) {
    if (!out) return false;
    if (index < 0 || index >= s_bredr_num_devices) return false;
    *out = s_bredr_devices[index];
    return true;
}

bool bt_host_bredr_device_get_by_bda(const uint8_t bda[6], bt_bredr_device_t *out) {
    if (!bda || !out) return false;
    for (int i = 0; i < s_bredr_num_devices; i++) {
        if (bda_equal(s_bredr_devices[i].bda, bda)) {
            *out = s_bredr_devices[i];
            return true;
        }
    }
    return false;
}

bool bt_host_bredr_connect(const uint8_t bda[6]) {
    if (!bda) return false;
    if (s_bredr_scanning) {
        memcpy(s_bredr_pending_connect_bda, bda, sizeof(s_bredr_pending_connect_bda));
        s_bredr_pending_connect = true;
        bt_host_scan_bredr_stop();
        return true;
    }
    if (s_le_scanning) {
        bt_host_scan_le_stop();
    }
    return hid_host_open(HID_HOST_TRANSPORT_BR_EDR, bda, 0);
}

bool bt_host_bredr_pair(const uint8_t bda[6]) {
    return bt_host_bredr_connect(bda);
}

void bt_host_bredr_bonds_print(void) {
    int n = esp_bt_gap_get_bond_device_num();
    if (n <= 0) {
        printf("bonds bredr: (none)\n");
        return;
    }

    esp_bd_addr_t list[16];
    if (n > (int)(sizeof(list) / sizeof(list[0]))) n = (int)(sizeof(list) / sizeof(list[0]));
    int want = n;
    esp_err_t err = esp_bt_gap_get_bond_device_list(&want, list);
    if (err != ESP_OK) {
        printf("bonds bredr: error: %s\n", esp_err_to_name(err));
        return;
    }

    printf("bonds bredr (%d):\n", want);
    for (int i = 0; i < want; i++) {
        char a[18] = {0};
        bda_to_str(list[i], a);
        printf("  %d: %s\n", i, a);
    }
}

bool bt_host_bredr_unpair(const uint8_t bda[6]) {
    if (!bda) return false;
    esp_err_t err = esp_bt_gap_remove_bond_device((uint8_t *)bda);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "bredr remove_bond_device failed: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

bool bt_host_bredr_pin_reply(const uint8_t bda[6], bool accept, const char *pin) {
    if (!bda) return false;
    esp_bt_pin_code_t pin_code;
    memset(pin_code, 0, sizeof(pin_code));
    uint8_t pin_len = 0;
    if (accept) {
        if (!pin || !*pin) {
            ESP_LOGE(TAG, "bt-pin: missing pin");
            return false;
        }
        size_t n = strlen(pin);
        if (n > ESP_BT_PIN_CODE_LEN) n = ESP_BT_PIN_CODE_LEN;
        pin_len = (uint8_t)n;
        memcpy(pin_code, pin, pin_len);
    }
    esp_err_t err = esp_bt_gap_pin_reply((uint8_t *)bda, accept, pin_len, pin_code);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "pin_reply failed: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

bool bt_host_bredr_ssp_passkey_reply(const uint8_t bda[6], bool accept, uint32_t passkey) {
    if (!bda) return false;
    esp_err_t err = esp_bt_gap_ssp_passkey_reply((uint8_t *)bda, accept, passkey);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ssp_passkey_reply failed: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

bool bt_host_bredr_ssp_confirm_reply(const uint8_t bda[6], bool accept) {
    if (!bda) return false;
    esp_err_t err = esp_bt_gap_ssp_confirm_reply((uint8_t *)bda, accept);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ssp_confirm_reply failed: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}
#else
void bt_host_scan_bredr_start(uint32_t) {
    printf("error: BR/EDR not supported on this target\n");
}

void bt_host_scan_bredr_stop(void) {
    // no-op
}

void bt_host_devices_bredr_print(const char *) {
    printf("devices bredr: not supported on this target\n");
}

void bt_host_devices_bredr_clear(void) {
    // no-op
}

void bt_host_devices_bredr_events_set_enabled(bool) {
    // no-op
}

bool bt_host_devices_bredr_events_get_enabled(void) {
    return false;
}

bool bt_host_bredr_device_get_by_index(int, bt_bredr_device_t *) {
    return false;
}

bool bt_host_bredr_device_get_by_bda(const uint8_t[6], bt_bredr_device_t *) {
    return false;
}

bool bt_host_bredr_connect(const uint8_t[6]) {
    ESP_LOGE(TAG, "bredr connect: not supported on this target");
    return false;
}

bool bt_host_bredr_pair(const uint8_t[6]) {
    ESP_LOGE(TAG, "bredr pair: not supported on this target");
    return false;
}

void bt_host_bredr_bonds_print(void) {
    printf("bonds bredr: not supported on this target\n");
}

bool bt_host_bredr_unpair(const uint8_t[6]) {
    ESP_LOGE(TAG, "bredr unpair: not supported on this target");
    return false;
}

bool bt_host_bredr_pin_reply(const uint8_t[6], bool, const char *) {
    ESP_LOGE(TAG, "bredr pin reply: not supported on this target");
    return false;
}

bool bt_host_bredr_ssp_passkey_reply(const uint8_t[6], bool, uint32_t) {
    ESP_LOGE(TAG, "bredr passkey reply: not supported on this target");
    return false;
}

bool bt_host_bredr_ssp_confirm_reply(const uint8_t[6], bool) {
    ESP_LOGE(TAG, "bredr confirm reply: not supported on this target");
    return false;
}
#endif
