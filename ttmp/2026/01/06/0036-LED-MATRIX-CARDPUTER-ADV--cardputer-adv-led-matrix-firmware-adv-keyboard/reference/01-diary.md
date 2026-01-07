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
RelatedFiles:
    - Path: 0036-cardputer-adv-led-matrix-console/README.md
      Note: |-
        User-facing usage notes
        Document matrix anim spin command
    - Path: 0036-cardputer-adv-led-matrix-console/main/matrix_console.c
      Note: |-
        Console commands for validation
        Matrix bring-up commands incl. safe test
        Add scroll text + default flipv
        Implement matrix anim spin (spin letters)
    - Path: 0036-cardputer-adv-led-matrix-console/main/max7219.c
      Note: MAX7219 SPI init/transfer handling
    - Path: 0036-cardputer-adv-led-matrix-console/main/tca8418.c
      Note: Minimal TCA8418 register helper over esp_driver_i2c
    - Path: 0036-cardputer-adv-led-matrix-console/main/tca8418.h
      Note: TCA8418 register/CFG/INT_STAT definitions + API
    - Path: ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/sources/local/Embedded text blinking patterns.md
      Note: Reference pseudocode for Animation 12 (spin letters)
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

**Commit (docs):** 609c343e9972844b32717900a4106b7982f91df1 — "docs(0036): add ticket workspace, playbook, diary"

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

**Commit (code):** b81b51d495ff3fc1c051069ccb7ba073c00bcb2d — "0036: add MAX7219 matrix console firmware"

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

## Step 6: Revert Bright/Gamma Mapping (Stability Baseline)

This step removes the `matrix bright` / `matrix gamma` commands and returns to the raw `matrix intensity 0..15` control. The goal is to eliminate any new moving parts while validating wiring and baseline reliability (especially while only 3 of 4 modules appear to respond).

### What I did
- Removed `matrix bright` and `matrix gamma` from the `matrix` command set.
- Removed the README section describing gamma-mapped brightness.
- Kept `matrix intensity` as the only brightness control.

### Why
- When the system feels electrically marginal/unreliable, reducing features helps isolate root causes.
- MAX7219 intensity is only 16 steps anyway; perceptual mapping can be reintroduced once the hardware path is stable.

### What worked
- Build succeeds after removing the brightness mapping code.

### What didn't work
- N/A

### What I learned
- Hardware bring-up benefits from “boring” baselines; defer ergonomic improvements until link stability is proven.

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- Whether the original instability was actually due to the brightness mapping changes or a coincidental wiring/port-lock issue.

### What should be done in the future
- Once the chain is stable (all 4 modules responding), re-add perceptual controls or add optional dithering as a separate step.

### Code review instructions
- Revert changes: `esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c`

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

## Step 7: Fix SPI Transfer Sizing + Add Safe Clock Control (Restore Reliability)

This step focuses on restoring a stable baseline after “it used to work, now it’s borked” reports and confusing scope captures. The main firmware-side issue was that the SPI bus was configured with an unrealistically small `max_transfer_sz`, which can cause truncated/garbled transfers once we send more than one MAX7219 frame (2 bytes per module) in a single transaction.

With transfer sizing corrected, this step also re-introduces a *safe* SPI clock control (`matrix spi [hz]`) and lowers the default SPI clock to improve edge quality on marginal wiring, without trying to free/reopen the SPI bus (which previously risked destabilizing the setup).

**Commit (code):** 98e0a2c68b2733fa2332bb8f627f04bc6b2893a5 — "0036: harden MAX7219 SPI and add clock control"

### What I did
- Fixed MAX7219 SPI bus configuration to allow multi-module transactions by setting `spi_bus_config_t.max_transfer_sz` to `2 * MAX7219_DEFAULT_CHAIN_LEN`.
- Lowered default MAX7219 SPI clock to `100 kHz` for better signal margin.
- Added `matrix spi [hz]` console command to read/set the SPI clock safely by removing/re-adding the device (no `spi_bus_free()`).
- Updated `matrix status` to print `spi_hz`.
- Built and flashed the firmware, and re-checked console bring-up via the non-TTY validation scripts.

### Why
- Chained MAX7219 updates are 8 bytes per row for 4 modules; the bus config must permit those transfers or behavior becomes unpredictable.
- A conservative SPI clock helps when wire lengths / breadboards / level shifting create slow edges or ringing.
- A REPL-accessible clock knob helps correlate scope observations with functional behavior without repeated reflashes.

### What worked
- `./build.sh build` succeeded.
- Flashing succeeded once `/dev/ttyACM0` was no longer locked.
- Boot shows `MAX7219 ready (auto-init)` and `matrix help` now includes `matrix spi`.

### What didn't work
- Flash initially failed because the serial port was already in use:
  - `./build.sh -p /dev/ttyACM0 flash`
  - Error: `Could not exclusively lock port /dev/ttyACM0: [Errno 11] Resource temporarily unavailable`

### What I learned
- Setting `spi_bus_config_t.max_transfer_sz` too low can produce “half works” symptoms that look electrical (odd edges, only some modules responding), even when the wiring is fine.
- A busy `/dev/ttyACM0` is easy to confuse with “broken flashing”; checking locks early saves time.

### What was tricky to build
- Re-adding a clock-control command without reintroducing the earlier “SPI reopen” complexity: the safer approach is to remove/re-add the device only, leaving the bus initialized.

### What warrants a second pair of eyes
- Whether `SPI_DMA_CH_AUTO` is necessary here; if instability persists, consider forcing `SPI_DMA_DISABLED` for these tiny (≤8 byte) transactions.
- Confirm the MAX7219 chain order assumption (“bytes[0] is closest to MCU”) matches the physical wiring, especially if module 4 remains unresponsive.

### What should be done in the future
- Validate with a scope after this change:
  - compare `matrix spi 100000` vs `matrix spi 1000000` and check edge shape + functional response.
- If only 3 of 4 modules still respond, treat it as a hardware chain/power/link issue (swap module order, inspect DIN/DOUT between module 3→4, check module 4 IC marking).

### Code review instructions
- Start at: `esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/max7219.c`
- Then: `esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c`
- Validate build: `cd esp32-s3-m5/0036-cardputer-adv-led-matrix-console && ./build.sh build`

### Technical details
- Reproduce the port-lock check:
  - `lsof /dev/ttyACM0`
  - `fuser /dev/ttyACM0`
- Quick REPL validation (non-TTY):
  - `python3 esp32-s3-m5/ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/scripts/0036_send_matrix_commands.py --port /dev/ttyACM0`

## Step 8: Add RAM-Based Safe Test + Confirm Root Cause Was Power

This step resolves the confusion where `matrix blink` looked stable but `matrix test on` produced “weird” or inconsistent behavior across modules. The key realization is that `matrix test on` switches the MAX7219 into DISPLAY_TEST mode, which behaves differently (and can stress the power path differently) than normal display-RAM updates used by blink and patterns.

To make bring-up less confusing and more repeatable, I added a RAM-based “safe test” command that turns the whole strip fully on/off using normal row writes (same mechanism as blink, but static). With that in place, we verified the underlying issue was the external power path (supply/ground/current capacity), not the firmware logic.

**Commit (code):** 125f6ab5edaa545902d27d0cf16abdc3f9f1aac8 — "0036: add RAM-based safe test command"

### What I did
- Added `matrix safe on|off` which fills the framebuffer with `0xFF`/`0x00` and flushes rows (no DISPLAY_TEST usage).
- Updated README to recommend `matrix safe` over `matrix test` for wiring bring-up.
- Used `matrix safe on` / `matrix safe off` alongside `matrix blink on` to validate consistent behavior.

### Why
- DISPLAY_TEST is a different operating mode than normal scanning and can lead to confusing “it works in blink but not test” symptoms.
- A RAM-based full-on/full-off command provides a clean, static test that matches normal updates and avoids mode changes.

### What worked
- `matrix safe on` behaves like the “on” half of blink, but steady.
- Once power was fixed, the earlier “weird test” behavior went away; the matrix behavior became consistent.

### What didn't work
- `matrix test on` was a poor bring-up tool under marginal power: it produced misleading patterns that looked like firmware timing/clock issues.

### What I learned
- For MAX7219 chains, “works in blink but not in display-test” is a strong hint to check power integrity (5V drop, ground reference, current limit), not just SPI.

### What was tricky to build
- Keeping the new command simple and obviously “same path as blink” (framebuffer + row flush), so it’s useful as a wiring/power diagnostic.

### What warrants a second pair of eyes
- N/A (small command addition).

### What should be done in the future
- When reproducing issues, start with:
  - `matrix safe on` / `matrix safe off`
  - then `matrix spi <hz>` sweeps if needed
  - reserve `matrix test on` only for verifying DISPLAY_TEST specifically.

### Code review instructions
- Command implementation: `esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c`
- README notes: `esp32-s3-m5/0036-cardputer-adv-led-matrix-console/README.md`

### Technical details
- Bring-up command sequence:
  - `matrix init`
  - `matrix intensity 4`
  - `matrix safe on`
  - `matrix safe off`

## Step 9: Add Frontmatter for Local “Embedded Text Blinking Patterns” Source

This step formalizes a useful local reference document (text rendering + animation pseudocode) by adding standard docmgr frontmatter. That makes it discoverable/searchable in the ticket workspace and clarifies its intended use as a reference source (not firmware code).

It also updates the ticket index so the source is listed under `ExternalSources`, keeping the workspace tidy as we accumulate supporting material for scrolling text and future keyboard-driven rendering.

**Commit (docs):** 4149a54b8864ee3763c3127dc2482e85730a5d65 — "docs(0036): add frontmatter for embedded text patterns source"

### What I did
- Added YAML frontmatter to:
  - `ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/sources/local/Embedded text blinking patterns.md`
- Linked the source from the ticket workspace index so it shows up under `ExternalSources`.

### Why
- Make the document searchable and clearly scoped as a local reference for later “text on matrix” work.

### What worked
- The source now has consistent metadata and is linked from the ticket index.

### What didn't work
- N/A

### What I learned
- Keeping sources “first-class” in the ticket workspace reduces future archeology when we revisit text rendering/animation ideas.

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- When we start scrolling/animation work, explicitly cite which parts of the source we actually implement and keep the firmware-side font assumptions aligned with hardware orientation flags.

### Code review instructions
- Frontmatter: `ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/sources/local/Embedded text blinking patterns.md`
- Index link: `ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/index.md`

### Technical details
- N/A

## Step 10: Add “matrix text” + Vertical Flip Option (First Text Rendering Bring-Up)

This step adds the first minimal text rendering path for the 4× 8×8 chain: a single `matrix text` command that renders exactly 4 characters (one character per module). This gives us an immediate way to confirm glyph rendering, module order, and orientation without implementing scrolling yet.

It also adds a `matrix flipv on|off` calibration knob because the most common bring-up failure for 8×8 modules is a vertical flip (e.g. `T` appears as an upside-down shape). The flip is implemented at the row-flush layer so it applies uniformly to all existing patterns/commands.

**Commit (code):** fa2ae1cb09a804d40198b2ca34278dcf73c87f15 — "0036: add matrix text + vertical flip"

### What I did
- Added a tiny 5×7 ASCII subset font (digits + A–Z) and a renderer to center it inside an 8×8 cell.
- Added `matrix text <ABCD>` and `matrix text <A> <B> <C> <D>` to render one char per module.
- Added `matrix flipv on|off` which flips the physical row mapping (fixes upside-down glyphs).
- Updated README to document the new commands.
- Built locally (`./build.sh build`). Flashing was skipped here because the port was in use; you can flash from your side.

### Why
- We need a “first text” command before attempting scrolling or keyboard integration; it makes orientation problems obvious and easy to correct.
- A vertical flip is a low-risk, high-value calibration flag for common MAX7219 module orientations.

### What worked
- Build succeeds with the new command set.
- `matrix flipv` applies globally since it sits in `fb_flush_row`.

### What didn't work
- Flash attempt failed while `/dev/ttyACM0` was locked by another process (expected in a multi-terminal setup).

### What I learned
- Flipping at the “row to DIGIT register” boundary is the cleanest way to correct vertical orientation without duplicating logic in every command.

### What was tricky to build
- Making sure `flipv` affects everything consistently (patterns, pixels, rows, text) without touching the SPI driver or introducing per-command conditionals.

### What warrants a second pair of eyes
- The 5×7 font bit convention (LSB=top row) vs the physical module orientation; if we later add `fliph`/rotate, we should verify the mapping with a photo and lock it down.

### What should be done in the future
- If text still looks “sideways”, add `matrix fliph on|off` or a `matrix rotate 0|90|180|270` abstraction (but only after we confirm the most common orientation on your specific module strip).
- Expand the font table or switch to a well-known public-domain 5×7 font once we’re confident about orientation.

### Code review instructions
- New command + orientation knob: `esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c`
- Command docs: `esp32-s3-m5/0036-cardputer-adv-led-matrix-console/README.md`

### Technical details
- Quick manual test after flashing:
  - `matrix init`
  - `matrix text TEST`
  - If upside-down: `matrix flipv on`

## Step 11: Default Vertical Flip + Add Smooth Scroll Command

This step turns `flipv` on by default (so text and patterns render in the “common” physical orientation without manual calibration) and adds the first scrolling-text animation. The goal is to move from static per-module glyphs (`matrix text`) to a smooth 1-pixel-per-frame scroll across the full 32×8 strip, while keeping everything controllable via `esp_console`.

The scroll implementation is intentionally minimal and “bring-up friendly”: it supports A–Z, 0–9, and space, scrolls at a configurable FPS, includes a small pause between cycles, and cleanly stops/restores the previous framebuffer when turned off.

**Commit (code):** 16981271bfb92c8c3b38f784c7fc38af77cc54e1 — "0036: default flipv and add smooth scroll"

### What I did
- Set `flipv` default to `on` so characters aren’t upside-down by default.
- Added `matrix scroll on <TEXT> [fps] [pause_ms]`, `matrix scroll off`, `matrix scroll status`.
- Implemented a column-based scroll renderer that reuses the existing framebuffer + `fb_flush_all()` path (so `flipv` and module-order handling still apply).
- Ensured other commands stop animations (`blink`/`scroll`) before changing the display, to avoid “fighting” updates.
- Built locally (`./build.sh build`).

### Why
- Manual calibration steps slow iteration; if the hardware is consistently upside-down, defaulting to `flipv on` reduces friction.
- Smooth scroll is the next incremental step toward “real” text output before we wire in the ADV keyboard events.

### What worked
- Build succeeded with the new scroll task + commands.
- Scrolling uses the same flush path as patterns/pixels, so orientation knobs apply consistently.

### What didn't work
- N/A

### What I learned
- Modeling the 32×8 display as 32 vertical “columns” is the simplest way to implement scrolling cleanly, then convert to the MAX7219 row-by-row representation at flush time.

### What was tricky to build
- Avoiding animation contention: a scroll task plus interactive commands requires a clear “stop animations before mutating” rule to keep output deterministic.

### What warrants a second pair of eyes
- Memory usage/lifecycle in `matrix scroll on/off` (heap allocation for the text column buffer) to ensure we don’t leak on repeated `scroll on` calls.

### What should be done in the future
- If we need punctuation/lowercase, expand the font table or switch to a broader font source once orientation is fully validated.
- Add optional “start/end hold when text fully visible” behavior if the loop pause isn’t sufficient.

### Code review instructions
- Scroll + flip default: `esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c`
- User-facing command notes: `esp32-s3-m5/0036-cardputer-adv-led-matrix-console/README.md`
- Build: `cd esp32-s3-m5/0036-cardputer-adv-led-matrix-console && ./build.sh build`

### Technical details
- After flashing:
  - `matrix init`
  - `matrix text TEST`  (should be right-side-up now; if not: `matrix flipv off`)
  - `matrix scroll on HELLO 15 250`
  - `matrix scroll off`

## Step 12: Make Chain Length Configurable (12 Modules / 96×8 and Beyond)

This step upgrades the firmware from a fixed “4 modules / 32×8” assumption to a configurable MAX7219 chain length, because the physical setup grew to 12 modules (96×8). The approach is intentionally conservative for embedded bring-up: we support up to a fixed maximum (16 modules) with static buffers, and use a runtime `matrix chain` command to tell the firmware how many modules are actually in the chain.

With the correct chain length set, all existing features (patterns, `px`, `text`, `scroll`, blink, safe test) automatically operate over the full width and stop doing confusing partial updates.

**Commit (code):** 1e11f2210d90fd9e3d3de2c03d0d2b6f593a0dfb — "0036: support configurable MAX7219 chain length"

### What I did
- Added `MAX7219_MAX_CHAIN_LEN` (16) and changed default chain length to 12.
- Added `matrix chain [n]` to get/set the active chain length at runtime.
- Updated framebuffer flush logic and patterns to respect `chain_len` instead of a hard-coded 4-module assumption.
- Updated SPI driver limits (`max_transfer_sz` and tx buffers) to support multi-module transactions up to the max chain length.
- Updated README to reflect 12-module default and variable-width commands.
- Built locally (`./build.sh build`).

### Why
- The hardware now uses 12 modules; the firmware must be able to address the full chain (96×8) without recompiling.
- A runtime knob avoids “mystery missing modules” when the physical chain is reconfigured.

### What worked
- Build succeeded; the firmware now has an explicit chain-length configuration path.

### What didn't work
- N/A

### What I learned
- Keeping a fixed maximum buffer size (instead of heap allocation) is a good trade-off for early bring-up: predictable memory, fewer failure modes, still flexible enough for common chain lengths.

### What was tricky to build
- Ensuring every command that iterates modules (`safe`, `row`, patterns, `onehot`, `scroll`) consistently uses the active chain length.

### What warrants a second pair of eyes
- Confirm the chosen `MAX7219_MAX_CHAIN_LEN=16` is sufficient for the expected “largest plausible” chain in this project; if not, bump it and re-verify stack/IRAM pressure.

### What should be done in the future
- Add `matrix rowm` support to scripts/validation flows when you start using per-module row patterns across >4 modules.

### Code review instructions
- Chain config + per-command loops: `esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c`
- SPI buffer sizing + chain max: `esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/max7219.c`
- Limits/defines: `esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/max7219.h`
- Docs: `esp32-s3-m5/0036-cardputer-adv-led-matrix-console/README.md`

### Technical details
- After flashing:
  - `matrix chain 12`
  - `matrix init`
  - `matrix safe on`
  - `matrix text HELLOWORLD12`  (renders first 12 chars)
  - `matrix scroll on HELLO 15 250`

## Step 13: Add Drop-Bounce + Wave Text Animations (REPL)

This step adds two additional text animations from the embedded animation notes: a “drop bounce” intro effect and a “wave” effect. Both are implemented as `esp_console` commands so we can iterate visually on the real 12-module (96×8) strip without rebuilding UI code or keyboard integration.

The implementation is deliberately simple and stable for bring-up: it reuses the same column-based text rasterization (5×7 font), keeps only one animation active at a time, and routes all rendering through the existing framebuffer + flush path so orientation (`flipv`) and module order (`reverse`) still apply.

**Commit (code):** 8cc47ffc71da919e930c74e93c4ffed41af5dc4c — "0036: add drop-bounce and wave text animations"

### What I did
- Added `matrix anim drop <TEXT> [fps] [pause_ms]` (sequential drop-in with small bounce).
- Added `matrix anim wave <TEXT> [fps]` (per-character sine-like vertical offsets).
- Added `matrix anim off` / `matrix anim status`.
- Added docmgr tasks for drop/wave; marked implementation tasks complete and left on-device validation as a separate task.
- Built locally (`./build.sh build`).

### Why
- These effects are useful “UX primitives” for later keyboard-driven text output (keypress → animate into place, status messages → wave attention).
- Implementing them as console-driven animations keeps bring-up iteration fast and reduces the risk of adding unrelated complexity.

### What worked
- Build succeeded.
- Animations reuse the existing framebuffer flush path, so they work across variable chain lengths and respect `flipv`/`reverse`.

### What didn't work
- N/A

### What I learned
- Treating the display as `width` columns (where `width = 8 * chain_len`) is still the cleanest abstraction as the strip grows; higher-level effects just become different ways to fill the column buffer.

### What was tricky to build
- Making sure only one animation runs at a time and that interactive commands reliably stop animation tasks before writing the display.

### What warrants a second pair of eyes
- The drop-bounce curve is a small hard-coded sequence; if you want it to feel “heavier” or “snappier”, we should tune the sequence or move to a small fixed-point easing function.

### What should be done in the future
- Add punctuation support in the font (or a wider font table) once we confirm the final physical orientation and desired glyph style.
- If we want mixed-case, add a lowercase table or a case-folding policy.

### Code review instructions
- Animation task + command parsing: `esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c`
- Updated command list: `esp32-s3-m5/0036-cardputer-adv-led-matrix-console/README.md`
- Tasks bookkeeping: `ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/tasks.md`

### Technical details
- Example usage after flashing:
  - `matrix chain 12`
  - `matrix init`
  - `matrix anim drop HELLO 15 250`
  - `matrix anim wave HELLO 15`
  - `matrix anim off`

## Step 14: Add Scroll-Wave + Flipboard Animations

This step adds two more “text motion” primitives: a wave-modulated scroll (so the scrolling text also ripples vertically), and a flipboard/split-flap style animation that flips between a list of centered words. These two effects cover common UI cases: long messages that need motion to draw attention, and short “status words” that should transition cleanly.

Both features are exposed via `esp_console` commands and intentionally reuse the same column-rendering + framebuffer flush path, so they automatically respect `chain_len`, `reverse`, and `flipv`.

**Commit (code):** 0d8cc04b32335bd2b11f663e6fa5847fb989d7ae — "0036: add scroll wave and flipboard"

### What I did
- Added `matrix scroll wave <TEXT> [fps] [pause_ms]` (scroll + per-character wave Y offsets).
- Added flipboard animation: `matrix anim flip <A|B|C...> [fps] [hold_ms]`.
- Added ticket tasks for scroll-wave + flipboard and marked implementation tasks complete (validation remains pending).
- Built locally (`./build.sh build`).

### Why
- Scroll-wave is a stronger visual cue than plain scrolling for “notice me” messages.
- Flipboard is a nice low-bandwidth way to cycle through a few short status words on the strip.

### What worked
- Build succeeded.
- Both effects operate over the configured chain width and share the same flush path as other patterns/commands.

### What didn't work
- N/A

### What I learned
- Keeping all “effects” as transformations over a column buffer pays off: scroll-wave becomes “scroll + shift columns”, and flipboard becomes “render + scale Y”.

### What was tricky to build
- Ensuring animation modes don’t fight each other: `stop_animations()` still serves as the single rule before any command mutates the display.

### What warrants a second pair of eyes
- Flipboard scaling math around the squash midpoint (very small scales): if you see flicker/odd lines, we may want to clamp the minimum scale more aggressively.

### What should be done in the future
- Validate on-device for subjective motion quality and tweak defaults (`fps`, `hold_ms`, flip duration) based on what looks best on your physical 12-module strip.

### Code review instructions
- Scroll-wave: `esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c`
- Flipboard: `esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c`
- Commands list: `esp32-s3-m5/0036-cardputer-adv-led-matrix-console/README.md`
- Task bookkeeping: `ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/tasks.md`

### Technical details
- Example usage after flashing:
  - `matrix chain 12`
  - `matrix init`
  - `matrix scroll wave HELLO 15 250`
  - `matrix anim flip COOL|NICE|OK 20 750`
  - `matrix anim off`

## Step 15: Fix Flipboard Timing (Hold Each Word, Including First)

This step fixes an off-by-design issue in the initial flipboard implementation: it would immediately start flipping the first word (no initial hold), and the “hold” period was associated with the *next* word rather than the current one. Visually, that felt like “COOL flips immediately” and the sequence could appear to get stuck on a later word depending on hold duration and how you were watching it.

The corrected behavior is: hold the current word for `hold_ms`, then flip to the next word; repeat forever, including flipping from the last word back to the first.

**Commit (code):** 1e715a0c63a2a3222e9794b1d08a7ff0c0bbf76f — "0036: fix flipboard hold/loop timing"

### What I did
- Changed the flipboard segment order to `hold -> flip` (instead of `flip -> hold(next)`).
- Kept the same flip duration (12 frames) and scaling curve; only the time partitioning changed.
- Built locally (`./build.sh build`).

### Why
- A flipboard reads like a “split flap”: you expect to see each word fully before it flips away.

### What worked
- Build succeeded.
- The animation now holds the first word and cycles cleanly through all entries.

### What didn't work
- N/A

### What I learned
- For word-cycling animations, “segment semantics” matter more than the easing curve: holding the wrong text makes the sequence feel broken even if the flip itself looks fine.

### What was tricky to build
- N/A (small logic change).

### What warrants a second pair of eyes
- Confirm the 12-frame flip duration is visually smooth at the chosen `fps` (e.g. at 20fps it’s a fast 0.6s transition).

### What should be done in the future
- If you want the flip to feel more “mechanical”, we can add a brief 1–2 frame fully-blank midline or random “noise” during the squash.

### Code review instructions
- Flipboard segment math: `esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c`

### Technical details
- After flashing:
  - `matrix anim flip COOL|NICE|OK 20 750`

## Step 16: Stop Other Animations When Switching Modes

This step fixes a practical UX issue: after adding more animation modes (`scroll`, `flipboard`, `drop`, `wave`, `blink`), it was possible to start a new mode without reliably stopping the old one. The most visible symptom was “scroll” and “flip” fighting each other when switching between them.

The fix is simple and makes the console predictable: any command that starts an animation-like mode now calls `stop_animations()` first (which disables all other animation tasks and restores the previous framebuffer snapshot).

**Commit (code):** 640c3fc687d2cfdfcf97168b2eb8ecfecb58aaec — "0036: stop other animations when switching modes"

### What I did
- Updated `matrix blink on ...` to call `stop_animations()` before enabling the blink task.
- Updated `matrix scroll on|wave ...` to call `stop_animations()` (instead of only disabling blink).
- Added a validation task to explicitly test mode switching on real hardware.

### Why
- Without a single rule (“starting a mode stops all others”), concurrent tasks can write the framebuffer alternately and the strip looks unstable.

### What worked
- Switching between `matrix anim flip ...` and `matrix scroll on|wave ...` no longer leaves both tasks enabled.

### What didn't work
- N/A

### What I learned
- As the REPL grows, “mode switching semantics” matter as much as the animations themselves; a small oversight in one command handler is enough to make the system feel broken.

### What was tricky to build
- N/A (small command-level fix).

### What warrants a second pair of eyes
- The current `stop_animations()` restores a saved framebuffer and flushes it; if the on-device transition flash is annoying, we may want a variant that stops tasks without restoring state when switching modes.

### What should be done in the future
- Run through a quick “mode switching matrix” on-device (scroll ↔ flip, scroll ↔ drop, flip ↔ drop, blink ↔ anything) and check the new validation task when confirmed.

### Code review instructions
- Command handlers and `stop_animations()`: `esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c`
- Tasks bookkeeping: `ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/tasks.md`

### Technical details
- Repro (previously problematic):
  - `matrix anim flip COOL|NICE|OK 20 750`
  - `matrix scroll on HELLO 15 250`
  - `matrix scroll off`

## Step 17: Keyboard Input (TCA8418) -> Typed Text on Matrix

This step brings up the Cardputer-ADV keyboard input path (TCA8418 over I²C + INT) inside the 0036 firmware. The goal is a tight debug loop: pressing keys should produce a raw event log line on the console and also update the LED matrix with the last `N` typed characters (where `N = chain_len`).

To keep the project small and easy to reason about, I implemented a minimal register-level TCA8418 helper (based on the vendor demo’s Adafruit-derived init sequence), then wired an ISR+queue+task loop that drains the TCA event FIFO and maps it into Cardputer-style `(row,col)` positions and printable characters (shift/caps/backspace).

**Commit (code):** cda10a977d04ee81a3cf4b88422ad380f4a61fc2 — "0036: add ADV keyboard input and typed text display"

### What I did
- Added a minimal `tca8418` register helper using ESP-IDF’s `esp_driver_i2c` master API.
- Initialized I²C on `GPIO8/9` and attached the keyboard `INT` on `GPIO11` (ISR enqueues a token; task drains FIFO).
- Added a `kbd` REPL command (`kbd status|on|off|clear|log`) and enabled keyboard bring-up automatically on boot.
- Implemented a small typed buffer (last ~128 chars) and render the last `chain_len` chars onto the strip; special keys:
  - `del` → backspace
  - `enter` → clear
  - shift/caps lock for letters

### Why
- We need a stable Phase-2 base where keyboard events are observable (raw logs) and have an immediate visual feedback path (matrix render) before we take on more complex “keyboard drives animations” behaviors.

### What worked
- Local build succeeded (`./build.sh build`).
- Keyboard event processing is decoupled from the REPL (ISR→queue→task), so key scanning doesn’t depend on console input.

### What didn't work
- N/A (needs on-device validation for press/release polarity and full key coverage).

### What I learned
- The vendor remap from TCA8418’s internal key numbering to the “Cardputer picture” coordinates is a useful abstraction layer: once you’re in `(row,col)` space, mapping to characters is straightforward.

### What was tricky to build
- Avoiding animation “fighting”: keyboard rendering uses `stop_animations()` before writing the framebuffer so keypresses always win.
- Using the newer `esp_driver_i2c` APIs (bus+device handles) instead of the legacy `driver/i2c.h` API.

### What warrants a second pair of eyes
- Confirm the TCA8418 event bit semantics on real hardware: this code assumes `bit7=1` means “pressed” (matching the vendor firmware), but this should be verified with real press/release logs.

### What should be done in the future
- Validate on device and check the new validation task; adjust event polarity if needed.
- Expand the 5×7 font table if we want punctuation to visibly render (right now non A–Z/0–9/space often appears blank).

### Code review instructions
- Keyboard bring-up + mapping + console command: `esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c`
- TCA8418 register helper: `esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/tca8418.c`
- Usage notes: `esp32-s3-m5/0036-cardputer-adv-led-matrix-console/README.md`
- Tasks bookkeeping: `ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/tasks.md`

### Technical details
- Expected wiring:
  - Keyboard: `SDA=GPIO8`, `SCL=GPIO9`, `INT=GPIO11`, address `0x34`
- Quick smoke test after flashing:
  - `matrix chain 12`
  - `matrix init`
  - Type on the Cardputer-ADV keyboard; look for `kbd: evt=...` logs and the last 12 chars on the strip.

## Step 18: Fix I2C Init Failure (Disable Async Mode)

This step fixes a keyboard bring-up regression on-device: the new `esp_driver_i2c` bus was configured with a non-zero `trans_queue_depth`, which enables ESP-IDF’s I²C “async transaction” mode. During the TCA8418 register-init sequence, that async mode quickly filled its internal ops list and hard-failed the init (`ops list is full` / `ESP_ERR_INVALID_STATE`).

The fix is to keep the bus in synchronous mode by setting `trans_queue_depth = 0` for our use case (simple, sequential register reads/writes). This removes the warning spam and prevents the queue overflow during init.

**Commit (code):** e243cc62dd2b77a926f5983648bb8006f938d265 — "0036: fix TCA8418 init by disabling I2C async mode"

### What I did
- Changed the I²C master bus config to `trans_queue_depth = 0` so the driver uses synchronous transactions.
- Rebuilt locally (`./build.sh build`).

### Why
- We don’t need async I²C for keyboard bring-up; we need reliability and clear errors.

### What worked
- Local build succeeded.

### What didn't work
- Previously observed failure on hardware:
  - `E i2c.master: ops list is full, please increase your trans_queue_depth`
  - `W kbd: keyboard init failed: ESP_ERR_INVALID_STATE`

### What I learned
- In ESP-IDF 5.4, setting `trans_queue_depth` enables async mode; it’s not just “queue sizing”. For simple polled devices, keeping it synchronous avoids surprising failure modes.

### What was tricky to build
- N/A (configuration fix).

### What warrants a second pair of eyes
- Verify on-device that TCA8418 init succeeds and keypress logging works with the synchronous bus config.

### What should be done in the future
- If we ever want async mode, wrap the TCA init writes into fewer transactions or explicitly size the queue and add backpressure.

### Code review instructions
- I²C bus config for keyboard: `esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c`

### Technical details
- After flashing:
  - Expect the earlier async warning and ops-list errors to be gone.
  - `kbd status` should report `ready=yes` if wiring is correct.

## Step 19: Feed Typed Text Into Active Text Animations (No Mode Fighting)

This step changes the keyboard UX from “keypresses always stop animations and render directly” to “keypresses update the currently active text mode”. In practice, that means: if you are running `matrix scroll ...` or `matrix anim ...`, typing updates the animation’s text live (and pressing Enter clears the typed buffer). Only when no text mode is active do we fall back to rendering the last `chain_len` characters directly on the strip.

To make that safe, I also had to address an underlying concurrency hazard: the scroll mode stores its rendered column stream in a heap buffer (`s_scroll_text_cols`) while the scroll task is reading it. Updating that pointer from the keyboard task can cause a use-after-free unless access is serialized. The solution here is a simple mutex around the scroll text buffer and its length during both update and per-frame readout.

**Commit (code):** 0c22e33019ce25ad73a1c18e95d7926a16d63523 — "0036: feed typed text into active scroll/anim modes"

### What I did
- Refactored keyboard rendering:
  - Keyboard no longer calls `stop_animations()` on every keypress.
  - Keypresses call a new “apply” function that updates the active mode’s text (scroll/anim) or renders directly if no mode is active.
  - `enter` clears the buffer and pushes an empty/blank text update.
- Made scroll text updates safe:
  - Added a mutex around `s_scroll_text_cols` / `s_scroll_text_w`.
  - Scroll task holds the mutex while it reads the current text buffer for a frame; updates hold it while swapping/freeing.
  - Added a `restart` hint so scroll restarts cleanly when the text changes.
- Added a small “restart” hook for text animations so they can restart from frame 0 when the typed text changes.
- Updated docs and checked off the new “feed typed text into active mode” task.

### Why
- It’s much more useful if the keyboard is an input source for the currently running presentation mode (scroll, wave, drop, flipboard) rather than forcibly cancelling it.
- Without the scroll-buffer mutex, live text updates can crash or corrupt the display due to a race between the scroll task and the keyboard task.

### What worked
- Local build succeeded (`./build.sh build`).

### What didn't work
- N/A (still needs on-device validation for “feel” and to ensure there’s no flicker/fighting when typing quickly).

### What I learned
- The moment you make animation parameters live-updatable, you need to treat animation state like shared mutable data: either make it lock-free by design (static buffers, single writer) or add explicit synchronization.

### What was tricky to build
- Balancing correctness vs. simplicity: the scroll task reads the buffer every frame, so the mutex needs to be held only briefly and predictably (not around delays), otherwise mode switching becomes sluggish.

### What warrants a second pair of eyes
- The scroll mutex + “restart on notify” logic: ensure there is no deadlock or long lock hold that could delay REPL commands or other tasks.

### What should be done in the future
- On-device validation:
  - Start `matrix scroll on ...` and type; confirm the message updates live and scroll doesn’t “fight” or crash.
  - Start `matrix anim drop ...` / `matrix anim wave ...` and type; confirm updates are visible and Enter clears.

### Code review instructions
- Keyboard → active mode routing: `esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c`
- Updated keyboard usage notes: `esp32-s3-m5/0036-cardputer-adv-led-matrix-console/README.md`
- Tasks bookkeeping: `ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/tasks.md`

### Technical details
- Example on-device scenario:
  - `matrix init`
  - `matrix scroll on HELLO 15 250`
  - Type `WORLD` → the scroller should update to include the typed string (sanitized to supported glyphs).
  - Press Enter → buffer clears (display becomes blank/space in the active mode).

## Step 20: Add Spin Letters Text Animation (Per-Character Card Flip)

This step adds a new text animation mode that makes each character “spin” around its vertical axis like a flipping card. The effect is implemented as a per-character **horizontal scaling** curve driven by `abs(cos(θ))`, staggered so letters begin spinning one after another, then a shared “spin out” phase collapses all letters together before an optional blank gap.

This is intentionally kept simple and deterministic: there is no “back face” rendering (we treat negative scale as the same as positive), and the renderer stays within each glyph’s 5-column cell so the animation remains readable on a low-resolution 8×8-per-module strip.

**Commit (code):** 6a671a8bac2ca4896bbf73aed5972f4251d32cd8 — "0036: add spin letters text animation"

### What I did
- Implemented `matrix anim spin <TEXT> [fps] [pause_ms]` in the matrix REPL.
- Added a horizontally-scaled text renderer:
  - `col_scale_x(...)` to map 5-column glyphs by a Q8 scale factor.
  - `render_text_centered_cols_scaled_x(...)` to render centered text while applying per-character scaling.
- Implemented the spin animation state machine in `text_anim_task()`:
  - Staggered per-character spin-in timing.
  - Full-size hold window.
  - Shared spin-out collapse + optional blank gap based on `pause_ms`.
- Updated the firmware README to document the new command.

### Why
- Spin/flip is a very “matrix-appropriate” way to animate short text without needing more vertical pixels or complex per-pixel effects.
- It complements the existing `drop` and `wave` modes, and is suitable for keyboard-driven “live text” since it supports immediate text replacement + restart.

### What worked
- Local build succeeded: `./build.sh build`.

### What didn't work
- N/A (not yet validated on-device for “feel” and legibility on the real MAX7219 chain).

### What I learned
- For 5-column glyphs, you can get a convincing card-flip illusion just by blanking the outer columns as the scale shrinks; you don’t necessarily need to remap columns across character boundaries.

### What was tricky to build
- Getting the cycle timing right: the exit phase should start after the last character has had time to spin-in and then hold, without an off-by-one “dead” delay.
- Balancing readability vs. effect: very small scale factors quickly make letters vanish; the animation needs a threshold (`< ~12.5% width`) to avoid noisy flicker.

### What warrants a second pair of eyes
- The float math in the animation task (`cosf`, `fabsf`) and its per-frame CPU cost; if we see missed frames on-device, we should consider a LUT-based cosine.

### What should be done in the future
- On-device validation:
  - `matrix anim spin HELLO 15 250` on a long chain (12 modules / 96×8) and confirm the effect is readable.
  - Type while spin mode is active and verify the animation restarts cleanly with the new text (Enter clears).

### Code review instructions
- Animation implementation: `esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c`
- Command docs: `esp32-s3-m5/0036-cardputer-adv-led-matrix-console/README.md`
- Task bookkeeping: `ttmp/2026/01/06/0036-LED-MATRIX-CARDPUTER-ADV--cardputer-adv-led-matrix-firmware-adv-keyboard/tasks.md`

### Technical details
- Command shape:
  - `matrix anim spin <TEXT> [fps] [pause_ms]`
  - `pause_ms` is used as the blank “gap” between cycles; the in-place hold window is a fixed number of frames.
