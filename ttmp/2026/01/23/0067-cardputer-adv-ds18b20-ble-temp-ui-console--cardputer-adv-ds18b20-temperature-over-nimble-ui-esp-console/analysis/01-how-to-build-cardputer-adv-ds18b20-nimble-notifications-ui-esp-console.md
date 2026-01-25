---
Title: 'How to build: Cardputer ADV DS18B20 → NimBLE notifications + UI + esp_console'
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
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../../../esp/esp-idf-5.4.1/examples/bluetooth/nimble/bleprph/main/gatt_svr.c
      Note: Reference for NimBLE static GATT database + access callbacks + notifications.
    - Path: ../../../../../../../../../../esp/esp-idf-5.4.1/examples/bluetooth/nimble/bleprph/main/main.c
      Note: Reference for NimBLE host init + advertising lifecycle in ESP-IDF.
    - Path: ../../../../../../../../../../esp/esp-idf-5.4.1/examples/peripherals/rmt/onewire/main/onewire_example_main.c
      Note: Reference for onewire_bus + ds18b20 component usage and device enumeration.
    - Path: 0019-cardputer-ble-temp-logger/main/ble_temp_logger_main.c
      Note: Reference for GATT temperature payload (int16 centi-degC) + notify loop patterns (Bluedroid).
    - Path: 0030-cardputer-console-eventbus/main/app_main.cpp
      Note: Reference for Cardputer M5GFX UI task + esp_console + esp_event bus decoupling.
    - Path: 0037-cardputer-adv-fan-control-console/sdkconfig.defaults
      Note: Reference baseline for CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y on Cardputer/ADV.
    - Path: 0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp
      Note: Reference for M5GFX canvas render loop and header/status overlay.
    - Path: components/wifi_console/wifi_console.c
      Note: Reference for esp_console REPL initialization over USB Serial/JTAG.
ExternalSources: []
Summary: "A build-oriented, implementation-level analysis for a Cardputer ADV firmware that reads DS18B20 temperature and streams it over NimBLE, with an on-device UI and an esp_console REPL for debugging."
LastUpdated: 2026-01-23T15:45:48.853726569-05:00
WhatFor: "To serve as a practical reference for wiring, ESP-IDF configuration, task architecture, NimBLE GATT design, and UI/console integration for a DS18B20 temperature probe on Cardputer ADV."
WhenToUse: "Use when starting a new Cardputer ADV firmware that needs DS18B20 temperature acquisition + BLE notifications + an on-device status UI + a USB Serial/JTAG esp_console for debugging."
---

# How to build: Cardputer ADV DS18B20 → NimBLE notifications + UI + esp_console

## Executive summary

We want a single firmware that does four things well, all at once:

1. **Reads temperature** from a **DS18B20** on a 1‑Wire bus.
2. **Publishes temperature** over **BLE** using **NimBLE** (GATT server; Read + Notify characteristic).
3. **Shows a small UI** on the Cardputer display: temperature, sample age, and BLE status (advertising / connected / notifications enabled).
4. Provides an **`esp_console` REPL** over **USB Serial/JTAG** for debugging and manual control.

The main engineering trick is not the APIs; it’s keeping the system *coherent*: one source of truth for the latest temperature sample, one owner of the display, and an event-driven model so that the BLE stack, UI loop, sensor timing, and console don’t block each other.

> **Callout: “thermocouple DS18B20”**
>
> DS18B20 is *not* a thermocouple front-end; it is a **digital 1‑Wire thermometer**. If you truly have a thermocouple (K‑type, etc.), swap the sensor stack for a thermocouple ADC (e.g., MAX31855/MAX31856/MAX6675 over SPI) but keep the BLE/UI/console architecture unchanged.

## What to copy from this repo (local patterns)

This repository already contains most of the “glue” patterns we need; we just need to combine them with the ESP-IDF DS18B20 + NimBLE examples.

- **UI task that owns display + renders status lines**: `0030-cardputer-console-eventbus/main/app_main.cpp` and `0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp`
- **esp_console over USB Serial/JTAG**: `components/wifi_console/wifi_console.c` (the backend selection code is the key)
- **Cardputer/ADV console baseline config**: `0037-cardputer-adv-fan-control-console/sdkconfig.defaults`
- **Temperature payload contract (int16 centi-°C)**: `0019-cardputer-ble-temp-logger/main/ble_temp_logger_main.c` (Bluedroid, but the payload choice is still good)

From ESP-IDF 5.4.1, the core references are:

- **DS18B20 via RMT-based 1‑Wire bus**: `/home/manuel/esp/esp-idf-5.4.1/examples/peripherals/rmt/onewire/main/onewire_example_main.c`
- **NimBLE peripheral lifecycle + GATT patterns**: `/home/manuel/esp/esp-idf-5.4.1/examples/bluetooth/nimble/bleprph/main/main.c` and `/home/manuel/esp/esp-idf-5.4.1/examples/bluetooth/nimble/bleprph/main/gatt_svr.c`

## The contract: what the phone app talks to

### GATT service design (simple, stable, mobile-friendly)

Use a custom 128-bit service with a single “temperature” characteristic:

- **Device name**: `ESP-TEMP` (or `CP_TEMP_ADV`)
- **Service UUID**: `A1B2C3D4-E5F6-47A0-9A12-8D3F4C5B6A70`
- **Temperature characteristic UUID**: `A1B2C3D4-E5F6-47A0-9A12-8D3F4C5B6A71`
  - Properties: **Read + Notify**
  - Value encoding: **`int16` little-endian**, temperature in **centi‑°C**
    - Example: `2356` → 23.56°C
    - Range: DS18B20 is -55°C..125°C → -5500..12500 fits comfortably in `int16`

Optional additions (don’t do them in the MVP unless needed):

- **Status characteristic** (Read): bitfield for sensor OK / last error / resolution / notification period.
- **Control characteristic** (Write): set notify period, force a conversion, select device address if multiple DS18B20 are present.

### Why centi‑°C `int16` is the right “wire type”

- Stable across languages and platforms (Swift/Kotlin/JS).
- No float encoding debates, no endianness surprises beyond 2 bytes.
- Lets you do exact rounding in UI (e.g., show 0.01°C) without float drift.

## Hardware: DS18B20 wiring on a Cardputer ADV

### DS18B20 basics (what the firmware should assume)

- 1‑Wire is a **single data wire** plus **GND** (and typically **VDD**).
- DS18B20 requires a **pull‑up resistor** (commonly **4.7kΩ**) from DATA to VDD.
- The ESP-IDF example explicitly warns: **parasite power mode is not supported**; wire VDD.

### Pin selection: treat GPIO as a configuration, not a constant

You can run 1‑Wire on almost any GPIO, but *Cardputer/ADV projects repurpose pins*.

Concrete “avoid” evidence in this repo:

- The Cardputer‑ADV keyboard example uses **I2C0 on GPIO8 (SDA) + GPIO9 (SCL)** and an interrupt on **GPIO11** (`0036-cardputer-adv-led-matrix-console/main/matrix_console.c`).

So:

- Make the 1‑Wire pin a **Kconfig option**: `CONFIG_TUTORIAL_0067_ONEWIRE_GPIO`.
- Default it to something safe *for your hardware build*, but assume it will change.

### Wiring diagram (single sensor)

```
Cardputer ADV (ESP32-S3)                      DS18B20

3V3  ---------------------------------------> VDD
GND  ---------------------------------------> GND
GPIOx --------------------------------------> DQ (DATA)
3V3  ---[ 4.7kΩ pull-up ]-------------------> DQ (DATA)
```

> **Callout: long cable / probe**
>
> If the DS18B20 is on a long lead (probe cable), expect more reflections/noise:
> - keep pull-up close to the MCU side
> - consider lowering resolution (faster conversions) and sampling slower
> - consider stronger pull-up (e.g., 2.2kΩ) if edges are too slow (watch current draw)

## Software architecture: make the whole system predictable

### The “one owner per resource” rule (prevents most bugs)

- **Display**: owned by a single UI task. Everyone else posts messages/events.
- **Temperature state**: owned by a “sensor manager” (task or component). Others read snapshots.
- **BLE stack**: NimBLE host runs in its own task; application code interacts via callbacks + thread-safe state and a small “notify request” function.
- **Console**: runs as a REPL task; commands must not touch display directly.

### The component boundaries (recommended)

Even if you implement the MVP in `main/`, designing as components keeps coupling low:

- `components/temp_probe_ds18b20/`
  - Creates 1‑Wire bus + DS18B20 device handle(s)
  - Owns the conversion/read schedule
  - Exposes “latest sample” and emits events on updates
- `components/ble_temp_nimble/`
  - Owns the GATT database and GAP callbacks
  - Exposes `ble_temp_notify_latest()` and connection status
- `components/temp_ui_m5gfx/`
  - UI task that displays temperature + BLE state
- `components/temp_console/`
  - Registers REPL commands (status, period, rescan, etc.)

### Event topology (diagram)

```
                   +--------------------+
                   |   esp_console REPL |
                   | (USB Serial/JTAG)  |
                   +----+-----------+---+
                        | commands  |
                        v           |
                +----------------+  |
                |   Event Bus    |  |
                |  (esp_event)   |  |
                +--+----------+--+  |
                   |          |     |
                   |          |     |
      temp updates  |          |  BLE status changes
                   v          v
          +------------+   +----------------+
          |  UI Task   |   |  BLE Service   |
          | (M5GFX)    |   |  (NimBLE)      |
          +-----+------+   +--------+-------+
                ^                   ^
                | read latest       | notify read/notify
                | snapshot          | char access callback
                |                   |
          +-----+-------------------+-----+
          |     Temp Probe Manager        |
          | (1-Wire + DS18B20 schedule)   |
          +-------------------------------+
```

## DS18B20 in ESP-IDF 5.4.1: driver model and timing

The ESP-IDF 5.4.1 example uses two layers:

1. **`onewire_bus`**: bus implementation that uses **RMT TX/RX** to generate/measure 1‑Wire waveforms.
2. **`ds18b20`**: a device layer that sits on top and gives you “trigger conversion” and “read temperature”.

### Pseudocode (single device)

```c
// init
onewire_bus_handle_t bus = onewire_new_bus_rmt(gpio);
ds18b20_device_handle_t dev = ds18b20_discover_first(bus); // via onewire iterator
ds18b20_set_resolution(dev, DS18B20_RESOLUTION_12B);

// loop
for (;;) {
  ds18b20_trigger_temperature_conversion(dev);
  // Conversion time depends on resolution (12-bit worst case ~750ms).
  vTaskDelay(pdMS_TO_TICKS(750));
  float temp_c;
  ds18b20_get_temperature(dev, &temp_c);
  publish_sample(temp_c);
}
```

### “Conversion time” is the real scheduler constraint

If you block a shared task for 750ms, you will eventually:

- miss UI frames,
- starve the console input,
- create ugly BLE notify jitter.

So the DS18B20 layer should be its own task (or a small state machine driven by a timer):

1. **Kick conversion**
2. Wait (delay)
3. **Read temperature**
4. Publish sample and repeat

### Multiple DS18B20 sensors (optional)

DS18B20 devices have unique 64-bit addresses. The ESP-IDF example enumerates devices on the bus. If you ever add more sensors, the firmware should:

- enumerate once at boot (or on `console> ds18 rescan`)
- store the addresses
- read each sequentially and publish “sensor index” or “address” in status

## NimBLE in ESP-IDF: the minimal, correct peripheral shape

### What “NimBLE host” wants you to do

The NimBLE stack is happiest when you accept its separation of roles:

- You define a **GATT database** (static structs, callbacks for reads/writes).
- You define GAP event handling (connect/disconnect/subscribe).
- You let `nimble_port_freertos_init()` run the host task.

### Threading model: treat NimBLE callbacks as “interrupt-like”

Do not do long work in NimBLE callbacks. In particular:

- **Do not talk to the display** in NimBLE callbacks.
- **Do not trigger DS18B20 conversions** in NimBLE callbacks.

Instead:

- update small atomic/shared state (`connected`, `conn_handle`, `subscribed`)
- post an event to the event bus if the UI needs to redraw

### GATT implementation sketch (temperature characteristic)

Key design choices:

- Store `latest_temp_centi` in a single global (or component-owned) variable.
- The characteristic read callback returns that value (2 bytes).
- Notification sends the same 2-byte payload to the subscribed peer.

Pseudocode:

```c
static volatile int16_t latest_temp_centi;
static uint16_t temp_char_handle;
static uint16_t conn_handle = BLE_HS_CONN_HANDLE_NONE;
static volatile bool subscribed;

// READ callback
int temp_access_cb(...) {
  int16_t v = latest_temp_centi;
  uint8_t le[2] = { v & 0xff, (v >> 8) & 0xff };
  os_mbuf_append(ctxt->om, le, 2);
  return 0;
}

// When a new temperature sample arrives
void ble_temp_notify_latest(void) {
  if (conn_handle == BLE_HS_CONN_HANDLE_NONE || !subscribed) return;
  int16_t v = latest_temp_centi;
  uint8_t le[2] = { v & 0xff, (v >> 8) & 0xff };
  struct os_mbuf *om = ble_hs_mbuf_from_flat(le, 2);
  ble_gatts_notify_custom(conn_handle, temp_char_handle, om);
}
```

### Advertising: include the service UUID

Mobile clients often scan with a “service filter” for stability. That only works if the peripheral advertises the service UUID.

So your advertising fields should include:

- flags (general discoverable, BR/EDR unsupported)
- complete local name
- your 128-bit service UUID

## UI: simplest usable design (M5GFX, text-first)

### A good MVP screen (what it should show)

At a minimum:

- Temperature: `23.56°C`
- Sample age: `age=0.4s` or `stale` if too old
- BLE status:
  - `adv=on/off`
  - `conn=connected/disconnected`
  - `notify=subscribed/not`
- Sensor status:
  - `ds18=ok` or last error code
  - bus GPIO and discovered ROM (optional)

### UI update strategy

Prefer event-driven redraw:

- new temperature sample → mark UI dirty
- BLE connect/disconnect/subscribe change → mark UI dirty
- timer tick (e.g., 250ms) → update “age” and redraw header

The M5GFX pattern in `0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp` is a good template:

- create a canvas sprite
- render into it
- `pushSprite()` every frame/tick

## Console: debugging without contaminating UART pins

### Baseline requirement

Use **USB Serial/JTAG** for the REPL by default:

```
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y
# CONFIG_ESP_CONSOLE_UART is not set
```

This is already the established repository convention (`0037-cardputer-adv-fan-control-console/sdkconfig.defaults`).

### Command set (recommended)

Keep commands small and composable; mirror the mental model of the system:

- `temp status` → last sample, age, last error, GPIO, ROM
- `temp period set <ms>` → sampling period
- `temp res set <9|10|11|12>` → DS18B20 resolution
- `temp rescan` → enumerate devices again (safe when disconnected)
- `ble status` → adv/conn/subscribed, conn params
- `ble adv on|off` → advertising control
- `ble notify on|off` → software gate (even if subscribed)

The `esp_console` initialization pattern in `components/wifi_console/wifi_console.c` is the copy/paste reference: pick backend by Kconfig, register commands, start REPL.

## Configuration: what goes in `sdkconfig.defaults`

### Console

- `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`
- disable UART console by default (only enable if you intentionally reserve pins)

### BLE: NimBLE not Bluedroid

In `sdkconfig.defaults` for the new firmware:

- enable BT + BLE
- enable **NimBLE**
- disable **Bluedroid**

Conceptually:

```
CONFIG_BT_ENABLED=y
CONFIG_BT_NIMBLE_ENABLED=y
# CONFIG_BT_BLUEDROID_ENABLED is not set
CONFIG_BT_CLASSIC_ENABLED=n
```

> **Callout: repo precedent**
>
> Tutorial `0019-cardputer-ble-temp-logger` uses Bluedroid and BLE 5 extended advertising APIs. For this ticket we *intentionally choose NimBLE* to keep the host lighter and to match NimBLE examples and patterns.

### Memory / stacks

Plan for:

- UI task stack: **~8k** (M5GFX init can be stack hungry; see `0066.../sim_ui.cpp`)
- main task stack: **increase** if you do heavy init in `app_main` (several tutorials use 8000)

## Build: how the new firmware should be shaped on disk

### Directory layout

Follow the existing tutorial layout:

```
0067-cardputer-adv-ds18b20-ble-temp-ui-console/
  CMakeLists.txt
  main/
    CMakeLists.txt
    app_main.c / app_main.cpp
    idf_component.yml   # pulls ds18b20 component
  sdkconfig.defaults
  partitions.csv
```

### `idf_component.yml` dependency for DS18B20

The ESP-IDF example uses:

```
dependencies:
  ds18b20: "^0.1.0"
```

This pulls the DS18B20 component (and its transitive dependencies) via the ESP-IDF component manager.

## Debugging checklist (practical failure modes)

### DS18B20 common failures

- “no devices found”
  - wrong GPIO
  - missing pull-up
  - parasite-power wiring (driver/example doesn’t support it)
- “intermittent reads”
  - cable noise / weak pull-up
  - GPIO conflicts with other peripherals

### BLE common failures

- phone can’t find device
  - you aren’t advertising the service UUID (or name differs)
- connects but no notifications
  - client didn’t subscribe (CCCD not set); show `subscribed` in UI/console
  - you notify too fast; add rate limiting

### Console common failures

- REPL never appears
  - console backend not enabled in `sdkconfig.defaults`
  - using UART console but pins are repurposed; prefer USB Serial/JTAG

## A minimal “end-to-end” test plan

1. Boot with sensor attached; confirm `temp status` reports “ok” and shows a plausible °C value.
2. Confirm UI updates at the expected cadence (age decreases, value changes when you heat/cool probe).
3. Scan from phone and connect; verify UI shows `connected`.
4. Subscribe to notifications; verify UI shows `subscribed` and phone receives int16 payloads.
5. Disconnect/reconnect; verify advertising restarts and the UI follows state.


