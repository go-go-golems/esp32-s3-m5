---
Title: Using ble> console to scan, connect, pair, and keylog a BLE keyboard
Ticket: 0020-BLE-KEYBOARD
Status: active
Topics:
    - ble
    - keyboard
    - esp32s3
    - console
    - cardputer
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0020-cardputer-ble-keyboard-host/main/ble_host.c
      Note: Implements scan/connect/pair/bonds backing the console
    - Path: 0020-cardputer-ble-keyboard-host/main/bt_console.c
      Note: |-
        Defines the ble> commands used in this playbook
        Defines ble> codes (and other commands) referenced by this playbook
    - Path: 0020-cardputer-ble-keyboard-host/main/keylog.c
      Note: Implements keylog output verification
ExternalSources: []
Summary: Step-by-step commands for using the on-device esp_console (ble>) to scan, connect, pair/bond, and print keyboard input over USB serial.
LastUpdated: 2025-12-30T21:06:06.794149804-05:00
WhatFor: ""
WhenToUse: ""
---



# Using ble> console to scan, connect, pair, and keylog a BLE keyboard

## Purpose

Teach the “operator workflow” for driving the firmware from the `ble>` console so we can reliably:

1) see the keyboard advertising,
2) connect to the *currently advertising* address (important if the keyboard uses privacy/RPA),
3) complete pairing/bonding if required,
4) enable `keylog` and verify typed keys show up over USB serial.

## Environment Assumptions

- Hardware: ESP32-S3 board flashed with `esp32-s3-m5/0020-cardputer-ble-keyboard-host/`.
- Console transport: USB Serial/JTAG. Typical port is `/dev/ttyACM0`.
- ESP-IDF environment:
  - `source /home/manuel/esp/esp-idf-5.4.1/export.sh`
- You have a BLE keyboard you can put into pairing mode.
- Known “identity” address we suspect may be stale if privacy is enabled:
  - `DC:2C:26:FD:03:B7`

## Commands

### 0) Start the monitor (recommended: tmux)

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0020-cardputer-ble-keyboard-host
source /home/manuel/esp/esp-idf-5.4.1/export.sh
idf.py -p /dev/ttyACM0 flash monitor
```

Wait for the `ble>` prompt.

### 1) Put the keyboard into pairing mode (do this first)

This is vendor-specific. The practical requirement is: it must be advertising as connectable and discoverable. Many keyboards only advertise briefly or only after a keypress; keep it “awake”.

### 2) Quick scan burst and capture the *current* advertising address

Run short scans, repeatedly, while the keyboard is in pairing mode:

```text
ble> scan on 5
```

After it completes, print the registry:

```text
ble> devices
```

What to look for in `devices` output:

- a device name that resembles the keyboard (often present only in scan response),
- and/or a strong RSSI when the keyboard is very close,
- and the address type column (`pub` vs `rand`).

If you don’t see the keyboard, repeat:

```text
ble> scan on 5
ble> devices
```

Operational note: if the keyboard uses privacy (RPA), the address may change; prefer connecting by the most-recent scan entry, not a memorized address.

### 3) Connect using the scan table index (preferred)

When you believe you have the keyboard in the list, connect by index:

```text
ble> connect <idx>
```

Why index is preferred:

- it preserves the correct discovered address type (`pub` vs `rand`),
- it avoids stale “identity address” problems.

### 4) If needed, pair / bond

After a successful connect, request encryption/bonding:

```text
ble> pair <addr>
```

You can copy `<addr>` from the `devices` output line you connected to.

If the device requests security interaction, the firmware will print hints. Respond with:

- Security request prompt:
  - `ble> sec-accept <addr> yes`
- Passkey entry request:
  - `ble> passkey <addr> 123456`
- Numeric comparison request (shows a passkey in the log):
  - `ble> confirm <addr> yes`

### 5) Turn on key logging

```text
ble> keylog on
```

Now type on the keyboard. You should see lines for key events printed over the monitor.

### 6) Disconnect / unpair as needed

```text
ble> disconnect
ble> bonds
ble> unpair <addr>
```

### 7) Print code tables (debugging helper)

If you see a hex code in logs and want the name/meaning, print the built-in tables:

```text
ble> codes
```

## Exit Criteria

- `connect <idx>` results in a stable connection (no immediate disconnect loop).
- If pairing is required, `pair <addr>` completes and the device appears in `bonds`.
- With `keylog on`, typing on the keyboard prints characters/events over USB serial.

## Notes

### Why you might want to type commands yourself

Yes: if you’re keeping the keyboard in pairing mode physically, you’re in the best position to time scans and connect attempts right when it is advertising. This materially improves success odds for devices that only advertise for short windows.

### Common failure modes and what to do

- Keyboard never appears in `devices`:
  - keep it awake (keypress), move it closer, repeat short scans.
  - assume privacy/RPA and ignore the “identity” address until you actually see it in scan.
- Connect fails with `status=0x85`:
  - ensure scanning is fully stopped before connecting (newer firmware should do this automatically).
- Disconnect reason `0x0100`:
  - often indicates connection attempt cancelled at L2CAP/GATT layer; most commonly “peer not currently connectable / not advertising at that address”.
