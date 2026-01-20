---
Title: Diary
Ticket: MO-031-ESP32C6-WS2811
Status: active
Topics:
    - esp-idf
    - esp32
    - gpio
    - tooling
    - tmux
    - flashing
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/0043-xiao-esp32c6-ws2811-4led-d1/main/Kconfig.projbuild
      Note: menuconfig parameters (GPIO
    - Path: esp32-s3-m5/0043-xiao-esp32c6-ws2811-4led-d1/main/main.c
      Note: WS2811 test pattern + startup logs
    - Path: esp32-s3-m5/0043-xiao-esp32c6-ws2811-4led-d1/main/ws281x_encoder.c
      Note: RMT encoder with configurable timings
    - Path: esp32-s3-m5/ttmp/2026/01/19/MO-031-ESP32C6-WS2811--esp32-c6-ws2811-drive-4-leds-on-d1/design-doc/01-ws2811-on-xiao-esp32c6-d1-wiring-firmware-plan.md
      Note: Wiring + rationale
    - Path: esp32-s3-m5/ttmp/2026/01/19/MO-031-ESP32C6-WS2811--esp32-c6-ws2811-drive-4-leds-on-d1/playbook/01-flash-monitor-ws2811-smoke-test.md
      Note: Smoke test steps
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-19T22:13:24.424428534-05:00
WhatFor: ""
WhenToUse: ""
---


# Diary

## Goal

Maintain a step-by-step implementation diary for MO-031 (XIAO ESP32C6 driving 4 WS2811 LEDs on D1), including wiring notes, exact commands, errors, and validation instructions.

## Step 1: Ticket Setup + Wiring/Plan Doc

Created the docmgr ticket workspace and wrote an initial design doc capturing the wiring, power, level shifting, and the firmware approach (RMT-based LED strip encoder). This establishes the “what/why” before we start coding and reduces the chance we burn time on avoidable hardware gotchas (missing common ground, marginal data voltage, etc.).

**Commit (code):** c6dd2a9 — "0043: rainbow pattern, default 10 LEDs"

### What I did
- Created the ticket: `docmgr ticket create-ticket --ticket MO-031-ESP32C6-WS2811 --title "ESP32-C6 WS2811: drive 4 LEDs on D1" --topics esp-idf,esp32,gpio,tooling,tmux,flashing`
- Added docs:
  - `docmgr doc add --ticket MO-031-ESP32C6-WS2811 --doc-type design-doc --title "WS2811 on XIAO ESP32C6 D1: wiring + firmware plan"`
  - `docmgr doc add --ticket MO-031-ESP32C6-WS2811 --doc-type playbook --title "flash/monitor + WS2811 smoke test"`
  - `docmgr doc add --ticket MO-031-ESP32C6-WS2811 --doc-type reference --title "Diary"`
- Added initial tasks via `docmgr task add`

### Why
- This ticket mixes hardware + firmware; documenting wiring and electrical constraints early prevents “it compiles but nothing lights” ambiguity.

### What worked
- Ticket workspace created under `ttmp/2026/01/19/MO-031-ESP32C6-WS2811--esp32-c6-ws2811-drive-4-leds-on-d1/`.

### What didn't work
- N/A

### What I learned
- N/A

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- Verify whether the strip’s data input needs a level shifter at 5V VDD for your specific WS2811 modules.

### What should be done in the future
- Once the hardware is wired, record the observed color order (GRB vs RGB) and any timing tweaks needed.

### Code review instructions
- Start with: `ttmp/2026/01/19/MO-031-ESP32C6-WS2811--esp32-c6-ws2811-drive-4-leds-on-d1/design-doc/01-ws2811-on-xiao-esp32c6-d1-wiring-firmware-plan.md`

### Technical details
- N/A

## Step 2: Implement WS2811 RMT Driver + Build

Implemented a minimal ESP-IDF firmware project for ESP32-C6 that drives a WS281x-style strip via the RMT TX peripheral, defaulting to XIAO ESP32C6 **D1 = GPIO1** and **4 LEDs**. The encoder is built from ESP-IDF primitives (bytes encoder + reset code), with timing parameters exposed in `menuconfig` so we can tune for the exact WS2811 variant if needed.

Built the firmware locally with `idf.py` to confirm it compiles and produces a flashable binary.

**Commit (code):** 0ef3fbd — "0043: add ESP32-C6 WS2811 4-LED driver (RMT)"  
**Commit (code):** 69b78bd — "0043: drop accidental datasheet"

### What I did
- Added firmware project: `0043-xiao-esp32c6-ws2811-4led-d1/`
- Implemented a configurable WS281x RMT encoder:
  - `main/ws281x_encoder.c`
  - `main/ws281x_encoder.h`
- Implemented a 4-LED “one-hot” chase pattern + startup logs:
  - `main/main.c`
- Verified build:
  - `source ~/esp/esp-idf-5.4.1/export.sh && cd 0043-xiao-esp32c6-ws2811-4led-d1 && idf.py set-target esp32c6 && idf.py build`

### Why
- RMT is the most reliable way to generate sub‑microsecond WS281x waveforms on ESP32-class chips without CPU busy-wait loops.
- Exposing timings in Kconfig makes it easy to handle real-world WS2811 timing tolerance differences.

### What worked
- `idf.py build` completed successfully for target `esp32c6`.

### What didn't work
- I accidentally staged a datasheet PDF that was present in the directory; removed it in a follow-up commit so the project stays source-only.

### What I learned
- Keeping `git add` scoped to specific files (not whole directories) avoids unintentionally committing local artifacts.

### What was tricky to build
- Converting nanosecond timing parameters into RMT ticks without accidentally producing 0-tick pulses (round-up logic).

### What warrants a second pair of eyes
- Default timing values (T0H/T0L/T1H/T1L/reset) may need adjustment for your exact WS2811 hardware; review the defaults against your strip’s datasheet if behavior is unstable.

### What should be done in the future
- Run the hardware smoke test and record the observed color order and any timing tweaks needed.

### Code review instructions
- Start with: `0043-xiao-esp32c6-ws2811-4led-d1/main/main.c`
- Review encoder math: `0043-xiao-esp32c6-ws2811-4led-d1/main/ws281x_encoder.c`
- Build: `source ~/esp/esp-idf-5.4.1/export.sh && cd 0043-xiao-esp32c6-ws2811-4led-d1 && idf.py set-target esp32c6 && idf.py build`

### Technical details
- N/A

## Step 3: Add Serial Progress Logs in Main Loop

Added a periodic `ESP_LOGI` line once per second during the main loop, so it’s easy to confirm in `idf.py monitor` that the firmware is running even if you can’t observe the D1 waveform with your current tools.

**Commit (code):** 86290dd — "0043: log loop progress over serial"

### What I did
- Logged `pos/led/phase` once per second from the pattern loop in `0043`.

### Why
- WS281x waveforms are fast and a multimeter/LED probe won’t show much; serial logs confirm liveness and that the loop is not stuck on an RMT call.

### What worked
- `idf.py build` succeeded after adding the periodic logging.

### What didn't work
- N/A

### What I learned
- ESP-IDF headers from other components (e.g. `esp_timer.h`) require adding that component to `idf_component_register(... PRIV_REQUIRES ...)`.

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- If you have a scope/logic analyzer, capture the D1 waveform to confirm timing and whether a level shifter is needed.

### Code review instructions
- Review: `0043-xiao-esp32c6-ws2811-4led-d1/main/main.c`

### Technical details
- N/A

## Step 4: Port Rainbow Pattern + Default to 10 LEDs

Ported the “rainbow” algorithm from the local inspiration file into the `0043` firmware, switching the default LED count to **10** and generating a full-strip rainbow that rotates over time. This makes it much easier to visually validate that all LEDs are being addressed, compared to the earlier single-pixel chase.

This change keeps the loop liveness logs, so you can still confirm the firmware isn’t hanging if the LEDs don’t light.

**Commit (code):** c6dd2a9 — "0043: rainbow pattern, default 10 LEDs"  
**Commit (docs):** f0cde3a — "MO-031 diary: Step 4 rainbow"

### What I did
- Read `0043-xiao-esp32c6-ws2811-4led-d1/inspiration/led_patterns.c` and ported the rainbow logic:
  - `hue_offset` advances over time based on `speed`
  - hue is distributed across the strip based on `spread`
- Updated defaults and added `menuconfig` knobs:
  - Default `LED count = 10`
  - Rainbow speed/saturation/spread controls

### Why
- A rainbow across the full strip makes “indexing works” obvious immediately.
- Making the count 10 matches the current hardware setup and reduces config friction.

### What worked
- `idf.py build` succeeded after the port.

### What didn't work
- N/A

### What I learned
- N/A

### What was tricky to build
- Keeping the rainbow math integer-safe and producing stable colors while still outputting GRB byte order.

### What warrants a second pair of eyes
- Verify the chosen defaults for `rainbow_spread_x10` match expectations (full 360° over the strip when set to 10).

### What should be done in the future
- N/A

### Code review instructions
- Review rainbow implementation: `0043-xiao-esp32c6-ws2811-4led-d1/main/main.c`
- Review config defaults: `0043-xiao-esp32c6-ws2811-4led-d1/main/Kconfig.projbuild`
- Build: `source ~/esp/esp-idf-5.4.1/export.sh && cd 0043-xiao-esp32c6-ws2811-4led-d1 && idf.py set-target esp32c6 && idf.py build`

### Technical details
- N/A

## Step 5: Faster + Smoother Rainbow + Intensity Modulation

Adjusted the animation loop to be significantly smoother by running at a configurable frame rate (default 16ms), and added a smooth sine-like intensity modulation so brightness “breathes” over time while the rainbow rotates. This makes the pattern more visually informative and helps catch wiring/timing issues quickly.

**Commit (code):** 852e8af — "0043: smoother rainbow + intensity modulation"

### What I did
- Added `menuconfig` knobs for:
  - animation frame time (`MO031_ANIM_FRAME_MS`)
  - intensity modulation min/period (`MO031_INTENSITY_MIN_PCT`, `MO031_INTENSITY_PERIOD_MS`)
- Implemented a small sine-easing function and used it to modulate intensity each frame.
- Made rainbow motion time-based so it’s smooth even if frame timing varies slightly.

### Why
- 250ms updates were “steppy” and made it harder to judge whether the signal was stable.
- Varying intensity helps confirm the full pipeline is working (data, timing, latch) and makes the pattern more obvious at low brightness.

### What worked
- `idf.py build` succeeded after the change.

### What didn't work
- N/A

### What I learned
- N/A

### What was tricky to build
- Keeping the intensity modulation stable and bounded while using integer math (avoid divide-by-zero, avoid overflow).

### What warrants a second pair of eyes
- The chosen default hue rotation rate may need tuning depending on taste; it’s currently tied to `rainbow_speed`.

### What should be done in the future
- N/A

### Code review instructions
- Review loop/timing/intensity changes: `0043-xiao-esp32c6-ws2811-4led-d1/main/main.c`
- Review Kconfig defaults: `0043-xiao-esp32c6-ws2811-4led-d1/main/Kconfig.projbuild`

### Technical details
- N/A

## Related

- `../design-doc/01-ws2811-on-xiao-esp32c6-d1-wiring-firmware-plan.md`
