---
Title: 'BLE GATT Notify Server and HID Keyboard Host: Two Recipes for ESP32-S3'
Ticket: MO-02-ESP32-DOCS
Status: active
Topics:
    - esp32
    - firmware
    - documentation
    - ble
    - gatt
    - hid
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/0019-cardputer-ble-temp-logger/main/ble_temp_logger_main.c
      Note: GATT notify server implementation
    - Path: esp32-s3-m5/0020-cardputer-ble-keyboard-host/main/bt_host.c
      Note: BLE HID host implementation
    - Path: esp32-s3-m5/0019-cardputer-ble-temp-logger/tools/ble_client.py
      Note: Python test client
ExternalSources:
    - URL: https://docs.espressif.com/projects/esp-idf/en/v5.4/esp32s3/api-reference/bluetooth/esp_gatts.html
      Note: ESP-IDF GATTS API
    - URL: https://docs.espressif.com/projects/esp-idf/en/v5.4/esp32s3/api-reference/bluetooth/esp_gattc.html
      Note: ESP-IDF GATTC API
Summary: 'Two practical BLE recipes: a GATT notify server for temperature streaming, and a BLE HID keyboard host with console-driven pairing.'
LastUpdated: 2026-01-12T14:00:00-05:00
WhatFor: 'Teach developers BLE peripheral and central patterns for ESP32-S3.'
WhenToUse: 'Use when implementing BLE temperature sensors or HID keyboard hosts.'
---

# BLE GATT Notify Server and HID Keyboard Host: Two Recipes for ESP32-S3

## Introduction

Bluetooth Low Energy (BLE) is essential for IoT devices, wearables, and wireless peripherals. This guide presents two practical recipes:

1. **GATT Notify Server** — A peripheral that streams temperature readings via notifications
2. **HID Keyboard Host** — A central that scans, pairs with, and receives input from BLE keyboards

Both recipes are complete, tested, and ready to adapt for your projects.

---

# Recipe 1: GATT Notify Server (Temperature Streamer)

## Overview

This recipe implements a BLE peripheral that:
- Advertises as "CP_TEMP_LOGGER"
- Exposes a custom GATT service with temperature and control characteristics
- Streams mock temperature values via notifications
- Allows clients to adjust the notification period

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         ESP32-S3 (Peripheral)                          │
│                                                                         │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                     GATT Service 0xFFF0                         │   │
│  │  ┌─────────────────────────────────────────────────────────┐    │   │
│  │  │  Temperature Char 0xFFF1                                 │    │   │
│  │  │  - Properties: READ, NOTIFY                              │    │   │
│  │  │  - Format: int16_le (centi-degrees Celsius)              │    │   │
│  │  │  - CCCD: 0x2902 (notification enable)                    │    │   │
│  │  └─────────────────────────────────────────────────────────┘    │   │
│  │  ┌─────────────────────────────────────────────────────────┐    │   │
│  │  │  Control Char 0xFFF2                                     │    │   │
│  │  │  - Properties: WRITE                                     │    │   │
│  │  │  - Format: uint16_le (notify period in milliseconds)     │    │   │
│  │  └─────────────────────────────────────────────────────────┘    │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  ┌──────────────┐                                                      │
│  │ Notify Task  │ ──────► esp_ble_gatts_send_indicate()               │
│  │ (1s period)  │                                                      │
│  └──────────────┘                                                      │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    │ BLE Notifications
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                      BLE Central (Phone/PC)                            │
│                                                                         │
│  - Connect to "CP_TEMP_LOGGER"                                         │
│  - Enable notifications on 0xFFF1                                      │
│  - Receive temperature updates                                         │
│  - Optionally write to 0xFFF2 to change period                         │
└─────────────────────────────────────────────────────────────────────────┘
```

## GATT Schema

| Element | UUID | Properties | Format |
|---------|------|------------|--------|
| Service | 0xFFF0 | — | — |
| Temperature | 0xFFF1 | READ, NOTIFY | int16_le (value × 100 = °C) |
| Temperature CCCD | 0x2902 | READ, WRITE | uint16_le (0x0001 = enable) |
| Control | 0xFFF2 | WRITE | uint16_le (period in ms, 0 = disabled) |

**Temperature format**: The value 2150 represents 21.50°C.

## Attribute Table Setup

ESP-IDF uses a static attribute table pattern:

```c
// Index enum for handle table
enum {
    IDX_SVC,
    IDX_CHAR_TEMP_DECL,
    IDX_CHAR_TEMP_VALUE,
    IDX_CHAR_TEMP_CCCD,
    IDX_CHAR_CTRL_DECL,
    IDX_CHAR_CTRL_VALUE,
    IDX_NB,  // Total count
};

static uint16_t s_handle_table[IDX_NB];

// Static UUIDs
static const uint16_t primary_service_uuid = 0x2800;
static const uint16_t char_decl_uuid = 0x2803;
static const uint16_t cccd_uuid = 0x2902;
static const uint16_t svc_uuid = 0xFFF0;
static const uint16_t temp_uuid = 0xFFF1;
static const uint16_t ctrl_uuid = 0xFFF2;

// Property flags
static const uint8_t char_prop_read_notify = 
    ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint8_t char_prop_write = 
    ESP_GATT_CHAR_PROP_BIT_WRITE;

// Initial values
static uint8_t temp_value[2] = {0x00, 0x00};
static uint8_t ctrl_value[2] = {0xE8, 0x03};  // 1000ms
static uint8_t cccd_value[2] = {0x00, 0x00};

// Attribute table
static const esp_gatts_attr_db_t gatt_db[IDX_NB] = {
    // Service Declaration
    [IDX_SVC] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
         sizeof(uint16_t), sizeof(svc_uuid), (uint8_t *)&svc_uuid},
    },

    // Temperature Characteristic Declaration
    [IDX_CHAR_TEMP_DECL] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&char_decl_uuid, ESP_GATT_PERM_READ,
         sizeof(uint8_t), sizeof(uint8_t), (uint8_t *)&char_prop_read_notify},
    },

    // Temperature Characteristic Value
    [IDX_CHAR_TEMP_VALUE] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&temp_uuid, ESP_GATT_PERM_READ,
         sizeof(temp_value), sizeof(temp_value), temp_value},
    },

    // Temperature CCCD
    [IDX_CHAR_TEMP_CCCD] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&cccd_uuid, 
         ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
         sizeof(cccd_value), sizeof(cccd_value), cccd_value},
    },

    // Control Characteristic Declaration
    [IDX_CHAR_CTRL_DECL] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&char_decl_uuid, ESP_GATT_PERM_READ,
         sizeof(uint8_t), sizeof(uint8_t), (uint8_t *)&char_prop_write},
    },

    // Control Characteristic Value
    [IDX_CHAR_CTRL_VALUE] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&ctrl_uuid, ESP_GATT_PERM_WRITE,
         sizeof(ctrl_value), sizeof(ctrl_value), ctrl_value},
    },
};
```

## BLE Stack Initialization

```c
static void ble_init(void) {
    // NVS required for BLE bonding
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || 
        err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    // Release Classic BT memory (BLE-only)
    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);

    // Initialize BT controller
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);
    esp_bt_controller_enable(ESP_BT_MODE_BLE);

    // Initialize Bluedroid
    esp_bluedroid_init();
    esp_bluedroid_enable();

    // Register callbacks
    esp_ble_gap_register_callback(gap_event_handler);
    esp_ble_gatts_register_callback(gatts_event_handler);
    esp_ble_gatts_app_register(PROFILE_APP_ID);
}
```

## Extended Advertising (BLE 5.0)

ESP32-S3 supports BLE 5.0 extended advertising:

```c
static esp_ble_gap_ext_adv_params_t s_ext_adv_params = {
    .type = ESP_BLE_GAP_SET_EXT_ADV_PROP_CONNECTABLE,
    .interval_min = 0x20,  // 20ms
    .interval_max = 0x40,  // 40ms
    .channel_map = ADV_CHNL_ALL,
    .filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
    .primary_phy = ESP_BLE_GAP_PHY_1M,
    .secondary_phy = ESP_BLE_GAP_PHY_1M,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .tx_power = EXT_ADV_TX_PWR_NO_PREFERENCE,
};

// Raw advertising data
static const uint8_t s_ext_adv_raw_data[] = {
    0x02, ESP_BLE_AD_TYPE_FLAG, 0x06,  // Flags: LE General Discoverable
    0x03, ESP_BLE_AD_TYPE_16SRV_CMPL, 0xF0, 0xFF,  // Service UUID 0xFFF0
    0x0F, ESP_BLE_AD_TYPE_NAME_CMPL,
    'C','P','_','T','E','M','P','_','L','O','G','G','E','R',
};

// Start advertising (callback-driven sequence)
static void start_advertising(void) {
    esp_ble_gap_ext_adv_set_params(EXT_ADV_HANDLE, &s_ext_adv_params);
    // In callback: esp_ble_gap_config_ext_adv_data_raw()
    // In callback: esp_ble_gap_ext_adv_start()
}
```

## GATTS Event Handler

```c
static void gatts_event_handler(esp_gatts_cb_event_t event, 
                                 esp_gatt_if_t gatts_if,
                                 esp_ble_gatts_cb_param_t *param) {
    switch (event) {
    case ESP_GATTS_REG_EVT:
        // Set device name
        esp_ble_gap_set_device_name("CP_TEMP_LOGGER");
        
        // Create attribute table
        esp_ble_gatts_create_attr_tab(gatt_db, gatts_if, IDX_NB, 0);
        break;

    case ESP_GATTS_CREAT_ATTR_TAB_EVT:
        // Save handles
        memcpy(s_handle_table, param->add_attr_tab.handles, 
               sizeof(uint16_t) * IDX_NB);
        
        // Start service
        esp_ble_gatts_start_service(s_handle_table[IDX_SVC]);
        
        // Start advertising
        start_advertising();
        break;

    case ESP_GATTS_CONNECT_EVT:
        s_connected = true;
        s_conn_id = param->connect.conn_id;
        s_gatts_if = gatts_if;
        break;

    case ESP_GATTS_DISCONNECT_EVT:
        s_connected = false;
        s_notify_enabled = false;
        start_advertising();  // Resume advertising
        break;

    case ESP_GATTS_WRITE_EVT:
        if (param->write.handle == s_handle_table[IDX_CHAR_TEMP_CCCD]) {
            // CCCD write: enable/disable notifications
            uint16_t v = param->write.value[0] | (param->write.value[1] << 8);
            s_notify_enabled = (v == 0x0001);
        }
        else if (param->write.handle == s_handle_table[IDX_CHAR_CTRL_VALUE]) {
            // Control write: change notify period
            uint16_t period = param->write.value[0] | (param->write.value[1] << 8);
            s_period_ms = period;
            xTaskNotifyGive(s_notify_task);  // Wake task to apply
        }
        break;
    }
}
```

## Notification Task

```c
static void notify_task(void *arg) {
    uint32_t seq = 0;

    while (true) {
        // Generate mock temperature (20.00°C to 29.99°C cycling)
        int16_t temp_centi = 2000 + (int16_t)(seq % 1000);
        seq++;

        // Update readable value
        uint8_t v[2] = {temp_centi & 0xFF, (temp_centi >> 8) & 0xFF};
        esp_ble_gatts_set_attr_value(s_handle_table[IDX_CHAR_TEMP_VALUE], 
                                      sizeof(v), v);

        // Send notification if enabled
        if (s_connected && s_notify_enabled && s_period_ms != 0) {
            esp_err_t err = esp_ble_gatts_send_indicate(
                s_gatts_if,
                s_conn_id,
                s_handle_table[IDX_CHAR_TEMP_VALUE],
                sizeof(v),
                v,
                false  // false = notification, true = indication
            );
            if (err != ESP_OK) {
                ESP_LOGW(TAG, "notify failed: %s", esp_err_to_name(err));
            }
        }

        // Sleep with task notification to handle period changes
        if (s_period_ms == 0) {
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        } else {
            ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(s_period_ms));
        }
    }
}
```

## Testing with Python (bleak)

The included `ble_client.py` uses the `bleak` library:

```bash
# Install bleak
pip install bleak

# Scan for devices
python ble_client.py scan

# Connect and read temperature
python ble_client.py run --read

# Subscribe to notifications for 30 seconds
python ble_client.py run --notify --duration 30

# Change notify period to 500ms
python ble_client.py run --set-period-ms 500 --notify --duration 10

# Stress test: reconnect 25 times
python ble_client.py stress --reconnects 25 --read
```

**Example output:**

```
selected: AA:BB:CC:DD:EE:FF CP_TEMP_LOGGER
connected: AA:BB:CC:DD:EE:FF (mtu=65)
read temp: 21.50 C (raw=6608)
subscribed for 10.0s ...
notify temp: 21.51 C (raw=6708)
notify temp: 21.52 C (raw=6808)
...
unsubscribed
```

---

# Recipe 2: BLE HID Keyboard Host

## Overview

This recipe implements a BLE central that:
- Scans for BLE peripherals (including HID keyboards)
- Provides an `esp_console` REPL for interactive control
- Connects to and pairs with BLE keyboards
- Decodes and logs HID keyboard reports

```
┌────────────────────────────────────────────────────────────────────────┐
│                     ESP32-S3 (Central / Host)                         │
│                                                                        │
│  ┌──────────────────────────────────────────────────────────────┐     │
│  │                    esp_console REPL                          │     │
│  │  ble> scan on 30                                             │     │
│  │  ble> devices                                                │     │
│  │  ble> connect 0                                              │     │
│  │  ble> pair 0                                                 │     │
│  │  ble> keylog on                                              │     │
│  └──────────────────────────────────────────────────────────────┘     │
│                                                                        │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐            │
│  │  bt_host.c   │    │  hid_host.c  │    │  keylog.c    │            │
│  │  - Scanning  │    │  - GATTC     │    │  - Report    │            │
│  │  - Pairing   │───►│  - HID svc   │───►│    decode    │            │
│  │  - Security  │    │  - Reports   │    │  - Console   │            │
│  └──────────────┘    └──────────────┘    └──────────────┘            │
└────────────────────────────────────────────────────────────────────────┘
                                    │
                                    │ HID Reports
                                    ▲
┌────────────────────────────────────────────────────────────────────────┐
│                   BLE HID Keyboard (Peripheral)                        │
│                                                                        │
│  - Advertising as keyboard                                            │
│  - HID over GATT Profile (HOGP)                                       │
│  - Sends boot keyboard reports                                        │
└────────────────────────────────────────────────────────────────────────┘
```

## Console Commands Reference

| Command | Description | Example |
|---------|-------------|---------|
| `scan on [sec]` | Start LE scan (default 30s) | `scan on 60` |
| `scan off` | Stop LE scan | `scan off` |
| `devices` | List all discovered devices | `devices` |
| `devices <filter>` | Filter by name/address | `devices keyboard` |
| `devices clear` | Clear device list | `devices clear` |
| `devices events on/off` | Toggle discovery events | `devices events off` |
| `connect <idx>` | Connect to device by index | `connect 0` |
| `connect <addr> [pub/rand]` | Connect by address | `connect AA:BB:CC:DD:EE:FF rand` |
| `pair <idx>` | Pair with device by index | `pair 0` |
| `pair <addr> [pub/rand]` | Pair by address | `pair AA:BB:CC:DD:EE:FF pub` |
| `disconnect` | Disconnect current device | `disconnect` |
| `keylog on/off` | Enable/disable key logging | `keylog on` |
| `bonds` | List bonded devices | `bonds` |
| `unpair <idx/addr>` | Remove bond | `unpair 0` |

## Scanning for Devices

```c
static esp_ble_scan_params_t s_scan_params = {
    .scan_type = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval = 0x50,  // 50ms
    .scan_window = 0x30,    // 30ms
    .scan_duplicate = BLE_SCAN_DUPLICATE_ENABLE,
};

void bt_host_scan_le_start(uint32_t seconds) {
    if (seconds == 0) seconds = 30;
    
    s_le_pending_scan_seconds = seconds;
    esp_ble_gap_set_scan_params(&s_scan_params);
    // In callback: esp_ble_gap_start_scanning(seconds)
}

// Handle scan results
static void handle_scan_result(const struct ble_scan_result_evt_param *scan_rst) {
    // Extract device name from advertising data
    uint8_t name_len = 0;
    uint8_t *name = esp_ble_resolve_adv_data_by_type(
        scan_rst->ble_adv,
        scan_rst->adv_data_len + scan_rst->scan_rsp_len,
        ESP_BLE_AD_TYPE_NAME_CMPL,
        &name_len
    );

    // Update device registry
    update_le_device_registry(
        scan_rst->bda,
        scan_rst->ble_addr_type,
        scan_rst->rssi,
        name
    );
}
```

## Device Registry

The host maintains a registry of discovered devices:

```c
typedef struct {
    uint8_t bda[6];           // Bluetooth Device Address
    uint8_t addr_type;        // BLE_ADDR_TYPE_PUBLIC or _RANDOM
    int rssi;                 // Signal strength
    char name[32];            // Device name
    uint32_t last_seen_ms;    // For stale device pruning
} bt_le_device_t;

#define MAX_LE_DEVICES 100
static bt_le_device_t s_le_devices[MAX_LE_DEVICES];
static int s_le_num_devices;
```

**Device list output:**

```
ble> devices
devices le: 3/100 (most recently seen)
idx  addr              type rssi  age_ms  name
0    aa:bb:cc:dd:ee:ff rand -52   1234    BT-KBD-2000
1    11:22:33:44:55:66 pub  -68   5678    Logitech K380
2    de:ad:be:ef:ca:fe rand -75   9012    (no name)
```

## Connect and Pair Flow

### Connection

```c
bool bt_host_le_connect(const uint8_t bda[6], uint8_t addr_type) {
    if (s_le_scanning) {
        // Must stop scan before connecting
        s_le_pending_connect = true;
        memcpy(s_le_pending_connect_bda, bda, 6);
        s_le_pending_connect_addr_type = addr_type;
        bt_host_scan_le_stop();
        return true;
    }
    
    // Direct connection via HID host profile
    return hid_host_open(HID_HOST_TRANSPORT_LE, bda, addr_type);
}
```

### Pairing

```c
bool bt_host_le_pair(const uint8_t bda[6], uint8_t addr_type) {
    if (s_le_connected && bda_equal(s_le_connected_bda, bda)) {
        // Already connected - just start encryption
        return esp_ble_set_encryption((uint8_t *)bda, 
                                       ESP_BLE_SEC_ENCRYPT_NO_MITM);
    }
    
    // Connect first, then pair in OPEN callback
    s_le_pending_pair = true;
    memcpy(s_le_pending_pair_bda, bda, 6);
    return bt_host_le_connect(bda, addr_type);
}
```

## Security Configuration

```c
static void set_le_security_defaults(void) {
    // Request bonding (store keys)
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_BOND;
    
    // No I/O capability = "Just Works" pairing
    esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;
    
    // Key distribution
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t key_size = 16;

    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, 1);
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, 1);
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, 1);
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, 1);
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, 1);
}
```

### Security Event Handling

```c
// In GAP callback
case ESP_GAP_BLE_SEC_REQ_EVT:
    // Peripheral requested security - auto-accept
    if (s_le_auto_accept_sec_req) {
        esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
    }
    break;

case ESP_GAP_BLE_NC_REQ_EVT:
    // Numeric comparison request - auto-confirm
    if (s_le_auto_confirm_nc) {
        esp_ble_confirm_reply(param->ble_security.key_notif.bd_addr, true);
    }
    break;

case ESP_GAP_BLE_AUTH_CMPL_EVT:
    if (param->ble_security.auth_cmpl.success) {
        ESP_LOGI(TAG, "pairing successful");
    } else {
        ESP_LOGW(TAG, "pairing failed: %u", 
                 param->ble_security.auth_cmpl.fail_reason);
    }
    break;
```

## HID Report Decoding

### Boot Keyboard Report Format

| Byte | Content |
|------|---------|
| 0 | Modifier keys (bitmask) |
| 1 | Reserved |
| 2-7 | Key codes (HID usage, 0 = none) |

**Modifier bits:**

| Bit | Key |
|-----|-----|
| 0 | Left Ctrl |
| 1 | Left Shift |
| 2 | Left Alt |
| 3 | Left GUI |
| 4 | Right Ctrl |
| 5 | Right Shift |
| 6 | Right Alt |
| 7 | Right GUI |

### Key Log Task

```c
static void keylog_task(void *arg) {
    uint8_t prev_keys[6] = {0};

    while (true) {
        keylog_msg_t m;
        xQueueReceive(s_q, &m, portMAX_DELAY);
        
        if (!s_enabled || m.len < 8) continue;

        const uint8_t modifiers = m.data[0];
        const bool shift = (modifiers & 0x22);  // Either Shift
        const uint8_t *keys = &m.data[2];

        // Raw dump
        printf("hid report:");
        for (int i = 0; i < m.len; i++) {
            printf(" %02x", m.data[i]);
        }
        printf("\n");

        // Detect new key presses
        for (int i = 0; i < 6; i++) {
            uint8_t kc = keys[i];
            if (kc == 0) continue;
            if (contains(prev_keys, kc)) continue;

            char ch;
            if (hid_usage_to_ascii(kc, shift, &ch)) {
                printf("key down: '%c'\n", ch);
            } else {
                printf("key down: usage=0x%02x\n", kc);
            }
        }

        // Detect key releases
        for (int i = 0; i < 6; i++) {
            uint8_t prev = prev_keys[i];
            if (prev == 0) continue;
            if (contains(keys, prev)) continue;
            printf("key up: usage=0x%02x\n", prev);
        }

        memcpy(prev_keys, keys, 6);
    }
}
```

### HID Usage to ASCII Mapping

```c
static bool hid_usage_to_ascii(uint8_t usage, bool shift, char *out) {
    // Letters: 0x04 = 'a', 0x1D = 'z'
    if (usage >= 0x04 && usage <= 0x1D) {
        char c = 'a' + (usage - 0x04);
        *out = shift ? toupper(c) : c;
        return true;
    }
    
    // Numbers: 0x1E = '1', 0x27 = '0'
    if (usage >= 0x1E && usage <= 0x27) {
        static const char unshift[] = "1234567890";
        static const char shifted[] = "!@#$%^&*()";
        *out = shift ? shifted[usage - 0x1E] : unshift[usage - 0x1E];
        return true;
    }
    
    // Special keys
    switch (usage) {
    case 0x28: *out = '\n'; return true;  // Enter
    case 0x2C: *out = ' '; return true;   // Space
    case 0x2D: *out = shift ? '_' : '-'; return true;
    // ... more mappings
    }
    
    return false;
}
```

## Example Session

```
ble> scan on 30
scan start: status=0x0000
[NEW] LE Device aa:bb:cc:dd:ee:ff BT-KBD-2000

ble> devices
devices le: 1/100 (most recently seen)
idx  addr              type rssi  age_ms  name
0    aa:bb:cc:dd:ee:ff rand -52   1234    BT-KBD-2000

ble> connect 0
scan stop: status=0x0000
gattc open: status=0x00 conn_id=0 mtu=65 addr=aa:bb:cc:dd:ee:ff

ble> pair 0
sec req: addr=aa:bb:cc:dd:ee:ff
auth complete: success

ble> keylog on

hid report id=0 len=8: 00 00 04 00 00 00 00 00
key down: usage=0x04 char='a'

hid report id=0 len=8: 00 00 00 00 00 00 00 00
key up: usage=0x04

hid report id=0 len=8: 02 00 05 00 00 00 00 00
key down: usage=0x05 char='B'
```

---

# Build and Flash

## GATT Notify Server (0019)

```bash
cd esp32-s3-m5/0019-cardputer-ble-temp-logger
idf.py set-target esp32s3
idf.py build flash monitor
```

## HID Keyboard Host (0020)

```bash
cd esp32-s3-m5/0020-cardputer-ble-keyboard-host
idf.py set-target esp32s3
idf.py build flash monitor
```

---

# Troubleshooting

## GATT Server Issues

| Problem | Cause | Solution |
|---------|-------|----------|
| Client can't find device | Advertising not started | Check `start_advertising()` is called |
| Notifications not received | CCCD not enabled | Client must write 0x0001 to CCCD |
| Connection drops | MTU mismatch | Negotiate MTU after connect |
| Extended adv fails | Old BLE version | Use legacy advertising APIs |

## HID Host Issues

| Problem | Cause | Solution |
|---------|-------|----------|
| Keyboard not found | Not advertising | Put keyboard in pairing mode |
| Connect fails | Wrong address type | Try both `pub` and `rand` |
| Pair fails | Security mismatch | Check keyboard pairing mode |
| No HID reports | HOGP not discovered | Check HID service discovery |
| Garbled output | Wrong report format | Verify boot keyboard mode |

## Common Pairing Failures

| Fail Reason | Meaning | Action |
|-------------|---------|--------|
| `0x05` | Pairing not supported | Keyboard may not support BLE |
| `0x06` | Encryption key too short | Increase `MAX_KEY_SIZE` |
| `0x08` | Numeric comparison failed | Confirm passkey |
| `0x0B` | Repeated attempts | Wait before retrying |

---

# Summary

These two recipes demonstrate the core BLE patterns for ESP32-S3:

**GATT Notify Server (Peripheral):**
- Use attribute tables for GATT service definition
- Handle CCCD writes to enable/disable notifications
- Use extended advertising for BLE 5.0 compatibility
- Separate notify loop from event callbacks

**HID Keyboard Host (Central):**
- Implement device scanning with registry management
- Handle address types (public vs. random)
- Use "Just Works" pairing for simple setup
- Decode HID boot keyboard reports for key logging

Both patterns are production-ready starting points for your BLE projects.

---

# References

- [ESP-IDF GATTS API](https://docs.espressif.com/projects/esp-idf/en/v5.4/esp32s3/api-reference/bluetooth/esp_gatts.html)
- [ESP-IDF GATTC API](https://docs.espressif.com/projects/esp-idf/en/v5.4/esp32s3/api-reference/bluetooth/esp_gattc.html)
- [ESP-IDF BLE Security](https://docs.espressif.com/projects/esp-idf/en/v5.4/esp32s3/api-reference/bluetooth/esp_gap_ble.html)
- [USB HID Usage Tables](https://usb.org/sites/default/files/hut1_22.pdf)
- [bleak Python Library](https://bleak.readthedocs.io/)
- `0019-cardputer-ble-temp-logger/` — GATT notify server source
- `0020-cardputer-ble-keyboard-host/` — HID host source
