## Tutorial 0038 — Cardputer-ADV Serial Terminal (ADV keyboard → screen + USB/UART) (ESP-IDF 5.4.1)

This tutorial is a **graphical text terminal** for Cardputer using the **Cardputer-ADV keyboard (TCA8418 over I²C)**:

- Type on the ADV keyboard → see what you type on the **LCD**
- The same bytes are sent to a menuconfig-selectable serial backend:
  - **USB-Serial-JTAG** (host `/dev/ttyACM*`), or
  - **GROVE UART** (configurable UART port + TX/RX GPIOs + baud; defaults RX=G1, TX=G2)

### Keyboard (Cardputer-ADV)

- I2C SDA: `GPIO8`
- I2C SCL: `GPIO9`
- INT: `GPIO11`
- Address (7-bit): `0x34`

### Relay hotkeys (local GPIO control)

If enabled (default), the firmware also controls a relay on `GPIO3` (active-low by default):

- `opt` + `1`: toggle relay
- `opt` + `2`: relay on
- `opt` + `3`: relay off

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
