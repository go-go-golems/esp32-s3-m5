## Cardputer Tutorial 0015 — Serial Terminal (keyboard → screen + UART/USB via ESP-IDF 5.4.1)

This tutorial is a **Cardputer serial terminal** app:

- Type on the built-in **keyboard** → see what you type on the **LCD**
- The same bytes are sent to a menuconfig-selectable serial backend:
  - **USB-Serial-JTAG** (host `/dev/ttyACM*`), or
  - **GROVE UART** (configurable UART port + TX/RX GPIOs + baud; defaults RX=G1, TX=G2)

It reuses known-good building blocks already present in this repo:

- **Display bring-up (M5GFX autodetect)**: `esp32-s3-m5/0011-cardputer-m5gfx-plasma-animation/`
- **Keyboard scan (GPIO matrix scan)**: `esp32-s3-m5/0007-cardputer-keyboard-serial/`
- **Typewriter buffer + redraw loop**: adapted from `esp32-s3-m5/0012-cardputer-typewriter/`

### Build (ESP-IDF 5.4.1)

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && \
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0015-cardputer-serial-terminal && \
idf.py set-target esp32s3 && idf.py build
```

### Flash + Monitor

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && \
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0015-cardputer-serial-terminal && \
idf.py flash monitor
```

### Flash tip: use stable by-id path + conservative baud

If `/dev/ttyACM0` disappears/re-enumerates during flashing, use the stable by-id path:

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && \
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0015-cardputer-serial-terminal && \
idf.py -p '/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:0E:BE:00-if00' -b 115200 flash monitor
```

### Configure (menuconfig)

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && \
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0015-cardputer-serial-terminal && \
idf.py menuconfig
```

Go to:

- `Tutorial 0015: Cardputer Serial Terminal`

### Expected behavior

- The LCD shows a simple terminal-style text buffer.
- Typing on the Cardputer keyboard:
  - updates the on-screen buffer immediately (local echo; configurable)
  - transmits bytes to the configured serial backend (USB-Serial-JTAG or UART)
- If “Show incoming bytes” is enabled, received serial data is appended to the on-screen buffer.

### GROVE defaults (G1/G2)

This tutorial treats the GROVE port as a simple external UART:

- **Default RX**: G1 (`GPIO1`)
- **Default TX**: G2 (`GPIO2`)

If you wire a USB↔UART adapter, remember to cross lines:

- Adapter TX → Cardputer RX (G1)
- Adapter RX → Cardputer TX (G2)



