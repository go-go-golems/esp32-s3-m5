# M5Printer Research Diary

Created: 2026-04-22
Goal: Research the M5Stack M5Printer (thermal printer) - how to print, how to program it, example code, datasheets, what others have done.

---

## Session Start

**Date**: 2026-04-22 16:00 UTC
**Workspace**: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0090-m5printer-research/`

## Research Approach

Following the hardware research playbook from:
- `ARTICLE - Playbook - Researching a Hardware Device from Firmware, Schematics, and Tickets.md`

Steps:
1. Collect sources (web searches via `surf kagi search`, defuddle)
2. Build mental model
3. Document findings
4. Identify gaps for further research

---

## Tools Used

- `surf kagi search --query "..." --max-results N` - Web search via Kagi
- `defuddle parse <url> --md -o <file>` - Extract clean markdown from pages
- Reference files saved to `reference/` folder

---

## Search History

### Search 001: M5Stack Printer thermal printer

**Command**: 
```bash
surf kagi search --query "M5Stack Printer thermal printer" --max-results 15
```

**Key Finding**: M5Printer is actually called "ATOM Thermal Printer Kit" (K118). Uses ATOM Lite (ESP32-Pico) controller + 58mm thermal printer.

**Result URLs**:
- https://docs.m5stack.com/en/atom/atom_printer - Official docs
- https://github.com/m5stack/ATOM-PRINTER - Firmware and examples
- https://www.reddit.com/r/M5Stack/comments/1ken25i/print_images_with_atom_thermal_printer_kit/ - Image printing with Atkinson Dithering
- https://hackaday.io/project/166521-m5stack-thermal-printer - DIY thermal printer prototype

**Saved to**: reference/01-docs-m5stack-atom-printer.md, reference/02-github-atom-printer-repo.md

---

### Search 002: ATOM-PRINTER M5Stack github source code firmware ESP32

**Command**:
```bash
surf kagi search --query "ATOM-PRINTER M5Stack github source code firmware ESP32" --max-results 10
```

**Key Finding**: Main firmware repo is https://github.com/m5stack/ATOM-PRINTER with Arduino examples and web firmware.

---

### Search 003: ATOM Printer thermal printer ESC POS commands programming

**Command**:
```bash
surf kagi search --query "ATOM Printer thermal printer ESC POS commands programming" --max-results 12
```

**Key Finding**: General ESC/POS programming resources found - POS-X manual, Epson reference, python-escpos library.

---

### Search 004: M5Stack ATOM Printer image printing Atkinson dithering

**Command**:
```bash
surf kagi search --query "M5Stack ATOM Printer image printing Atkinson dithering" --max-results 12
```

**Key Finding**: Reddit post about image printing with Atkinson dithering - https://github.com/MaxBittker/dithering
Saved: reference/03-reddit-image-printing.md, reference/04-dithering-repo.md

---

### Search 005: M5Stack ATOM Printer Arduino example code library

**Command**:
```bash
surf kagi search --query "M5Stack ATOM Printer Arduino example code library" --max-results 12
```

**Key Finding**: Main Arduino library at https://github.com/m5stack/M5Atom

---

### Search 006: ATOM Printer K118 datasheet pinout specifications

**Command**:
```bash
surf kagi search --query "ATOM Printer K118 datasheet pinout specifications" --max-results 10
```

**Key Finding**: Official PDF datasheet available at:
- https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/static/pdf/static/en/atom/atom_printer.pdf
- Command protocol PDF: https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/atombase/atom_pritner/ATOM_PRINTER_CMD_v1.06.pdf

---

### Search 007: M5Stack ATOM Printer project DIY thermal receipt

**Command**:
```bash
surf kagi search --query "M5Stack ATOM Printer project DIY thermal receipt" --max-results 12
```

**Key Findings**:
- Hackaday project for thermal printer prototype
- DIY Camera with Thermal Printer on Hackster
- M5Stack community MQTT discussion

---

### Search 008: ATOM Printer UART serial protocol command format

**Command**:
```bash
surf kagi search --query "ATOM Printer UART serial protocol command format" --max-results 10
```

**Key Finding**: Serial settings confirmed: UART 9600bps 8N1
Pin mapping: G23=TX, G33=RX, G19=CTS

---

### Search 009: M5Stack ATOM Printer WiFi web server API HTTP POST printing

**Command**:
```bash
surf kagi search --query "M5Stack ATOM Printer WiFi web server API HTTP POST printing" --max-results 10
```

**Key Finding**: AP mode WiFi SSID: ATOM_PRINTER-xxxx
Web interface at 192.168.4.1

---

### Search 010: thermal printer 58mm ESC POS BMP image printing python

**Command**:
```bash
surf kagi search --query "thermal printer 58mm ESC POS BMP image printing python" --max-results 10
```

**Key Finding**: python-escpos library supports thermal printers
Saved: reference/13-python-escpos.md, reference/14-python-escpos-docs.md

---

### Search 011: thermal printer ATOM PRINTER firmware source code github clone

**Command**:
```bash
surf kagi search --query "thermal printer ATOM PRINTER firmware source code github clone" --max-results 8
```

**Key Finding**: Multiple Arduino thermal printer libraries available:
- https://github.com/wearemothership/Thermal-Printer-Library
- https://github.com/bitbank2/Thermal_Printer
- https://github.com/sparkfun/Thermal_Printer

---

### Search 012: M5Stack thermal printer power adapter 12V 2.5A requirements

**Command**:
```bash
surf kagi search --query "M5Stack thermal printer power adapter 12V 2.5A requirements" --max-results 8
```

**Key Finding**: Power requirements confirmed:
- 12V DC (5.5mm specification)
- 2.5A recommended for optimal print quality
- **Does NOT come with power adapter**

---

### Search 013: M5Stack thermal printer 58mm paper roll replacement installation

**Command**:
```bash
surf kagi search --query "M5Stack thermal printer 58mm paper roll replacement installation" --max-results 8
```

**Key Finding**: Standard 58mm thermal paper rolls
Thermal paper specs: 58mm±0.05mm width, 0.05~0.1mm thickness, max diameter ≤40mm

---

### Search 014: ATOM Printer UIFlow MicroPython Blockly programming

**Command**:
```bash
surf kagi search --query "ATOM Printer UIFlow MicroPython Blockly programming" --max-results 8
```

**Key Finding**: UIFlow Blockly blocks available for Atom Printer
Saved: reference/22-uiflow-blockly.md

---

### Search 015: thermal printer BMP raster image command format 384 dots width

**Command**:
```bash
surf kagi search --query "thermal printer BMP raster image command format 384 dots width" --max-results 8
```

**Key Finding**: 58mm printer width = 384 pixels
Saved: reference/24-adafruit-image-guide.md, reference/25-escpos-imaging.md

---

### Search 016: M5Stack Atom thermal printer quick start guide tutorial

**Command**:
```bash
surf kagi search --query "M5Stack Atom thermal printer quick start guide tutorial" --max-results 8
```

**Key Finding**: Quick start guide PDF: https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/static/pdf/static/en/guide/hobby_kit/atom_printer/usage.pdf

---

## Commands Used to Save Reference Files

```bash
# Official docs
defuddle parse https://docs.m5stack.com/en/atom/atom_printer --md -o reference/01-docs-m5stack-atom-printer.md
defuddle parse https://docs.m5stack.com/en/products/sku/K118 --md -o reference/06-docs-sku-k118.md

# GitHub repos
defuddle parse https://github.com/m5stack/ATOM-PRINTER --md -o reference/02-github-atom-printer-repo.md
defuddle parse https://github.com/m5stack/M5Atom --md -o reference/08-m5atom-library.md
defuddle parse https://github.com/python-escpos/python-escpos --md -o reference/13-python-escpos.md

# Firmware source
defuddle parse https://github.com/m5stack/ATOM-PRINTER/blob/master/examples/PRINTER_FW/ATOM_PRINTER_WEB.cpp --md -o reference/12-firmware-web-cpp.md
defuddle parse https://github.com/m5stack/ATOM-PRINTER/blob/master/examples/PRINTER_FW/PRINTER_FW.ino --md -o reference/15-printer-fw-ino.md

# Community & projects
defuddle parse https://www.reddit.com/r/M5Stack/comments/1ken25i/print_images_with_atom_thermal_printer_kit/ --md -o reference/03-reddit-image-printing.md
defuddle parse https://github.com/MaxBittker/dithering --md -o reference/04-dithering-repo.md
defuddle parse https://community.m5stack.com/topic/3816/atom-thermal-printer-mqtt --md -o reference/09-community-mqtt.md
defuddle parse https://hackaday.io/project/166521-m5stack-thermal-printer --md -o reference/10-hackaday-project.md
defuddle parse https://hackster.io/m5stack/diy-camera-with-thermal-printer-59399c --md -o reference/11-hackster-diy-camera.md

# Technical references
defuddle parse https://pos-x.com/download/escpos-programming-manual/ --md -o reference/05-escpos-manual.md
defuddle parse https://python-escpos.readthedocs.io/ --md -o reference/14-python-escpos-docs.md
defuddle parse https://docs.m5stack.com/en/arduino/projects/hat/hat_thermal --md -o reference/17-hat-thermal-arduino.md
defuddle parse https://docs.cirkitdesigner.com/component/a0e181d3-c496-4287-a78b-ef19419d3b2e/m5stack-atom-lite --md -o reference/18-atom-lite-pinout.md
defuddle parse https://github.com/wearemothership/Thermal-Printer-Library --md -o reference/19-thermal-printer-library.md
defuddle parse https://github.com/bitbank2/Thermal_Printer --md -o reference/20-bitbank-thermal-printer.md
defuddle parse https://cnx-software.com/2021/11/29/wireless-thermal-printer-kit-features-m5stack-atom-lite-controller/ --md -o reference/21-cnx-software-review.md
defuddle parse https://docs.m5stack.com/en/uiflow/blockly/media_trans/atom_printer --md -o reference/22-uiflow-blockly.md
defuddle parse https://m5stack.lang-ship.com/catalog/products/base/k118_atom-printer/ --md -o reference/23-unofficial-guide.md
defuddle parse https://learn.adafruit.com/ble-thermal-cat-printer-with-circuitpython/creating-your-own-images --md -o reference/24-adafruit-image-guide.md
defuddle parse https://escpos.readthedocs.io/en/latest/imaging.html --md -o reference/25-escpos-imaging.md
```

---

## Key Findings Summary

### Device Identification

The "M5Printer" is actually the **M5Stack ATOM Thermal Printer Kit**:
- **SKU**: K118
- **Controller**: ATOM Lite (ESP32-PICO-D4, 4MB Flash)
- **Printer**: 58mm thermal printer
- **Price**: ~$59

### Hardware Specifications

| Spec | Value |
|------|-------|
| Printing method | Thermal printing |
| Print width | 58mm |
| Resolution | 203dpi (8 dots/mm) |
| Print speed | 60mm/s |
| Max dots/line | 384 |
| Paper roll diameter | ≤40mm |
| Power | DC 12V, 2.5A |
| Lifespan | 50km printing distance |
| Interface | UART TTL 9600bps 8N1 |
| GPIO pins | G23=TX, G33=RX, G19=CTS |

### Communication Methods

1. **AP Mode (Default)**:
   - Connect to WiFi: `ATOM_PRINTER-xxxx`
   - Open browser: http://192.168.4.1
   - Type text, click print

2. **MQTT Mode**:
   - Subscribe/Publish to topic = MAC address (e.g., `xx:xx:xx:xx:xx:xx`)
   - M5Stack MQTT server
   - Payload formats:
     - `TEXT,10,1:Hello` - Text at position 10, size 1
     - `BAR:1234` - Barcode
     - `QR:1234` - QR code

3. **Serial UART**:
   - Direct serial commands
   - 9600 baud, 8N1
   - Custom protocol

### Serial Protocol Commands

From ATOM_PRINTER_CMD_v1.06.pdf:

**Initialization**: `0x1B, 0x40`

**Text Formatting**:
- Set bold: `0x1B, 0x47, 0x01/0x00`
- Set underline: `0x1B, 0x2D, 0x01/0x00`
- Set character size: `0x1D, 0x21, ((X&0x0f)<<4) | (Y&0x0f)`

**Print BMP**:
```
0x1D, 0x76, 0x30, 
(W/8)&0x00ff, ((W/8)>>8)&0x00ff,  // width in bytes
H&0x00ff, (H>>8)&0x00ff,           // height
[data...]
```

### What Others Have Built

1. **Image Printing with Atkinson Dithering** (Reddit)
   - Fork of ATOM-PRINTER with HTML page
   - Converts any image to 1-bit grayscale
   - Atkinson dithering algorithm

2. **DIY Camera with Thermal Printer** (Hackster)
   - M5Camera + ATOM Printer
   - Take photo, print immediately

3. **MQTT Temperature Logger** (Community)
   - Print sensor data via MQTT
   - NodeRED integration

4. **Hackathon Thermal Printer** (Hackaday)
   - Workshop project
   - Receipt printing examples

### Programming Options

1. **Arduino**: M5Atom library + serial commands
2. **UIFlow/Blockly**: Visual programming
3. **MicroPython**: Via UIFlow2
4. **MQTT**: From any MQTT client
5. **Web**: Built-in web interface

### Libraries to Use

- `M5Atom` - Arduino library for ATOM Lite
- `python-escpos` - Python ESC/POS commands
- `Thermal-Printer-Library` - We Are Mothership Arduino lib
- `Thermal_Printer` - BitBank library (BLE support)

### Next Steps

1. Clone firmware repo: `git clone https://github.com/m5stack/ATOM-PRINTER.git`
2. Download ATOM_PRINTER_CMD_v1.06.pdf for protocol reference
3. Try AP mode first to verify hardware works
4. Build custom Arduino firmware using M5Atom library
5. Implement image printing with dithering

---

## Reference Files Created

```
reference/
├── MANIFEST.md              # Master list of all sources
├── 01-docs-m5stack-atom-printer.md
├── 02-github-atom-printer-repo.md
├── 02-github-atom-printer-readme.md
├── 03-reddit-image-printing.md
├── 04-dithering-repo.md
├── 05-escpos-manual.md
├── 06-docs-sku-k118.md
├── 08-m5atom-library.md
├── 09-community-mqtt.md
├── 10-hackaday-project.md
├── 11-hackster-diy-camera.md
├── 12-firmware-web-cpp.md
├── 13-python-escpos.md
├── 14-python-escpos-docs.md
├── 15-printer-fw-ino.md
├── 16-uart-cmd-control.md
├── 17-hat-thermal-arduino.md
├── 18-atom-lite-pinout.md
├── 19-thermal-printer-library.md
├── 20-bitbank-thermal-printer.md
├── 21-cnx-software-review.md
├── 22-uiflow-blockly.md
├── 23-unofficial-guide.md
├── 24-adafruit-image-guide.md
├── 25-escpos-imaging.md
└── 26-bitmap-printing.md
```

## Commands Used to Save Reference Files

(Previous commands in diary)...

---

## Developer Guide Created

**File**: `docs/DEVELOPER-GUIDE.md`

Created a comprehensive developer and user guide covering:
- Initial setup and WiFi configuration
- MQTT printing (Python, JavaScript examples)
- HTTP/Web API (all endpoints documented)
- Serial UART programming (Arduino examples)
- Image printing with dithering (Python code)
- Troubleshooting and LED status meanings

### HTTP API Endpoints Documented
- `GET /print` - Print text/QR/barcode
- `POST /wifi_config` - Configure WiFi
- `POST /mqtt_config` - Configure MQTT
- `GET /device_status` - Get device status
- `POST /bmp_size` - Set BMP dimensions
- `POST /bmp` - Upload BMP image

### MQTT Payload Formats
- Text: `TEXT,<position>,<size>:<content>`
- Barcode: `BAR:<content>`
- QR Code: `QR:<content>`

---

## Session End

**Time**: 2026-04-22 17:30 UTC
**Files saved**: 26 reference files + manifest + developer guide
**Searches performed**: 16 searches
**Key output**: `docs/DEVELOPER-GUIDE.md`
