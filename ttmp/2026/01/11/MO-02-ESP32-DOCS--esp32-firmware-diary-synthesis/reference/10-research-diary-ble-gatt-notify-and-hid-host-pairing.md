---
Title: 'Research Diary: BLE GATT Notify and HID Host Pairing'
Ticket: MO-02-ESP32-DOCS
Status: active
Topics:
    - esp32
    - firmware
    - documentation
    - ble
    - gatt
    - hid
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/0019-cardputer-ble-temp-logger/main/ble_temp_logger_main.c
      Note: GATT notify server implementation
    - Path: esp32-s3-m5/0020-cardputer-ble-keyboard-host/main/bt_host.c
      Note: BLE HID host scanning and pairing
    - Path: esp32-s3-m5/0020-cardputer-ble-keyboard-host/main/keylog.c
      Note: HID report decoding and key logging
ExternalSources: []
Summary: 'Research diary for the BLE GATT notify and HID host pairing guide.'
LastUpdated: 2026-01-12T14:00:00-05:00
WhatFor: 'Document the research trail for the BLE recipes guide.'
WhenToUse: 'Reference when updating or extending BLE documentation.'
---

# Research Diary: BLE GATT Notify and HID Host Pairing

## Goal

Investigate two BLE patterns in the ESP32-S3 tutorials:
1. GATT notify server streaming temperature
2. BLE HID keyboard host with console-driven pairing

---

## Step 1: Review GATT Notify Server (0019)

### GATT Schema

The temperature logger exposes a simple service:

```
Service UUID: 0xFFF0
├── Temperature Characteristic: 0xFFF1
│   ├── Properties: READ + NOTIFY
│   ├── Format: int16_le (centi-degrees Celsius)
│   └── CCCD: 0x2902 (enable notifications)
└── Control Characteristic: 0xFFF2
    ├── Properties: WRITE
    └── Format: uint16_le (notify period in ms, 0 = disabled)
```

### Handle Table Pattern

The attribute table is defined statically with an enum for indices:

```c
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
```

After `esp_ble_gatts_create_attr_tab()` completes, handles are copied:
```c
memcpy(s_handle_table, param->add_attr_tab.handles, sizeof(uint16_t) * IDX_NB);
```

### Extended Advertising

The server uses BLE 5.0 extended advertising for better compatibility:

```c
esp_ble_gap_ext_adv_params_t params = {
    .type = ESP_BLE_GAP_SET_EXT_ADV_PROP_CONNECTABLE,
    .interval_min = 0x20,
    .interval_max = 0x40,
    .primary_phy = ESP_BLE_GAP_PHY_1M,
    .secondary_phy = ESP_BLE_GAP_PHY_1M,
};

// Callback-driven sequence:
// 1. esp_ble_gap_ext_adv_set_params()
// 2. ESP_GAP_BLE_EXT_ADV_SET_PARAMS_COMPLETE_EVT → config data
// 3. ESP_GAP_BLE_EXT_ADV_DATA_SET_COMPLETE_EVT → start adv
// 4. esp_ble_gap_ext_adv_start()
```

### Notify Loop

Temperature is streamed from a dedicated task:

```c
static void notify_task(void *arg) {
    uint32_t seq = 0;
    
    while (true) {
        int16_t temp = mock_temp_centi(seq++);
        set_temp_attr(temp);  // Update readable value
        
        if (s_connected && s_notify_enabled && s_period_ms != 0) {
            esp_ble_gatts_send_indicate(s_gatts_if, s_conn_id,
                                        s_handle_table[IDX_CHAR_TEMP_VALUE],
                                        2, value, false /* not confirm */);
        }
        
        // Sleep with task notification for period changes
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(s_period_ms));
    }
}
```

### CCCD Handling

Client enables notifications by writing 0x0001 to the CCCD:

```c
static void handle_write_cccd(const esp_ble_gatts_cb_param_t *param) {
    uint16_t v = param->write.value[0] | (param->write.value[1] << 8);
    s_notify_enabled = (v == 0x0001);
}
```

### Control Characteristic

Writing to 0xFFF2 changes the notify period:

```c
static void handle_write_control(const esp_ble_gatts_cb_param_t *param) {
    uint16_t period_ms = param->write.value[0] | (param->write.value[1] << 8);
    s_period_ms = period_ms;
    xTaskNotifyGive(s_notify_task);  // Wake loop to apply new period
}
```

---

## Step 2: Review Python Client

The `ble_client.py` tool uses the `bleak` library:

```python
# 16-bit UUIDs normalized to 128-bit
TEMP_UUID = "0000fff1-0000-1000-8000-00805f9b34fb"
CTRL_UUID = "0000fff2-0000-1000-8000-00805f9b34fb"

async def connect_once(address, set_period_ms, do_read, do_notify, duration):
    async with BleakClient(address) as client:
        # Write control if specified
        if set_period_ms is not None:
            payload = struct.pack("<H", set_period_ms)
            await client.write_gatt_char(CTRL_UUID, payload)
        
        # Read current value
        if do_read:
            v = await client.read_gatt_char(TEMP_UUID)
            temp = struct.unpack("<h", v)[0] / 100.0
            print(f"temp: {temp:.2f} C")
        
        # Subscribe to notifications
        if do_notify:
            def on_notify(handle, data):
                temp = struct.unpack("<h", data)[0] / 100.0
                print(f"notify: {temp:.2f} C")
            
            await client.start_notify(TEMP_UUID, on_notify)
            await asyncio.sleep(duration)
            await client.stop_notify(TEMP_UUID)
```

Commands:
- `python ble_client.py scan` — List BLE devices
- `python ble_client.py run --read --notify --duration 30` — Read + subscribe
- `python ble_client.py run --set-period-ms 500` — Change notify period

---

## Step 3: Review BLE HID Host (0020)

### Architecture

The HID host is modular:

```
app_main.c      — Entry, init subsystems
bt_host.c       — BLE/BR-EDR scanning, pairing, security
bt_console.c    — esp_console commands
hid_host.c      — HID profile client (GATTC for LE, HIDH for BR)
keylog.c        — HID report decoding and printing
```

### Initialization

```c
void app_main(void) {
    esp_event_loop_create_default();
    
    keylog_init();     // Report queue + task
    bt_host_init();    // BT controller + Bluedroid + callbacks
    hid_host_init();   // HID profile init
    
    bt_console_start(); // Start REPL
}
```

### Device Scanning

```c
void bt_host_scan_le_start(uint32_t seconds) {
    esp_ble_gap_set_scan_params(&s_scan_params);
    // In callback:
    esp_ble_gap_start_scanning(seconds);
}

// Scan results update device registry:
static void handle_scan_result(const struct ble_scan_result_evt_param *scan_rst) {
    // Extract name from adv data
    uint8_t *name = esp_ble_resolve_adv_data_by_type(
        scan_rst->ble_adv, eir_len, ESP_BLE_AD_TYPE_NAME_CMPL, &name_len);
    
    // Update registry with bda, addr_type, rssi, name
    update_le_device_registry(scan_rst->bda, addr_type, rssi, name);
}
```

### Device Registry

```c
typedef struct {
    uint8_t bda[6];
    uint8_t addr_type;  // BLE_ADDR_TYPE_PUBLIC or _RANDOM
    int rssi;
    char name[32];
    uint32_t last_seen_ms;
} bt_le_device_t;

static bt_le_device_t s_le_devices[MAX_LE_DEVICES];
static int s_le_num_devices;
```

### Connect and Pair Flow

The console command flow:
1. `scan on 30` — Start LE scan for 30 seconds
2. `devices` — List discovered devices
3. `connect 0` — Connect to device index 0
4. `pair 0` — Start encryption (pair)
5. `keylog on` — Enable HID report logging

```c
bool bt_host_le_connect(const uint8_t bda[6], uint8_t addr_type) {
    if (s_le_scanning) {
        // Stop scan first, then connect in callback
        s_le_pending_connect = true;
        memcpy(s_le_pending_connect_bda, bda, 6);
        bt_host_scan_le_stop();
        return true;
    }
    return hid_host_open(HID_HOST_TRANSPORT_LE, bda, addr_type);
}

bool bt_host_le_pair(const uint8_t bda[6], uint8_t addr_type) {
    if (s_le_connected && bda_equal(s_le_connected_bda, bda)) {
        // Already connected, just start encryption
        return start_encryption_for_peer(bda);
    }
    // Connect first, then pair in OPEN callback
    s_le_pending_pair = true;
    memcpy(s_le_pending_pair_bda, bda, 6);
    return bt_host_le_connect(bda, addr_type);
}
```

### Security Setup

```c
static void set_le_security_defaults(void) {
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_BOND;
    esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;  // Just Works
    uint8_t key_size = 16;
    
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, 1);
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, 1);
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, 1);
}
```

### HID Report Processing

The keylog task decodes boot keyboard reports:

```c
static void keylog_task(void *arg) {
    uint8_t prev_keys[6] = {0};
    
    while (true) {
        keylog_msg_t m;
        xQueueReceive(s_q, &m, portMAX_DELAY);
        
        // Boot keyboard report: [mods][reserved][key0-key5]
        const uint8_t modifiers = m.data[0];
        const uint8_t *keys = &m.data[2];
        
        // Detect new key presses
        for (int i = 0; i < 6; i++) {
            if (keys[i] && !contains(prev_keys, keys[i])) {
                // Convert HID usage to ASCII
                char ch;
                if (hid_kbd_usage_to_ascii(keys[i], shift, &ch)) {
                    printf("key down: '%c'\n", ch);
                }
            }
        }
        
        // Detect key releases
        for (int i = 0; i < 6; i++) {
            if (prev_keys[i] && !contains(keys, prev_keys[i])) {
                printf("key up: usage=0x%02x\n", prev_keys[i]);
            }
        }
        
        memcpy(prev_keys, keys, 6);
    }
}
```

---

## Step 4: Console Commands

### GATT Server (0019)

No console commands — purely passive server.

### HID Host (0020)

| Command | Description |
|---------|-------------|
| `scan on [seconds]` | Start LE scan (default 30s) |
| `scan off` | Stop LE scan |
| `devices` | List discovered devices |
| `devices <filter>` | Filter by name/address substring |
| `devices clear` | Clear device list |
| `devices events on/off` | Toggle [NEW]/[CHG]/[DEL] output |
| `connect <idx>` | Connect to device by index |
| `connect <addr> [pub/rand]` | Connect by address |
| `pair <idx>` | Pair with device by index |
| `pair <addr> [pub/rand]` | Pair by address |
| `disconnect` | Disconnect current device |
| `keylog on/off` | Enable/disable HID report logging |
| `bonds` | List bonded devices |
| `unpair <idx/addr>` | Remove bond |

### Example Session

```
ble> scan on 30
scan start: status=0x0000
[NEW] LE Device aa:bb:cc:dd:ee:ff BT-KBD
ble> devices
devices le: 3/100 (most recently seen)
idx  addr              type rssi  age_ms  name
0    aa:bb:cc:dd:ee:ff rand -52   1234    BT-KBD
1    ...
ble> connect 0
gattc open: status=0x00 conn_id=0 mtu=65
ble> pair 0
auth complete: success
ble> keylog on
hid report id=0 len=8: 00 00 04 00 00 00 00 00
key down: usage=0x04 char='a'
```

---

## Quick Reference

### GATT Server APIs

| API | Purpose |
|-----|---------|
| `esp_ble_gatts_register_callback()` | Register GATTS event handler |
| `esp_ble_gatts_app_register()` | Register application profile |
| `esp_ble_gatts_create_attr_tab()` | Create attribute table |
| `esp_ble_gatts_start_service()` | Start the service |
| `esp_ble_gatts_send_indicate()` | Send notification/indication |
| `esp_ble_gatts_set_attr_value()` | Update readable value |

### HID Host APIs

| API | Purpose |
|-----|---------|
| `esp_ble_gap_start_scanning()` | Start LE scan |
| `esp_ble_gattc_open()` | Connect to device |
| `esp_ble_set_encryption()` | Start LE pairing |
| `esp_ble_gap_security_rsp()` | Accept security request |
| `esp_ble_passkey_reply()` | Enter passkey |
| `esp_ble_confirm_reply()` | Confirm numeric comparison |

### HID Report Format (Boot Keyboard)

| Byte | Content |
|------|---------|
| 0 | Modifiers (bit0=LCtrl, bit1=LShift, bit2=LAlt, bit3=LGUI, bit4-7=right) |
| 1 | Reserved |
| 2-7 | Key codes (HID usage, 0=none) |

### HID Usage to ASCII (Subset)

| Usage | Char |
|-------|------|
| 0x04-0x1D | a-z |
| 0x1E-0x27 | 1-0 |
| 0x28 | Enter |
| 0x2C | Space |
