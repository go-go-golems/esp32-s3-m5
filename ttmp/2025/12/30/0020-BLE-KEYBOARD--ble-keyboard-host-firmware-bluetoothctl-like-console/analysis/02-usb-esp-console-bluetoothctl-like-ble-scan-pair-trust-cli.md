---
Title: USB esp_console — bluetoothctl-like BLE scan/pair/trust CLI
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
RelatedFiles: []
ExternalSources: []
Summary: 'Design for a USB Serial/JTAG esp_console REPL that exposes bluetoothctl-like commands: scan, connect, pair, unpair, trust, info, bonds.'
LastUpdated: 2025-12-30T20:04:36.819368253-05:00
WhatFor: ""
WhenToUse: ""
---

# USB `esp_console` — bluetoothctl-like BLE scan/pair/trust CLI

## Executive summary

We want a control plane that feels like a tiny subset of Linux `bluetoothctl`, but runs on the ESP32-S3 itself and is reachable over **USB Serial/JTAG**. The control plane should:

- drive scanning and show discovered devices (address, RSSI, name, hints that it’s a keyboard)
- initiate connect / disconnect
- manage pairing/bonding (pair, unpair, list bonds)
- implement “trust” as an app policy (auto-reconnect) on top of the bond DB
- show status/debug info (current state, connection, security mode, HID reports seen)

This analysis anchors the console on ESP-IDF `esp_console` (not a custom REPL), using a known-good in-repo pattern.

## Reference implementations already in this repo

### `esp_console` over USB Serial/JTAG

- `esp32-s3-m5/0013-atoms3r-gif-console/main/console_repl.cpp`
  - shows:
    - `esp_console_new_repl_usb_serial_jtag()`
    - `esp_console_cmd_register()` for multiple commands
    - `esp_console_start_repl()`

### Known-good sdkconfig defaults for USB console

- `esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/sdkconfig.defaults`
- `esp32-s3-m5/0017-atoms3r-web-ui/sdkconfig.defaults`

Both set:

- `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`
- `CONFIG_ESP_CONSOLE_UART_DEFAULT=n`
- `CONFIG_ESP_CONSOLE_UART_CUSTOM=n`
- `CONFIG_ESP_CONSOLE_USB_CDC=n`

## Console architecture (recommended)

### Layering

Keep the console “thin” and delegate all BLE behavior to a `ble_host` module:

```
  esp_console cmd handlers
        |
        v
  bt_console.c  (parse args; print)
        |
        v
  ble_host.c    (scan/connect/security/bonds state machine)
        |
        v
  GAP + GATTC callbacks / esp_hidh events
```

This avoids a common failure mode: “console handler does BLE work directly while a callback is also mutating state”.

### Console thread model

`esp_console_start_repl()` runs the line editor / command loop in its own task(s). The safest design is:

- command handlers enqueue “requests” to the `ble_host` task (or take a mutex and call a non-blocking API)
- GAP/GATTC callbacks never block on console I/O (they can enqueue log lines)

Pseudocode:

```c
// cmd handler thread
int cmd_scan(int argc, char **argv) {
  ble_host_request_scan(/*on=*/true, /*seconds=*/30);
  printf("scan started (30s)\n");
  return 0;
}

// BLE task
void ble_host_task(void*) {
  for (;;) {
    req = wait_for_request_or_callback_event();
    apply_state_machine(req);
  }
}
```

## Proposed command set (v0)

Aim for a small, scriptable, copy/paste-friendly set.

### Discovery

- `scan on [seconds]` / `scan off`
- `devices` (list recent scan results as a registry)

### Connection

- `connect <index|addr>`
- `disconnect [addr]`

### Pairing / bonding / trust

- `pair <addr>`
- `bonds`
- `unpair <addr>`
- `trust <addr>` / `untrust <addr>`

### Interactive security prompts (only if needed)

If GAP events indicate a pending prompt, provide explicit commands:

- `passkey <addr> <6digits>` → `esp_ble_passkey_reply(...)`
- `confirm <addr> yes|no` → `esp_ble_confirm_reply(...)`
- `sec-accept <addr> yes|no` → `esp_ble_gap_security_rsp(...)`

## Mapping `bluetoothctl` concepts to ESP-IDF primitives

### “paired”

Paired/bonded devices are tracked by Bluedroid’s security database:

- `esp_ble_get_bond_device_num()`
- `esp_ble_get_bond_device_list(...)`
- `esp_ble_remove_bond_device(...)`

### “trusted”

ESP-IDF does not expose a separate “trust store”. Implement trust as an app policy:

- `trusted = (bonded && nvs_flag_trust[address] == true)`
- `ble_host` uses this to decide whether to auto-connect on boot / after disconnect

## GAP events to surface to the console

Hook these in your `esp_ble_gap_register_callback(gap_cb)` handler and print succinct messages (or enqueue them to a “console log” queue):

- `ESP_GAP_BLE_AUTH_CMPL_EVT` (authentication complete)
- `ESP_GAP_BLE_SEC_REQ_EVT` (peer requests security)
- `ESP_GAP_BLE_PASSKEY_NOTIF_EVT` / `ESP_GAP_BLE_PASSKEY_REQ_EVT`
- `ESP_GAP_BLE_NC_REQ_EVT` (numeric comparison)

## `esp_console` implementation sketch (from repo patterns)

Follow the same shape as `esp32-s3-m5/0013-atoms3r-gif-console/main/console_repl.cpp`:

1) Build REPL (USB Serial/JTAG):

```c
esp_console_repl_t *repl = NULL;
esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
repl_config.prompt = "ble> ";

esp_console_dev_usb_serial_jtag_config_t hw_config =
  ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();

ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl));
```

2) Register commands:

```c
esp_console_cmd_t cmd = {};
cmd.command = "scan";
cmd.help = "scan on [seconds] | scan off";
cmd.func = &cmd_scan;
ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
```

3) Start REPL:

```c
esp_console_register_help_command();
ESP_ERROR_CHECK(esp_console_start_repl(repl));
```

## Suggested device registry output format

Prefer a stable, grep-friendly output:

```
idx  addr              type rssi  kbd  name
0    d1:22:33:44:55:66 rand -58   yes  Keychron K3
1    12:34:56:78:9a:bc pub  -72   no   (no name)
```

## Failure modes to design around

- “connect while scanning” edge cases: stop scan before opening a device, or enforce via state machine.
- security prompts without UI: for keyboards that demand passkey/numeric comparison, the console must provide `passkey/confirm` commands.
- bond DB vs “trust”: avoid claiming “trusted” if not bonded; treat trust as an app-level policy layered on bonds.
- log spam: HID input can be frequent; gate key logging behind `keylog on/off` or rate-limit prints.

