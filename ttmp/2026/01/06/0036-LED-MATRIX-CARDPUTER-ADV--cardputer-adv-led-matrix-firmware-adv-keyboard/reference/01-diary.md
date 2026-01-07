---
Title: Diary
Ticket: 0036-LED-MATRIX-CARDPUTER-ADV
Status: active
Topics:
    - esp-idf
    - esp32s3
    - cardputer
    - console
    - usb-serial-jtag
    - keyboard
    - sensors
    - serial
    - debugging
    - flashing
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-06T19:33:00.521454858-05:00
WhatFor: ""
WhenToUse: ""
---

# Diary

## Goal

Track the work for ticket `0036-LED-MATRIX-CARDPUTER-ADV`, including decisions, commands run, and artifacts created, so the firmware can be developed incrementally (console → LED matrix → ADV keyboard) without losing context.

## Step 1: Create Ticket Workspace + Survey Console Patterns

This step sets up the docmgr workspace for the ticket, imports the “Cardputer vs Cardputer-ADV” notes, and writes an initial survey of how existing `esp32-s3-m5/` firmwares use `esp_console` over USB Serial/JTAG. It establishes the repo-consistent decision to prefer USB Serial/JTAG for interactive consoles on Cardputer/ADV projects (UART pins are frequently repurposed for peripherals/protocols).

No firmware code is written yet; the output is documentation and a staged task list that we can execute in small, verifiable increments.

### What I did
- Created the ticket workspace: `docmgr ticket create-ticket --ticket 0036-LED-MATRIX-CARDPUTER-ADV ...`
- Imported the comparison notes into ticket sources: `docmgr import file --ticket 0036-LED-MATRIX-CARDPUTER-ADV --file /tmp/cardputer-vs-cardputer-adv.md --name "Cardputer ADV comparison"`
- Added ticket documents:
  - `ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/design-doc/01-survey-existing-esp32-s3-m5-firmwares-esp-console-usb-serial-jtag.md`
  - `ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/reference/01-diary.md`
- Surveyed existing console patterns by grepping for `esp_console_new_repl_*`, `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG`, and `usb_serial_jtag` usage across the repo.
- Populated the ticket task list with phased tasks (console-driven LED matrix first, keyboard integration second).

### Why
- Establish a single “source of truth” ticket workspace and import the baseline hardware notes.
- Avoid re-solving the same console backend decisions by explicitly documenting existing, working patterns in this repo.
- Keep progress measurable by planning tasks that map directly to incremental hardware bring-up.

### What worked
- `docmgr` ticket creation and import worked as expected and updated the ticket index.
- The repo already contains multiple strong references for USB Serial/JTAG `esp_console` REPL startup (`0030`, `0025`, `0029`, `0013`), which we can reuse almost verbatim when the firmware implementation starts.

### What didn't work
- N/A (no blockers encountered in this step).

### What I learned
- In this repo, the common “production-ready” pattern is to compile-time select the REPL backend using `#if CONFIG_ESP_CONSOLE_*` guards (example: `0030`, `0029`) because the backend helpers are only compiled when the corresponding Kconfig option is enabled.
- Several firmwares also use USB Serial/JTAG without `esp_console` (manual read/write of the `usb_serial_jtag` driver), which is useful for tiny control planes but is not the default choice for this ticket.

### What was tricky to build
- Choosing a task breakdown that matches how hardware bring-up actually fails: pinout/orientation verification must come before higher-level features like fonts and keymaps.
- Avoiding accidental UART console assumptions; on Cardputer/ADV, UART is often not “free”.

### What warrants a second pair of eyes
- The Cardputer-ADV MAX7219 **chip select pin** choice: the M5 demo firmware uses `GPIO5` for LoRa NSS; verify the intended header/pinout so we don’t collide with on-board peripherals.
- The Cardputer-ADV keyboard I²C pin assumptions (often cited as `GPIO8/GPIO9`) should be confirmed against authoritative board docs or the demo firmware’s init path.

### What should be done in the future
- Execute Phase 0 tasks first: verify and document the real pinout/wiring for MAX7219 + TCA8418 on the specific Cardputer-ADV setup.
- Start the firmware implementation by cloning the `0030` / `0025` USB Serial/JTAG `esp_console` patterns into a new `0036` project directory and then add a minimal MAX7219 driver + REPL commands.

### Code review instructions
- Start at: `ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/design-doc/01-survey-existing-esp32-s3-m5-firmwares-esp-console-usb-serial-jtag.md`
- Confirm the tasks are sensible: `docmgr task list --ticket 0036-LED-MATRIX-CARDPUTER-ADV`
- Validate the ticket workspace: `docmgr doctor --ticket 0036-LED-MATRIX-CARDPUTER-ADV --stale-after 30`

### Technical details
- Imported notes live at: `ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/sources/local/01-cardputer-adv-comparison.md`
- Primary in-repo references for `esp_console` over USB Serial/JTAG:
  - `esp32-s3-m5/0030-cardputer-console-eventbus/main/app_main.cpp`
  - `esp32-s3-m5/0025-cardputer-lvgl-demo/main/console_repl.cpp`
  - `esp32-s3-m5/0029-mock-zigbee-http-hub/main/wifi_console.c`
  - `esp32-s3-m5/0013-atoms3r-gif-console/main/console_repl.cpp`

## Step 2: Add tmux Playbook + Build/Flash 0036 Phase-1 Firmware (4× MAX7219)

This step adds a ticket-local tmux playbook for reliable flash/monitor/REPL workflows and then implements the Phase-1 firmware: an `esp_console` REPL over USB Serial/JTAG that controls a **4-module chained MAX7219** (32×8 strip). It also captures “non-interactive validation” scripts because `idf.py monitor` requires a TTY and fails in automation/non-TTY contexts.

The result is a minimal bring-up harness: you can `matrix init`, run obvious order/orientation patterns, and set individual pixels across the 32×8 strip. This provides a stable base before we add the ADV keyboard (TCA8418) input path.

### What I did
- Added tmux playbook:
  - `ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/playbook/01-tmux-workflow-build-flash-monitor-esp-console-usb-serial-jtag.md`
- Created a new firmware project:
  - `esp32-s3-m5/0036-cardputer-adv-led-matrix-console/`
- Implemented MAX7219 driver with 4-device chain support + basic REPL commands (`matrix init/status/clear/test/intensity/row/row4/px/pattern/reverse`).
- Built and flashed to `/dev/ttyACM0` using USB Serial/JTAG.
- Saved the “scripts used during validation” into the ticket:
  - `ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/scripts/0036_read_boot_log.py`
  - `ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/scripts/0036_send_matrix_commands.py`

### Why
- tmux is the most reliable way to avoid serial-port locking conflicts while iterating quickly.
- For this harness environment, `idf.py monitor` cannot be used without a TTY; the scripts provide a reliable way to read logs and send `esp_console` commands when running non-interactively.
- The real hardware setup is 4 chained 8×8 modules; the firmware must support “chain order calibration” (`matrix reverse`) and per-module row writes (`matrix row4`).

### What worked
- `./build.sh build` and `./build.sh -p /dev/ttyACM0 flash` worked with ESP-IDF 5.4.1.
- Boot logs and the `matrix>` REPL prompt were successfully captured via `0036_read_boot_log.py`.
- The command sequence in `0036_send_matrix_commands.py` successfully exercised `matrix help`, `matrix init`, `matrix pattern ids`, and `matrix px` commands.

### What didn't work
- `idf.py monitor` fails in non-interactive contexts with: `Monitor requires standard input to be attached to TTY`.

### What I learned
- For test automation or “agent-run” workflows, `idf.py monitor` is not a reliable primitive; use pyserial-based scripts or run monitor in a real terminal/tmux session.
- MAX7219 chain order is easiest to calibrate with a per-module identifying pattern; a simple “reverse module order” switch removes a common source of confusion.

### What was tricky to build
- MAX7219 daisy-chain semantics: the first shifted bytes land on the furthest device, so per-device writes must reverse order at transmit time.
- Keeping “logical x=0..31” mapping usable even when the physical module chain order is reversed.

### What warrants a second pair of eyes
- Pin assumptions for `CS` on the intended Cardputer-ADV wiring (avoid collisions with other SPI devices).
- Whether `SPI2_HOST` is the best choice for the target board’s pin routing; if SPI bus conflicts emerge, we may need a configurable host/pin map.

### What should be done in the future
- Add `matrix rotate/flip` options once the physical module orientation is observed (many modules are rotated/flipped).
- Move on to Phase 2: bring up TCA8418 keyboard over I²C + INT, and map keypresses to LED matrix patterns/text.

### Code review instructions
- Start at firmware: `esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c`
- MAX7219 chain implementation: `esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/max7219.c`
- tmux workflow: `ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/playbook/01-tmux-workflow-build-flash-monitor-esp-console-usb-serial-jtag.md`
- Non-TTY validation scripts:
  - `ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/scripts/0036_read_boot_log.py`
  - `ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/scripts/0036_send_matrix_commands.py`

### Technical details
- Build: `cd esp32-s3-m5/0036-cardputer-adv-led-matrix-console && ./build.sh build`
- Flash: `cd esp32-s3-m5/0036-cardputer-adv-led-matrix-console && ./build.sh -p /dev/ttyACM0 flash`
- Boot log capture: `ttmp/.../scripts/0036_read_boot_log.py --port /dev/ttyACM0 --reset --seconds 3`
- REPL command send: `ttmp/.../scripts/0036_send_matrix_commands.py --port /dev/ttyACM0`

## Step 3: Slow SPI Clock + Add Blink Mode (Signal/Visibility Improvements)

This step improves “bring-up UX” when wiring is marginal or the visual patterns aren’t obvious. It slows the MAX7219 SPI clock to increase signal margin, and adds a continuous blink mode that toggles full-on/full-off with configurable pauses so you can confirm the strip is responding even if individual pixels look rotated or mirrored.

It also makes the bare `matrix` command print help (instead of returning a non-zero error code), reducing confusion when using `idf.py monitor`, which reports any non-zero return value as an error.

### What I did
- Reduced MAX7219 SPI clock default to `1 MHz`:
  - `esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/max7219.h`
  - `esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/max7219.c`
- Added `matrix blink on [on_ms] [off_ms]`, `matrix blink off`, `matrix blink status`:
  - `esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c`
- Made bare `matrix` print help and return success:
  - `esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c`
- Built and flashed to `/dev/ttyACM0`, then verified blink mode via the non-TTY serial sender.

### Why
- Slower SPI reduces ringing/edge-shape issues on longer wires and breadboards.
- A blinking “all on / all off” mode is a much stronger bring-up signal than static patterns when orientation/ordering is uncertain.
- Avoiding non-zero return codes for `matrix` alone prevents false-negative “ERROR” messages in `idf.py monitor`.

### What worked
- Flash/build succeeded; blink mode toggles the strip with configurable timing.

### What didn't work
- N/A

### What I learned
- Visibility and “strong” patterns matter for hardware bring-up: blink and full-test modes catch issues faster than subtle glyphs.

### What was tricky to build
- Ensuring the blink task doesn’t fight with interactive commands; the implementation stops blink automatically when running other matrix commands.

### What warrants a second pair of eyes
- Whether the chosen `1 MHz` SPI clock should be even lower for the specific wiring harness, or made configurable via a console command.

### What should be done in the future
- Add rotate/flip calibration options once the physical orientation is confirmed.

### Code review instructions
- SPI speed: `esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/max7219.c`
- Blink mode + command UX: `esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c`

## Step 4: Add Gamma-Mapped Brightness Command (Perceptual Ramp)

This step improves brightness control ergonomics by adding a “human-ish” brightness command on top of MAX7219’s 4-bit intensity register. MAX7219 only supports 16 intensity steps (`0..15`), and those steps are not perceptually linear, so the firmware now provides `matrix bright 0..100` which maps through a configurable gamma curve (default `2.2`) into the closest intensity step.

This doesn’t create more hardware steps, but it makes low-end brightness control much easier to dial in (where the difference between intensity 1 and 2 can feel large).

### What I did
- Added `matrix bright <0..100>` which gamma-maps to `matrix intensity 0..15`.
- Added `matrix gamma <1.0..3.0>` to tune the curve (default `2.2`).
- Updated help text and README brightness notes.

### Why
- MAX7219 “intensity” is a coarse, non-linear control; gamma mapping better matches perceived brightness changes.

### What worked
- Build succeeded; commands register and return useful feedback (`brightness`, mapped `intensity`, `gamma`).

### What didn't work
- N/A

### What I learned
- The “right” gamma depends on viewing conditions; making it configurable avoids hard-coding a single subjective choice.

### What was tricky to build
- Keeping it simple while acknowledging the hard 16-step limit (no flicker-inducing dithering in this step).

### What warrants a second pair of eyes
- Whether we want to add optional temporal dithering (for more apparent steps) later; it can introduce flicker.

### What should be done in the future
- If 16 steps are still too coarse, add an optional dithering mode (alternating intensity steps over time).

### Code review instructions
- Brightness/gamma commands: `esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c`

## Step 5: Identify “Which Chip Is This?” (MCU vs MAX7219/Clone)

This step clarifies what we can and can’t identify from software logs alone. The setup contains at least two relevant ICs: the ESP32-S3 MCU (which ESP-IDF tools can identify precisely), and the LED-matrix driver ICs (MAX7219-style) on the 8×8 modules, which are typically write-only and often clones with compatible behavior.

The practical outcome is: use ESP-IDF tooling (`espefuse.py`) to identify the MCU/module variant, and use physical inspection (chip marking/photo) to identify the LED driver IC family/clone. Don’t burn time trying to “detect” the MAX7219 clone from firmware without a readback path.

### What I did
- Recorded the recommended procedure to identify the MCU with ESP-IDF tools.
- Recorded the limitations for identifying MAX7219-vs-clone from firmware.

### Why
- Several MAX7219-style drivers exist (MAX7219/MAX7221 + multiple clones). They present the same register interface but can differ electrically.
- For debugging hardware issues (e.g. only 3 of 4 modules responding), it’s important to separate “MCU identity” from “LED driver identity”.

### What worked
- N/A (documentation-only step).

### What didn't work
- N/A

### What I learned
- MAX7219 chains are effectively write-only in this wiring (no MISO), so firmware can’t reliably fingerprint the LED driver IC.

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- If we later suspect a clone-specific electrical constraint, we should confirm by reading the top-marking on the IC or sourcing a known-good MAX7219/7221 module for A/B testing.

### What should be done in the future
- Capture a close-up photo of the 8×8 module IC top-marking (especially the “4th module” if it behaves differently).
- Run `espefuse.py` summary on the target board once the serial port is free (no monitor holding it).

### Code review instructions
- N/A (docs-only step).

### Technical details
- Identify MCU variant/efuses:
  - `source ~/esp/esp-idf-5.4.1/export.sh && espefuse.py -p /dev/ttyACM0 summary`
- Identify LED driver IC:
  - inspect the chip marking on the module (or take a close-up photo)
