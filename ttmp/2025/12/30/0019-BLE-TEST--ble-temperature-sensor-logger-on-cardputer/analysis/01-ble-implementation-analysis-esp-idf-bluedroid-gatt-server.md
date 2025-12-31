---
Title: BLE Implementation Analysis - ESP-IDF Bluedroid GATT Server
Ticket: 0019-BLE-TEST
Status: active
Topics:
    - ble
    - cardputer
    - sensors
    - esp32s3
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: Analysis of ESP-IDF Bluedroid BLE stack, GATT server implementation patterns, and existing BLE code in the codebase for implementing a temperature sensor logger.
LastUpdated: 2025-12-30T11:25:06.619233048-05:00
WhatFor: Understanding BLE implementation options and existing code patterns for building a BLE temperature sensor logger
WhenToUse: When implementing BLE GATT server functionality for sensor data streaming
---

# BLE Implementation Analysis - ESP-IDF Bluedroid GATT Server

## Executive Summary

This document analyzes BLE implementation options for ESP32-S3, focusing on the **Bluedroid** stack (used in existing codebase) versus **NimBLE** (alternative). The codebase already contains a BLE HID keyboard implementation using Bluedroid, which provides a reference pattern for implementing a custom GATT server for temperature sensor data.

**Key Finding**: The M5Cardputer-UserDemo project uses **ESP-IDF v4.4.6** with **Bluedroid** BLE stack (`esp_gatts_api.h`, `esp_ble_gap_api.h`). For consistency and code reuse, we should use Bluedroid rather than NimBLE, even though NimBLE is lighter weight.

## ESP-IDF BLE Stack Options

### Bluedroid (Current Choice)

**Status**: Used in existing codebase (`M5Cardputer-UserDemo`)

**APIs**:
- `esp_gatts_api.h` - GATT Server APIs
- `esp_gattc_api.h` - GATT Client APIs  
- `esp_gap_ble_api.h` - BLE GAP (Generic Access Profile) APIs
- `esp_bt_main.h` - Bluetooth controller initialization
- `esp_hidd.h` - HID Device abstraction (optional, for HID profiles)

**Characteristics**:
- More feature-rich, supports both Classic BT and BLE
- Higher memory footprint (~50-100KB)
- Well-documented, mature API
- Used in ESP-IDF examples for GATT servers

**Initialization Pattern**:
```cpp
// 1. Initialize NVS (required for BT)
nvs_flash_init();

// 2. Initialize BT controller
esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
esp_bt_controller_init(&bt_cfg);
esp_bt_controller_enable(ESP_BT_MODE_BLE);

// 3. Initialize Bluedroid stack
esp_bluedroid_init();
esp_bluedroid_enable();

// 4. Register GAP callback
esp_ble_gap_register_callback(ble_gap_event_handler);

// 5. Register GATTS callback
esp_ble_gatts_register_callback(gatts_event_handler);

// 6. Create GATT services/characteristics
esp_ble_gatts_create_service(...);
```

### NimBLE (Alternative)

**Status**: Not used in codebase, but available in ESP-IDF

**APIs**:
- `host/ble_hs.h` - BLE Host Stack APIs
- `services/gap/ble_svc_gap.h` - GAP Service
- `services/gatt/ble_svc_gatt.h` - GATT Service

**Characteristics**:
- Lighter weight (~30-50KB memory)
- BLE-only (no Classic BT support)
- Simpler API, more callback-driven
- Commonly recommended for new BLE-only projects

**Recommendation**: **Use Bluedroid** for this project because:
1. Consistency with existing codebase
2. Existing BLE keyboard implementation provides reference
3. More examples and documentation available
4. Memory constraints are manageable (Cardputer has 512KB SRAM, BLE overhead is acceptable)

## Existing BLE Implementation Analysis

### Location

**Primary Reference**: `M5Cardputer-UserDemo/main/apps/utils/ble_keyboard_wrap/`

**Key Files**:
- `ble_keyboard_wrap.cpp` - Main BLE HID keyboard wrapper
- `ble_keyboard_wrap.h` - Public API
- `esp_hid_gap.c` - GAP event handling and advertising setup
- `esp_hid_gap.h` - GAP API definitions

### Architecture Pattern

The existing implementation follows this pattern:

1. **Initialization Layer** (`ble_keyboard_wrap_init`):
   - NVS flash init
   - BT controller init/enable
   - Bluedroid init/enable
   - GAP callback registration
   - GATTS callback registration
   - HID device initialization (uses `esp_hidd.h` abstraction)

2. **GAP Event Handling** (`esp_hid_gap.c`):
   - Connection/disconnection events
   - Security/pairing events
   - Advertising start/stop
   - Scan results (for client mode)

3. **GATTS Event Handling** (`ble_keyboard_wrap.cpp`):
   - Service creation
   - Characteristic read/write
   - Notifications/indications
   - Connection parameter updates

### Key APIs Used

#### GAP (Generic Access Profile)

```cpp
// Register GAP callback
esp_ble_gap_register_callback(ble_gap_event_handler);

// Configure advertising data
esp_ble_adv_data_t ble_adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x0006,  // 7.5ms
    .max_interval = 0x0010,  // 20ms
    .appearance = appearance,
    .service_uuid_len = sizeof(service_uuid),
    .p_service_uuid = (uint8_t *)service_uuid,
    .flag = 0x6,
};
esp_ble_gap_config_adv_data(&ble_adv_data);

// Start advertising
esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x20,      // 32 * 0.625ms = 20ms
    .adv_int_max = 0x30,      // 48 * 0.625ms = 30ms
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};
esp_ble_gap_start_advertising(&adv_params);
```

#### GATTS (GATT Server)

```cpp
// Register GATTS callback
esp_ble_gatts_register_callback(gatts_event_handler);

// Create service
esp_ble_gatts_create_service(gatts_if, &service_uuid, handle_num, &service_handle);

// Add characteristic
esp_ble_gatts_add_char(service_handle, &char_uuid,
                       ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                       ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY,
                       &char_value, NULL);

// Start service
esp_ble_gatts_start_service(service_handle);

// Send notification
esp_ble_gatts_send_indicate(gatts_if, conn_id, char_handle,
                             value_len, value, false);
```

### Event Handler Pattern

The existing code uses a switch-based event handler:

```cpp
static void gatts_event_handler(esp_gatts_cb_event_t event,
                                esp_gatt_if_t gatts_if,
                                esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
    case ESP_GATTS_REG_EVT:
        // Service registration complete
        break;
    case ESP_GATTS_CREATE_EVT:
        // Service/characteristic created
        break;
    case ESP_GATTS_CONNECT_EVT:
        // Client connected
        conn_id = param->connect.conn_id;
        break;
    case ESP_GATTS_DISCONNECT_EVT:
        // Client disconnected
        conn_id = ESP_GATT_ILLEGAL_CONNECTION_ID;
        break;
    case ESP_GATTS_WRITE_EVT:
        // Characteristic write request
        break;
    case ESP_GATTS_READ_EVT:
        // Characteristic read request
        break;
    case ESP_GATTS_CONF_EVT:
        // Notification/indication confirmation
        break;
    default:
        break;
    }
}
```

## GATT Service Design for Temperature Sensor

### Service UUID

Use a custom 128-bit UUID (avoid standard service UUIDs unless implementing a standard profile):

```cpp
// Custom Temperature Sensor Service UUID
static const uint8_t temp_sensor_service_uuid[16] = {
    0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78,
    0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78
};
```

### Characteristics

**1. Temperature Value (Notify/Read)**
- UUID: Custom 128-bit
- Properties: `ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY`
- Data Format: `int16_t` (temperature in 0.01°C units, e.g., 2500 = 25.00°C)
- Or: `float` (4 bytes, IEEE 754)

**2. Sample Rate Control (Write)**
- UUID: Custom 128-bit
- Properties: `ESP_GATT_CHAR_PROP_BIT_WRITE`
- Data Format: `uint16_t` (sample rate in milliseconds, e.g., 1000 = 1Hz)

**3. Status/Control (Read/Write)**
- UUID: Custom 128-bit
- Properties: `ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE`
- Data Format: `uint8_t` (bit flags: bit 0 = sensor enabled, bit 1 = logging enabled)

### CCCD (Client Characteristic Configuration Descriptor)

Automatically created by ESP-IDF when characteristic has NOTIFY/INDICATE properties. Client enables notifications by writing `0x0001` or `0x0002` to CCCD handle.

## Implementation Checklist

Based on existing code patterns:

- [ ] Initialize NVS flash
- [ ] Initialize BT controller (`ESP_BT_MODE_BLE`)
- [ ] Initialize Bluedroid stack
- [ ] Register GAP callback
- [ ] Register GATTS callback
- [ ] Create custom GATT service
- [ ] Add temperature characteristic (READ + NOTIFY)
- [ ] Add sample rate characteristic (WRITE)
- [ ] Add status characteristic (READ + WRITE)
- [ ] Start service
- [ ] Configure advertising data (include service UUID)
- [ ] Start advertising
- [ ] Handle connection events (track `conn_id`)
- [ ] Handle write events (sample rate, status)
- [ ] Handle read events (temperature, status)
- [ ] Send notifications when temperature updates
- [ ] Handle disconnect (restart advertising)

## Memory Considerations

**Bluedroid Memory Usage** (approximate):
- Controller: ~20-30KB
- Bluedroid stack: ~50-80KB
- GATT services: ~5-10KB per service
- **Total BLE overhead: ~75-120KB**

**Cardputer SRAM**: 512KB total
- Display buffers: ~64KB
- BLE stack: ~100KB
- **Remaining for application**: ~350KB (sufficient for sensor reading + logging)

## ESP-IDF Version Compatibility

**Current**: ESP-IDF v4.4.6 (from `M5Cardputer-UserDemo/README.md`)

**BLE API Stability**: Bluedroid APIs are stable across v4.4.x and v5.x. No breaking changes expected.

## Related Files

- `M5Cardputer-UserDemo/main/apps/utils/ble_keyboard_wrap/ble_keyboard_wrap.cpp` - Main BLE implementation
- `M5Cardputer-UserDemo/main/apps/utils/ble_keyboard_wrap/esp_hid_gap.c` - GAP event handling
- `M5Cardputer-UserDemo/main/apps/utils/ble_keyboard_wrap/esp_hid_gap.h` - GAP API definitions

## External References

- [ESP-IDF Bluedroid GATT Server Guide](https://docs.espressif.com/projects/esp-idf/en/v4.4.6/esp32/api-guides/bluetooth/esp-gatt.html)
- [ESP-IDF BLE GAP Guide](https://docs.espressif.com/projects/esp-idf/en/v4.4.6/esp32/api-guides/bluetooth/esp-ble-gap.html)
- [ESP-IDF BLE Examples](https://github.com/espressif/esp-idf/tree/v4.4.6/examples/bluetooth)

## Next Steps

1. Create minimal GATT server example (service + one characteristic)
2. Test connection from Linux host (`bluetoothctl` or `bleak`)
3. Add temperature sensor integration
4. Implement notification streaming
5. Add sample rate control
6. Add status/control characteristic
