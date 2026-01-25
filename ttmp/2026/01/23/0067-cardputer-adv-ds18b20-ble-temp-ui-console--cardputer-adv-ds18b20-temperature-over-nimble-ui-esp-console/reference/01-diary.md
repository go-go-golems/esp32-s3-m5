---
Title: Diary
Ticket: 0067-cardputer-adv-ds18b20-ble-temp-ui-console
Status: active
Topics:
    - cardputer
    - ble
    - sensors
    - ui
    - console
    - esp-idf
    - esp32s3
    - debugging
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ttmp/2026/01/23/0067-cardputer-adv-ds18b20-ble-temp-ui-console--cardputer-adv-ds18b20-temperature-over-nimble-ui-esp-console/analysis/01-how-to-build-cardputer-adv-ds18b20-nimble-notifications-ui-esp-console.md
      Note: Primary analysis deliverable produced by this diary.
    - Path: ttmp/2026/01/23/0067-cardputer-adv-ds18b20-ble-temp-ui-console--cardputer-adv-ds18b20-temperature-over-nimble-ui-esp-console/index.md
      Note: Ticket index that links to documents and tracks status.
ExternalSources: []
Summary: 'Research diary for ticket 0067: DS18B20 temperature acquisition + NimBLE GATT notify + Cardputer UI + esp_console debugging.'
LastUpdated: 2026-01-23T15:45:52.35409261-05:00
WhatFor: To record the concrete research steps, repo references, and decisions used to produce the build analysis for the Cardputer ADV DS18B20 BLE temperature firmware.
WhenToUse: Use when reviewing how the analysis was produced, reproducing the research, or extending the design into an implementation ticket.
---


# Diary

## Goal

Capture the research trail for designing a Cardputer ADV firmware that reads a DS18B20 temperature probe and streams it over NimBLE BLE, with an on-device UI and a USB Serial/JTAG `esp_console` for debugging.

## Step 1: Create ticket, gather references, write build analysis

The goal of this step was to build a “map” of what already exists in this repository (UI patterns, console patterns, BLE payload conventions) and what lives in ESP-IDF 5.4.1 (DS18B20 + NimBLE examples), then synthesize those into a single implementation-level analysis document in the new ticket workspace.

This step intentionally focused on architecture and reference paths, not on writing firmware code. The deliverable is a structured analysis document with wiring notes, configuration guidance, event-driven architecture, and pseudocode.

### Prompt Context

**User prompt (verbatim):**

````text
idf is in ~/esp/esp-idf-5.4.1

Create a new ticket 0067-... (you decide the name) and analyze how to create a cardputer ADV firmware that reads the temperature using a thermocouple ds18b20 and then send the data over NimBLE . We wnat to make a little UI to display it and also show bluetooth status, as well as an esp_console to debug things.

here's some ideas for the start: 

Below is a clean “temperature probe on ESP32 → phone app” BLE design that works well on **iOS (CoreBluetooth)** and **Android (BluetoothGatt)**, with an **ESP-IDF (NimBLE) GATT server sketch**.

---

## 1) BLE contract (GATT design)

### Option A (recommended for simplicity): Custom service + notify characteristic

* **Advertised name**: `ESP-TEMP`
* **Service UUID** (128-bit): `A1B2C3D4-E5F6-47A0-9A12-8D3F4C5B6A70`
* **Characteristic UUID** (temperature): `A1B2C3D4-E5F6-47A0-9A12-8D3F4C5B6A71`

  * Properties: **Read + Notify**
  * Payload: **int16 little-endian, temperature in centi-°C**

    * Example: `2356` means `23.56°C`
    * Signed so you can represent below zero.

### Option B: Use the standard Health Thermometer Service

Espressif even provides an example that exposes it. This is “most interoperable,” but encoding is a bit more annoying. ([ESP Component Registry][1])

---

## 2) ESP32 peripheral (ESP-IDF + NimBLE) sketch

Espressif’s BLE “getting started / data exchange” and NimBLE GATT server example walk through the structure. ([Espressif Systems][2])

### ESP-IDF high-level flow

1. Init NVS
2. Init controller + NimBLE host
3. Define GATT database (service + temperature char)
4. Start advertising (include service UUID)
5. Periodically read temperature probe → update global value → **notify subscribed centrals**

### C skeleton (ESP-IDF NimBLE)

```c
// main.c (sketch)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

static const char *TAG = "ble_temp";

// ---- UUIDs (custom) ----
static const ble_uuid128_t g_svc_uuid =
    BLE_UUID128_INIT(0x70,0x6a,0x5b,0x4c,0x3f,0x8d,0x12,0x9a,0xa0,0x47,0xf6,0xe5,0xd4,0xc3,0xb2,0xa1);

static const ble_uuid128_t g_temp_chr_uuid =
    BLE_UUID128_INIT(0x71,0x6a,0x5b,0x4c,0x3f,0x8d,0x12,0x9a,0xa0,0x47,0xf6,0xe5,0xd4,0xc3,0xb2,0xa1);

static uint16_t g_temp_chr_handle;
static volatile int16_t g_latest_temp_centi = 0; // centi-degC
static volatile uint16_t g_conn_handle = BLE_HS_CONN_HANDLE_NONE;

// Replace with your real sensor read:
static int16_t read_temp_centi(void) {
    // e.g. 23.56C -> 2356
    return 2356;
}

static int temp_chr_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                              struct ble_gatt_access_ctxt *ctxt, void *arg) {
    // Read returns current temp as int16 LE
    uint8_t b[2];
    int16_t v = g_latest_temp_centi;
    b[0] = (uint8_t)(v & 0xff);
    b[1] = (uint8_t)((v >> 8) & 0xff);
    os_mbuf_append(ctxt->om, b, sizeof(b));
    return 0;
}

static const struct ble_gatt_svc_def gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &g_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &g_temp_chr_uuid.u,
                .access_cb = temp_chr_access_cb,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &g_temp_chr_handle,
            },
            {0}
        },
    },
    {0}
};

static void advertise_start(void) {
    struct ble_gap_adv_params adv_params = {0};
    struct ble_hs_adv_fields fields = {0};

    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    const char *name = "ESP-TEMP";
    fields.name = (uint8_t*)name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;

    // Advertise service UUID
    fields.uuids128 = (ble_uuid128_t*)&g_svc_uuid;
    fields.num_uuids128 = 1;
    fields.uuids128_is_complete = 1;

    ble_gap_adv_set_fields(&fields);

    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER, &adv_params, NULL, NULL);
    ESP_LOGI(TAG, "Advertising started");
}

static int gap_event_cb(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0) {
                g_conn_handle = event->connect.conn_handle;
                ESP_LOGI(TAG, "Connected");
            } else {
                ESP_LOGI(TAG, "Connect failed; restart adv");
                advertise_start();
            }
            break;

        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "Disconnected; restart adv");
            g_conn_handle = BLE_HS_CONN_HANDLE_NONE;
            advertise_start();
            break;

        default:
            break;
    }
    return 0;
}

static void notify_temp_if_connected(void) {
    if (g_conn_handle == BLE_HS_CONN_HANDLE_NONE) return;

    uint8_t b[2];
    int16_t v = g_latest_temp_centi;
    b[0] = (uint8_t)(v & 0xff);
    b[1] = (uint8_t)((v >> 8) & 0xff);

    struct os_mbuf *om = ble_hs_mbuf_from_flat(b, sizeof(b));
    ble_gatts_notify_custom(g_conn_handle, g_temp_chr_handle, om);
}

static void sensor_task(void *arg) {
    while (1) {
        g_latest_temp_centi = read_temp_centi();
        notify_temp_if_connected();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void ble_host_task(void *param) {
    nimble_port_run(); // blocks
    nimble_port_freertos_deinit();
}

void app_main(void) {
    nvs_flash_init();

    nimble_port_init();
    ble_svc_gap_init();
    ble_svc_gatt_init();

    ble_svc_gap_device_name_set("ESP-TEMP");

    ble_gatts_count_cfg(gatt_svcs);
    ble_gatts_add_svcs(gatt_svcs);

    ble_hs_cfg.sync_cb = []() {
        advertise_start();
    };

    // register GAP callback (connect/disconnect)
    ble_hs_cfg.gap_event_cb = gap_event_cb;

    xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 5, NULL);
    nimble_port_freertos_init(ble_host_task);
}
```

Notes:

* This is intentionally “single-connection, single characteristic” to keep it minimal.
* Espressif’s NimBLE GATT server tutorials are the best reference for turning this sketch into a full project. ([Espressif Systems][2])
* If you’d rather use Bluedroid, Espressif has a walkthrough for the classic `gatt_server` example too. ([GitHub][3])

---

## 3) iOS app design (SwiftUI + CoreBluetooth)

CoreBluetooth central flow is: `CBCentralManager` → scan → connect → discover services/characteristics → subscribe → receive updates. ([Apple Developer][4])

### Info.plist

Add:

* `NSBluetoothAlwaysUsageDescription`

### Architecture

* `BluetoothManager: NSObject, ObservableObject`

  * owns `CBCentralManager`
  * publishes: `state`, `devices[]`, `connectedPeripheral`, `temperatureC`
* SwiftUI screens:

  * Scan list
  * Detail screen with temperature + connect/disconnect

### Swift pseudocode (CoreBluetooth)

```swift
final class BluetoothManager: NSObject, ObservableObject, CBCentralManagerDelegate, CBPeripheralDelegate {
  @Published var devices: [CBPeripheral] = []
  @Published var temperatureC: Double? = nil

  private var central: CBCentralManager!
  private var peripheral: CBPeripheral?
  private var tempChar: CBCharacteristic?

  private let svc = CBUUID(string: "A1B2C3D4-E5F6-47A0-9A12-8D3F4C5B6A70")
  private let chr = CBUUID(string: "A1B2C3D4-E5F6-47A0-9A12-8D3F4C5B6A71")

  override init() {
    super.init()
    central = CBCentralManager(delegate: self, queue: .main)
  }

  func centralManagerDidUpdateState(_ central: CBCentralManager) {
    if central.state == .poweredOn {
      central.scanForPeripherals(withServices: [svc], options: nil) // scan
    }
  }

  func centralManager(_ central: CBCentralManager,
                      didDiscover p: CBPeripheral,
                      advertisementData: [String: Any],
                      rssi RSSI: NSNumber) {
    if !devices.contains(p) { devices.append(p) }
  }

  func connect(_ p: CBPeripheral) {
    peripheral = p
    p.delegate = self
    central.connect(p, options: nil)
  }

  func centralManager(_ central: CBCentralManager, didConnect p: CBPeripheral) {
    p.discoverServices([svc])
  }

  func peripheral(_ p: CBPeripheral, didDiscoverServices error: Error?) {
    let s = p.services?.first(where: {$0.uuid == svc})
    if let s { p.discoverCharacteristics([chr], for: s) }
  }

  func peripheral(_ p: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {
    tempChar = service.characteristics?.first(where: {$0.uuid == chr})
    if let c = tempChar {
      p.setNotifyValue(true, for: c)     // subscribe to notify :contentReference[oaicite:5]{index=5}
      p.readValue(for: c)                // optional initial read
    }
  }

  func peripheral(_ p: CBPeripheral, didUpdateValueFor c: CBCharacteristic, error: Error?) {
    guard c.uuid == chr, let data = c.value, data.count >= 2 else { return }
    let raw = Int16(littleEndian: data.withUnsafeBytes { $0.load(as: Int16.self) })
    temperatureC = Double(raw) / 100.0
  }
}
```

Resources:

* Apple CoreBluetooth central concepts / data transfer: ([Apple Developer][5])
* CBPeripheral usage: ([Apple Developer][6])

---

## 4) Android app design (Kotlin + BluetoothLeScanner/BluetoothGatt)

Android’s official guide for transferring BLE data (scan/connect/notifications) is here: ([Android Developers][7])

### Permissions (modern Android)

* Android 12+ typically needs runtime permissions like `BLUETOOTH_SCAN` and `BLUETOOTH_CONNECT` (and sometimes location depending on target/flows). Follow the official doc for your target SDK. ([Android Developers][7])

### Kotlin pseudocode (scan → connect → subscribe)

Key gotcha: notifications are a **two-step process**: `setCharacteristicNotification()` AND write the CCC descriptor (`0x2902`). ([Android Developers][7])

```kotlin
val svc = UUID.fromString("A1B2C3D4-E5F6-47A0-9A12-8D3F4C5B6A70")
val chr = UUID.fromString("A1B2C3D4-E5F6-47A0-9A12-8D3F4C5B6A71")
val ccc = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")

fun startScan() {
  val scanner = bluetoothAdapter.bluetoothLeScanner
  val filter = ScanFilter.Builder().setServiceUuid(ParcelUuid(svc)).build()
  val settings = ScanSettings.Builder().build()
  scanner.startScan(listOf(filter), settings, scanCallback)  // :contentReference[oaicite:11]{index=11}
}

val scanCallback = object : ScanCallback() {
  override fun onScanResult(callbackType: Int, result: ScanResult) {
    // show device; connect on user selection
    result.device.connectGatt(context, false, gattCallback)
  }
}

val gattCallback = object : BluetoothGattCallback() {
  override fun onConnectionStateChange(gatt: BluetoothGatt, status: Int, newState: Int) {
    if (newState == BluetoothProfile.STATE_CONNECTED) gatt.discoverServices()
  }

  override fun onServicesDiscovered(gatt: BluetoothGatt, status: Int) {
    val service = gatt.getService(svc)
    val characteristic = service.getCharacteristic(chr)

    gatt.setCharacteristicNotification(characteristic, true)

    val desc = characteristic.getDescriptor(ccc)
    desc.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
    gatt.writeDescriptor(desc)  // required for notifications :contentReference[oaicite:12]{index=12}
  }

  override fun onCharacteristicChanged(gatt: BluetoothGatt, characteristic: BluetoothGattCharacteristic) {
    if (characteristic.uuid != chr) return
    val b = characteristic.value
    val raw = (b[0].toInt() and 0xff) or ((b[1].toInt() and 0xff) shl 8)
    val signed = raw.toShort()
    val tempC = signed.toDouble() / 100.0
    // update UI
  }
}
```

---

## 5) “If possible” cross-platform (single codebase)

If you want **one app** for iOS + Android, two pragmatic choices:

### Flutter

* `flutter_blue_plus` is widely used for BLE central role (scan/connect/notify). ([Dart packages][8])
* Good if you want to ship quickly and keep one UI codebase.

### React Native

* `react-native-ble-plx` is the common pick. ([GitHub][9])

Given your use case (simple notify characteristic), both are fine.

---

## 6) Debug checklist (saves hours)

* ESP32 advertises the **service UUID** you filter on (otherwise you “see nothing”).
* Characteristic has **Notify** property, and client writes CCC (Android must do both steps). ([droidcon][10])
* If iOS connects but never gets updates, confirm you call `setNotifyValue(true, for:)` after discovering the characteristic. ([Apple Developer][11])
* If you want a standards-based shortcut, consider the Espressif “health thermometer service” example. ([ESP Component Registry][1])

---

If you tell me **which temperature probe** you’re using (DS18B20 / thermistor ADC / MAX31865 / etc.), I’ll swap the `read_temp_centi()` stub for the right ESP-IDF driver shape without changing the BLE plumbing.

[1]: https://components.espressif.com/components/espressif/ble_services/versions/0.1.0/examples/ble_services/ble_hts?language=&utm_source=chatgpt.com "0.1.0 - Example ble_services/ble_hts - ESP Component Registry"
[2]: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/ble/get-started/ble-data-exchange.html?utm_source=chatgpt.com "Data Exchange - ESP32 - — ESP-IDF Programming Guide ..."
[3]: https://github.com/espressif/esp-idf/blob/master/examples/bluetooth/bluedroid/ble/gatt_server/tutorial/Gatt_Server_Example_Walkthrough.md?utm_source=chatgpt.com "GATT Server Example Walkthrough - espressif/esp-idf"
[4]: https://developer.apple.com/documentation/corebluetooth/cbcentralmanager?utm_source=chatgpt.com "CBCentralManager | Apple Developer Documentation"
[5]: https://developer.apple.com/documentation/CoreBluetooth/transferring-data-between-bluetooth-low-energy-devices?utm_source=chatgpt.com "Transferring Data Between Bluetooth Low Energy Devices"
[6]: https://developer.apple.com/documentation/corebluetooth/cbperipheral?utm_source=chatgpt.com "CBPeripheral | Apple Developer Documentation"
[7]: https://developer.android.com/develop/connectivity/bluetooth/ble/transfer-ble-data?utm_source=chatgpt.com "Transfer BLE data | Connectivity"
[8]: https://pub.dev/packages/flutter_blue_plus?utm_source=chatgpt.com "flutter_blue_plus | Flutter package"
[9]: https://github.com/dotintent/react-native-ble-plx?utm_source=chatgpt.com "dotintent/react-native-ble-plx"
[10]: https://www.droidcon.com/2024/01/16/android-bluetooth-and-ble-the-modern-way-a-complete-guide/?utm_source=chatgpt.com "Android, Bluetooth and BLE the Modern Way: A Complete ..."
[11]: https://developer.apple.com/documentation/corebluetooth/cbcharacteristicproperties/notify?utm_source=chatgpt.com "notify | Apple Developer Documentation"


Look around the project and find relevant firmwares. Create a detaiuled analysis file like a textbook written by peter norvig on how to build and what files to reference and which drivers and how it all works, with prose paragraphs and bullet points and callouts for the fundamentals and diagrams and pseudocode and filenames.

Store the analysis in the ticket. 

Keep a detailed diary as you research at intermediate stpes as well.

Once done, upload to remarkable.
```` 

**Assistant interpretation:** Create a new `docmgr` ticket in `ttmp/`, survey existing Cardputer/ADV firmware examples in this repo, and write a detailed “how to build” analysis for a DS18B20-based temperature probe firmware that uses NimBLE to notify a phone, with an on-device UI and a USB Serial/JTAG `esp_console` REPL; then upload the resulting docs to reMarkable.

**Inferred user intent:** Get a high-quality blueprint (wiring + architecture + file references) that makes writing the actual firmware straightforward and minimizes integration mistakes (sensor timing, BLE notifications, UI/task ownership, console contamination).

**Commit (code):** N/A (docs-only work in `ttmp/`)

### What I did
- Confirmed `docmgr` was configured and created ticket `0067-cardputer-adv-ds18b20-ble-temp-ui-console`.
- Surveyed local reference firmwares:
  - `0019-cardputer-ble-temp-logger` for payload and notification expectations.
  - `0030-cardputer-console-eventbus` for UI ownership + `esp_event` decoupling + console integration.
  - `0037-cardputer-adv-fan-control-console` for USB Serial/JTAG console defaults.
  - `0066-cardputer-adv-ledchain-gfx-sim` for M5GFX canvas rendering patterns.
  - `components/wifi_console/wifi_console.c` for a production-grade `esp_console` REPL init pattern.
- Surveyed ESP-IDF 5.4.1 reference examples:
  - `/home/manuel/esp/esp-idf-5.4.1/examples/peripherals/rmt/onewire` for DS18B20 usage (`onewire_bus` + `ds18b20` component).
  - `/home/manuel/esp/esp-idf-5.4.1/examples/bluetooth/nimble/bleprph` for NimBLE peripheral lifecycle and GATT patterns.
- Wrote the analysis document in the ticket:
  - `ttmp/2026/01/23/0067-cardputer-adv-ds18b20-ble-temp-ui-console--cardputer-adv-ds18b20-temperature-over-nimble-ui-esp-console/analysis/01-how-to-build-cardputer-adv-ds18b20-nimble-notifications-ui-esp-console.md`
- Related the key reference files into the analysis doc via `docmgr doc relate`.
- Uploaded a bundled PDF of the analysis + diary to reMarkable using `remarquee upload bundle`, and verified it exists in the target remote folder.

### Why
- The fastest path to a correct implementation is to reuse known-good patterns already in the repo (UI/task ownership, REPL initialization) and anchor hardware/stack behavior to official ESP-IDF examples for DS18B20 and NimBLE.

### What worked
- `docmgr ticket create-ticket` created a correctly structured ticket workspace.
- The repo already contains solid building blocks for the UI loop and `esp_console` USB Serial/JTAG initialization.
- ESP-IDF 5.4.1 contains direct, buildable examples for both DS18B20 (via component manager) and NimBLE (bleprph).
- `remarquee upload bundle` successfully produced and uploaded a single PDF bundle for reading on-device.

### What didn't work
- Tried `docmgr doc search --limit ...` out of habit; `--limit` is not a supported flag for this docmgr build.

### What I learned
- This repo’s Cardputer/ADV conventions strongly favor **USB Serial/JTAG** for `esp_console`, which aligns well with a “debuggable sensor + BLE” firmware.
- Cardputer/ADV pin usage is highly project-specific; even within this repo, examples show specific pins reserved for keyboard/I2C, so DS18B20 GPIO should be configurable.

### What was tricky to build
- Ensuring the analysis stays NimBLE-focused even though several existing repo BLE examples use Bluedroid (the analysis explicitly calls this out and points to NimBLE examples).
- Recording the full user prompt verbatim while it contains nested code fences; used a 4-backtick fence to preserve exact content.

### What warrants a second pair of eyes
- Hardware GPIO recommendations: the analysis intentionally avoids “declaring a GPIO safe” without board-level confirmation; if you have the Cardputer ADV schematic/pin map, validate the suggested “make it configurable” approach and pick a sane default.
- Sensor terminology: if the physical probe is actually a thermocouple, the sensor stack should change (MAX3185x-style) even though the BLE/UI/console architecture can remain the same.

### What should be done in the future
- Turn this analysis into an actual firmware folder (a new `0067-.../` project) and validate:
  - DS18B20 readings on the chosen GPIO with proper pull-up.
  - NimBLE advertising/connect/notify stability with iOS and Android.
  - UI refresh and console responsiveness under load.

### Code review instructions
- Start with the analysis doc:
  - `ttmp/2026/01/23/0067-cardputer-adv-ds18b20-ble-temp-ui-console--cardputer-adv-ds18b20-temperature-over-nimble-ui-esp-console/analysis/01-how-to-build-cardputer-adv-ds18b20-nimble-notifications-ui-esp-console.md`
- Skim the linked references to sanity-check the “copy patterns” sections:
  - `components/wifi_console/wifi_console.c`
  - `0030-cardputer-console-eventbus/main/app_main.cpp`
  - `/home/manuel/esp/esp-idf-5.4.1/examples/peripherals/rmt/onewire/main/onewire_example_main.c`
  - `/home/manuel/esp/esp-idf-5.4.1/examples/bluetooth/nimble/bleprph/main/gatt_svr.c`

### Technical details
- Commands run (representative):
  - `docmgr ticket create-ticket --ticket 0067-cardputer-adv-ds18b20-ble-temp-ui-console ...`
  - `rg -n "nimble_port|ble_hs|NIMBLE|CONFIG_BT_NIMBLE" -S .`
  - `rg -n "ds18b20|onewire|1-?wire" -S .`
  - `rg -n "ds18b20" -S /home/manuel/esp/esp-idf-5.4.1`
  - `docmgr doc relate --doc ... --file-note "...:..." ...`
  - `remarquee status`
  - `remarquee upload bundle --dry-run ... --remote-dir "/ai/2026/01/23/0067-cardputer-adv-ds18b20-ble-temp-ui-console" --toc-depth 2`
  - `remarquee upload bundle ... --remote-dir "/ai/2026/01/23/0067-cardputer-adv-ds18b20-ble-temp-ui-console" --toc-depth 2`
  - `remarquee cloud ls /ai/2026/01/23/0067-cardputer-adv-ds18b20-ble-temp-ui-console --long --non-interactive`
