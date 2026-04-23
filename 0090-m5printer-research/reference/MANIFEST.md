# M5Printer / ATOM Printer Research - Source Manifest

**Created**: 2026-04-22
**Research Focus**: M5Stack ATOM Printer (K118) - thermal printer kit with ESP32-Pico controller

---

## Collected Sources

### Official Documentation

| # | File | Source URL | Description |
|---|------|------------|-------------|
| 01 | 01-docs-m5stack-atom-printer.md | https://docs.m5stack.com/en/atom/atom_printer | Official M5Stack docs - specs, pinmap, commands |
| 06 | 06-docs-sku-k118.md | https://docs.m5stack.com/en/products/sku/K118 | SKU K118 product page |
| 22 | 22-uiflow-blockly.md | https://docs.m5stack.com/en/uiflow/blockly/media_trans/atom_printer | UIFlow Blockly blocks for printer |
| 17 | 17-hat-thermal-arduino.md | https://docs.m5stack.com/en/arduino/projects/hat/hat_thermal | Hat Thermal Arduino tutorial |

### GitHub Repositories

| # | File | Source URL | Description |
|---|------|------------|-------------|
| 02 | 02-github-atom-printer-repo.md | https://github.com/m5stack/ATOM-PRINTER | Main firmware repo |
| 02 | 02-github-atom-printer-readme.md | https://github.com/m5stack/ATOM-PRINTER/blob/master/README.md | README with usage instructions |
| 12 | 12-firmware-web-cpp.md | https://github.com/m5stack/ATOM-PRINTER/blob/master/examples/PRINTER_FW/ATOM_PRINTER_WEB.cpp | Web interface firmware source |
| 15 | 15-printer-fw-ino.md | https://github.com/m5stack/ATOM-PRINTER/blob/master/examples/PRINTER_FW/PRINTER_FW.ino | Main firmware Arduino sketch |
| 16 | 16-uart-cmd-control.md | https://github.com/m5stack/M5Atom/blob/master/examples/ATOM_BASE/ATOM_QRCode/UART_CMD_CONTROL/UART_CMD_CONTROL.ino | UART command control example |
| 08 | 08-m5atom-library.md | https://github.com/m5stack/M5Atom | M5Stack Atom Arduino library |

### Third-Party Libraries

| # | File | Source URL | Description |
|---|------|------------|-------------|
| 19 | 19-thermal-printer-library.md | https://github.com/wearemothership/Thermal-Printer-Library | Arduino thermal printer library |
| 20 | 20-bitbank-thermal-printer.md | https://github.com/bitbank2/Thermal_Printer | BitBank thermal printer library (BLE) |
| 13 | 13-python-escpos.md | https://github.com/python-escpos/python-escpos | Python ESC/POS library |
| 14 | 14-python-escpos-docs.md | https://python-escpos.readthedocs.io/ | python-escpos documentation |

### Community & Projects

| # | File | Source URL | Description |
|---|------|------------|-------------|
| 03 | 03-reddit-image-printing.md | https://www.reddit.com/r/M5Stack/comments/1ken25i/print_images_with_atom_thermal_printer_kit/ | Reddit: image printing with Atkinson dithering |
| 04 | 04-dithering-repo.md | https://github.com/MaxBittker/dithering | Thermal printer image dithering algorithms |
| 09 | 09-community-mqtt.md | https://community.m5stack.com/topic/3816/atom-thermal-printer-mqtt | M5Stack community: MQTT setup |
| 10 | 10-hackaday-project.md | https://hackaday.io/project/166521-m5stack-thermal-printer | Hackaday: thermal printer prototype |
| 11 | 11-hackster-diy-camera.md | https://hackster.io/m5stack/diy-camera-with-thermal-printer-59399c | Hackster: DIY camera with thermal printer |
| 21 | 21-cnx-software-review.md | https://cnx-software.com/2021/11/29/wireless-thermal-printer-kit-features-m5stack-atom-lite-controller/ | CNX Software review |

### Technical References

| # | File | Source URL | Description |
|---|------|------------|-------------|
| 05 | 05-escpos-manual.md | https://pos-x.com/download/escpos-programming-manual/ | ESC/POS programming manual |
| 25 | 25-escpos-imaging.md | https://escpos.readthedocs.io/en/latest/imaging.html | ESC/POS imaging commands |
| 24 | 24-adafruit-image-guide.md | https://learn.adafruit.com/ble-thermal-cat-printer-with-circuitpython/creating-your-own-images | Adafruit: creating thermal printer images |
| 26 | 26-bitmap-printing.md | https://kapusta.cc/2017/12/16/printing-bitmaps-with-a-thermal-printer/ | Bitmap printing tutorial |
| 18 | 18-atom-lite-pinout.md | https://docs.cirkitdesigner.com/component/a0e181d3-c496-4287-a78b-ef19419d3b2e/m5stack-atom-lite | ATOM Lite pinout reference |
| 23 | 23-unofficial-guide.md | https://m5stack.lang-ship.com/catalog/products/base/k118_atom-printer/ | M5Stack unofficial guide |

---

## Key GitHub Repos to Clone

```bash
# Main ATOM Printer firmware
git clone https://github.com/m5stack/ATOM-PRINTER.git

# M5Stack Atom Arduino library
git clone https://github.com/m5stack/M5Atom.git

# Python ESC/POS library
git clone https://github.com/python-escpos/python-escpos.git

# Thermal printer dithering
git clone https://github.com/MaxBittker/dithering.git

# Alternative Arduino thermal printer library
git clone https://github.com/wearemothership/Thermal-Printer-Library.git

# BitBank thermal printer (BLE support)
git clone https://github.com/bitbank2/Thermal_Printer.git
```

---

## Key Datasheets / PDFs to Download

| Document | URL |
|----------|-----|
| ATOM Printer PDF (official) | https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/static/pdf/static/en/atom/atom_printer.pdf |
| ATOM Printer Command Protocol v1.06 | https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/atombase/atom_pritner/ATOM_PRINTER_CMD_v1.06.pdf |
| Atom Printer Guide PDF | https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/static/pdf/static/en/guide/hobby_kit/atom_printer/usage.pdf |
| ESC/POS Command Manual (POS-X) | https://pos-x.com/download/escpos-programming-manual/ |
| ESC/POS Command Specs (Star Micronics) | https://starmicronics.com/support/Mannualfolder/escpos_cm_en.pdf |
| ESC/POS Commands PDF (MikroElektronika) | https://download.mikroe.com/documents/datasheets/ESC-POS_commands.pdf |

---

## Search History

See `diary/00-diary.md` for detailed search history and commands used.

---

## Summary of Research Findings

The M5Printer is actually the **ATOM Thermal Printer Kit (SKU: K118)**:
- Uses **ATOM Lite** (ESP32-PICO-D4, 4MB Flash) as controller
- 58mm thermal printer, 203dpi, 60mm/s
- **12V DC power required** (2.5A recommended)
- Communication: UART 9600bps 8N1 (G23=TX, G33=RX, G19=CTS)

Three ways to use:
1. **AP Mode**: Connect to `ATOM_PRINTER-xxxx` WiFi, open 192.168.4.1 web page
2. **MQTT**: Publish to topic = MAC address, payloads like `TEXT,10,1:Hello`, `BAR:1234`, `QR:1234`
3. **Serial UART**: Direct serial commands (ESC/POS-like protocol)

For full documentation, see `diary/00-diary.md`.
