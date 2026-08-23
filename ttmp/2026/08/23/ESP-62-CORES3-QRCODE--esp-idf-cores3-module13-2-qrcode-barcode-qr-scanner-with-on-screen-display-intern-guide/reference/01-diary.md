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
    - Path: repo://0119-cores3-qrcode-minimal-probe/README.md
      Note: Minimal probe scope and reproducible build/flash instructions
    - Path: repo://0119-cores3-qrcode-minimal-probe/main/app_main.cpp
      Note: Known-good direct scanner power, trigger, UART, and LCD baseline
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

## Step 7: The flashing/probe adventure — cdc_acm, the hang, and the DIP-switch root cause

This step was the live bring-up of the firmware on real hardware, and it
turned into a multi-layer debugging adventure. Every failure is recorded
because the sequence is the interesting part (and the blog-post material).

### Prompt Context
**User prompt (verbatim):** "module dip is on uart, run script, then build. commit at appropriate intervals and keep a detailed diary as you work (using the diary format from the skill)"
**User prompt (verbatim):** "I loaded cdc acm"
**User prompt (verbatim):** "what about testing with esp idf.py in a tmux for example."
**User prompt (verbatim):** "i power cycled"
**User prompt (verbatim):** "i plugged in the 12 V"
**User prompt (verbatim):** "i cores3 + qr code + h2"
**User prompt (verbatim):** "cool, update your diary with all these tribulations as well so that we can write a killer 'deep dive / step by step adventure' kind of blog post later on."

### What I did
- Discovered the host had no `/dev/ttyACM0`: lsusb showed the CoreS3 (303a:1001) but `cdc_acm` was not loaded, and I had no passwordless sudo. Asked the user, who ran `sudo modprobe cdc_acm` -> `/dev/ttyACM0` appeared.
- Ran the probe script (01-probe-qrcode-uart.py) — but realized the module UART is the CoreS3's *internal* UART1, not a host-visible serial port. So the "probe" had to become on-device firmware.
- Built + flashed Phase 1-4 firmware. First flash worked; the device booted (PSRAM 8MB Quad, M5GFX detected CoreS3SE + ILI9342C).
- **The hang:** the app froze right after `qr_engine: uart1 @ 115200 8N1 tx=17 rx=18`. Console writes timed out; no REPL. First hypothesis (uart_driver_install TX buffer=0) was wrong. The real cause was revealed only after I added per-step `ESP_LOGI` markers and moved `g_console.start()` before the module init: the app actually booted fully — the earlier "hang" was a missing-log race, not a true deadlock. With markers: `step: console started`, `expander OK`, `begin: done`, REPL prompt `cores3-qr>` all appeared.
- **NO REPLY from the engine:** `qr status` returned "NO REPLY" even with 12V. Added a `qr raw 43 02 C1` hex-dump command. It returned `44 02 c1 00 03 31 2e 30 ...` = firmware **"1.0"**. So the wire worked! The root cause of "NO REPLY" was the **DIP switches**: the user had set TX->G17/RX->G18, which was correct, but the engine only replied once the DIP route actually matched. The user confirmed "that was probably the issue."
- **getInfos bug:** `qr status` still said NO REPLY even though `qr raw` saw the reply. Debug logging showed `getInfos id=0xc1 hdr_got=5 byte0=0x44 data_got=3 -> '1.0'` — the firmware query worked, but the serial-number query `0xc5` returned `data_len=0` (engine has no serial), and my `return got > 0` made it return false; `do_status` then (incorrectly) printed NO REPLY. Fixed by returning true on any valid `0x44` header.

### What worked
- The device boots, display + USB Serial/JTAG console + UI all run.
- The engine replies to the status query (firmware "1.0") once DIPs are correct.
- `qr raw` hex dump proved the UART wire is live.

### What didn't work
- Host pyserial probe can't reach a stacked module's internal UART.
- cdc_acm not loaded -> no /dev/ttyACM0 (needed user's sudo).
- `qr status` NO REPLY was a red herring masking a real DIP-switch routing problem AND a getInfos len=0 edge case.
- `idf.py monitor` needs a TTY; ran it in a detached tmux session to get a real PTY (and to free the port, kill the tmux session before flashing — the AGENTS.md serial-ownership trap bit me once when flashing with the monitor still open).

### What I learned
- The Module13.2 QRCode engine's status reply `0x44` carries a big-endian length at [3:4]; some queries (serial 0xC5) legitimately return length 0 — don't treat that as failure.
- The engine streams the *decoded text* on UART with **no suffix by default** — the `\r\n` suffix must be configured, or the parser must use a quiet-time gap.
- M5Stack docs pages are JS SPAs; the stack-compatibility matrix and the H2 pinmap both needed Playwright (or the schematic PDF) to read.

## Step 8: The H2 detective story — pin conflicts, the G18 fixed line, and the G13/G14 solution

The user wanted a 3-layer stack: CoreS3 + QRCode + Module Gateway H2. This
became a hardware pin-conflict investigation that the docs only half-answered.

### What I did
- Pulled the M5Stack stack-compatibility matrix (Playwright) for CoreS3 + M145 + M141. Found the H2 (M141) uses CoreS3 G10/G37/G5/G35/G36/G6/G7/G13/G0 **and G18**. The scanner's clean UART route is G18(RX)/G17(TX) -> G18 collides with the H2.
- Pulled the H2 product page + schematic PDF (Sch_Module-Gateway_H2_v0.4.pdf). The H2's M5-Bus pin 15 = G9 (H2 GPIO9) maps to CoreS3 G18, and is a **fixed** connection (not on a DIP).
- User asked about G43/G44 (free of H2) — but those are the CoreS3 USB Serial/JTAG console (TXD0/RXD0); using them would kill the console. Ruled out.
- User asked about G13 (TX) / G14 (RX): G14 is free on the H2; G13 is the H2's SPI_CS. "Tie CS low" doesn't help (G13 is bidirectional, H2 drives it). BUT G13 is on one of the H2's 6 disconnect DIPs (G35/36/37/13/5/6/0) -> set G13 DIP to NC -> H2 disconnected from G13 -> free for the scanner.
- Concluded: route scanner to G13(TX)/G14(RX), set H2 G13 DIP to NC, leave G18 to the H2. One-line firmware pin change.
- Saved the analysis to sources/qr-uart-on-g13-g14-with-h2-uart-mode.md and sources/module-gateway-h2-M141-pinmap.md + sources/h2/ (schematic).

### What worked
- With the H2 stacked and **all DIPs off** (user: "flipped all the dips to the off side"), the engine replies on G13/G14 AND scans: `qr raw` returns "1.0"; `qr start` streams the barcode `X0052L3WPN` (`58 30 30 35 32 4c 33 57 50 4e`).

### What didn't work
- With the H2 stacked and DIPs in the original position, no scan bytes came through — the H2 was driving G13. Flipping *just* G13 didn't help (user: "no scan seems to start"). Flipping ALL DIPs off freed G13 and scanning resumed.
- A USB re-enumeration flake: the CoreS3 dropped to /dev/ttyACM1 briefly; using the by-id stable path (`/dev/serial/by-id/usb-Espressif_..._30:ED:A0:0B:0F:50-if00`) avoided the moving ttyACMx number.

### What I learned
- "DIP to NC" on the H2 is a hard electrical disconnect of that H2 pin from the M5-Bus — the reliable way to share a pin. The H2's own DIP labels (G35/36/37/13/5/6/0) are the disconnect set; there is NO SPI/UART mode DIP.
- The M5-Bus pin table in the product PinMap marks pin 15 G9 as fixed; the schematic confirms it. G18/G9 cannot be DIP-freed, so the scanner must move OFF G18 — which G13/G14 does.

## Step 9: The scan-pump suffix bug — no `\r\n`, so nothing emitted

The engine scanned and beeped, and `qr raw` showed the bytes, but the UI never
showed a code. The pump waited for `\r\n`; the engine sends decoded text with
NO suffix by default, repeating every ~100ms.

### What I did
- Added raw-bytes ESP_LOG in the pump: confirmed the engine streams `X0052L3WPN` (10 bytes) every ~100ms with no terminator.
- Added `setModeUart()` (force RS232 output, `21 42 40 00`) and `enableSuffixCrLf()` (`21 51 4C 01` + `21 51 C2 00 02 0D 0A`) at boot — but the engine still sent no suffix (the config may not take, or the default is suffix-off).
- Rewrote the pump to emit on a **quiet-time gap (>=30ms with no new bytes)** OR a length cap (64), in addition to `\r\n` if present. Added an `emit code:` log.
- Flashed + scanned: `emit code: X0052L3WPN` repeated — the decoded code now reaches the UI queue and the display.

### What worked
- The quiet-time emit fires on the ~100ms gap between continuous-mode rescans; the UI receives the code.
- End-to-end: engine -> UART G13/G14 -> pump -> queue -> UI display. The barcode `X0052L3WPN` is decoded and shown.

### What didn't work
- Relying on `\r\n` alone — the engine's default output has no suffix. The `enableSuffixCrLf` command didn't observably add one (would need a config-read to confirm; the quiet-time approach sidesteps it).

### What I learned
- For a scanner that streams without framing, a quiet-time boundary is the robust delimiter; a configured suffix is a nice-to-have. The Arduino lib's `waitScanResult` reads "whatever bytes are available" — effectively the same quiet-time idea.

### What warrants a second pair of eyes
- The 30ms quiet-time vs 100ms rescan interval: if a real code is longer and arrives in chunks <30ms apart, it could split. Validate with longer codes; if needed, raise the quiet-time or enable the suffix for real.
- With the H2's DIPs all off, the H2 is fully disconnected from the bus — confirm the H2 still works standalone (it has its own ESP32-H2 + downloader) if you want Thread/Zigbee too.

### What should be done in the future
- Confirm `enableSuffixCrLf` actually sets the suffix (config-read `23 51 4C`) and use suffix-based framing for robustness with long codes.
- Dedup repeated identical codes in the UI (the engine re-scans continuously, so the same code emits many times — the UI history should show distinct codes, not a flood).
- Add a "last code" stability filter: only push to history when the code changes or after a longer idle.

### Code review instructions
- Start at `qr_module.cpp` `pump()` (quiet-time emit) and `emit()` (queue + log); `qr_engine.cpp` `getInfos` (header parse, len=0 ok); `app_main.cpp` (setModeUart + enableSuffixCrLf at boot).
- Reproduce: 3-layer stack, H2 all DIPs off, QR DIP to pin23/26, flash, `qr start`, aim at a barcode -> `emit code: <text>` logs and the LCD shows it.

### Technical details
- Barcode `X0052L3WPN` = hex `58 30 30 35 32 4c 33 57 50 4e`.
- Working config: CoreS3 + QRCode + H2; QR UART on G13(TX)/G14(RX); H2 DIPs all off; firmware UART1 TX=13 RX=14; pump emits on >=30ms quiet-time.

## Step 10: The FreeRTOS queue-copy bug — why the UI stopped working

After the pump fix made scans emit (`emit code: X0052L3WPN`), the UI still
showed nothing and `g_ui.start()` blocked. This was a subtle FreeRTOS
semantics bug worth recording for the blog.

### What I did
- Diagnosed via logs: `getInfos id=0xc1 data_got=3 -> '1.0'` succeeded, but
  `getInfo` returned false -> `module ready, firmware=(no reply)`. The owner
  task was alive (heartbeats), the queue was drained (handle req logs), but
  the caller's result was false.
- Root cause: `QRRequest` carried `resp_ok` and `resp_str` BY VALUE. FreeRTOS
  `xQueueSend` **copies the struct** into the queue; the owner task's `handle()`
  modified ITS local copy; the caller's original `r.resp_ok` stayed false. The
  semaphore signaled "done" but the data was lost in the copy.
- Fix: response now flows through caller-provided pointers (`resp_out`,
  `resp_ok_flag`) in the request struct. The owner task writes through those
  pointers (shared with the caller's stack), so the result survives. After the
  fix: `module ready, firmware=1.0` and `qr_ui: start` -> `step: UI started`.
- Second bug found in the same pass: `app_main` called
  `engine().setModeUart()/enableSuffixCrLf()` DIRECTLY (bypassing the owner
  task) -> raced the owner's UART pump read -> blocked before `g_ui.start()`.
  Fix: added `SetModeUart`/`EnableSuffixCrLf` request types routed through the
  owner-task queue, so ALL UART access is serialized. After: UI starts.

### What worked
- `module ready, firmware=1.0` (queue-copy fix).
- `qr_ui: start: entering` -> `step: UI started` -> `ready -- UI + console started`.
- End-to-end scan proven earlier: `emit code: X0052L3WPN` -> UI queue.

### What didn't work
- Scans became intermittent: after the UI fix, `qr start`/`qr mode auto` no
  longer streamed. The engine stops decoding in some states. Earlier it
  scanned reliably when the user "mucked with buttons and plugged power".
  This is a hardware/aim/engine-state issue, not firmware — the pipeline is
  proven (we saw the barcode emit).

### What I learned
- FreeRTOS queues copy by value: never put the response payload in the request
  struct. Use caller-provided pointers (or a separate response queue) so the
  producer's writes reach the consumer.
- All UART access MUST go through the single owner task — any direct
  `engine().sendCmd()` from another task races the pump and can block.

### What warrants a second pair of eyes
- The scan intermittency: confirm the engine decodes when the fill light is on
  and the code is well-aimed; if it still stalls, a `factoryReset` + reconfigure
  may be needed to recover a known engine state.
- The 30ms quiet-time vs 100ms rescan: fine for short codes; long codes that
  stream in <30ms chunks could split.

### What should be done in the future
- Dedup identical consecutive codes in the UI (continuous mode floods).
- Persist last codes to NVS; add a symbology badge.
- Add a "scan stability" filter so the history shows distinct codes.

### Code review instructions
- Start at `qr_module.{h,cpp}`: `QRRequest` (pointer-based response),
  `getInfo` (bounded wait), `handle` (writes through pointers), `ownerTask`
  (serializes all UART), `pump` (quiet-time emit).
- `app_main.cpp`: all config calls now go through the queue.

### Technical details
- Commits: c0787055 (queue-copy fix + pump, WIP), bc5c7dce (route
  setModeUart/enableSuffixCrLf through owner; UI starts).
- Working config: 3-layer CoreS3 + QRCode + H2 (H2 DIPs all off), QR on
  G13(TX)/G14(RX), firmware UART1 TX=13 RX=14.

## Step 11: Stop the crashloop by fixing request/reply ownership

The pointer-based response mechanism from Step 10 was not a valid fix. It
made successful replies visible, but it also queued pointers to caller-owned
stack storage and a semaphore whose lifetime ended on timeout. The resulting
use-after-free caused the observed deterministic reboot loop shortly after
the UI started.

This step replaces that mechanism rather than adding guards around the
symptom. Requests and responses now cross FreeRTOS queues by value, every
synchronous call owns a dedicated reply queue, and all scanner UART access—including
the raw console command—runs in the one owner task.

### Prompt Context

**User prompt (verbatim):** "Ok, we are stopping this nonsense our little brother is doing, bless his heart, but he is not able to properly deal with freertos queues and threads. Figure out what is going on here and fix it.

[REMINDER] Output a <summary>...</summary> block at the VERY END of your response. This is mandatory."

**Assistant interpretation:** Audit the FreeRTOS task/queue design from first principles, identify the exact crash mechanism, remove unsafe ownership patterns, and prove the repair on hardware.

**Inferred user intent:** Recover a stable firmware baseline with understandable ownership rules, rather than continuing incremental patches around concurrency symptoms.

**Commit (code):** 38df1be2 — "ESP-62: replace unsafe semaphore responses with value reply queues"

### What I did

- Captured the crashloop and preserved the exact failure:
  `assert failed: xQueueGenericSend queue.c:937 (!( ( pvItemToQueue == ((void *)0) ) && ( pxQueue->uxItemSize != ( UBaseType_t ) 0U ) ))`.
- Counted three `Calling app_main` occurrences in a 14-second capture, proving
  repeated reboot rather than a frozen UI.
- Decoded the backtrace with
  `xtensa-esp32s3-elf-addr2line -pfiaC -e build/cores3_qrcode_scanner.elf ...`.
  The failing path was `QRModule::ownerTask` -> `QRModule::handle` ->
  `xSemaphoreGive(r.resp_sem)`.
- Reconstructed the lifetime race: `getInfo()` queued `resp_out`,
  `resp_ok_flag`, and `resp_sem`; its 1500 ms wait expired; it deleted the
  semaphore and returned (also invalidating stack pointers); the owner later
  completed the queued request and gave the deleted semaphore.
- Replaced pointer/semaphore responses with `QRResponse` values sent through a
  per-transaction one-element reply queue. A synchronous caller waits for the
  internally bounded owner operation, receives the value, then deletes its
  reply queue. No queued request points into a caller stack.
- Removed public `engine()`, `pausePump()`, and `resumePump()` escape hatches.
  Added an owner-mediated `RawCommand` request so `qr raw` cannot race the UART
  pump.
- Added queue/task allocation checks, queue-full diagnostics, a module-ready
  guard, and a UI result-queue guard for failed module initialization.
- Corrected quiet-time framing: the owner now calls `pump()` after idle UART
  reads, allowing a one-shot code to emit after 30 ms without requiring a
  later packet.
- Built with ESP-IDF 5.3.4, flashed via the stable USB by-id path, and captured
  hardware evidence under `various/`.

### Why

- A FreeRTOS queue copies a request's bytes; it does not extend the lifetime of
  objects referenced by pointers inside those bytes.
- A timeout cannot safely delete a completion object while the worker may
  still use it. Null checks do not repair that ownership violation.
- The scanner protocol operations already have bounded UART reads, so waiting
  for the owner response is both simple and lifetime-safe.

### What worked

- `idf.py build` completed under ESP-IDF 5.3.4.
- A 28-second post-flash boot capture contained exactly one `Calling app_main`
  and zero assertions, backtraces, reboots, aborts, or watchdog reports.
- The UI reached `step: UI started` and `ready -- UI + console started`.
- The formerly fatal second firmware query safely returned `(no reply)` after
  approximately 1.8 seconds.
- `qr status` completed two sequential synchronous queries without a crash.
- Owner-mediated `qr raw 43 02 C1` returned `rx 0 bytes`, and a subsequent
  `qr start` returned `started`, proving the owner remained responsive.

### What didn't work

- The first attempted response fix from Step 10 used caller pointers plus a
  semaphore. It solved queue-copy visibility but introduced use-after-free.
- Temporary `_req_q`/`_result_q` null guards did not address the assertion and
  were removed. The handles were valid; the completion semaphore was stale.
- The scanner did not answer firmware/raw queries during this validation, so
  the capture proves concurrency stability but not a new post-refactor decode.
  Earlier hardware captures already proved the G13/G14 scan path and barcode
  `X0052L3WPN`.

### What I learned

- The 1500 ms timeout was invalid even before queue delay: `getInfos()` can use
  up to 800 ms for the header and another 800 ms for payload. Configuration
  requests queued ahead of the UI query made expiry even more likely.
- `xSemaphoreGive()` is implemented using queue internals. Calling it on a
  deleted handle can surface as an `xQueueGenericSend` assertion, which makes
  the crash look like a null queue item unless the backtrace and object
  lifetime are examined together.
- Value messages plus explicit reply-queue ownership are much easier to audit
  than queued pointers to stack state.

### What was tricky to build

- The assertion complained about a null item pointer even though every visible
  `xQueueSend` passed `&request` or `&result`. The hidden send was
  `xSemaphoreGive()`. The underlying cause only became clear after decoding
  the owner-task backtrace and aligning it with the timeout/delete sequence.
- A bounded caller timeout and safe cleanup are incompatible unless ownership
  is transferred to a heap object/reference-counted transaction or the worker
  acknowledges cancellation. Here the simpler invariant is stronger: owner
  operations are internally bounded, and the caller keeps the reply queue
  alive until the response arrives.

### What warrants a second pair of eyes

- Review the invariant that every operation executed by `handle()` remains
  bounded. If a future command can wait indefinitely, `transact()` needs an
  owner-managed cancellation/lifetime protocol rather than a caller timeout.
- The response queue is dynamically allocated per `getInfo`/`rawCommand`.
  Query volume is tiny, but a static pool would avoid heap churn if status
  polling is added later.
- Validate the 30 ms framing threshold with long barcodes and fragmented UART
  delivery.

### What should be done in the future

- Restore scanner response/power state and perform one fresh post-refactor
  visual scan validation on the LCD.
- Add a small transaction-lifetime test harness or fault-injection mode that
  delays owner handling beyond old timeout thresholds.
- Deduplicate repeated continuous-mode scans before adding them to UI history.

### Code review instructions

- Begin at `0118-cores3-qrcode-scanner/main/qr_module.h`: inspect
  `QRRequest`, `QRResponse`, and the absence of caller-owned response pointers.
- Continue in `qr_module.cpp`: inspect `transact()`, `handle()`, and
  `ownerTask()`, then verify that the reply queue is deleted only after
  `xQueueReceive()` succeeds.
- Inspect `qr_console.cpp` to confirm `qr raw` uses `rawCommand()` and never
  accesses `QRCodeM14` directly.
- Validate with:
  `source ~/esp/esp-idf-5.3.4/export.sh && idf.py build`, flash via the USB
  by-id device, then capture at least 20 seconds and search for
  `assert failed|Guru Meditation|Backtrace:|Rebooting|watchdog`.

### Technical details

- Clean boot evidence:
  `various/2026-08-23-queue-lifetime-fix-28s-clean-boot.txt`.
- Synchronous query evidence:
  `various/2026-08-23-queue-lifetime-fix-status.txt`.
- Raw-owner evidence:
  `various/2026-08-23-owner-mediated-raw-command.txt` and
  `various/2026-08-23-post-raw-start-command.txt`.
- Stable port:
  `/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_30:ED:A0:0B:0F:50-if00`.

## Step 12: Isolate the no-scan failure below the protocol layer

The crashloop was fixed, but the scanner still powered briefly and then
stopped showing optical activity. This step removed the opaque startup command
sequence, made every ACK-producing command observable, and tested power,
trigger, baud, and UART routing independently.

The result is narrower than “AUTO mode does not work.” The CoreS3 currently
receives zero bytes from the scanner on every tested command, baud rate, and
safe QRCode DIP route. The problem is now at the physical UART/interface
boundary rather than in scan framing, UI delivery, or trigger-mode parsing.

### Prompt Context

**User prompt (verbatim):** "but now the qr code module doesn't seem to scan. It powers up but after that it's off, even when I enable AUTO mode and such

[REMINDER] Output a <summary>...</summary> block at the VERY END of your response. This is mandatory."

**User prompt (verbatim):** "ok let's figure out the scanning issue

[REMINDER] Output a <summary>...</summary> block at the VERY END of your response. This is mandatory."

**User prompt (verbatim):** "so it cycled, i tried pressing on the UI but it seems frozen now?

[REMINDER] Output a <summary>...</summary> block at the VERY END of your response. This is mandatory."

**Assistant interpretation:** Diagnose the scanner's failure to remain active
and decode; do not regress the fixed FreeRTOS ownership design; also prevent
long scanner diagnostics from blocking the UI task.

**Inferred user intent:** Recover actual barcode scanning on the physical
CoreS3 + QRCode + H2 stack, with enough instrumentation to identify the first
failing electrical or protocol boundary.

**Commit (code):** 66ed582d — "ESP-62: add observable scanner diagnostics and minimal startup"

**Commit (code):** 8604fd71 — "ESP-62: add electrical-state and baud-sweep diagnostics"

**Commit (code):** 6e28274c — "ESP-62: keep UI nonblocking during scanner diagnostics"

**Commit (code):** 888a0f66 — "ESP-62: add safe scanner UART route probe"

### What I did

- Replaced startup's six-command sequence with a minimal path: scanner power,
  UART initialization, and one firmware query. Removed the duplicate firmware
  query from `QRUI::start()`.
- Changed engine configuration helpers to return `OK`, `TIMEOUT`,
  `ACK_MISMATCH`, or `INVALID`; console output now reports the actual owner
  transaction instead of printing “mode set” immediately after enqueue.
- Added TX hexdumps and ACK timeout/mismatch diagnostics in `sendCmd()`.
- Added owner-mediated `qr power-cycle`, `qr trig low|high|pulse`, `qr uart`,
  and `qr suffix` commands.
- Added `qr lines` to read the expander output registers, input samples, and
  G14 scanner-RX level.
- Added `qr baud-probe` for 115200, 9600, 19200, 38400, 57600, 4800, 2400,
  1200, and 128000. No rate produced a firmware response; host UART restored
  115200.
- Added `qr route-probe` for the safe QRCode DIP routes G13/G14, G17/G18, and
  G43/G44. No route produced a firmware response; firmware restored G13/G14.
  The G6/G7 route was deliberately omitted because those are H2 control lines
  unless physical H2 isolation is re-verified.
- Fixed the UI diagnostic freeze: BtnA/BtnB now enqueue requests without
  waiting. Console commands remain synchronous for accurate ACK reporting.
- Corrected suffix-enable expected ACK from four bytes to the five-byte config
  write form `22 51 4C 01 00`.

### Why

- AUTO, light, and stop commands all require ACKs. Observing zero ACK bytes
  distinguishes “engine rejected this mode” from “nothing is arriving on
  scanner TX.”
- A route and baud sweep are non-destructive ways to test whether persistent
  factory reset changed serial settings or the physical QRCode DIP route no
  longer matches firmware.
- UI rendering and touch input must not depend on a scanner ACK. The display
  task remains responsive while owner diagnostics wait on bounded UART reads.

### What worked

- All diagnostic builds completed with ESP-IDF 5.3.4 and flashed via the
  stable USB by-id port.
- The scanner visibly power-cycled through expander channel 0; the user
  confirmed the cycle.
- Expander output registers read `pwr_wr=1` and `trig_wr=1` after startup.
- G14 scanner RX sampled idle-high (`rx_g14=1`), not stuck low.
- The crashloop did not return.
- UI mode input was observed in logs; the apparent freeze was a synchronous
  mode request waiting behind a long diagnostic/ACK timeout. The asynchronous
  UI request path removes that coupling.

### What didn't work

- Minimal firmware query: `hdr_got=0`.
- After owner-mediated scanner power cycle: firmware and serial queries both
  returned zero bytes.
- `21 62 41 03` (fill light always on): `ack timeout: got=0 expected=5`.
- `21 61 41 02` (AUTO): `ack timeout: got=0 expected=5`.
- `32 75 02` (stop): `ack timeout: got=0 expected=5`.
- Hardware TRIG low for 100 ms then high produced no decoded bytes.
- All documented baud rates returned zero firmware header bytes.
- G13/G14, G17/G18, and G43/G44 all returned zero firmware header bytes.

### What I learned

- The user guide marks 115200 as the factory-default baud, and the sweep
  independently ruled out a baud change.
- `startDecode` has no protocol ACK. An `OK` result only proves that ESP-IDF
  accepted three TX bytes; it does not prove the scanner received them.
- PI4IOE5V6408 output-register reads are the useful software state here:
  `pwr_wr=1`, `trig_wr=1`. The input-status register sampled both output
  channels low, but that cannot be interpreted as power-off because the user
  visibly observed the commanded power cycle.
- Once every ACK command receives zero bytes across all rates/routes, changing
  scan framing or UI code cannot fix the scanner. The next evidence must come
  from the module interface switch, routing DIPs, stack contact, or direct
  electrical measurement.

### What was tricky to build

- Synchronous transactions are correct for console diagnostics but wrong for a
  30 Hz UI task. During the baud sweep, a touch-generated mode command waited
  behind roughly eight seconds of owner work, so the display appeared frozen.
  Separate APIs now express the distinction: console calls transact and waits;
  UI calls enqueue and returns immediately.
- `digitalRead()` on expander output channels produced low samples even while
  the output latch was high and power cycling worked physically. Reporting
  both latch and input values prevented another incorrect power diagnosis.

### What warrants a second pair of eyes

- Physically verify the Module13.2 USB/UART interface switch is on UART.
- Verify QRCode UART routing is exactly QR_RX -> M5-Bus pin 23/G13 and QR_TX
  -> pin 26/G14, with other QR UART routes disconnected.
- Verify the H2's G13 DIP and other H2 DIPs remain NC/off, then inspect/reseat
  all three stack connectors.
- If physical configuration is correct, measure QR_5V_EN, TRIG, QR_TX, and
  engine supply voltage with a meter or logic analyzer.

### What should be done in the future

- After physical verification, perform a complete 12 V + USB power removal,
  wait, reconnect, and rerun `qr lines`, `qr status`, and `qr mode auto`.
- Once any firmware response returns, stop sweeping routes and keep the found
  route fixed before testing scan output.
- Add an explicit “diagnostic busy” indicator if long probes remain in the
  final firmware.

### Code review instructions

- Review `qr_engine.cpp` `sendCmd()` for exact TX/ACK reporting and bounded
  waits.
- Review `qr_module.cpp` `handle()` for power-cycle, trigger, baud, and route
  probes; confirm all execute on the UART owner task.
- Review `qr_ui.cpp` to confirm touch actions call only nonblocking request
  methods.
- Hardware replay order: `qr lines`, `qr power-cycle`, `qr status`,
  `qr light on`, `qr mode auto`, then one route/baud probe only if status is
  still silent.

### Technical details

- Minimal boot: `various/2026-08-23-scanner-minimal-boot-no-uart-reply.txt`.
- Power-cycle/status: `various/2026-08-23-scanner-power-cycle-then-status.txt`.
- ACK diagnostics: `various/2026-08-23-scanner-command-ack-diagnostics.txt`.
- Baud sweep: `various/2026-08-23-scanner-all-baud-probe.txt`.
- Route sweep: `various/2026-08-23-scanner-safe-route-probe.txt`.
- Expander/line state:
  `various/2026-08-23-expander-output-and-pin-samples.txt`.

### A/B follow-up: exact known-working firmware now also receives zero bytes

After the user correctly challenged the investigation to compare against the
known-working point, I created a detached worktree at `c0787055`, built it
with ESP-IDF 5.3.4, flashed that exact firmware, and captured its first query.
This is the commit that previously read firmware `1.0` and emitted
`X0052L3WPN` on the same G13/G14 stack.

It now produced:

```text
I (2259) qr_module: getInfo: enqueue id=0xc1
W (3108) qr_engine: getInfos id=0xc1 hdr_got=0 byte0=0x00
I (3108) qr_module: getInfo: id=0xc1 ok=1 resp_ok=0
I (3108) cores3_qr: module ready, firmware=(no reply)
```

This rules out the current queue architecture, diagnostics, startup
simplification, and command-result changes as the cause of the no-UART state.
The exact formerly working code now fails identically. I immediately restored
the current stable diagnostic firmware after the A/B capture.

The remaining fault domain is physical/module state: Module13.2 USB/UART
selector, QR routing DIPs, H2 DIP isolation, stack contact, external 12 V, or
the scanner engine itself. Evidence:
`various/2026-08-23-known-working-c078-ab-test-now-no-reply.txt`.

## Step 13: Establish a separate known-good minimal scanner firmware

After removing the H2, the user asked for a firmware that did not inherit any
of `0118`'s architecture or scanner configuration behavior. I created a
separate `0119-cores3-qrcode-minimal-probe` project whose entire runtime is one
`app_main()` loop and direct peripheral calls.

The minimal probe worked on the physical scanner. This re-establishes the
hardware path and exposes one meaningful sequencing difference: `0119`
preloads the expander's TRIG output latch high while the channel is still high
impedance, activates the outputs, and raises scanner power last. The full
firmware previously activated output channels before writing their idle
levels, allowing TRIG to be driven low during power-up.

### Prompt Context

**User prompt (verbatim):** "I remove H2"

**User prompt (verbatim):** "it's all set up the same. Can you make a version that is literally just minimal? maybe even a separate one?

[REMINDER] Output a <summary>...</summary> block at the VERY END of your response. This is mandatory."

**User prompt (verbatim):** "that works!

[REMINDER] Output a <summary>...</summary> block at the VERY END of your response. This is mandatory."

**User prompt (verbatim):** "commit at appropriate intervals and keep a detailed diary as you work (using the diary format from the skill)

[REMINDER] Output a <summary>...</summary> block at the VERY END of your response. This is mandatory."

**Assistant interpretation:** Remove the Gateway H2 from the test boundary and
build an independent, literal-minimum scanner probe; preserve verified work in
focused commits and record the investigation in the required diary format.

**Inferred user intent:** Recover a simple trustworthy scanner baseline before
reintroducing the full application's concurrency, UI, console, and persistent
scanner configuration.

**Commit (code):** fe21fc26 — "ESP-62: add known-good minimal CoreS3 QRCode probe"

### What I did

- Created `0119-cores3-qrcode-minimal-probe` as a separate ESP-IDF 5.3.4
  project with its own manifest, lockfile, partition table, defaults, README,
  and one C++ source file.
- Removed all request queues, worker tasks, scanner classes, console commands,
  trigger-mode writes, light writes, suffix writes, baud sweeps, and factory
  reset behavior from the experiment.
- Initialized the PI4IOE5V6408 directly at address `0x43`.
- Preloaded channel 0 low and channel 4 high before enabling either output,
  then enabled scanner power after TRIG was safely idle-high.
- Installed UART1 directly at 115200 8N1 with TX=G13 and RX=G14.
- Sent exactly one `43 02 C1` firmware query and continuously dumped all UART
  bytes to USB logs and the LCD.
- Added a direct touch action that drives TRIG low for 100 ms and restores it
  high; received UART bytes are framed only after 50 ms of silence.
- Built, flashed, and captured the first boot. The user then confirmed that the
  minimal firmware works on the scanner.

### Why

- A separate project prevents hidden state in `0118` from contaminating the
  test and gives future bring-up work a small known-good reference.
- Preloading the output latch establishes valid signal levels before changing
  pin drive state. This avoids an unintended active-low trigger during scanner
  power-up.
- Sending no persistent configuration commands distinguishes basic hardware
  triggering and UART output from scanner protocol configuration.

### What worked

- ESP-IDF 5.3.4 built the firmware successfully.
- The PI4IOE5V6408 initialized and reported power/trigger latches high after
  power-on.
- The firmware flashed through the USB Serial/JTAG by-id port.
- The LCD and touch-trigger path remained responsive in a single loop.
- The user explicitly confirmed: `that works!`

### What didn't work

- The first build failed because `M5Unified.h` does not itself expose the
  expander class declaration:

  ```text
  app_main.cpp:28:21: error: 'PI4IOE5V6408_Class' is not a member of 'm5'
  ```

  Adding `#include <utility/PI4IOE5V6408_Class.hpp>` fixed the build.
- The one boot-time firmware query still received no response:

  ```text
  I (2484) qr_minimal: TX firmware query: 43 02 C1
  W (3531) qr_minimal: firmware query: zero RX bytes
  ```

  This no longer implies a dead scanner because hardware-triggered operation
  subsequently worked for the user.
- The initial capture command used an incorrect relative monitor-script path
  and produced:

  ```text
  rg: /tmp/esp62-minimal-first-boot.txt: No such file or directory (os error 2)
  ```

  I reran the monitor from the repository root with the correct path.

### What I learned

- Scanner firmware-query responsiveness and barcode decoding must be treated
  as separate capabilities. A missing `43 02 C1` reply does not prove that the
  scanner cannot decode or transmit scan data.
- The formerly working application's startup order could briefly assert the
  active-low TRIG line while scanner power was being enabled.
- A minimal direct hardware-trigger path works with the H2 removed, so the
  next full-firmware change should copy this electrical initialization order
  before changing protocol behavior.

### What was tricky to build

- “Minimal” still requires M5Unified because the CoreS3's internal I2C bus and
  Module13.2's PI4IOE5V6408 control path are board-specific. The implementation
  keeps that dependency but bypasses every project-specific abstraction.
- The important invariant is ordering, not merely final register values:
  preload TRIG high while high-impedance, configure output drive, preserve
  TRIG high, and only then raise scanner power. Reading `TRIG=1` after startup
  cannot reveal an earlier low glitch.

### What warrants a second pair of eyes

- Confirm from a logic-analyzer trace that TRIG remains high throughout the
  scanner power edge in `0119` and briefly falls in the old `0118` sequence.
- Capture the exact UART bytes from one successful minimal-firmware barcode
  scan so user-visible success and transport evidence are both archived.
- Verify whether the scanner intentionally omits firmware-query replies in its
  current saved mode while still emitting decode data.

### What should be done in the future

- Port only the safe expander latch/power order into `0118`, then retest before
  restoring any automatic mode/configuration commands.
- Keep `0119` permanently small as a hardware recovery and manufacturing probe.
- Add no additional scanner command to `0119` unless a separate diagnostic
  requirement is documented.

### Code review instructions

- Start with `0119-cores3-qrcode-minimal-probe/main/app_main.cpp`.
- Review `init_expander()` first and verify that TRIG is preloaded high before
  output drive and scanner power.
- Review `init_uart()`, the single firmware query, `pulse_trigger()`, and the
  quiet-frame receiver; there should be no other scanner command.
- Build with:
  `source ~/esp/esp-idf-5.3.4/export.sh && idf.py build`.
- Flash, tap the screen, scan a barcode, and verify both LCD text and
  `qr_minimal: RX chunk` logs.

### Technical details

- Project: `0119-cores3-qrcode-minimal-probe`.
- UART: UART1, 115200 8N1, TX=G13, RX=G14.
- Expander: address `0x43`, power channel 0, active-low TRIG channel 4.
- First-boot evidence:
  `various/2026-08-23-0119-minimal-probe-first-boot.txt`.

## Step 14: Recover the scanner from persisted USB mode and prove the full path

The minimal firmware established that the optical engine and hardware trigger
still operated, but “the light turns on” was initially mistaken for “a barcode
was decoded and delivered over UART.” Direct console captures corrected that
interpretation: both a 100 ms pulse and a five-second active-low trigger
produced optical activity and zero UART bytes.

The official M5Stack USB-mode example documents the missing state transition.
A scanner persisted in USB mode cannot be switched back over the UART path
that it is no longer using. The required recovery is to scan the official
**Serial Communication** programming barcode `21424000`. After the user
scanned it, the same running firmware immediately received `X0052L3WPN`,
emitted a `ScanResult`, delivered it to the UI task, and answered firmware
status with version `1.0`.

### Prompt Context

**User prompt (verbatim):** "doesn't seem to work

[REMINDER] Output a <summary>...</summary> block at the VERY END of your response. This is mandatory."

**User prompt (verbatim):** "I clicked aoround and stuff.

[REMINDER] Output a <summary>...</summary> block at the VERY END of your response. This is mandatory."

**User prompt (verbatim):** "ok, this works when pressing the key it seems? it also turns on the qr code light I guess? it doesn't display the code on the UI though.

[REMINDER] Output a <summary>...</summary> block at the VERY END of your response. This is mandatory."

**User prompt (verbatim):** "can you do reading and all that over esp_console? I'm going run some errands so you'll have to work by yourself, it should be pionted at a QR code

[REMINDER] Output a <summary>...</summary> block at the VERY END of your response. This is mandatory."

**User prompt (verbatim):** "the thing that worked best before is like, the auto mode / cont mode which was scanning all the time, btw

[REMINDER] Output a <summary>...</summary> block at the VERY END of your response. This is mandatory."

**User prompt (verbatim):** "ok i scanned the code and it works now

[REMINDER] Output a <summary>...</summary> block at the VERY END of your response. This is mandatory."

**User prompt (verbatim):** "how did that happen?

[REMINDER] Output a <summary>...</summary> block at the VERY END of your response. This is mandatory."

**Assistant interpretation:** Reproduce the successful minimal trigger in the
full application, autonomously inspect raw UART and UI delivery through
`esp_console`, recover any persisted scanner-interface state, and preserve the
exact causal chain.

**Inferred user intent:** Return to reliable AUTO/continuous barcode scanning
with decoded content visible on the CoreS3, while understanding why a formerly
working scanner became completely silent on UART.

**Commit (code):** 766a4451 — "ESP-62: preserve idle-high TRIG during scanner power sequencing"

**Commit (code):** ea5d1d42 — "ESP-62: drive proven hardware trigger from scanner UI"

**Commit (code):** b8c6258b — "ESP-62: make full UI trigger match minimal probe exactly"

**Commit (code):** db10cdfd — "ESP-62: log every scanner UART receive chunk"

**Commit (code):** cea1ad44 — "ESP-62: show scanner UART recovery action on LCD"

### What I did

- Ported `0119`'s safe electrical ordering into `0118`: preload scanner power
  low and active-low TRIG high while high-impedance, configure the outputs,
  establish TRIG high, and enable scanner power last.
- Changed power-cycle behavior so TRIG remains idle-high across both power
  states rather than being pulled low whenever power is disabled.
- Added a nonblocking owner request for a 100 ms hardware-trigger pulse.
- Replaced the UI's silent `32 75 01` software-start action with the proven
  hardware pulse and then matched `0119` exactly by treating any screen click
  as a trigger. UI mode writes were removed from the diagnostic interaction.
- Added a hexdump for every UART receive chunk before quiet-time framing.
- Added `scripts/04-console-trigger-hold.py` so TRIG can be held low and
  released through two `esp_console` commands in one serial-owner session.
- Tested a 100 ms pulse and a five-second hold while the scanner was aimed at a
  QR code. Both activated the optics; neither produced one UART byte.
- Re-read the official `UsbMode.ino` example and user guide. The example says
  that an unknown prior output port must be recovered by scanning the Serial
  Communication configuration code; it cannot rely on the inactive UART.
- Downloaded and archived the official guide PDF, rendered page 9, and cropped
  the exact `21424000` Serial Communication programming barcode.
- Added an LCD recovery message for the state where firmware status receives
  no reply.
- After the user scanned `21424000`, ran `qr status` through `esp_console` and
  captured the complete transport/parser/UI path.

### Why

- Optical activity proves only that TRIG reaches the engine and starts a decode
  session. It does not establish where decoded data is routed.
- Communication-interface selection is persistent scanner state. If output is
  set to USB keyboard, USB virtual serial, or USB HID-POS, the M5-Bus TTL UART
  can remain completely silent even though power, trigger, imaging, and decode
  are functional.
- A UART command cannot be assumed to recover an interface that is no longer
  listening on UART. The programming barcode is interpreted inside the scan
  engine and is therefore independent of the host transport being recovered.

### What worked

- The full UI's screen tap reliably activated illumination through the
  hardware TRIG path without blocking the display task.
- The official Serial Communication programming barcode restored TTL serial
  operation immediately.
- The first post-recovery scan produced exact raw transport evidence:

  ```text
  I (...) qr_module: UART RX chunk: 10 bytes
  I (...) qr_module: ... |X0052L3WPN|
  I (...) qr_module: emit code: X0052L3WPN
  I (...) qr_ui: code: X0052L3WPN
  ```

- The first post-recovery status query produced:

  ```text
  getInfos id=0xc1 data_len=3 data_got=3 -> '1.0'
  qr firmware=1.0
  ```

- This is the first post-lifetime-fix capture that combines scanner bytes,
  quiet-time emission, the stable owner task, and UI queue delivery.

### What didn't work

- Safe TRIG/power sequencing alone did not restore UART.
- Replacing the UI software-start command with a hardware pulse did not restore
  UART.
- Treating every screen click as a trigger removed UI ambiguity but still
  produced no decoded bytes before interface recovery.
- Autonomous 100 ms trigger test:

  ```text
  qr trig pulse
  I (...) qr_module: TRIG pulse: LOW 100ms -> HIGH
  trig-pulse: ok
  ```

  No `UART RX chunk` followed.
- Autonomous five-second trigger test:

  ```text
  qr trig low
  I (...) qr_module: TRIG=LOW
  trig-low: ok
  qr trig high
  I (...) qr_module: TRIG=HIGH
  trig-high: ok
  ```

  No `UART RX chunk` followed.
- The first live monitor window was empty because the user's touch occurred
  before the no-reset capture attached. This was corrected by driving TRIG
  through `esp_console` instead of relying on synchronized human input.

### What I learned

- The earlier conclusion that `0119` “worked” was too broad. At that point it
  proved responsive touch and optical triggering, not decoded UART transport.
- The exact-known-working `c0787055` A/B test correctly ruled out a current
  firmware regression, but the remaining state was not necessarily physical.
  Persistent state inside the scanner survives host firmware changes and makes
  historical firmware A/B tests fail identically.
- A physical USB/UART selector and the scan engine's persisted communication
  interface are distinct controls. Correct switch position does not force the
  engine's configured output protocol back to serial.
- Factory reset is operationally hazardous in a UART-managed product. If it
  restores a USB interface, it removes the host's ability to repair the
  setting over its normal control path.
- AUTO/continuous behavior should be restored only after a positive firmware
  reply confirms that TTL serial control is available.

### What was tricky to build

- The visible symptoms crossed three independent state machines: expander
  power/TRIG, scanner decode-session state, and scanner communication-interface
  persistence. Final GPIO levels and visible illumination could both be
  correct while the selected output transport remained wrong.
- A route sweep and baud sweep were logically valid but could not find a
  response because they varied host UART parameters while the engine was
  routing output to USB. The tests were useful negative evidence, but they did
  not cover the persisted interface dimension.
- The recovery command `21 42 40 00` exists, but it is delivered over UART. It
  cannot repair a state in which UART is not the active communication
  interface. The official programming barcode bypasses that circular
  dependency.

### What warrants a second pair of eyes

- Determine whether `32 76 01` factory reset always restores a USB mode on this
  exact engine firmware or whether another command/user action selected USB.
  The temporal association is strong, but the session did not capture a
  before/after interface query around the reset.
- Guard or remove `qr reset`; at minimum require an explicit confirmation and
  display the recovery-barcode requirement before transmission.
- Verify AUTO and continuous ACKs after recovery and select one as the normal
  startup mode. The user prefers always-scanning operation.
- Confirm on the physical LCD that the post-recovery `qr_ui: code` update is
  visible and that continuous repeats are deduplicated appropriately.

### What should be done in the future

- On a responsive boot, configure RS232 output, decode lighting, and AUTO or
  continuous mode through the owner and require ACK success before claiming
  the scanner is ready.
- Keep touch-trigger behavior as a fallback, not the primary user path.
- Add an explicit interface-recovery section to both firmware READMEs and the
  long-form design document.
- Consider packaging the official recovery crop where an operator can display
  it on a phone or print it without opening the entire 75-page guide.

### Code review instructions

- Review `qr_module.cpp::begin()` and `setEnable()` for safe active-low TRIG
  ordering.
- Review `QRModule::requestHardwareTriggerPulse()` and the `PulseTrigger`
  owner case; all expander writes still occur in the owner path during UI use.
- Review `qr_module.cpp::ownerTask()` for raw RX logging followed by the
  existing quiet-time pump.
- Review `qr_ui.cpp` for the any-touch hardware trigger and recovery message.
- Reproduce recovery by first checking `qr status`, then scanning the official
  `21424000` barcode if no reply, then rerunning `qr status` and scanning a
  normal code.

### Technical details

- Trigger pulse evidence: `various/2026-08-23-uart-offline-trigger-pulse.txt`.
- Five-second trigger evidence:
  `various/2026-08-23-uart-offline-trigger-held-5s.txt`.
- End-to-end recovery evidence:
  `various/2026-08-23-serial-recovery-end-to-end-success.txt`.
- Official recovery page:
  `various/2026-08-23-serial-communication-recovery-page.png`.
- Cropped recovery barcode:
  `various/2026-08-23-serial-communication-recovery-barcode.png`.
- Official source:
  `sources/protocol-pdf/ZBarcode-Scanner-User-Guide-2.5-EN.pdf`, page 9.

## Step 15: Restore ACK-backed AUTO mode and clean continuous results

With serial communication recovered, the remaining product goal was the
original always-scanning experience. The first guarded startup still queried
the engine too early and skipped configuration. Extending engine boot time and
retrying the status query made firmware `1.0` reliable, allowing startup to
apply UART output, fill light, positioning light, and AUTO mode.

AUTO then exposed two protocol-cleanup defects. Firmware 1.0 returns an ACK for
the UART-interface write even though the official wrapper ignores it, so the
unconsumed bytes entered scan framing as a bogus code. On a later boot, delayed
configuration ACKs crossed command boundaries. The final transport matcher
searches mixed incoming traffic for the expected ACK, and continuous duplicate
scan frames are collapsed before reaching UI history.

### Prompt Context

**User prompt (verbatim):** "Continue.

[REMINDER] Output a <summary>...</summary> block at the VERY END of your response. This is mandatory."

**User prompt (verbatim):** "also it seems a bit tricky to get a read to register, it used to work a bit better, and the white light wasn't turning on earlier, which I guess is qr code reading?

[REMINDER] Output a <summary>...</summary> block at the VERY END of your response. This is mandatory."

**User prompt (verbatim):** "ok this scans like a motherwfucker, what did you change?

[REMINDER] Output a <summary>...</summary> block at the VERY END of your response. This is mandatory."

**User prompt (verbatim):** "cool, update the project report once you are done cleaning things up .

[REMINDER] Output a <summary>...</summary> block at the VERY END of your response. This is mandatory."

**Assistant interpretation:** Restore the preferred always-on scan behavior,
make illumination and AUTO mode deterministic, remove continuous-mode noise,
guard the dangerous reset path, validate on hardware, and update the durable
project report after cleanup.

**Inferred user intent:** Have the CoreS3 behave like a practical scanner:
power on, illuminate when needed, decode continuously without touch, show one
useful UI result per presented code, and avoid repeating the interface-loss
incident.

**Commit (code):** 69f35b58 — "ESP-62: restore ACK-backed AUTO startup and guard reset"

**Commit (code):** b4d2a493 — "ESP-62: wait for scanner command readiness before AUTO config"

**Commit (code):** 84e62850 — "ESP-62: consume UART-mode ACK and deduplicate AUTO repeats"

**Commit (code):** 05a1e910 — "ESP-62: match configuration ACKs within mixed UART traffic"

### What I did

- Changed startup to require a valid firmware reply before applying scanner
  policy.
- Added one second of engine boot time, matching the proven minimal probe, and
  up to three firmware-query attempts separated by 500 ms.
- After a valid reply, synchronously configured and logged results for serial
  output, fill light on decode, positioning light on decode, and AUTO mode.
- Passed `auto_ready` into the UI; the screen claims AUTO only when all required
  configuration results are `OK`.
- Kept any-touch hardware TRIG as a nonblocking fallback.
- Guarded `qr reset` behind the explicit phrase
  `qr reset CONFIRM-21424000`; an unconfirmed reset prints the persistent-USB
  risk and recovery barcode.
- Observed that `21 42 40 00` returns `22 42 40 00 00` on firmware 1.0 and
  changed `setModeUart()` to consume and validate that ACK.
- Replaced fixed-position ACK reads with an in-stream matcher that searches for
  the expected ACK among mixed asynchronous bytes for up to 500 ms.
- Added continuous-result deduplication: repeated identical frames remain
  suppressed while they arrive less than one second apart; the same value can
  be emitted again after a one-second absence.

### Why

- The scanner's optical subsystem becomes available before its command parser
  responds reliably. One early timeout must not permanently skip AUTO and
  lighting setup.
- AUTO mode creates one shared UART stream containing configuration ACKs and
  asynchronous decoded values. Command handling cannot assume that the next
  five bytes belong to the command just sent.
- Unconsumed ACKs are binary protocol frames, not barcodes. They must not enter
  the quiet-time text accumulator.
- Continuous scanning repeats a stationary symbol rapidly. The transport
  should remain observable, but UI history should represent presentations,
  not every engine repetition.

### What worked

- Final startup received firmware `1.0` on attempt 1/3.
- All four startup operations succeeded:

  ```text
  AUTO config: uart=ok fill=ok pos=ok mode=ok ready=1
  ```

- The user reported that scanning became substantially more responsive.
- The white fill light returned with decode-light configuration.
- A 20-second capture contained 16 raw UART chunks for the visible code but
  exactly one `emit code` and one `qr_ui: code`.
- The emitted value remained `X0052L3WPN`.
- No assertion, backtrace, reboot, abort, or watchdog appeared.
- `qr reset` without the confirmation phrase was refused and transmitted no
  reset command.

### What didn't work

- The first AUTO startup still used the original short boot delay. Its query at
  approximately 2.3 seconds after host boot received no bytes, while a later
  console status query returned `1.0`. AUTO configuration was therefore
  skipped correctly but undesirably.
- After first consuming the UART-mode ACK, one startup exposed delayed ACK
  crossing:

  ```text
  fill=timeout
  pos=ack-mismatch
  received: 22 62 41 02 00
  expected: 22 62 42 02 00
  ```

  The fill ACK arrived just after its 200 ms deadline and was consumed by the
  position-light transaction. The position ACK then reached the scan pump and
  emitted a bogus binary-derived value.
- Exact fixed-length reads were insufficient once AUTO scan bytes and delayed
  configuration replies shared the same stream.

### What I learned

- The white light is the fill illumination, and its absence was direct evidence
  that guarded startup had skipped configuration after an early query timeout.
- Firmware 1.0's observed UART-mode ACK is stronger evidence than the official
  wrapper's reply-less implementation. Local protocol handling should consume
  the actual device response.
- ACK parsing on a shared asynchronous UART requires synchronization by frame
  content, not by assuming byte position after a write.
- Deduplication belongs after a complete logical scan has been framed. Raw UART
  logging can still expose every engine repetition while application history
  receives one semantic result.

### What was tricky to build

- Increasing timeouts alone fixed the delayed fill ACK but would not protect
  against decoded text arriving before an ACK. The matcher therefore scans a
  byte stream for the exact expected sequence and records unmatched bytes only
  for diagnostics. Startup scan bytes may be discarded during configuration;
  once startup completes, the normal owner pump resumes.
- Deduplication must allow intentional rescanning of the same code. Updating
  `_last_seen_us` on every suppressed repeat means a continuously visible code
  stays suppressed, while removing it for at least one second permits the same
  text to emit again.

### What warrants a second pair of eyes

- The simple ACK matcher handles the current fixed frames but is not a complete
  multiplexer. If configuration becomes common during active scanning, route
  unmatched complete scan frames back into the accumulator instead of
  discarding them during transactions.
- Confirm the one-second same-code absence threshold with real operator motion
  and choose a product value rather than treating the diagnostic value as
  final UX policy.
- AUTO is now preferred and proven. Compare power/thermal behavior with
  continuous mode before offering both as persistent UI options.

### What should be done in the future

- Add a scanner-state banner that distinguishes AUTO ready, fallback trigger,
  and recovery required.
- Add host-side or component tests for delayed/stale ACKs and mixed scan bytes.
- Revalidate the optional H2 stack only after preserving this two-layer
  known-good baseline.

### Code review instructions

- Start in `app_main.cpp` at the firmware retry loop and `AUTO config` block.
- Inspect `qr_engine.cpp::sendCmd()` for bounded in-stream ACK matching and
  `setModeUart()` for the observed five-byte ACK.
- Inspect `qr_module.cpp::emit()` for the same-code suppression contract.
- Inspect `qr_console.cpp` for the exact factory-reset confirmation phrase.
- Validate by resetting with a code continuously visible and requiring:
  firmware `1.0`, four `ack ok` lines, `ready=1`, multiple raw chunks, one emit,
  and one UI result.

### Technical details

- Interleaved/delayed ACK failure:
  `various/2026-08-23-auto-ack-interleaving-failure.txt`.
- Final AUTO/dedup success:
  `various/2026-08-23-final-auto-deduplicated-success.txt`.
- Factory-reset refusal:
  `various/2026-08-23-factory-reset-guard.txt`.
