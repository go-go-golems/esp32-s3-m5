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
      Note: User-facing usage notes
    - Path: 0036-cardputer-adv-led-matrix-console/main/matrix_console.c
      Note: |-
        Console commands for validation
        Matrix bring-up commands incl. safe test
    - Path: 0036-cardputer-adv-led-matrix-console/main/max7219.c
      Note: MAX7219 SPI init/transfer handling
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
