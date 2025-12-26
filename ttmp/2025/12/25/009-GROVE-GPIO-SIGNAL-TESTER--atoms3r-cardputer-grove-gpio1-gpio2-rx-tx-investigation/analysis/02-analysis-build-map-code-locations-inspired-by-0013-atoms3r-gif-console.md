---
Title: 'Analysis: Build map + code locations (inspired by 0013-atoms3r-gif-console)'
Ticket: 009-GROVE-GPIO-SIGNAL-TESTER
Status: active
Topics:
    - esp32s3
    - esp-idf
    - atoms3r
    - cardputer
    - gpio
    - uart
    - serial
    - usb-serial-jtag
    - debugging
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/0013-atoms3r-gif-console/main/Kconfig.projbuild
      Note: Reference for project-level Kconfig menus (console binding + debug toggles)
    - Path: esp32-s3-m5/0013-atoms3r-gif-console/main/console_repl.cpp
      Note: Reference implementation of esp_console REPL binding (USB Serial/JTAG) + command registration
    - Path: esp32-s3-m5/0013-atoms3r-gif-console/main/display_hal.cpp
      Note: AtomS3R display bring-up + present API to reuse for LCD status UI
    - Path: esp32-s3-m5/0013-atoms3r-gif-console/main/uart_rx_heartbeat.cpp
      Note: Reference pattern for non-invasive RX edge counting via GPIO ISR + periodic deltas
ExternalSources: []
Summary: 'Where to implement the GROVE GPIO1/GPIO2 signal tester in this repo: the best reference projects, the build system knobs, and the exact file/symbol locations to reuse (esp_console USB Serial/JTAG + LCD status UI + GPIO ISR edge counter).'
LastUpdated: 2025-12-24T19:07:20.724765899-05:00
WhatFor: Stop re-deriving how to structure/build the signal tester by pointing to concrete existing code (0013 + 0015) and the minimal set of new modules/components we’ll need.
WhenToUse: Read before starting implementation (Milestone B in tasks.md) or when onboarding someone new to the project who needs to find the right code patterns quickly.
---


## Goal

Identify the exact **code locations**, **symbols**, and **build/config patterns** needed to implement ticket `009-GROVE-GPIO-SIGNAL-TESTER` as an **AtomS3R-only firmware tool**, taking primary inspiration from `esp32-s3-m5/0013-atoms3r-gif-console` (clean modularity + `esp_console`).

We still reference `esp32-s3-m5/0015-cardputer-serial-terminal` as an optional *peer-device* reference (e.g., for how a Cardputer might drive GROVE UART or for general text UI patterns), but **we are not building Cardputer firmware as part of this ticket**.

This is a “where to look + how to build” map (not an implementation).

## Quick repo orientation (where this work lives)

- Ticket docs root (docmgr): `esp32-s3-m5/ttmp/`
- Ticket workspace:
  - `esp32-s3-m5/ttmp/2025/12/25/009-GROVE-GPIO-SIGNAL-TESTER--atoms3r-cardputer-grove-gpio1-gpio2-rx-tx-investigation/`
- Spec (what we’re building): `analysis/01-spec-grove-gpio-signal-tester.md`
- Tasks (what to do next): `tasks.md`

## Recommended implementation structure (AtomS3R-only)

Per scope clarification: implement **one** ESP-IDF project for AtomS3R, and treat the peer as external (Cardputer, USB↔UART dongle, or scope generator).

Suggested project directory name (example): `00xx-atoms3r-grove-gpio-signal-tester/`.

### Implementation status (now exists in-repo)

The AtomS3R project has been created as:

- `esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/`

High-signal entrypoints/modules:

- `main/hello_world_main.cpp` — init display/backlight/canvas, start UI + console, run event loop
- `main/console_repl.cpp` — USB Serial/JTAG `esp_console` commands → `CtrlEvent`
- `main/control_plane.{h,cpp}` — queue transport for `CtrlEvent`
- `main/signal_state.cpp` — single-writer state machine that applies GPIO TX/RX config
- `main/gpio_tx.cpp` — TX patterns (high/low/square/pulse) using `esp_timer`
- `main/gpio_rx.cpp` — RX edge counter via GPIO ISR (GPIO-owned mode)
- `main/lcd_ui.cpp` — LCD text status UI (mode/pin/tx/rx counters)

## “Inspiration anchor”: why `0013` is the right modular template

`esp32-s3-m5/0013-atoms3r-gif-console/` demonstrates a clean house style:

- a thin `app_main` orchestrator: `main/hello_world_main.cpp`
- small focused modules under `main/`:
  - `main/console_repl.{h,cpp}` — `esp_console` REPL + command registration
  - `main/control_plane.{h,cpp}` — ISR → queue event bus
  - `main/uart_rx_heartbeat.{h,cpp}` — RX edge counter instrumentation (Kconfig-gated)
  - `main/display_hal.{h,cpp}` + `main/backlight.{h,cpp}` — board display/backlight HAL
- reusable components are pulled in via CMake `EXTRA_COMPONENT_DIRS` rather than copy/paste

### Key build modularity mechanism: `EXTRA_COMPONENT_DIRS`

File: `esp32-s3-m5/0013-atoms3r-gif-console/CMakeLists.txt`

- Symbol/setting: `set(EXTRA_COMPONENT_DIRS ...)`
- It points at:
  - `../../M5Cardputer-UserDemo/components/M5GFX` (vendored M5GFX/LovyanGFX)
  - `../components` (repo-shared ESP-IDF components for `esp32-s3-m5/*`)

This is the main “keep each tutorial directory small, reuse shared code” mechanism to emulate.

## “We need this exact thing”: esp_console over USB Serial/JTAG

Primary reference: `esp32-s3-m5/0013-atoms3r-gif-console/main/console_repl.cpp`

### Symbols to reuse

- `void console_start(void)`
- `static void console_register_commands(void)`
- `static void console_start_usb_serial_jtag(void)`:
  - calls `esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl)`
  - then `esp_console_register_help_command()`
  - then registers project commands via `esp_console_cmd_register(...)`
  - then `esp_console_start_repl(repl)`

### Kconfig (project-level) structure to reuse

File: `esp32-s3-m5/0013-atoms3r-gif-console/main/Kconfig.projbuild`

- Menu: `Tutorial 0013: Console / esp_console`
- Choice: `TUTORIAL_0013_CONSOLE_BINDING_*`
  - `TUTORIAL_0013_CONSOLE_BINDING_USB_SERIAL_JTAG` depends on `ESP_CONSOLE_USB_SERIAL_JTAG`
  - `TUTORIAL_0013_CONSOLE_BINDING_GROVE_UART` depends on `ESP_CONSOLE_UART_CUSTOM`
- Note the help text explicitly reminds you the **ESP-IDF console component settings** still control what transports are compiled.

### `sdkconfig.defaults` knobs that matter

File: `esp32-s3-m5/0013-atoms3r-gif-console/sdkconfig.defaults`

- For USB Serial/JTAG REPL, you’ll typically want:
  - `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`
  - and likely ensure “Channel for console output” is consistent with your chosen control plane.

High-value caution (from existing docs in this repo):

- `esp_console_new_repl_usb_serial_jtag()` does a lot of init internally in ESP-IDF 5.x; avoid double-init.
  - See: `esp32-s3-m5/ttmp/.../008-ATOMS3R-GIF-CONSOLE.../playbooks/01-esp-console-repl-usb-serial-jtag-quickstart.md`

## GPIO RX edge counter (core of this ticket)

You effectively want a generalized version of `0013`’s “UART RX heartbeat”, but for selectable pin (GPIO1/GPIO2), selectable edge mode, and with UI.

### Minimal edge counter implementation pattern (already in-repo)

File: `esp32-s3-m5/0013-atoms3r-gif-console/main/uart_rx_heartbeat.cpp`

Key symbols/patterns:

- `gpio_set_intr_type(rx_gpio, GPIO_INTR_ANYEDGE)`
- `gpio_install_isr_service(0)` (tolerate `ESP_ERR_INVALID_STATE`)
- `gpio_isr_handler_add(rx_gpio, uart_rx_edge_isr, arg)`
- ISR uses atomics:
  - `__atomic_fetch_add(&s_uart_rx_edges, 1, __ATOMIC_RELAXED);`
- periodic task logs deltas:
  - `uart_rx_heartbeat_task()` prints `edges` and `d_edges` every `CONFIG_*_INTERVAL_MS`

Important design note in the header comment (relevant to our tester):

- If a UART peripheral “owns” RX, you must avoid stealing pin mux state.
- In `0013`, the heartbeat explicitly warns: **do not `gpio_config()` that pin** if UART is using it.

For the signal tester tool:

- In “pure GPIO RX mode” we *do* own the pin, so configuring it as input/pulls is fine.
- In any “UART mode” (optional Milestone E), follow `0013`’s non-invasive rule.

### Alternative (ISR → queue) reference

File: `esp32-s3-m5/0003-gpio-isr-queue-demo/main/hello_world_main.c`

This is the canonical “ISR posts event to a queue, task processes” pattern; it’s useful if we want RX events to feed the LCD UI without heavy atomic reads (or if we want timestamp bursts).

## LCD status UI (AtomS3R)

Primary reference for AtomS3R display bring-up: `esp32-s3-m5/0013-atoms3r-gif-console/main/display_hal.{h,cpp}` and `main/backlight.{h,cpp}`.

Generic “text UI on a canvas” pattern (useful even on AtomS3R) is demonstrated clearly in `esp32-s3-m5/0015-cardputer-serial-terminal/main/hello_world_main.cpp`:

- `m5gfx::M5GFX display; display.init();`
- `M5Canvas canvas(&display); canvas.createSprite(w, h);`
- On update:
  - `canvas.fillScreen(TFT_BLACK);`
  - `canvas.setTextColor(TFT_WHITE, TFT_BLACK);`
  - `canvas.setCursor(0, 0);`
  - `canvas.printf(...)` for multiple lines
  - `canvas.pushSprite(0, 0);`
  - optionally `display.waitDMA();` to avoid tearing

AtomS3R display bring-up API (from `0013`) exposes:

- `bool display_init_m5gfx(void)`
- `m5gfx::M5GFX &display_get(void)`
- `void display_present_canvas(M5Canvas &canvas)`

## Peer-device note: Cardputer GPIO1/GPIO2 conflict risk (still relevant, but not our firmware)

File: `esp32-s3-m5/0015-cardputer-serial-terminal/main/Kconfig.projbuild`

This contains an explicit warning about the Cardputer keyboard scanner:

- `TUTORIAL_0015_AUTODETECT_IN01` may probe/claim GPIO1/GPIO2 as alternate keyboard pins.
- If you try to use GROVE UART on GPIO1/GPIO2 while keyboard autodetect is active, you can get pin conflicts.

Implication for testing:

- If you use a Cardputer as the peer device, be aware Cardputer firmware may have **GPIO1/GPIO2 conflicts** (e.g. keyboard autodetect). That can create false negatives where Cardputer “TX” isn’t actually on the expected pin.

## Pin mapping sources (to avoid “label vs GPIO” confusion)

The recurring failure mode this ticket is addressing is “we think we’re testing GROVE G2 / GPIO2, but we might not be.”

High-signal in-repo references that anchor GROVE Port.A to GPIO numbers:

- `esp32-s3-m5/0013-atoms3r-gif-console/main/Kconfig.projbuild`
  - Help text explicitly states AtomS3R default: `G1=GPIO1`, `G2=GPIO2` (and reminds RX/TX is a convention).
- `esp32-s3-m5/0015-cardputer-serial-terminal/main/Kconfig.projbuild`
  - Help text explicitly states Cardputer GROVE labels: `G1 is typically GPIO1`, `G2 is typically GPIO2`.
- `esp32-s3-m5/ttmp/2025/12/21/003-ANALYZE-ATOMS3R-USERDEMO--.../reference/02-pinout-atoms3-atoms3r-project-relevant.md`
  - Shows AtomS3R “External I2C (Ex_I2C / Port.A)” uses GPIO `1` and `2` (SCL/SDA mapping), which matches the same physical connector pins used as “G1/G2”.

## How to build (what commands / what to copy)

The tutorials in this repo consistently build with ESP-IDF 5.4.1. Example commands (from `0013` and `0015` READMEs):

- `source /home/manuel/esp/esp-idf-5.4.1/export.sh`
- `idf.py set-target esp32s3`
- `idf.py build`
- `idf.py flash monitor`

### What to copy from existing projects (minimum viable skeleton)

When creating the new signal tester projects, the minimal “copy shape” from `0013`/`0015` is:

- Top-level:
  - `CMakeLists.txt` (with `EXTRA_COMPONENT_DIRS` like `0013`)
  - `README.md` (build/flash instructions)
  - `sdkconfig.defaults` (reproducible defaults; enable USB Serial/JTAG console transport)
- `main/`:
  - `CMakeLists.txt` (list `.cpp` sources, declare `PRIV_REQUIRES console driver` etc.)
  - `Kconfig.projbuild` (mode/pin options, debug toggles)
  - `hello_world_main.cpp` (thin orchestrator)
  - modules:
    - `console_repl.{h,cpp}` (register `mode`, `pin`, `tx`, `rx`, `status` commands)
    - `control_plane.{h,cpp}` (queue + event types)
    - `gpio_tx.{h,cpp}` (drive high/low/square/pulse)
    - `gpio_rx.{h,cpp}` (edge counter + pull config + stats snapshot)
    - `lcd_ui.{h,cpp}` (periodic render based on state + stats)
    - board display HAL: reuse pattern from `0013` (AtomS3R) vs `0015` (Cardputer)

## Existing repo docs worth reusing directly

These are “already written” guides that should be referenced while implementing the AtomS3R tester:

- `esp_console` quickstart (USB Serial/JTAG, init pitfalls):
  - `esp32-s3-m5/ttmp/2025/12/23/008-ATOMS3R-GIF-CONSOLE--.../playbooks/01-esp-console-repl-usb-serial-jtag-quickstart.md`
- Firmware organization house style (thin orchestrator + modules + Kconfig gating):
  - `esp32-s3-m5/ttmp/2025/12/23/008-ATOMS3R-GIF-CONSOLE--.../reference/04-firmware-organization-guidelines-esp-idf.md`
- UART console debugging mental model (useful when interpreting results):
  - `esp32-s3-m5/ttmp/2025/12/23/008-ATOMS3R-GIF-CONSOLE--.../analysis/03-uart-console-fundamentals-and-debugging-analysis.md`

## Where to look in ESP-IDF itself (when repo examples aren’t enough)

On ESP-IDF 5.4.x, these local sources are the authoritative “what does this helper really do?”:

- `$IDF_PATH/components/console/esp_console.h`
- `$IDF_PATH/components/console/esp_console.c`
- `$IDF_PATH/components/esp_vfs_console/` (stdio/VFS glue)
- `$IDF_PATH/components/esp_driver_usb_serial_jtag/` (USB Serial/JTAG VFS backend)
- `$IDF_PATH/components/esp_driver_uart/` (UART VFS backend)

## Next actions (to turn this analysis into implementation)

Tie-in to `tasks.md` milestones:

- Milestone A:
  - Lock the CLI contract in `analysis/01-spec-grove-gpio-signal-tester.md` (commands already drafted there).
  - Add a pin mapping reference table (AtomS3R vs Cardputer) and explicitly note Cardputer keyboard conflict risk.
- Milestone B:
  - Create the two projects (Option 1) using `0013` and `0015` as templates, following the module list above.
  - Ensure default control plane uses USB Serial/JTAG to avoid “debugging the thing being debugged”.

