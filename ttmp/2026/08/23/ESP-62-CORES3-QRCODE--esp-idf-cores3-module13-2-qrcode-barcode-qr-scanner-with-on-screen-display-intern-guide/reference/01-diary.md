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
    - Path: repo://0118-cores3-qrcode-scanner/main/qr_engine.cpp
      Note: Protocol port — diary Steps 3-6
    - Path: repo://0118-cores3-qrcode-scanner/main/qr_ui.cpp
      Note: UI task — diary Step 5
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

**Commit (code):** b49b37f4 — "ESP-62: CoreS3 + Module13.2 QRCode scanner — ticket, sources, design guide, diary"

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

### Delivery
- reMarkable bundle uploaded to `/ai/2026/08/23/ESP-62-CORES3-QRCODE` (design doc + diary + sources manifest as one PDF, ToC depth 2).
- Verified present via `remarquee cloud search QRCode`: folder `/ai/2026/08/23/ESP-62-CORES3-QRCODE` + file `ESP-62 CoreS3 Module13.2 QRCode Scanner — Design & Implementation Guide`.
- Note: `remarquee cloud ls`/`find` print a glazed-help "No examples found!" quirk for some paths; `cloud search <term>` is the reliable verification.

## Step 3: Phase 0 probe reality + Phase 1 skeleton + display boot

This step reconciled the "run the probe" instruction with the physical setup
(module stacked on the CoreS3, DIP on UART), then created and built the
firmware skeleton. The host pyserial probe cannot reach the module directly
because the module UART is the CoreS3's UART1 (internal); the real probe is
on-device firmware. Building is host-side and needs no hardware.

### Prompt Context
**User prompt (verbatim):** "module dip is on uart, run script, then build. commit at appropriate intervals and keep a detailed diary as you work (using the diary format from the skill)"
**User prompt (verbatim):** "also, before starting, print a brutalist work slip with the overall plan and phases, then before each phase, print a slip about the phase itself, and when done, write a summary slip. do that for each phase."
**Assistant interpretation:** The module DIP is set to UART; run the probe, then build the firmware; commit at intervals; keep a diary; print brutalist work slips (overall plan + per-phase pre/summary) for each phase.
**Inferred user intent:** Actually implement the ESP-62 firmware with progress tracking via thermal slips and a diary, committing as phases complete.
**Commit (code):** 9981d58f — "ESP-62 P1: CoreS3 QRCode scanner skeleton + display boot"

### What I did
- Checked serial devices: no /dev/ttyUSB* or /dev/ttyACM* nodes; lsusb found an Espressif USB JTAG/serial debug unit (303a:1001, the CoreS3, serial 30:ED:A0:0B:0F:50) but `cdc_acm` driver is not loaded (no passwordless sudo), so no /dev/ttyACM0.
- Concluded the host probe script can't run live: (a) no tty node, and (b) the module UART is the CoreS3's UART1, not a host-visible serial port. The on-device firmware IS the probe.
- Validated the probe script: `python3 -m py_compile` OK, `--help` OK.
- Sourced IDF 5.3.4 (`unset IDF_PYTHON_ENV_PATH; source ~/esp/esp-idf-5.3.4/export.sh`).
- Created `0118-cores3-qrcode-scanner/`: root CMakeLists, main/CMakeLists, main/idf_component.yml (m5unified ~0.2.18, m5gfx ~0.2.25), sdkconfig.defaults (CoreS3 quad PSRAM + USB Serial/JTAG + 16MB custom partitions), partitions.csv (4MB factory), main/app_main.cpp (M5Unified boot + ILI9341 banner + USB console heartbeat).
- `idf.py set-target esp32s3` (fetched m5stack__m5gfx + m5stack__m5unified managed components) then `idf.py build`.

### Why
- Building is host-side and needs no device, so progress is unblocked even though the live flash/probe waits on `cdc_acm`.
- Reusing the 0114 M5Unified/M5GFX managed-component versions (0.2.18 / 0.2.25) keeps the build identical to a known-good repo firmware.

### What worked
- Phase 1 build clean: `cores3_qrcode_scanner.bin` = 0x6f060 (~447 KB), 89% of the 4 MB factory partition free.
- gitignore correctly excludes build/, managed_components/, sdkconfig; dependencies.lock is committed (reproducibility).

### What didn't work
- `source .../export.sh | tail` ran in a subshell, so `idf.py` was not on PATH afterward. Fixed by sourcing without a pipe (`source ... > /dev/null 2>&1`).
- First build failed: `cfg.serial_baudrate` — that field only exists under `ARDUINO` (M5Unified config_t guards it with `#if defined(ARDUINO)`); under ESP-IDF it's absent. Removed the line (USB Serial/JTAG console is set up by IDF via sdkconfig, not M5Unified).
- Second build failed: `-Werror=format` — `M5.Display.width()/height()` return `int32_t` (long on xtensa), `%d` expects int. Fixed by casting to `(int)`.

### What I learned
- M5Unified's `config_t::serial_baudrate` is Arduino-only; in ESP-IDF the console is the IDF USB Serial/JTAG, configured by `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y` in sdkconfig.defaults.
- ESP-IDF 5.3.4 + `-Werror=all` treats `int32_t` vs `%d` as an error on xtensa (int is 32-bit but `int32_t` is `long`); always cast pixel counts to `int` in ESP_LOG format strings.

### What was tricky to build
- The cdc_acm blocker: the CoreS3 enumerates (lsusb shows 303a:1001) but with no driver bound, so there is no `/dev/ttyACM0`. `sudo modprobe cdc_acm` is needed to flash/monitor; I can't run it without a password. Per AGENTS.md "manual reset coordination", I asked the user explicitly rather than retrying serial opens.

### What warrants a second pair of eyes
- The `sdkconfig.defaults` PSRAM setting (`SPIRAM_MODE_QUAD`) vs 0114's octal — confirmed quad for CoreS3 from `0095-cores3-wifi-bench`; a wrong choice fails to boot. Verify against a real CoreS3 at flash time.
- `M5.Display.setRotation(1)` for landscape — confirm the CoreS3's native orientation when the device boots (visual check on the LCD).

### What should be done in the future
- Once the user runs `sudo modprobe cdc_acm`, flash Phase 1 and confirm the banner renders + USB console logs (the on-device proof of Phase 1).
- Add a udev rule so cdc_acm binds automatically on plug (avoid the per-session modprobe).

### Code review instructions
- Start at `0118-cores3-qrcode-scanner/main/app_main.cpp` (M5.begin, canvas banner, heartbeat).
- Reproduce build: `unset IDF_PYTHON_ENV_PATH; source ~/esp/esp-idf-5.3.4/export.sh; cd 0118-cores3-qrcode-scanner; idf.py set-target esp32s3; idf.py build` — expect `cores3_qrcode_scanner.bin`.

### Technical details
- Managed components resolved: `m5stack__m5gfx` 0.2.25, `m5stack__m5unified` 0.2.18 (matches 0114 dependencies.lock).
- Bootloader 0x5620 bytes (33% free); app 0x6f060 bytes (89% of 4 MB factory free).
- Brutalist slips: printer at 192.168.0.126 is offline ("no route to host"); slip YAMLs archived in `various/slip-*.yaml` for later reprinting.

## Step 4: Phase 2 — scanner driver (UART + I2C expander) + status probe

This step ported the official Arduino protocol layer to ESP-IDF and added the
on-device probe (`qr status`), which is the real Phase-0 "run the probe"
realized on the CoreS3. The scanner engine is driven over UART1 and
power/trigger over the PI4IOE5V6408 I2C expander.

### Prompt Context
**User prompt (verbatim):** (see Step 3)
**Commit (code):** bd0e0a5b — "ESP-62 P2: scanner driver (UART + I2C expander) + status console probe"

### What I did
- `qr_engine.{h,cpp}`: ESP-IDF UART port of qrcode_m14.cpp — `sendCmd()` (flush, write, match ACK), `getInfos()` (status read 0x43 -> reply 0x44 with big-endian len at [3:4]), `startDecode/stopDecode/setTriggerMode/setFillLightMode/setPosLightMode/setModeUart`. UART1, 115200 8N1, RX buffer 1024.
- `qr_module.{h,cpp}`: PI4IOE5V6408 @0x43 on M5.In_I2C (ch0=QR_5V_EN, ch4=TRIG, both output+pullup), power on + 300ms, UART init, scan pump task (50ms read poll -> line accumulator with \r\n suffix + 50ms quiet-time -> ScanResult queue depth 8).
- `qr_console.{h,cpp}`: esp_console REPL over USB Serial/JTAG; `qr status` reads firmware (0xC1) + serial (0xC5), prints a clear "NO REPLY -> check 12V/DIP/stack" hint on failure.
- `app_main.cpp`: M5 boot, module init (continuous trigger + on-decode lights), firmware version on the LCD banner, console start.
- `main/CMakeLists.txt`: added the 3 new sources.

### Why
- The on-device `qr status` IS the probe the user asked for: it exercises the full UART+I2C path and reports the engine firmware, proving the stack works before the UI.
- Using M5Unified's PI4IOE5V6408_Class (same class the Arduino lib uses) keeps the expander control identical to the reference and avoids a hand-written I2C driver.

### What worked
- Build clean: 513 KB app, 88% of 4 MB factory free; no warnings/errors.
- The expander constructor defaults (0x43, &m5::In_I2C) match the module exactly.

### What didn't work
- `std::unique_ptr<m5::PI4IOE5V6408_Class>` in the header with only a forward-declaration failed: the unique_ptr destructor needs the complete type (`sizeof` assert). Fixed by including `<utility/PI4IOE5V6408_Class.hpp>` in the header instead of forward-declaring.
- `xQueueOverwrite` requires a queue of depth 1; I wanted a history buffer (depth 8). Switched to `xQueueSend(...,0)` (drop newest if full).

### What I learned
- For a `unique_ptr<T>` member, T must be complete at the class's destructor point — either include the header or declare/define ~Class() out-of-line in the .cpp. Simplest is including the header.
- esp_console REPL on USB Serial/JTAG is a one-call setup (`esp_console_new_repl_usb_serial_jtag` + `esp_console_start_repl`); the `console` component is a default in IDF 5.3.4 (no manifest entry needed when the build isn't trimmed).

### What was tricky to build
- Keeping the protocol bytes byte-exact with the Arduino reference while using the ESP-IDF UART driver (tick-based `uart_read_bytes` instead of `delay()`+poll). The `getInfos` 5-byte header parse (len at [3:4] big-endian) is the easy part to get wrong.

### What warrants a second pair of eyes
- The scan-pump quiet-time (50ms) and \r\n suffix assumption — validate against the real module once flashed; if reads glue/split, tune or set the suffix explicitly via `21 51 C2 00 02 0D 0A`.
- TRIG idle-high assumption (active-low per the protocol): `setEnable(true)` drives TRIG high; continuous mode triggers via `startDecode` (software), so TRIG polarity shouldn't matter for the MVP, but verify if hardware trigger is used.

### What should be done in the future
- Once flashed, run `qr status` on USB console — a firmware-version reply is the proof Phase 0/2 succeeded. A "no reply" means 12V/DIP/stack issue (the console message says so).
- Expand `qr` commands in Phase 4 (start/stop/mode/light/brightness/beep/reset/info).

### Code review instructions
- Start at `qr_engine.cpp` (`sendCmd`, `getInfos`) and `qr_module.cpp` (`begin`, `pump`).
- Reproduce build: source IDF 5.3.4, `idf.py build` in `0118-cores3-qrcode-scanner`.

### Technical details
- Status reply framing: `44 <pid> <fid> <len_hi> <len_lo> <data...>`; `getInfos` copies `len` bytes from offset 5.
- App size 0x7d330 (~513 KB); bootloader 0x5620.

## Step 5: Phase 3 — on-screen UI (current code + history + buttons)

This step added the user-facing display: a single task owns the CoreS3 LCD,
drains the scan queue, and renders the decoded code plus a recent-history
list. Buttons toggle scanning and cycle the trigger mode.

### Prompt Context
**User prompt (verbatim):** (see Step 3)
**Commit (code):** 79eeb454 — "ESP-62 P3: on-screen UI (current code + history + buttons)"

### What I did
- `qr_ui.{h,cpp}`: `QRUI::start()` spawns a `qr_ui` task with a `UIState` (scanning flag, trigger mode, firmware, last code, history ring of 6). The task loop calls `M5.update()`, handles BtnA (toggle scan -> start/stop decode) and BtnB (cycle KEY->CONT->AUTO->PULSE->MOTION), drains the result queue (updates last code + pushes history, marks dirty), and redraws via an M5Canvas sprite when dirty.
- Layout (320x240 landscape): top bar (NAVY) with scan state + mode; firmware line; center last code large yellow wrapped; recent history list; footer bar with button hints.
- `app_main.cpp`: removed the static banner; hands the display to the UI task (now the only task calling M5.update/draw). app_main loop just sleeps (heartbeat removed to avoid a second M5.update race).

### Why
- One display-owner task avoids M5GFX races (design guide §6.2). The UI is the product surface: aim, see the code, see history.
- Buttons give a no-USB interaction path (handheld use); the console remains for operator/debug commands.

### What worked
- Build clean: 514 KB app, 88% free; no warnings.
- Dirty-flag redraw keeps the loop cheap (only redraws on scan/button change).

### What didn't work
- (Nothing blocking this step.)

### What I learned
- M5GFX `setTextWrap(true)` lets the large-font code wrap by spaces; reset to false after to keep the history list on fixed rows.
- Mode enum has an unused value 3; the BtnB cycle skips it (`while (m==3) m=(m+1)%6`).

### What was tricky to build
- Avoiding double `M5.update()`: the previous app_main loop called `M5.update()` every 100ms while the UI task also calls it at 30Hz. Two callers can double-fire button events. Fixed by making app_main's loop a pure sleep and the UI task the sole M5.update() caller.

### What warrants a second pair of eyes
- The history ring `memmove` when full: confirm no off-by-one truncates the newest entry. (Shift oldest out, append newest at `hist_count`.)
- Visual: confirm landscape orientation + font sizes read well on the real LCD at flash time.

### What should be done in the future
- Persist last codes to NVS and show a count; add a symbology badge once the engine reports it.
- Touch zones for start/stop (CoreS3 is touch) instead of only physical buttons.

### Code review instructions
- Start at `qr_ui.cpp` (`draw`, `push_history`, `ui_task`).
- Reproduce build: source IDF 5.3.4, `idf.py build` in `0118-cores3-qrcode-scanner`.

### Technical details
- App size 0x7d7f0 (~514 KB); UI task stack 6144, priority 4.

## Step 6: Phase 4 — full console command set + README + reproducibility

This step completed the operator console and proved the build is reproducible
from a clean tree, finishing the implementation phases (1-4) from the design
guide. Flashing + the live `qr status` probe remain blocked on the host loading
`cdc_acm` (needs the user's sudo).

### Prompt Context
**User prompt (verbatim):** (see Step 3)
**Commit (code):** 52d01d75 — "ESP-62 P4: full qr console (start/stop/mode/light/brightness/beep/reset) + README"

### What I did
- Engine: added `factoryReset()` (32 76 01), `setDecodeSuccessBeep(count)` (21 63 42), `setFillLightBrightness(pct)` (21 62 48) — exact bytes from qrcode_m14.cpp.
- Console: expanded `qr` to status/info/start/stop/mode/light/brightness/beep/reset with a `print_usage()` helper and arg validation.
- README.md: hardware (12V, DIP-UART, UART1 G17/G18, expander 0x43), build (IDF 5.3.4), flash (cdc_acm note), console use, file layout.
- Reproducibility: `idf.py fullclean` (removes build/ + managed_components/, keeps sdkconfig per AGENTS.md) then `idf.py build` from a clean shell rebuilt to the same 0x7dea0 (~515 KB) app with no errors/warnings.

### Why
- A full console gives the operator/interor complete runtime control without re-flashing, and the `qr status` NO-REPLY hint points directly at the three common hardware causes (12V, DIP, stack).
- Fullclean reproducibility is the design guide's Phase 4 done-criterion and catches stale-sdkconfig/managed-component drift.

### What worked
- Fullclean build reproduced byte-identical app size (0x7dea0) on the second build.
- The full command set compiled with no new warnings.

### What didn't work
- (Nothing blocking; cdc_acm remains a host-side prerequisite for flashing, not a firmware issue.)

### What I learned
- `idf.py fullclean` removes `managed_components/` too, so the rebuild re-fetches m5stack__m5gfx/m5unified from the lock — confirms dependencies.lock is correct and committed.
- The `qr reset` (factory reset) command writes to the engine's persistent storage; the console prints a "use with care" hint.

### What was tricky to build
- Keeping the console arg-parsing simple without a subcommand framework: a flat `strcmp` dispatcher + a usage helper is enough and matches the repo's 0095 wifi_console style.

### What warrants a second pair of eyes
- The `qr reset` command persists the engine to factory defaults — it is irreversible and should be documented as such (README says "careful").
- `brightness` uses `atoi` with no range guard beyond the engine's own clamp — acceptable (engine clamps 0-100).

### What should be done in the future
- Flash on real hardware once `cdc_acm` is loaded; run `qr status` to confirm the on-device probe; aim at codes to validate the scan pump/terminator (the 50ms quiet-time + \\r\\n assumption).
- Add a udev rule so `cdc_acm` binds automatically on CoreS3 plug (avoid the per-session `sudo modprobe`).

### Code review instructions
- Start at `qr_console.cpp` (`cmd_qr`, `print_usage`) and `qr_engine.cpp` (new methods).
- Reproduce: `source ~/esp/esp-idf-5.3.4/export.sh; cd 0118-cores3-qrcode-scanner; idf.py fullclean; idf.py build` — expect 0x7dea0 app, exit 0.

### Technical details
- App 0x7dea0 (~515 KB), 88% of 4 MB factory free; bootloader 0x5620.
- All 5 phases of the design guide's build plan (P0 probe-via-firmware, P1 skeleton+display, P2 driver+probe, P3 UI, P4 console+polish) are implemented and build-clean.
