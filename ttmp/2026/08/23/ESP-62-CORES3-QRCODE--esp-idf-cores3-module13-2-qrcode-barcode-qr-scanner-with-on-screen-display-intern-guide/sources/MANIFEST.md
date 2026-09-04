# Sources — Provenance & Manifest

All sources gathered for the ESP-62 CoreS3 + Module13.2 QRCode design. Every
entry records the original URL, what it contains, and why it matters for this
firmware. Binary PDFs are committed alongside extracted plain-text where
useful (the protocol PDF is the single most important reference).

## Module product documentation

| File | Origin URL | Contents | Why it matters |
|------|------------|----------|----------------|
| `docs/01-Module13.2-QRCode-product-page.txt` | https://docs.m5stack.com/en/products/sku/M145 | Product page: specs, M5-Bus pinmap, IO expander, Grove ports, DC power, code support table | Confirms this is the CoreS3 stacking module; DC 9-24V center-positive input (user's 12V); IO expander PI4IOE5V6408 @ I2C 0x43; QR_5V_EN + TRIG channels; CoreS3 bus mapping (G18=PC_RX←QR_TX, G17=PC_TX→QR_RX, G12/G11=In_I2C) |
| `docs/Module13.2_QRCode.pdf` | https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/static/pdf/static/en/module/Module13.2_QRCode.pdf | Official product PDF (same content as product page) | Offline/committed canonical copy of the product page |

## Protocol & engine reference (most important)

| File | Origin URL | Contents | Why it matters |
|------|------------|----------|----------------|
| `protocol-pdf/Module13.2-QRCode-Protocol-EN.pdf` | https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/1163/Module13.2-QRCode-Protocol-EN.pdf | Binary protocol PDF | Canonical protocol spec (committed) |
| `protocol-pdf/Module13.2-QRCode-Protocol-EN.txt` | (pdftotext of above) | Full UART protocol: 0x21/0x22 config, 0x32/0x33 control, 0x43/0x44 status, 0x60/0x61 image; full PID/FID parameter tables | **Primary reference for the ESP-IDF driver.** Defines every command byte, the 4-byte framing (TYPE/PID/FID/PARAM), FID length-encoding bits, trigger modes, light modes, USB modes, code-enable tables |
| `protocol-pdf/ZBarcode-Scanner-User-Guide-2.5-EN.txt` | https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/1163/ZBarcode_Scanner_User_Guide-2.5-EN.pdf | ZBarcode engine user guide: serial settings (8N1 fixed), baud rates, trigger/continuous/auto/pulse/sense modes, output format, suffix | Explains scan-output behavior and trigger semantics in depth (the M14-Pro engine is a ZBarcode-class scanner) |
| `engine-datasheet/M14-Pro-installation-guide-EN.txt` | https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/1163/M14-Pro_EN_2025_07_21_15_21_38.pdf | M14-Pro CMOS engine installation guide | Engine mechanical/electrical reference (the scanner engine the module wraps) |

## Official Arduino library (porting reference)

Mirrored from GitHub `m5stack/M5Module-QRCode` (branch `main`). This is the
authoritative reference implementation of the UART protocol and the IO-expander
control. The ESP-IDF firmware ports this logic.

| File | Origin URL | Contents |
|------|------------|----------|
| `arduino-lib/src/M5ModuleQRCode.h` | https://raw.githubusercontent.com/m5stack/M5Module-QRCode/main/src/M5ModuleQRCode.h | `M5ModuleQRCode` wrapper: Config_t (pin_tx/pin_rx/baudrate/serial/i2c/addr), begin(), setEnable(), setTriggerLevel(), update(), available(), getScanResult(), onScanResult() |
| `arduino-lib/src/M5ModuleQRCode.cpp` | .../M5ModuleQRCode.cpp | PI4IOE5V6408 init (ch0=QR_5V_EN, ch4=TRIG), UART init 115200 8N1, scan-result pump |
| `arduino-lib/src/qrcode_m14.h` | .../qrcode_m14.h | `QRCodeM14` protocol class: enums (TriggerMode, FillLight, PosLight, CmdResult), all API decls |
| `arduino-lib/src/qrcode_m14.cpp` | .../qrcode_m14.cpp | **The protocol bytes.** `sendCmd()` framing, every command's exact bytes + expected ACK, `getInfos()` status query with 5-byte header (data size at [3:4] MSB-first) |
| `arduino-lib/src/debug.h` | .../debug.h | Debug logging macros |
| `arduino-lib/examples/*.ino` | .../examples/* | Reference sketches: ContinuousMode (G17 TX/G16 RX on Basic v2.7), AutoMode, PulseMode, MotionSensingMode, UsbMode |
| `arduino-lib/library.properties` | .../library.properties | Library metadata / Arduino-ESP32 dependency |

## In-repo references (not copied; cited by absolute path)

These existing repo files shape the architecture decisions (display stack,
console, IDF version):

- `/home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0114-papers3-pulp-os/` — most recent ESP-IDF + M5Unified + M5GFX firmware; pins IDF 5.3.4, target `esp32s3`, managed_components `m5stack__m5unified` + `m5stack__m5gfx`.
- `/home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0095-cores3-wifi-bench/` — only existing CoreS3 ESP-IDF firmware (headless); `sdkconfig.defaults` shows USB Serial/JTAG console + PSRAM + 240 MHz baseline.
- `/home/manuel/code/wesen/go-go-golems/esp32-s3-m5/AGENTS.md` — build rules (S3 → IDF 5.3.4; USB Serial/JTAG console; component deps in `main/idf_component.yml`).

## User-provided hardware facts (recorded verbatim from the session)

- "The device is connected over USB." → CoreS3 console/flash + host link via USB Serial/JTAG.
- "I pointed the device at a barcode" → the module is mounted and physically aimed at codes.
- "I plugged in 12 V into the qr code reader" → the Module13.2 QRCode DC jack is powered at 12 V (within the 9-24 V range; center-positive 5.5×2.1 mm). This powers the whole stack via the internal DC-DC step-down to 5 V, including the CoreS3.
