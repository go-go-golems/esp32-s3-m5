---
Title: BLE Debugging Playbook and Implementation Guide
Ticket: 0019-BLE-TEST
Status: active
Topics:
    - ble
    - cardputer
    - sensors
    - esp32s3
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: Comprehensive debugging playbook and step-by-step implementation guide for ESP32-S3 BLE sensor projects, optimized for fast iteration and reliable reproduction.
LastUpdated: 2025-12-30T11:25:06.619233048-05:00
WhatFor: Reference guide for implementing and debugging BLE sensor projects on ESP32-S3
WhenToUse: During implementation, debugging, and troubleshooting of BLE temperature sensor logger
---

# BLE Debugging Playbook and Implementation Guide

## Goal

This document provides a **debugging + interactive programming playbook** for "ESP32-S3 BLE sensor → Ubuntu" projects, optimized for **fast iteration**, **clear observability**, and **reliable reproduction** of bugs. It also includes a step-by-step implementation guide for building a BLE temperature sensor logger.

## Context

This playbook is designed for ESP32-S3 projects using:
- **ESP-IDF v4.4.6** (Bluedroid BLE stack)
- **Cardputer hardware** (ESP32-S3, 512KB SRAM, no PSRAM)
- **Ubuntu host** (BlueZ stack)
- **Python `bleak`** client library

The playbook covers both **implementation steps** and **debugging tactics** for common BLE issues.

---

## 0) Mindset: the loop you run every time

**Loop:** *Repro → Observe → Narrow → Fix → Verify → Regression*

* **Repro:** smallest steps to trigger it, always the same
* **Observe:** logs + traces + host-side capture
* **Narrow:** disable layers until it stops; re-enable one by one
* **Fix:** one change at a time
* **Verify:** run the same repro + a stress run
* **Regression:** add a script/test so it never comes back

---

## 1) Your "Golden Path" baseline (keep this working)

Keep a known-good baseline you can always return to:

1. **Start from an ESP-IDF example** that already does BLE GATT + notify (or adapt from existing BLE keyboard code).

2. **Have a tiny Linux client** (e.g., `bleak`) that:
   * scans by name
   * connects
   * reads one characteristic
   * enables notifications and prints values

3. **Pin versions**
   * ESP-IDF version (tag/commit)
   * host dependencies (`pip freeze` or venv)

If you ever get stuck, you return to: *example + minimal host client*.

---

## 2) Instrumentation you add on day 1 (non-negotiable)

### Firmware logging standards

* Put a **monotonic ms timestamp** in logs (or ensure IDF timestamps are on).

* Log at least these state transitions:
  * boot complete
  * sensor init OK/FAIL
  * advertising start (include address + adv params)
  * connect (conn_handle, peer addr, status)
  * MTU update
  * subscribe/unsubscribe (notify enabled?)
  * disconnect (reason)
  * notify send (size, error code if any)

### "Health counters" (cheap, hugely helpful)

Keep counters you can dump periodically (or on command):

```c
typedef struct {
    uint32_t adv_starts;
    uint32_t connects_ok;
    uint32_t connect_fail;
    uint32_t disconnects;
    uint32_t subscribes;
    uint32_t notifies_sent;
    uint32_t notify_fail;
    uint32_t sensor_reads_ok;
    uint32_t sensor_reads_fail;
    uint32_t heap_min_free;
    uint32_t largest_free_block;
} ble_health_counters_t;

static ble_health_counters_t g_health = {0};
```

### Crash visibility

Enable at least one:
* **Core dump to flash** (best) or UART
* Make sure you can decode it (`idf.py monitor` shows backtrace; core dump tooling can fully decode)

### On-device interactive toggles

Add one "debug control" input:
* UART command (simple parser), or
* a BLE "control" characteristic (WRITE), or
* a GPIO button

Use it for: changing notify rate, forcing disconnect, turning sensor on/off, toggling extra logs.

---

## 3) Layered bring-up checklist (only move forward when stable)

### Layer A — Boot + runtime sanity

✅ Goals:
* clean boot every time
* no watchdog resets
* stable heap

Checks:
* `idf.py monitor` shows no repeated panics
* run 5–10 reboots
* check minimum free heap is stable

### Layer B — Sensor alone (no BLE yet)

✅ Goals:
* sensor read loop is stable
* error handling works

Checks:
* log sensor reads at low rate
* intentionally unplug/misconfigure sensor (confirm failure path is clean)
* ensure no blocking I2C calls inside timing-critical contexts

### Layer C — BLE advertising only

✅ Goals:
* always discoverable
* name + service UUID appear

Host checks:
* `bluetoothctl` scan sees it consistently

### Layer D — Connect + GATT read (no notifications yet)

✅ Goals:
* connect works every time
* read characteristic returns expected bytes

Host checks:
* connect/disconnect repeatedly (20+ times)

### Layer E — Notifications

✅ Goals:
* host subscribes
* notifications arrive at target rate

Checks:
* verify `subscribe` event occurs
* verify notify send call returns OK
* test a low rate first (1 Hz), then increase

### Layer F — Stress + edge cases

* disconnect while notifying
* reconnect quickly
* suspend/resume laptop
* multiple centrals nearby (phone + laptop)

---

## 4) Firmware-side debugging tactics (ESP-IDF / ESP32-S3)

### A) Turn logs into "structured events"

When BLE issues happen, raw logs can be chaotic. Prefer consistent single-line events:

Example style:

```c
ESP_LOGI(TAG, "EVT BLE_CONNECT status=%d peer=%02x:%02x:%02x:%02x:%02x:%02x conn=%d",
         status, peer[0], peer[1], peer[2], peer[3], peer[4], peer[5], conn_id);

ESP_LOGI(TAG, "EVT BLE_SUBSCRIBE chr=%d notify=%d", char_handle, notify_enabled);

ESP_LOGI(TAG, "EVT SENSOR_READ ok=%d v=%.2f", ok, temp_c);

ESP_LOGI(TAG, "EVT BLE_NOTIFY len=%d rc=%d", len, ret_code);
```

### B) Use GDB + JTAG when it's not obvious

If you hit:
* intermittent crashes
* heap corruption
* weird timing bugs

Set up:
* OpenOCD + `xtensa-esp32s3-elf-gdb`
* break on panic
* watchpoints on suspicious buffers

### C) Heap and stack sanity

Common real bugs in BLE projects:

* notifying with a buffer that goes out of scope
* stack overflow in a task that formats data
* heap fragmentation from frequent alloc/free

Mitigations:

```c
// Avoid per-notify malloc - use fixed buffer
static uint8_t notify_buffer[20];  // Fixed size

// Track min-free heap
void update_heap_stats(void) {
    size_t free_heap = esp_get_free_heap_size();
    size_t min_free = esp_get_minimum_free_heap_size();
    size_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
    
    g_health.heap_min_free = min_free;
    g_health.largest_free_block = largest;
}
```

### D) Don't do heavy work in callbacks

Rule of thumb:

* In BLE callbacks: **copy data + signal a task**
* In a dedicated task: do sensor reads, packing, notifications

```c
// BAD: Heavy work in callback
void gatts_write_cb(...) {
    float temp = read_sensor();  // BLOCKING!
    ble_notify(temp);
}

// GOOD: Signal task from callback
void gatts_write_cb(...) {
    xSemaphoreGive(sensor_read_sem);  // Signal task
}

void sensor_task(void *arg) {
    while (1) {
        xSemaphoreTake(sensor_read_sem, portMAX_DELAY);
        float temp = read_sensor();
        ble_notify(temp);
    }
}
```

---

## 5) Host-side debugging playbook (Ubuntu / BlueZ)

### Quick "is Linux BLE healthy?" checks

```bash
rfkill list
systemctl status bluetooth
journalctl -u bluetooth -n 200 --no-pager
bluetoothctl show
```

### Use `btmon` to see what's actually happening at HCI level

This is your "Wireshark for Bluetooth" baseline:

```bash
sudo btmon
```

If connects fail, you'll often see the reason here (timeouts, pairing errors, etc.).

### Restart stack when state is wedged

```bash
sudo systemctl restart bluetooth
```

### Debug BlueZ itself (when needed)

Run bluetoothd in foreground with debug (advanced):

* stop system service, run `bluetoothd -n -d` (varies by distro setup)

### If using Python bleak

Turn on verbose logging in your script (and print exceptions). Many "firmware bugs" are actually:

* wrong UUID
* wrong endianness / payload size
* forgetting to `start_notify`
* stale cached device address

```python
import logging
logging.basicConfig(level=logging.DEBUG)

from bleak import BleakScanner, BleakClient

async def main():
    # Enable debug logging
    import sys
    logging.getLogger("bleak").setLevel(logging.DEBUG)
    
    # Your code here
```

---

## 6) BLE-specific triage table (symptom → likely cause → what to check)

### "Device not found in scan"

Likely:
* advertising not started (or stops after error)
* wrong adv interval / channel issues
* power/RF/antenna issues

Check:
* firmware log: `advertise_start`
* `bluetoothctl scan on`
* test with phone app too (to separate Linux issues)

### "Connect sometimes fails"

Likely:
* controller busy / timing
* advertising params too aggressive
* bonding state confusion
* laptop power management

Check:
* `btmon` for failure reason
* firmware logs: connect status codes
* try disabling bonding temporarily to isolate

### "Reads work, notifications don't"

Likely:
* host never subscribed (CCCD not set)
* you notify before subscribe
* wrong characteristic handle
* notify returns error but you ignore it

Check:
* log subscribe events
* log notify return codes
* confirm host calls `start_notify`

### "Works for a while then stops"

Likely:
* memory leak / fragmentation
* task starvation
* connection parameter issues
* your sensor read blocks and stalls BLE task

Check:
* periodic health dump: heap min free, notify_fail count
* move sensor reads to separate task + queue
* reduce notify rate and see if it stabilizes

### "Disconnects under load"

Likely:
* too many notifications / too large payload
* connection interval/MTU mismatch
* RF interference

Check:
* send at 1 Hz (baseline) then scale
* log MTU + connection params
* test next to laptop vs far away

---

## 7) "Interactive programming" tricks that save hours

### A) Firmware "debug console" commands (UART)

Add 10 simple commands:

```c
void uart_debug_task(void *arg)
{
    char cmd[64];
    while (1) {
        int len = uart_read_bytes(UART_NUM_0, cmd, sizeof(cmd)-1, portMAX_DELAY);
        cmd[len] = '\0';
        
        if (strcmp(cmd, "stat\n") == 0) {
            dump_counters();
        } else if (strncmp(cmd, "rate ", 5) == 0) {
            int hz = atoi(cmd + 5);
            set_notify_rate(hz);
        } else if (strcmp(cmd, "adv on\n") == 0) {
            esp_ble_gap_start_advertising(&adv_params);
        } else if (strcmp(cmd, "adv off\n") == 0) {
            esp_ble_gap_stop_advertising();
        } else if (strcmp(cmd, "disc\n") == 0) {
            esp_ble_gap_disconnect(conn_id);
        }
    }
}
```

### B) Firmware "control characteristic" (WRITE)

Same commands, but over BLE. This is great when UART access is annoying.

### C) Host script modes

Make your Linux script support:

```python
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('--scan', action='store_true')
parser.add_argument('--connect-once', action='store_true')
parser.add_argument('--subscribe', action='store_true')
parser.add_argument('--duration', type=int, default=60)
parser.add_argument('--stress', action='store_true')
parser.add_argument('--reconnects', type=int, default=200)

args = parser.parse_args()

if args.scan:
    # Scan only
elif args.stress:
    # Stress test: reconnect N times
elif args.subscribe:
    # Subscribe and log for duration
```

This turns "random debugging" into "repeatable experiments".

---

## 8) Reliability & regression checklist (run before you trust it)

Run these and record results:

1. **100 reconnects**
   * connect → read → subscribe 3s → disconnect

2. **30 minutes streaming**
   * stable rate, no drift, no missing bursts

3. **Laptop suspend/resume**
   * does firmware recover? does host reconnect?

4. **Out-of-range + return**
   * device resumes advertising/connect properly

5. **Bad sensor state**
   * sensor unplugged / errors don't crash BLE

---

## 9) A minimal "debug kit" I recommend you keep handy

* UART cable + stable power
* a phone BLE app (acts as a second independent central)
* `btmon` always available
* a single known-good test script (`bleak`) committed in the repo

---

## Implementation Guide: ESP32-S3 BLE Temperature Sensor Logger

### Step-by-Step Implementation Path

Below is an end-to-end path that works well for "ESP32 broadcasts sensor values to Linux".

---

## 1) Choose the roles (recommended)

* **ESP32-S3:** BLE **Peripheral** + **GATT Server**
  * Exposes a custom "Sensor Service"
  * Updates a characteristic and sends **notifications**

* **Ubuntu:** BLE **Central** + **GATT Client**
  * Connects, enables notifications, receives updates

This is the cleanest model for streaming sensor values.

---

## 2) Start from existing BLE code (Bluedroid recommended)

ESP-IDF supports two BLE hosts; **Bluedroid** is used in the existing codebase.

**Reference Implementation**: `M5Cardputer-UserDemo/main/apps/utils/ble_keyboard_wrap/`

That code already does:
* advertising
* a GATT service
* characteristic updates + notifications

You'll replace the "HID keyboard" service with a "temperature sensor" service.

---

## 3) Enable BLE in menuconfig

```bash
idf.py menuconfig
```

Common settings:

* **Component config → Bluetooth**
  * Enable Bluetooth
  * Controller only: No (we need host stack)
  * Bluedroid Enable: Yes

* Ensure you're building for **ESP32-S3** target:

```bash
idf.py set-target esp32s3
```

---

## 4) Define a custom service + characteristic

Pick UUIDs:

* Service UUID: `12345678-1234-5678-1234-56789abcdef0`
* Characteristic UUID (notify/read): `12345678-1234-5678-1234-56789abcdef1`

For Bluedroid in ESP-IDF, you define GATT like this (C):

```c
#include "esp_gatts_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_defs.h"

// Custom UUIDs
static const uint8_t temp_sensor_service_uuid[16] = {
    0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
    0x34, 0x12, 0x78, 0x56, 0x34, 0x12, 0x56, 0x12
};

static const uint8_t temp_char_uuid[16] = {
    0xf1, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
    0x34, 0x12, 0x78, 0x56, 0x34, 0x12, 0x56, 0x12
};

static uint16_t temp_char_handle;
static uint16_t conn_id = ESP_GATT_ILLEGAL_CONNECTION_ID;
static esp_gatt_if_t gatts_if = ESP_GATT_IF_NONE;

// Characteristic value (temperature as int16_t in 0.01°C units)
static uint8_t temp_value[2] = {0};

static void gatts_event_handler(esp_gatts_cb_event_t event,
                                esp_gatt_if_t gatts_if,
                                esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
    case ESP_GATTS_REG_EVT:
        gatts_if = param->reg.status == ESP_GATT_OK ? param->reg.gatts_if : ESP_GATT_IF_NONE;
        break;
    case ESP_GATTS_CREATE_EVT:
        // Service created
        break;
    case ESP_GATTS_CONNECT_EVT:
        conn_id = param->connect.conn_id;
        ESP_LOGI(TAG, "Connected, conn_id=%d", conn_id);
        break;
    case ESP_GATTS_DISCONNECT_EVT:
        conn_id = ESP_GATT_ILLEGAL_CONNECTION_ID;
        ESP_LOGI(TAG, "Disconnected");
        break;
    case ESP_GATTS_WRITE_EVT:
        // Handle write to characteristic
        break;
    case ESP_GATTS_READ_EVT:
        // Handle read from characteristic
        esp_gatts_rsp_t rsp = {0};
        rsp.attr_value.handle = param->read.handle;
        rsp.attr_value.len = 2;
        memcpy(rsp.attr_value.value, temp_value, 2);
        esp_ble_gatts_send_response(gatts_if, conn_id, param->read.trans_id,
                                     ESP_GATT_OK, &rsp);
        break;
    default:
        break;
    }
}
```

---

## 5) Advertising + connection tracking

You want to advertise your service UUID and track when Linux connects:

```c
static void advertise(void);

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_CONNECT_EVT:
        if (param->connect.status == ESP_BT_STATUS_SUCCESS) {
            conn_id = param->connect.conn_id;
            ESP_LOGI(TAG, "Connected");
        } else {
            conn_id = ESP_GATT_ILLEGAL_CONNECTION_ID;
            advertise();  // Restart advertising
        }
        break;
    case ESP_GAP_BLE_DISCONNECT_EVT:
        conn_id = ESP_GATT_ILLEGAL_CONNECTION_ID;
        advertise();  // Restart advertising
        break;
    case ESP_GAP_BLE_SUBSCRIBE_EVT:
        ESP_LOGI(TAG, "Notifications %s", param->subscribe.is_notify ? "enabled" : "disabled");
        break;
    default:
        break;
    }
}

static void advertise(void)
{
    esp_ble_adv_data_t adv_data = {
        .set_scan_rsp = false,
        .include_name = true,
        .include_txpower = true,
        .min_interval = 0x0006,  // 7.5ms
        .max_interval = 0x0010,    // 20ms
        .appearance = 0x0000,     // Unknown
        .service_uuid_len = 16,
        .p_service_uuid = (uint8_t *)temp_sensor_service_uuid,
        .flag = 0x6,
    };
    esp_ble_gap_config_adv_data(&adv_data);

    esp_ble_adv_params_t adv_params = {
        .adv_int_min = 0x20,      // 20ms
        .adv_int_max = 0x30,      // 30ms
        .adv_type = ADV_TYPE_IND,
        .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
        .channel_map = ADV_CHNL_ALL,
        .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
    };
    esp_ble_gap_start_advertising(&adv_params);
}
```

---

## 6) Send sensor updates via notifications

Whenever you sample the sensor (timer/task), update the characteristic and notify:

```c
static void notify_temperature(int16_t temp_cents)
{
    if (conn_id == ESP_GATT_ILLEGAL_CONNECTION_ID) {
        return;  // Not connected
    }

    // Update characteristic value
    temp_value[0] = (uint8_t)(temp_cents & 0xFF);
    temp_value[1] = (uint8_t)((temp_cents >> 8) & 0xFF);

    // Send notification
    esp_ble_gatts_send_indicate(gatts_if, conn_id, temp_char_handle,
                                 2, temp_value, false);
}
```

Call `notify_temperature(new_value)` at whatever interval you want.

---

## 7) Flash and run

```bash
idf.py build flash monitor
```

You should see it advertising as your device name.

---

# Ubuntu side (BlueZ): connect + subscribe to notifications

## Option A (simple + nice): Python "bleak" client

Install:

```bash
python3 -m pip install bleak
```

Example client that connects and prints notifications:

```python
import asyncio
from bleak import BleakScanner, BleakClient

DEVICE_NAME = "esp32-s3-temp-sensor"
CHAR_UUID = "12345678-1234-5678-1234-56789abcdef1"

def on_notify(_, data: bytearray):
    # 16-bit little-endian, 0.01°C units
    temp_cents = int.from_bytes(data, byteorder="little", signed=True)
    temp_c = temp_cents / 100.0
    print(f"Temperature: {temp_c:.2f}°C")

async def main():
    dev = await BleakScanner.find_device_by_filter(
        lambda d, ad: d.name == DEVICE_NAME
    )
    if not dev:
        raise SystemExit("Device not found")

    async with BleakClient(dev) as client:
        await client.start_notify(CHAR_UUID, on_notify)
        print("Subscribed, Ctrl+C to quit")
        while True:
            await asyncio.sleep(1)

asyncio.run(main())
```

Run it (sometimes needs permissions; easiest is with sudo, better is set capabilities):

```bash
python3 client.py
```

## Option B (CLI-only): bluetoothctl + gatt tools

* `bluetoothctl` to scan/pair/connect
* then a GATT client tool to enable notifications (BlueZ has `btgatt-client` on some distros; `gatttool` exists but is often deprecated)

---

## Data modeling tips (so it stays sane)

* Put your sensor packet in a **binary struct** (little-endian) or **CBOR** if you want flexibility.

* Keep payload under ~20 bytes initially (notifications are fine with more, but start simple).

* Use **one characteristic per "stream"** (e.g., accel, temp, humidity), or one packed characteristic if you prefer.

---

## If you want two-way exchange

Add another characteristic with `WRITE`:

* Linux writes commands: sample rate, calibration, etc.

* ESP handles it in the characteristic access callback (`ESP_GATTS_WRITE_EVT`).

---

## Usage Examples

### Example 1: Minimal BLE GATT Server

See implementation guide above for complete code structure.

### Example 2: Debugging Connection Issues

```bash
# Terminal 1: Monitor firmware logs
idf.py monitor

# Terminal 2: Monitor HCI layer
sudo btmon

# Terminal 3: Run Python client with debug logging
python3 -m logging DEBUG client.py
```

### Example 3: Stress Testing

```python
# stress_test.py
import asyncio
from bleak import BleakClient

async def stress_connect(addr, count=100):
    for i in range(count):
        try:
            async with BleakClient(addr) as client:
                await asyncio.sleep(0.1)
                print(f"Connect {i+1}/{count} OK")
        except Exception as e:
            print(f"Connect {i+1}/{count} FAILED: {e}")
        await asyncio.sleep(0.5)

asyncio.run(stress_connect("AA:BB:CC:DD:EE:FF", 100))
```

---

## Related Documents

- [BLE Implementation Analysis](./analysis/01-ble-implementation-analysis-esp-idf-bluedroid-gatt-server.md)
- [Temperature Sensor Analysis](./analysis/02-temperature-sensor-and-i2c-access-analysis.md)
