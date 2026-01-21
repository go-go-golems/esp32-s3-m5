# Tutorial 0047 — Cardputer-ADV LVGL List + Chain Encoder navigation (U207 over Grove UART) (ESP-IDF 5.4.1)

This firmware brings up **LVGL** on Cardputer-ADV (via **M5GFX**) and shows a scrollable **list** that you can navigate using an attached **M5Stack Chain Encoder (U207)** over the **Grove UART** pins.

## Wiring (as tested)

- Chain Encoder `TX1` → Cardputer `G1` (RX, default `GPIO1`)
- Chain Encoder `RX`  → Cardputer `G2` (TX, default `GPIO2`)

UART settings (per protocol): `115200 8N1`.

## Build

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0047-cardputer-adv-lvgl-chain-encoder-list && \
./build.sh build
```

## Flash + Monitor (single process; avoids port-lock issues)

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0047-cardputer-adv-lvgl-chain-encoder-list && \
./build.sh -p /dev/ttyACM0 flash monitor
```

Or use the stable by-id path:

```bash
./build.sh -p /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_* flash monitor
```

### If flashing is flaky (USB-Serial/JTAG reconnects mid-flash)

If you see `Lost connection, retrying...` and then a pySerial error like `Could not configure port: (5, 'Input/output error')`,
try:

- Lower baud: `./build.sh -p <port> -b 115200 flash`
- Unplug/replug the USB cable (or power-cycle the device) so `/dev/ttyACM*` reappears, then retry.

## tmux helper

```bash
./build.sh tmux-flash-monitor
```

## What to expect (smoke test)

- On screen: a list of items with one focused/highlighted row.
- Rotate encoder: focus moves up/down and list scrolls.
- Click encoder: logs a “clicked item” message over USB Serial/JTAG.

## Playbooks / references

- Flash/monitor workflow (tmux, one-pane `flash monitor`): `ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/playbook/01-tmux-workflow-build-flash-monitor-esp-console-usb-serial-jtag.md`
- Build/flash/monitor validation checklist patterns: `ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/design/07-playbook-build-flash-monitor-validate.md`
