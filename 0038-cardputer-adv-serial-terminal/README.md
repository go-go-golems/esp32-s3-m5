## Tutorial 0038 — Cardputer-ADV Serial Terminal (ADV keyboard → screen + USB/UART) (ESP-IDF 5.4.1)

This tutorial is a **graphical text terminal** for Cardputer using the **Cardputer-ADV keyboard (TCA8418 over I²C)**:

- Type on the ADV keyboard → see what you type on the **LCD**
- The same bytes are sent to a menuconfig-selectable serial backend:
  - **USB-Serial-JTAG** (host `/dev/ttyACM*`), or
  - **GROVE UART** (configurable UART port + TX/RX GPIOs + baud; defaults RX=G1, TX=G2)

In parallel, the firmware starts an **`esp_console` REPL over USB Serial/JTAG** (prompt `adv>`), so you can run interactive commands without stealing the UART pins.

By default, the serial backend is set to **UART** so the USB Serial/JTAG transport stays available for `esp_console`. If you switch the backend to **USB-Serial-JTAG**, the firmware will skip starting `esp_console` to avoid fighting over USB RX.

### Keyboard (Cardputer-ADV)

- I2C SDA: `GPIO8`
- I2C SCL: `GPIO9`
- INT: `GPIO11`
- Address (7-bit): `0x34`

### Fan relay (GPIO3) control

If enabled (default), the firmware controls a relay on `GPIO3` (active-low by default) via:

- On-device hotkeys:
  - `opt` + `1`: toggle
  - `opt` + `2`: on
  - `opt` + `3`: off
  - `opt` + `4`: preset `slow` (2s/2s)
  - `opt` + `5`: preset `pulse` (0.25s on / 2.75s off)
  - `opt` + `6`: preset `double`
  - `opt` + `7`: stop animation
- On-device command line: type `fan ...` and press Enter (runs locally; not sent to the serial backend).
- Host REPL over USB Serial/JTAG: `help`, `fan help`, `fan preset`, etc.

### Build (ESP-IDF 5.4.1)

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0038-cardputer-adv-serial-terminal && \
./build.sh build
```

### Flash + Monitor

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0038-cardputer-adv-serial-terminal && \
./build.sh -p /dev/ttyACM0 flash monitor
```

### tmux helper

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0038-cardputer-adv-serial-terminal && \
./build.sh tmux-flash-monitor
```
