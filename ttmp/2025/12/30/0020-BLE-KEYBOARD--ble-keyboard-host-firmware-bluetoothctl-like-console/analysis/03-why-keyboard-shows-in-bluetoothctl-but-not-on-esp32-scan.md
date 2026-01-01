---
Title: Why keyboard shows in bluetoothctl but not on ESP32 scan
Ticket: 0020-BLE-KEYBOARD
Status: active
Topics:
    - ble
    - keyboard
    - esp32s3
    - console
    - cardputer
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0020-cardputer-ble-keyboard-host/main/ble_host.c
      Note: Shows BLE-only controller init and scan parsing details that affect discovery
    - Path: 0020-cardputer-ble-keyboard-host/main/bt_console.c
      Note: Defines scan/devices/connect/pair console workflow referenced in the investigation
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-31T18:42:27.936052663-05:00
WhatFor: ""
WhenToUse: ""
---


# Why keyboard shows in `bluetoothctl` but not on ESP32 scan

## Executive summary

This symptom usually has one of two root causes:

1) The keyboard is **not advertising over BLE (LE)** at all (it is **Classic BR/EDR HID** or dual-mode but currently in BR/EDR-only mode). Linux can discover it via BR/EDR, but the ESP32 firmware is currently **BLE-only**, so it will never see it.

2) Linux shows the keyboard in `bluetoothctl` because it is a **known/paired device**, not necessarily because it was **discovered just now** via scanning/advertising. The ESP32 only sees devices that are actively advertising during its scan window.

The fastest way to confirm which case you’re in is to use `bluetoothctl`’s transport-specific scan commands:

- If the keyboard appears in `bluetoothctl scan le`, it’s discoverable via BLE.
- If it appears only in `bluetoothctl scan bredr`, it’s Classic BR/EDR (or at least *only discoverable that way right now*).

## Key fact: our ESP32 firmware is BLE-only

In `esp32-s3-m5/0020-cardputer-ble-keyboard-host/main/ble_host.c`, the firmware:

- releases Classic memory: `esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT)`
- enables BLE-only mode: `esp_bt_controller_enable(ESP_BT_MODE_BLE)`

That means:

- it can only see devices that advertise via **LE advertising**.
- it cannot discover or connect to **BR/EDR HID** devices at all.

If your keyboard is “Bluetooth” but actually Classic-only (very common), Linux will see it immediately, and the ESP32 will never see it — no matter how long you scan.

## What `bluetoothctl devices` really means (don’t get fooled)

`bluetoothctl devices` lists “known” devices in BlueZ’s device database, not “devices discovered in the last scan window”. It can include:

- previously paired devices,
- devices discovered days ago,
- currently connected devices.

To distinguish “known” from “discovered right now”, look for fields like `RSSI` in `bluetoothctl info <addr>` (RSSI typically appears when the device is currently visible/recently seen), and run transport-specific scans (next section).

## Minimal investigation: confirm LE vs BR/EDR from `bluetoothctl`

### 1) Make sure discovery output is “fresh”

In a terminal:

```bash
bluetoothctl scan off
bluetoothctl remove <ADDR>   # optional: forget the device to avoid “cached” confusion
```

Then power-cycle the keyboard (or toggle pairing mode again).

### 2) Force LE scan and look for the keyboard

```bash
bluetoothctl scan le
```

If the keyboard is BLE-discoverable, you should see `[NEW] Device ...` events during this scan. Then:

```bash
bluetoothctl devices
bluetoothctl info <ADDR>
```

Interpretation:

- Appears during `scan le` ⇒ the keyboard is advertising via BLE (LE).
- Does not appear during `scan le` (even when in pairing mode) ⇒ likely not advertising via BLE (or not advertising BLE to “strangers” due to bonding/connection state).

### 3) Force BR/EDR scan and look for the keyboard

```bash
bluetoothctl scan bredr
```

Interpretation:

- Appears during `scan bredr` but not during `scan le` ⇒ the keyboard is Classic BR/EDR discoverable (and may be Classic-only).
- Appears during both ⇒ dual-mode (or the device has both transports visible in your environment).

### 4) Use `info` output to sanity-check the transport

Run:

```bash
bluetoothctl info <ADDR>
```

Fields that strongly hint at transport:

- **BR/EDR devices** often show a “Class” field (Bluetooth Class of Device), and UUIDs like “Human Interface Device”.
- **LE devices** often show “AddressType” (public/random), “Appearance”, and may show GATT-y UUIDs (Battery Service, Device Information, HID over GATT, etc.).

If your output only makes sense for BR/EDR, the ESP32 BLE scanner won’t ever see it.

## If it’s BLE on Linux but not on ESP32: likely causes

If `bluetoothctl scan le` clearly discovers the keyboard, but the ESP32 still doesn’t:

### Hypothesis A: the keyboard is connected/bonded to Linux and not advertising generally

Some devices advertise less (or differently) when connected, and some may use directed advertising to reconnect to a known host.

Quick test:

- Disconnect keyboard from Linux Bluetooth (or power off the Linux controller):
  - `bluetoothctl disconnect <ADDR>`
  - or toggle Linux Bluetooth off, then put keyboard into pairing mode again.
- On the ESP32, do short scan bursts while the keyboard is awake:
  - `ble> scan on 5`
  - `ble> devices`

If it appears only when Linux is out of the picture, it’s not a protocol mismatch — it’s an advertising/connection-state behavior.

### Hypothesis B: address privacy (RPA) makes you “look at the wrong thing”

If the keyboard uses privacy, its address can rotate. Linux may show an identity address (from a bond) while the air uses an RPA. Our console now supports connecting/pairing by scan index to avoid this.

ESP32 approach:

- `ble> scan on 5`
- watch `[NEW]` events; use `devices <substr>` to filter by partial address if needed
- connect/pair by index:
  - `ble> connect <idx>`
  - `ble> pair <idx>`

### Hypothesis C: LE Extended Advertising / modern PHY edge cases

Less common, but possible: the device is using features the ESP32-S3 controller/IDF stack doesn’t surface via legacy scan APIs or current scan params.

What to look for:

- Linux sees it in LE scan but the ESP32 never does, even with Linux Bluetooth off and keyboard in pairing mode.

If we land here, next steps are firmware-level:

- experiment with scan duplicate filtering, interval/window, and (if needed) the ESP-IDF extended scanning APIs.
- capture “air truth” with `btmon` on Linux to see exactly what advertising reports are present.

## What to do next (decision tree)

1) Run `bluetoothctl scan le` and `bluetoothctl scan bredr`.
2) Record:
   - which scan mode discovers the keyboard,
   - `bluetoothctl info <ADDR>` output for that device.
3) Then:
   - **Only `bredr` works** → the keyboard is Classic BR/EDR discoverable; our firmware (BLE-only) won’t see it. The fix is product/firmware direction: implement Classic HID host support or pick a BLE HID keyboard.
   - **`le` works** but ESP32 doesn’t → likely “connected/bonded advertising behavior” or an ESP32 scanning mismatch; we should compare `btmon` LE advertising reports to what ESP-IDF delivers in GAP callbacks.

## Notes: what changes would be needed if the keyboard is Classic-only

Supporting a Classic HID keyboard is a different stack from BLE HOGP:

- stop releasing Classic memory
- enable `ESP_BT_MODE_BTDM` (dual mode)
- implement BR/EDR discovery + HID host (not GATT/HOGP)

That’s a larger scope change and would be a different ticket decision, but the linux-side transport confirmation above tells us whether it’s necessary.
