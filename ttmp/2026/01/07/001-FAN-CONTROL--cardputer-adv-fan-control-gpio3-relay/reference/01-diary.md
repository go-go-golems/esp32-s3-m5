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
      Note: Defines the `fan` REPL command and animation verbs (including presets)
    - Path: 0037-cardputer-adv-fan-control-console/main/fan_relay.c
      Note: GPIO3 relay init + set on/off helpers (polarity)
    - Path: 0037-cardputer-adv-fan-control-console/sdkconfig.defaults
      Note: USB Serial/JTAG console defaults
    - Path: 0038-cardputer-adv-serial-terminal/README.md
      Note: On-device graphical terminal + ADV keyboard + local fan control + esp_console notes
    - Path: 0038-cardputer-adv-serial-terminal/main/Kconfig.projbuild
      Note: Serial-backend Kconfig defaults (UART backend by default to free USB for esp_console)
    - Path: 0038-cardputer-adv-serial-terminal/main/console_repl.cpp
      Note: |-
        esp_console REPL over USB Serial/JTAG and fan command registration
        USB Serial/JTAG esp_console REPL
    - Path: 0038-cardputer-adv-serial-terminal/main/fan_cmd.cpp
      Note: |-
        Shared `fan` command implementation used by both local UI and esp_console
        Shared fan command implementation
    - Path: 0038-cardputer-adv-serial-terminal/main/fan_control.cpp
      Note: Task-based fan relay animation engine (presets + blink/tick/beat/burst)
    - Path: 0038-cardputer-adv-serial-terminal/main/hello_world_main.cpp
      Note: |-
        On-device terminal UI, keyboard handling, and local `fan ...` command execution
        On-device terminal + local fan command mode
    - Path: 0038-cardputer-adv-serial-terminal/sdkconfig.defaults
      Note: |-
        Defaults: USB Serial/JTAG console + UART serial backend + active-low relay on GPIO3
        Defaults for console/backend/relay
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

**Commit (code):** bfafc7a5738204c14543d8c120deefc386e69a7f — "Add 0037 fan relay console and 0038 ADV on-screen terminal"

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

## Step 2: Add slow presets + shared `fan` command for on-device UI + USB REPL

This step expanded the initial `fan` REPL functionality with slow, human-observable presets (seconds between toggles), and then moved the same command implementation into the Cardputer-ADV on-device graphical terminal firmware (`0038`). The intent is that you can control the relay both from the host (USB Serial/JTAG `esp_console`) and locally (ADV keyboard + screen), with the same verbs and behavior.

To keep the Cardputer’s UART pins free (often repurposed for peripherals/protocol links), `0038` now defaults its “terminal serial backend” to UART while reserving USB Serial/JTAG for `esp_console`. If the terminal backend is switched to USB, the firmware intentionally skips starting `esp_console` to avoid competing for USB RX.

**Commit (code):** 69572d48ca4af275b4b57be6c730e6180c155559 — "0037/0038: fan relay presets + on-device fan commands"

### What I did
- Added named `fan preset ...` patterns and made the default timings slower in `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0037-cardputer-adv-fan-control-console/main/fan_console.c`
- Implemented a shared fan relay animation engine and command parser in `0038`:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0038-cardputer-adv-serial-terminal/main/fan_control.cpp`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0038-cardputer-adv-serial-terminal/main/fan_cmd.cpp`
- Hooked the shared command into both:
  - On-device UI: type `fan ...` then press Enter (handled in `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0038-cardputer-adv-serial-terminal/main/hello_world_main.cpp`)
  - USB REPL: `esp_console` `fan` command (implemented in `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0038-cardputer-adv-serial-terminal/main/console_repl.cpp`)
- Updated 0038 defaults so USB Serial/JTAG is reserved for `esp_console` (`/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0038-cardputer-adv-serial-terminal/main/Kconfig.projbuild` and `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0038-cardputer-adv-serial-terminal/sdkconfig.defaults`)
- Built both firmwares:
  - `cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0038-cardputer-adv-serial-terminal && ./build.sh fullclean build`
  - `cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0037-cardputer-adv-fan-control-console && ./build.sh fullclean build`

### Why
- “Seconds between toggles” presets make relay polarity/wiring checks obvious without needing a logic analyzer.
- Sharing the same command implementation between local UI and USB REPL avoids divergence and makes it easier to debug.
- Reserving USB Serial/JTAG for `esp_console` follows the repo’s Cardputer/ESP32-S3 convention and avoids UART console conflicts.

### What worked
- `0038` can now control the relay from the screen/keyboard and from `esp_console` using the same verbs.
- Default timings are slow enough to see/hear a relay click and observe a fan/indicator response.
- Both projects build cleanly after a `fullclean`.

### What didn't work
- I initially ran `docmgr validate frontmatter` with a path that included the `ttmp/` prefix, which caused docmgr to look under `.../ttmp/ttmp/...` and fail:
- `docmgr validate frontmatter --doc ttmp/2026/01/07/.../reference/01-diary.md --suggest-fixes`
- Error: `no such file or directory`
- Fix: pass paths relative to the docmgr root (no leading `ttmp/`), e.g. `--doc 2026/01/07/.../reference/01-diary.md`.

### What I learned
- If the on-screen terminal tries to read/write USB Serial/JTAG directly, it will fight with `esp_console` for RX; picking a default backend that doesn’t share the transport avoids intermittent “missing input” issues.

### What was tricky to build
- Keeping the UI terminal behavior (“send bytes to backend”) while adding a local command mode (`fan ...` on Enter) without accidentally forwarding the same command over the backend.

### What warrants a second pair of eyes
- Transport contention: confirm the chosen defaults match how you want to use the terminal (UART backend) vs USB REPL (esp_console).
- Relay polarity: confirm the relay input is truly active-low on your board/wiring and that GPIO3 is appropriate for your relay module.

### What should be done in the future
- Consider adding a tiny “fan demo” screen mode that cycles through presets automatically (hands-free wiring validation).
- Optionally add a host-side script (non-interactive) to send `fan preset slow`, `fan on/off`, etc., and parse responses.

### Code review instructions
- `0038` entry point and UI integration: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0038-cardputer-adv-serial-terminal/main/hello_world_main.cpp`
- Shared fan control logic: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0038-cardputer-adv-serial-terminal/main/fan_control.cpp` and `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0038-cardputer-adv-serial-terminal/main/fan_cmd.cpp`
- USB REPL bring-up: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0038-cardputer-adv-serial-terminal/main/console_repl.cpp`
- Validate on device:
  - On the host (`idf.py monitor` on `/dev/ttyACM*`): `fan status`, `fan preset slow`, `fan stop`
  - On device: `opt+4` (slow preset), `opt+7` (stop), or type `fan preset slow` then Enter

### Technical details
- Presets are implemented as alternating on/off step durations; see `fan_control_presets()` in `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0038-cardputer-adv-serial-terminal/main/fan_control.cpp`
