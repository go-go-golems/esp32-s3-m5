# 0118-cores3-qrcode-scanner (ESP-62)

ESP-IDF firmware for the **M5Stack CoreS3** (ESP32-S3) stacked with the
**Module13.2 QRCode** (SKU M145). Reads 1D/2D barcodes and QR codes and shows
each decoded code on the CoreS3 LCD.

## Hardware

- **CoreS3** (ESP32-S3, ILI9341 320×240 LCD, USB Serial/JTAG).
- **Module13.2 QRCode** stacked under the CoreS3; on-module **DIP switch set to UART**.
- **12 V DC** on the module barrel jack (5.5×2.1 mm, center-positive) — required to
  power the scanner engine + the whole stack. USB alone is not enough.
- The CoreS3 drives the engine over **UART1 (TX=G13 → QR_RX on M5-Bus pin 23,
  RX=G14 ← QR_TX on pin 26)** at 115200 8N1, and controls power (`QR_5V_EN`) + hardware trigger (`TRIG`) through
  the module's **PI4IOE5V6408** I2C GPIO expander at address **0x43** (on In_I2C).

## Build (IDF 5.3.4)

```bash
unset IDF_PYTHON_ENV_PATH
source ~/esp/esp-idf-5.3.4/export.sh   # 5.3.4 pinned (AGENTS.md)
cd 0118-cores3-qrcode-scanner
idf.py set-target esp32s3
idf.py build
```

## Flash + monitor

The CoreS3's USB Serial/JTAG needs the `cdc_acm` driver; if `/dev/ttyACM0` is
missing, load it once: `sudo modprobe cdc_acm`.

```bash
idf.py -p /dev/ttyACM0 flash monitor
```

## Use

- **On screen:** aim at a code and tap anywhere. The UI enqueues a 100 ms
  active-low hardware TRIG pulse; decoded UART text appears in the center and
  recent codes scroll in the history list. The diagnostic UI does not write
  trigger-mode settings.
- **USB console** (`cores3-qr> `):
  ```
  qr status            # read module firmware + serial (the on-device probe)
  qr start | stop      # software trigger
  qr mode <key|cont|auto|pulse|sense>
  qr light <off|decode|on>
  qr brightness <0-100>
  qr beep <on|off>
  qr reset             # factory reset (careful)
  ```

If `qr status` returns **NO REPLY** but a hardware trigger still activates the
illumination/aiming light, the scanner may have persisted a USB communication
mode. UART commands cannot recover that state. Scan the official **Serial
Communication** programming barcode `21424000` from page 9 of
`ZBarcode-Scanner-User-Guide-2.5-EN.pdf`, then power-cycle and rerun
`qr status`. A ready-to-display crop is archived in the ESP-62 ticket at
`various/2026-08-23-serial-communication-recovery-barcode.png`.

## Layout

- `main/app_main.cpp` — boot: M5Unified → module → UI → console.
- `main/qr_engine.{h,cpp}` — M14-Pro UART protocol (port of the official Arduino
  `qrcode_m14.cpp`).
- `main/qr_module.{h,cpp}` — PI4IOE5V6408 power/TRIG + UART1 + scan pump → queue.
- `main/qr_console.{h,cpp}` — esp_console REPL over USB Serial/JTAG.
- `main/qr_ui.{h,cpp}` — M5GFX display task (current code + history + buttons).

See the design guide:
`ttmp/2026/08/23/ESP-62-CORES3-QRCODE--esp-idf-cores3-module13-2-qrcode-barcode-qr-scanner-with-on-screen-display-intern-guide/design-doc/01-...-guide.md`
