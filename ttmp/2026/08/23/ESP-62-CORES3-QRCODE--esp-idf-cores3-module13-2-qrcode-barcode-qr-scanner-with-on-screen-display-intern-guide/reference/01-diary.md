---
Title: Investigation diary
Ticket: ESP-62-CORES3-QRCODE
Status: active
Topics: []
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: abs:///home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/23/ESP-62-CORES3-QRCODE--esp-idf-cores3-module13-2-qrcode-barcode-qr-scanner-with-on-screen-display-intern-guide/design-doc/01-cores3-module13.2-qrcode-scanner-analysis-design-and-implementation-guide.md
      Note: Primary deliverable this diary accompanies
    - Path: abs:///home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/23/ESP-62-CORES3-QRCODE--esp-idf-cores3-module13-2-qrcode-barcode-qr-scanner-with-on-screen-display-intern-guide/sources/arduino-lib/src/qrcode_m14.cpp
      Note: Porting reference for the ESP-IDF driver
    - Path: abs:///home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/23/ESP-62-CORES3-QRCODE--esp-idf-cores3-module13-2-qrcode-barcode-qr-scanner-with-on-screen-display-intern-guide/sources/protocol-pdf/Module13.2-QRCode-Protocol-EN.txt
      Note: Protocol source that shaped the design
    - Path: repo://ttmp/2026/08/23/ESP-62-CORES3-QRCODE--esp-idf-cores3-module13-2-qrcode-barcode-qr-scanner-with-on-screen-display-intern-guide/scripts/01-probe-qrcode-uart.py
      Note: Reusable probe created during investigation
ExternalSources: []
Summary: ""
LastUpdated: 0001-01-01T00:00:00Z
WhatFor: ""
WhenToUse: ""
---


# Diary

## Goal

Capture the investigation, source-gathering, and design work for the ESP-62
CoreS3 + Module13.2 QRCode barcode/QR scanner firmware so an intern can pick
up implementation from the design doc without re-deriving the context.

## Step 1: Orient on the repo and confirm the device

This step established what firmwares already exist, what the repo's display
and console conventions are, and disambiguated which physical hardware the
user has. The key outcome is that the "QR-Code expansion" is the
**Module13.2 QRCode (SKU M145)**, a CoreS3 stacking module, and the user's
12 V supply is the correct and required external power.

### Prompt Context

**User prompt (verbatim):** "Create a new docmgr ticket to create a CoreS3 + QR-Code expansion firmware to read barcodes and qr codes and display them on screen. The device is connected over USB. Look at other firmwares here. Look at the documentation for the emodule and save all sources you find useful in the sources/ directory of the ticket, and store all scripts you write in the scripts/ directory (for reuse later, and for tracking what you are doing)."

**User prompt (verbatim):** "ipointed the device at a barcode, I also don't knowif it needs external power actually."

**User prompt (verbatim):** "actually i plugged in 12 V into the qr code reader"

**Assistant interpretation:** Build a docmgr ticket + intern-ready design/implementation guide for an ESP-IDF firmware that pairs a CoreS3 with a M5Stack QR-code expansion module, reads barcodes/QR codes, and shows them on the CoreS3 screen; the CoreS3 connects to the host over USB; gather and save all useful module documentation in sources/; save scripts in scripts/; diary + commit; upload to reMarkable.

**Inferred user intent:** A reusable, evidence-backed onboarding package so an intern can implement the firmware end-to-end, with the hardware unknowns (which module, whether external power is needed) resolved up front.

**Commit (code):** (none yet — research/design only this step)

### What I did
- Read `AGENTS.md` (build rules), `docs/project-index.yaml`, the `0095-cores3-wifi-bench` CoreS3 firmware, and `0040/0041` camera firmwares.
- Searched the repo for any existing QR/barcode/quirc work (`rg -il "quirc|barcode|qr.?code|qrcode|zxing"`): none found.
- Confirmed the repo's ESP-IDF display stack is **M5Unified + M5GFX** (via `0114-papers3-pulp-os/managed_components/m5stack__m5unified` + `m5stack__m5gfx`, IDF 5.3.4, target esp32s3).
- Web-searched (Kagi) the M5Stack QR modules: two candidates — Unit QRCode (Grove) and Module13.2 QRCode (stacking). CoreS3 + "expansion" + 12 V → **Module13.2 QRCode (M145)**.
- Fetched the product page (Playwright, JS-rendered SPA — defuddle returned empty) and downloaded the official product PDF via curl.

### Why
- The user's hardware facts ("connected over USB", "pointed at a barcode", "plugged in 12 V") had to be mapped to a specific SKU before any protocol/design work could be credible. The Module13.2 QRCode is the only M5Stack QR product that (a) stacks on the CoreS3 and (b) has a 9–24 V DC jack, matching 12 V exactly.

### What worked
- The product PDF text extraction confirmed: DC 9–24 V center-positive (12 V is in range), IO expander PI4IOE5V6408 @ I2C 0x43, QR_5V_EN + TRIG channels, M5-Bus pinmap, CoreS3 bus mapping (G18=PC_RX←QR_TX, G17=PC_TX→QR_RX, G12/G11=In_I2C), supported symbologies.
- Found the official Arduino library `m5stack/M5Module-QRCode` and mirrored its source + examples into `sources/arduino-lib/`.

### What didn't work
- `defuddle parse <m5stack docs url> --md` returned 0 bytes for all three docs pages (they are JS SPAs). Worked around by using the Playwright browser snapshot for the JS pages and `curl`+`pdftotext` for the linked PDFs.

### What I learned
- M5Stack docs pages are client-rendered; defuddle (no JS) can't read them — use Playwright snapshots or the linked OSS PDFs.
- The repo's "Arduino libs on ESP-IDF" pattern (M5Unified/M5GFX as managed components in `main/idf_component.yml`) means the official `M5Module-QRCode` library is almost directly reusable; only the UART layer needs ESP-IDF glue.
- CoreS3 uses **quad** PSRAM (`0095`); PaperS3/0114 uses **octal** — must not copy 0114's PSRAM block to a CoreS3 project.

### What was tricky to build
- Disambiguating Unit QRCode vs Module13.2 QRCode: both exist, both have I2C/UART switches. The deciding evidence was (a) "CoreS3 + expansion" implies a stacking module, and (b) the 12 V DC jack only exists on Module13.2 QRCode (the Unit QRCode is Grove-powered at 5 V). I cross-checked the Kagi result "Module13.2 PPS requires an external DC 9–36V power supply, otherwise it will show no I2C device" — same family, same power model, reinforcing that 12 V is required for the engine to enumerate/work.
- Mapping M5-Bus pins to CoreS3 GPIOs: the bus table lists both module-side and CoreS3-side pins; the DIP switch `(SW)` rows must be set to route the engine UART to Port-C UART (pins 15/16 → G18/G17), not the NC positions.

### What warrants a second pair of eyes
- The CoreS3 UART pin choice (UART1, RX=G18, TX=G17) and the DIP-switch setting it implies — verify against the actual DIP-switch diagram on the module before flashing. A wrong DIP setting makes the UART appear dead and is easy to misread as a firmware bug.
- The PSRAM mode (quad, not octal) — a wrong config boots fail.

### What should be done in the future
- Confirm on real hardware whether the 12 V back-feed fully powers the CoreS3 for USB flashing without a separate USB supply (open question Q1 in the design doc).
- Validate the scan-result terminator (`\r\n` default) against the actual module config; tune the quiet-time parser if needed.

### Code review instructions
- Start at the design doc `design-doc/01-...-guide.md` §3 (hardware) and §4 (protocol).
- Validate hardware assumptions against `sources/docs/01-Module13.2-QRCode-product-page.txt` (M5-Bus table) and the user's 12 V statement.
- Run `scripts/01-probe-qrcode-uart.py` against the module to confirm the protocol before any firmware is written.

### Technical details
- Command bytes are in `sources/arduino-lib/src/qrcode_m14.cpp` (e.g. start decode = `32 75 01`; set trigger mode continuous = `21 61 41 02`).
- Status reply framing: `44 <pid> <fid> <len_hi> <len_lo> <data...>`; length is big-endian at offset [3:4] (see `getResponseDataSize()`).

## Step 2: Author the design/implementation guide, sources manifest, and helper scripts

This step converted the gathered evidence into the deliverable: a sources
manifest, two reusable scripts, and the intern-ready design/implementation
guide, then wired them into the ticket bookkeeping.

### Prompt Context
**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Produce the actual ticket deliverables (design doc + diary + scripts + sources manifest) and update docmgr bookkeeping so the ticket is self-contained and upload-ready.

**Inferred user intent:** A complete, navigable, evidence-backed package the intern can act on, plus the diary + commits the user asked for.

**Commit (code):** (this step's commit follows below)

### What I did
- Wrote `sources/MANIFEST.md` (provenance for every source: URL, contents, why it matters).
- Wrote `scripts/01-probe-qrcode-uart.py` — host-side pyserial probe implementing the key protocol commands (status read, start/stop, trigger mode) + a `--scan` stream mode, mirroring `qrcode_m14.cpp`.
- Wrote `scripts/02-bringup-build-flash.sh` — documented build/flash helper respecting AGENTS.md (IDF 5.3.4, USB Serial/JTAG, by-id port discovery).
- Wrote `design-doc/01-...-guide.md` — 12 sections: executive summary, problem/scope, hardware (with block diagram), protocol (with tables + pseudocode), current-state analysis, architecture (with task diagram + pin table), 4 decision records, phased implementation plan, config/API references, test strategy, risks/alternatives, file references.

### Why
- The user asked for a detailed analysis/design/implementation guide for a new intern with prose, bullets, pseudocode, diagrams, API references, and file references. Anchoring every claim to a saved source (or an absolute in-repo path) keeps it evidence-based per the ticket-research skill.

### What worked
- Reusing the official Arduino `qrcode_m14.cpp` as the porting reference let the protocol pseudocode in §4 be concrete (exact bytes) rather than speculative.
- The block diagram + M5-Bus pin table make the hardware story unambiguous for an intern.

### What didn't work
- Nothing blocking this step.

### What I learned
- The `getResponseDataSize` 5-byte header parsing in the Arduino lib applies to **status/config/control replies**, while **scan results are streamed as raw bytes** — the design doc calls this out explicitly (§4.5) so the intern doesn't over-engineer a framed parser for scan output.

### What was tricky to build
- Keeping the design doc intern-accurate about pins: the Arduino example uses Basic v2.7 pins (G17 TX / G16 RX), but CoreS3 Port-C UART is G17 TX / G18 RX. The doc states the CoreS3-specific pins and warns not to copy the example pins verbatim.

### What warrants a second pair of eyes
- The §9.1 `sdkconfig.defaults` (quad PSRAM + custom partitions) — confirm against a real CoreS3 build before the intern commits it.
- The Phase plan's firmware directory name `0118-cores3-qrcode-scanner` is a placeholder (next free number after 0117); confirm the actual next slot when implementation starts.

### What should be done in the future
- Promote `qr_engine` + `qr_module` to `components/qrcode_m14/` if a second device needs the driver (mirror ESP-61's NFC-component extraction).

### Code review instructions
- Read `design-doc/01-...-guide.md` end to end; cross-check every "file evidence" pointer against `sources/`.
- Run `docmgr doctor --ticket ESP-62-CORES3-QRCODE --stale-after 30` before upload.

### Technical details
- Probe script protocol constants are taken verbatim from `qrcode_m14.cpp` (e.g. `CMD_GET_FW = 43 02 C1`, `ACK_TRIG_CONT = 22 61 41 02 00`).
