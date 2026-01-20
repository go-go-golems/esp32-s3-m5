---
Title: 0044 Build/Flash + LED Console Smoke Test
Ticket: MO-032-ESP32C6-LED-PATTERNS-CONSOLE
Status: active
Topics:
    - esp32
    - animation
    - console
    - serial
    - esp-idf
    - usb-serial-jtag
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0044-xiao-esp32c6-ws281x-patterns-console/README.md
      Note: Build/flash entrypoints
    - Path: 0044-xiao-esp32c6-ws281x-patterns-console/main/led_console.c
      Note: REPL commands used in smoke test
    - Path: 0044-xiao-esp32c6-ws281x-patterns-console/main/led_task.c
      Note: Queue protocol semantics referenced
    - Path: 0044-xiao-esp32c6-ws281x-patterns-console/main/main.c
      Note: Boot + task + console start
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-20T15:26:06.241643987-05:00
WhatFor: Repeatable procedure to build/flash the 0044 ESP32-C6 WS281x patterns firmware and validate the esp_console REPL + core animations.
WhenToUse: Use after changes to 0044 firmware modules (driver/patterns/task/console) to confirm it builds, flashes, and the LED console commands behave as expected.
---


# 0044 Build/Flash + LED Console Smoke Test

## Purpose

Build and flash `0044-xiao-esp32c6-ws281x-patterns-console` and validate:

- WS281x output is running (default pattern renders).
- Console comes up over USB Serial/JTAG and accepts commands.
- Pattern switching/config updates work at runtime via queue control.
- WS281x driver staging/apply works (reinit boundary).

## Environment Assumptions

- ESP-IDF 5.4.1 installed at `~/esp/esp-idf-5.4.1/`.
- Target hardware: Seeed XIAO ESP32C6 (or equivalent ESP32-C6 board with USB Serial/JTAG console access).
- WS2811/WS2812-style LED chain + 5V supply.
- Linux device node for the board is known (commonly `/dev/ttyACM0` for USB Serial/JTAG).

Wiring baseline (typical):

- Board `GND` → strip `GND`
- External `5V` → strip `5V`
- Board `D1` (GPIO1 default) → strip `DIN` (use a 5V level shifter when possible)

Console backend baseline:

- USB Serial/JTAG via `sdkconfig.defaults` (avoid UART pin collisions).

## Commands

```bash
source ~/esp/esp-idf-5.4.1/export.sh
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0044-xiao-esp32c6-ws281x-patterns-console

idf.py set-target esp32c6
idf.py build
```

Flash + monitor:

```bash
idf.py -p /dev/ttyACM0 flash monitor
```

Optional tmux workflow (flash/monitor + command pane):

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console
./scripts/tmux_flash_monitor.sh /dev/ttyACM0
```

Console smoke session (run inside the monitor session):

```text
help
led help
led status

led rainbow set --speed 5 --sat 100 --spread 10
led chase set --speed 20 --tail 5 --fg #00ffff --bg #000044 --dir forward --fade 1
led breathing set --speed 8 --color #ff00ff --min 10 --max 255 --curve sine
led sparkle set --speed 10 --density 8 --fade 30 --mode random --bg #050505

led brightness 10
led frame 25
led pause
led resume
led clear

led log on
led log status
led log off

led ws status
led ws set count 50
led ws apply
```

Expected `idf.py build` output (excerpt):

```text
Bootloader binary size 0x5570 bytes. 0x2a90 bytes (33%) free.
...
xiao_esp32c6_ws281x_patterns_console_0044.bin binary size 0x339a0 bytes. Smallest app partition is 0x100000 bytes. 0xcc660 bytes (80%) free.

Project build complete. To flash, run:
 idf.py flash
```

## Exit Criteria

- `idf.py build` succeeds for `esp32c6` target.
- After flashing, you see a log line similar to: `esp_console started over USB Serial/JTAG`.
- `led status` prints a coherent snapshot (running/paused, pattern, WS cfg).
- Pattern commands visibly change the animation on the LED chain.
- `led ws set ...` requires `led ws apply` before it takes effect; after apply the strip continues running.

## Notes

- If `led ws apply` changes `count`, the firmware reinitializes the driver and pattern engine; expect a brief blink/off during reinit.
- Periodic status logging is opt-in; use `led log on` to enable and `led log off` to disable.
- If you ever see a UART console backend selected, check `0044-xiao-esp32c6-ws281x-patterns-console/sdkconfig.defaults`.
