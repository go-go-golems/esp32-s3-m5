---
Title: Diary
Ticket: 001-FAN-CONTROL
Status: active
Topics:
    - esp-idf
    - esp32s3
    - cardputer
    - console
    - usb-serial-jtag
    - gpio
    - animation
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0036-cardputer-adv-led-matrix-console/main/matrix_console.c
      Note: Reference pattern for esp_console + task-based animations
    - Path: 0037-cardputer-adv-fan-control-console/README.md
      Note: User-facing command list and build/flash notes
    - Path: 0037-cardputer-adv-fan-control-console/main/fan_console.c
      Note: |-
        Defines  REPL command and animation verbs
        Defines fan REPL command and animation verbs
    - Path: 0037-cardputer-adv-fan-control-console/main/fan_relay.c
      Note: GPIO3 relay init + set on/off helpers (polarity)
    - Path: 0037-cardputer-adv-fan-control-console/sdkconfig.defaults
      Note: USB Serial/JTAG console defaults
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-07T18:05:33.0173888-05:00
WhatFor: ""
WhenToUse: ""
---



# Diary

## Goal

Record the step-by-step implementation of `001-FAN-CONTROL`: a Cardputer-ADV `esp_console` firmware that controls a fan relay on GPIO3 and provides named on/off animation verbs for validation.

## Step 1: Create GPIO3 relay console + animation verbs

This step created a new ESP-IDF firmware project (`0037-cardputer-adv-fan-control-console`) using the same `esp_console` over USB Serial/JTAG pattern as ticket `0036`, but reduced to a single GPIO output (GPIO3) intended to drive a relay controlling a fan.

The firmware exposes a `fan` REPL command with manual control (`fan on|off|toggle`) and a small set of named “animations” that toggle the GPIO at different intervals (`blink`, `strobe`, `tick`, `beat`, `burst`). A small relay helper module (`fan_relay.*`) provides the requested on/off functions and encapsulates active-high vs active-low behavior.

**Commit (code):** N/A (not committed)

### What I did
- Created a new firmware project directory: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0037-cardputer-adv-fan-control-console`
- Implemented relay GPIO helpers: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0037-cardputer-adv-fan-control-console/main/fan_relay.c`
- Implemented `esp_console` REPL + `fan` command + animation task: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0037-cardputer-adv-fan-control-console/main/fan_console.c`
- Created the docmgr ticket workspace and this diary doc

### Why
- GPIO3 is repurposed as a simple fan relay control pin, so we want a USB-based REPL (not UART) for interactive bring-up without interfering with other UART-driven peripherals/protocols.
- Named toggle patterns make it easy to validate wiring/polarity and demonstrate control without writing custom scripts.

### What worked
- `esp_console_new_repl_usb_serial_jtag(...)` pattern is consistent with `0036` and starts a REPL with a dedicated prompt (`fan> `).
- The animation runner is isolated from manual on/off state: starting an animation saves the current manual state; `fan stop` restores it.
- Firmware builds cleanly via `./build.sh build`.

### What didn't work
- I accidentally used backticks in a `docmgr doc relate --file-note ...` argument under zsh, triggering command substitution and an error:
- `docmgr doc relate ... --file-note "/.../fan_console.c:Defines \`fan\` REPL command and animation verbs"` → `zsh:1: command not found: fan`
- Fix: remove backticks or single-quote the whole `--file-note` argument.

### What I learned
- Reusing the `0036` pattern keeps the console transport consistent (USB Serial/JTAG) and avoids the common double-init pitfall (`esp_console_init()` + `esp_console_new_repl_*`).

### What was tricky to build
- Ensuring “stop animations” restores the last manual state so `fan stop` is predictable (especially if an animation ends on “on”).

### What warrants a second pair of eyes
- GPIO3 polarity and board wiring assumptions: confirm whether the relay expects active-high or active-low, and whether GPIO3 is safe/available on this Cardputer-ADV wiring.
- Concurrency: the animation task shares config globals with the REPL task; the patterns follow the style used elsewhere in this repo, but it’s worth sanity-checking for races.

### What should be done in the future
- Add a small non-TTY command sender (like `0036_send_matrix_commands.py`) so the animation verbs can be validated automatically from a host script.

### Code review instructions
- Start at `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0037-cardputer-adv-fan-control-console/main/fan_console.c` (`cmd_fan`, `anim_task`, `fan_console_start`).
- Review relay semantics in `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0037-cardputer-adv-fan-control-console/main/fan_relay.c` (`fan_relay_set` inversion).
- Validate by building/flashing and running: `fan on`, `fan off`, `fan blink 250 250`, `fan stop`.

### Technical details
- Firmware project docs: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0037-cardputer-adv-fan-control-console/README.md`
- `esp_console` reference implementation used as a template: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c`
- Build used for validation: `cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0037-cardputer-adv-fan-control-console && ./build.sh build`
