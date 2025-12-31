---
Title: "E2E Test: Cardputer BLE Temp Logger (BLE 5.0 ext adv) with tmux"
Ticket: 0019-BLE-TEST
Status: active
Topics:
    - ble
    - esp32s3
    - cardputer
    - esp-idf
    - linux
DocType: playbook
Intent: short-term
Owners: []
RelatedFiles:
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0019-cardputer-ble-temp-logger/main/ble_temp_logger_main.c
      Note: Firmware (Bluedroid GATTS + BLE 5.0 extended advertising)
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0019-cardputer-ble-temp-logger/tools/ble_client.py
      Note: Host-side Python client (bleak) for scan/connect/read/notify/control
ExternalSources: []
Summary: Run the firmware, monitor logs, and validate scanning/connecting/notifications from Linux using a tmux 2-pane workflow.
LastUpdated: 2025-12-31T00:00:00Z
WhatFor: "Fast manual validation of the full pipeline: firmware advertising + host connect + GATT read + notifications."
WhenToUse: "After flashing the 0019 firmware or when debugging BLE advertising/notify behavior."
---

# E2E Test: Cardputer BLE Temp Logger (BLE 5.0 ext adv) with tmux

## Preconditions

- Cardputer connected via USB (USB-Serial-JTAG shows up as `/dev/ttyACM0` and/or `/dev/serial/by-id/...Espressif...`).
- BlueZ installed and Bluetooth adapter powered on.
- ESP-IDF 5.4.1 installed at `/home/manuel/esp/esp-idf-5.4.1`.

Project:

- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0019-cardputer-ble-temp-logger`

## tmux layout

In one terminal:

```bash
tmux new -s 0019-ble && tmux split-window -h
```

Left pane: firmware flash+monitor. Right pane: host BLE client work.

## Pane A (left): flash + monitor

### Prefer the script (handles ESP-IDF env + port selection)

```bash
bash tools/run_fw_flash_monitor.sh
```

If you need to override the port or action:

```bash
PORT='/dev/ttyACM0' ACTION='flash monitor' bash tools/run_fw_flash_monitor.sh
```

### Manual command (equivalent)

Use a stable by-id port if available:

```bash
PORT='/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:0E:BE:00-if00' && \
source /home/manuel/esp/esp-idf-5.4.1/export.sh && \
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0019-cardputer-ble-temp-logger && \
idf.py -p "$PORT" flash monitor
```

If you only have `/dev/ttyACM0`:

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && \
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0019-cardputer-ble-temp-logger && \
idf.py -p /dev/ttyACM0 flash monitor
```

### Expected firmware log landmarks

- `boot: CP_TEMP_LOGGER`
- Extended advertising sequence:
  - `ext adv params set: status=0 ...`
  - `ext adv data set: status=0 ...`
  - `ext adv start complete: status=0 ...`
- On connect:
  - `connect: conn_id=...`
- On subscription:
  - `cccd write: 0x0001 => notify=1`
- Periodic stats lines:
  - `stats reason=periodic ... notifies_ok=... heap_min=...`

## Pane B (right): host-side BLE validation

### Prefer the script

Scan:

```bash
bash tools/run_host_ble_client.sh --timeout 5 scan
```

Connect + read + notify:

```bash
bash tools/run_host_ble_client.sh run --name CP_TEMP_LOGGER --set-period-ms 1000 --read --notify --duration 20
```

### 1) Confirm adapter is up

```bash
bluetoothctl show | sed -n '1,80p'
```

If powered off:

```bash
bluetoothctl power on
```

### 2) Install the host client dependency (bleak)

The repo’s client script uses `bleak`. Install once:

```bash
python3 -m pip install --user --upgrade bleak
```

### 3) Scan for the device

```bash
python3 /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0019-cardputer-ble-temp-logger/tools/ble_client.py scan --timeout 5
```

You’re looking for a name containing:

- `CP_TEMP_LOGGER`

### 4) Connect + read + subscribe (notifications)

```bash
python3 /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0019-cardputer-ble-temp-logger/tools/ble_client.py run \
  --name CP_TEMP_LOGGER \
  --set-period-ms 1000 \
  --read \
  --notify --duration 20
```

Expected output:

- `read temp: XX.XX C`
- repeated `notify temp: ...`

### 5) Change notify rate via control characteristic

```bash
python3 /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0019-cardputer-ble-temp-logger/tools/ble_client.py run \
  --name CP_TEMP_LOGGER \
  --set-period-ms 250 \
  --notify --duration 10
```

### 6) Disable the notify loop (period=0)

```bash
python3 /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0019-cardputer-ble-temp-logger/tools/ble_client.py run \
  --name CP_TEMP_LOGGER \
  --set-period-ms 0 \
  --read
```

## Optional: capture HCI traffic (btmon)

If you want a ground-truth trace:

```bash
sudo btmon
```

Tip: put this in a third tmux window (`tmux new-window`) so it can run long.

## Troubleshooting checklist

- **Device not found during scan**:
  - Verify firmware logs show `ext adv start complete: status=0`.
  - Run `bluetoothctl scan on` in another shell and see if `CP_TEMP_LOGGER` appears.
  - Check for rfkill:

```bash
rfkill list && sudo rfkill unblock bluetooth
```

- **Connect works but notifications never show**:
  - Confirm firmware logs show CCCD write `0x0001` (subscription).
  - Re-run the client with `--read` to confirm GATT read path works.


