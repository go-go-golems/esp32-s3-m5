---
title: "M5Printer / ATOM Thermal Printer Kit (K118)"
tags:
  - project
  - m5stack
  - thermal-printer
  - esp32
  - atom
  - embedded
  - hardware
created: 2026-04-22
repo: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0090-m5printer-research
status: active
type: project
---

# M5Printer / ATOM Thermal Printer Kit (K118)

## Overview

The "M5Printer" is actually the **M5Stack ATOM Thermal Printer Kit (SKU: K118)** - a DIY thermal printer kit featuring the ATOM Lite (ESP32-PICO-D4) IoT controller paired with a 58mm thermal printer.

## Quick Specs

| Spec | Value |
|------|-------|
| Controller | ATOM Lite (ESP32-PICO-D4, 4MB Flash) |
| Print width | 58mm |
| Resolution | 203dpi (8 dots/mm) |
| Speed | 60mm/s |
| Max dots/line | 384 |
| Power | DC 12V, 2.5A (not included) |
| Interface | UART TTL 9600bps 8N1 |
| GPIO | G23=TX, G33=RX, G19=CTS |
| Price | ~$59 |

## Communication Methods

1. **AP Mode**: Connect to `ATOM_PRINTER-xxxx`, open http://192.168.4.1
2. **MQTT**: Publish to topic = MAC address
3. **Serial UART**: Direct ESC/POS-like commands

## Project Structure

```
0090-m5printer-research/
├── index.md                    # This file
├── diary/
│   └── 00-diary.md            # Detailed research diary
├── reference/
│   ├── MANIFEST.md             # Source manifest
│   ├── 01-docs-m5stack-atom-printer.md
│   ├── 02-github-atom-printer-repo.md
│   ├── 03-reddit-image-printing.md
│   ├── 04-dithering-repo.md
│   ├── 05-escpos-manual.md
│   └── ... (25 reference files total)
└── source/                     # Cloned repositories
    ├── ATOM-PRINTER/           # Main firmware repo
    ├── M5Atom/                 # Arduino library
    ├── python-escpos/         # Python library
    ├── Thermal-Printer-Library/
    └── dithering/             # Image dithering
```

## Key Resources

- **Official Docs**: https://docs.m5stack.com/en/atom/atom_printer
- **GitHub Repo**: https://github.com/m5stack/ATOM-PRINTER
- **Protocol Specs**: `ATOM_PRINTER_CMD_v1.06.pdf`

## Next Steps

- [ ] Verify hardware works with AP mode
- [ ] Download ATOM_PRINTER_CMD_v1.06.pdf
- [ ] Build custom Arduino firmware
- [ ] Implement image printing with dithering
- [ ] Set up MQTT integration

## Related Notes

- [[ARTICLE - Playbook - Researching a Hardware Device from Firmware, Schematics, and Tickets]]
