---
Title: 'Postmortem: screenshot PNG over USB-Serial/JTAG caused WDT (root cause + fix)'
Ticket: 0024-B3-SCREENSHOT-WDT
Status: active
Topics:
    - cardputer
    - m5gfx
    - screenshot
    - usb-serial-jtag
    - watchdog
    - esp-idf
    - debugging
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../../../esp/esp-idf-5.4.1/components/esp_driver_usb_serial_jtag/src/usb_serial_jtag.c
      Note: Reference for usb_serial_jtag_write_bytes return semantics when driver uninitialized
    - Path: esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/screenshot_png.cpp
      Note: Root cause and fix location (driver guard + chunked writes + bounded retries)
    - Path: esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/sdkconfig
      Note: Console configuration context (UART default
    - Path: esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/tools/capture_screenshot_png.py
      Note: Host capture protocol for validation
ExternalSources: []
Summary: Explains why the screenshot-to-serial path could busy-loop and trip Task WDT, and how we fixed it (driver init guard + chunked writes + bounded retries/yield).
LastUpdated: 2026-01-01T21:35:18.061123699-05:00
WhatFor: ""
WhenToUse: ""
---


# Postmortem: screenshot PNG over USB-Serial/JTAG caused WDT (root cause + fix)

## Audience / Context

This is written for a new developer joining the stack. It assumes:

- ESP32-S3 + ESP-IDF v5.4.1
- Cardputer hardware
- A graphics demo firmware (`0022-cardputer-m5gfx-demo-suite`) that can generate a PNG screenshot using M5GFX (`display.createPng`) and stream it to the host via USB-Serial/JTAG.

## Feature involved (B3 Screenshot to serial)

The demo-suite supports a screenshot feature that sends a framed PNG over the same USB connection used for flashing/monitoring:

- Frame format:
  - `PNG_BEGIN <len>\n` + raw PNG bytes + `\nPNG_END\n`
- Device-side generator:
  - `m5gfx::M5GFX::createPng(&len, x, y, w, h)`
- Transport:
  - `usb_serial_jtag_write_bytes()` (binary-safe; `printf` is not)

Relevant implementation:

- `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/screenshot_png.cpp`
  - `screenshot_png_to_usb_serial_jtag()`
  - `serial_write_all()` (pre-fix: infinite retry loop)

The user typically navigates to the “B3 Screenshot” scene and presses `P` to trigger the send.

## Symptoms

On-device logs repeatedly printed:

- `E usb_serial_jtag: usb_serial_jtag_write_bytes(...): The driver hasn't been initialized`

Then Task Watchdog fired, showing CPU0 stuck in the main task and a backtrace into our screenshot send loop:

- `serial_write_all()` → `usb_serial_jtag_write_bytes()` → WDT

The reproduction often correlated with being on the B3 screen (then pressing `P`), but the core trigger is the screenshot send attempt itself.

## Crash artifacts (previous stacktrace / backtrace)

The full captured log + backtrace is recorded in the original bug report:

- `esp32-s3-m5/ttmp/2026/01/01/0024-B3-SCREENSHOT-WDT--cardputer-screenshot-png-to-usb-serial-jtag-can-wdt-when-driver-uninitialized/analysis/01-bug-report-usb-serial-jtag-write-bytes-not-initialized-busy-loop-causes-wdt-during-screenshot.md`

Key frames from that backtrace (the “why did we wedge” proof):

- `usb_serial_jtag_write_bytes()` (ESP-IDF) emitted “driver hasn't been initialized”
- `serial_write_all()` in `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/screenshot_png.cpp` spun in a tight retry loop
- Task watchdog then triggered because IDLE0 did not run/reset the WDT in time

If you capture a new backtrace, keep the raw log as-is and use `xtensa-esp32s3-elf-addr2line` (or `idf.py monitor`’s built-in decoding) against the exact `.elf` that was flashed to map addresses to file/line numbers.

## Root cause (what was actually wrong)

Two independent bugs compounded into a wedge:

### 1) Calling USB-Serial/JTAG writes when the driver is not installed

ESP-IDF’s USB-Serial/JTAG driver is not guaranteed to be installed just because:

- the chip supports USB-Serial/JTAG, or
- the board is physically connected, or
- the same USB port was used for flashing.

In this project’s config, the console is UART by default and USB-Serial/JTAG is configured as a secondary console:

- `CONFIG_ESP_CONSOLE_UART_DEFAULT=y`
- `CONFIG_ESP_CONSOLE_SECONDARY_USB_SERIAL_JTAG=y`
- `# CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG is not set`

Therefore, the app must not assume the driver exists.

In ESP-IDF v5.4.1, `usb_serial_jtag_write_bytes()` includes:

```c
ESP_RETURN_ON_FALSE(p_usb_serial_jtag_obj != NULL, ESP_ERR_INVALID_ARG, ..., "The driver hasn't been initialized");
```

So when the driver is not installed, the function returns a negative `esp_err_t` value (e.g. `ESP_ERR_INVALID_ARG`) and logs the error.

### 2) Infinite busy-loop on write failure (`<= 0`)

The pre-fix code treated any `n <= 0` as “retry immediately”:

- `n < 0` (permanent error: driver not installed) would retry forever
- `n == 0` (timeout acquiring internal resources) would retry forever

Because `usb_serial_jtag_write_bytes()` logged an error each time when the driver was missing, the loop also spammed logging. Together, this:

- prevented the main task from returning to its normal frame loop,
- starved the idle task, and
- tripped Task WDT (`IDLE0` not resetting watchdog).

### 3) (Secondary) Attempting to send huge buffers as a single ringbuffer item

Even when the driver is installed, the driver uses a ringbuffer internally and `usb_serial_jtag_write_bytes(src, size, ...)` attempts to enqueue a single item of length `size`. A full-screen PNG can be many kilobytes; enqueuing that as a single ringbuffer item can fail (leading to `0` return and retries).

So even with the driver installed, the “single huge write” approach is fragile.

## Fix (what changed and why it works)

**Code fix commit:** `da2f85f` — "Fix: harden screenshot PNG send over USB-Serial/JTAG"

Changes in `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/screenshot_png.cpp`:

1. **Install/guard the driver**
   - Added `ensure_usb_serial_jtag_driver_ready()`:
     - if `usb_serial_jtag_is_driver_installed()`: OK
     - else attempt `usb_serial_jtag_driver_install(...)` and fail fast if it fails
2. **Chunk writes**
   - Added `kTxChunkBytes = 128` and only enqueue small chunks per call.
   - Avoids trying to push an entire PNG as one ringbuffer item.
3. **Bound retries + yield**
   - `n < 0`: treat as fatal and return `false` (no infinite loop).
   - `n == 0`: treat as “couldn’t enqueue”:
     - decrement a retry budget
     - `vTaskDelay(1)` to let other tasks (including IDLE) run
     - return failure when the budget expires.

This combination prevents the “retry forever” wedge, even in the worst case (no driver, no host, host not reading, etc.).

## Validation status

- Build succeeded under ESP-IDF 5.4.1.
- Firmware flashed to the connected Cardputer.
- A monitor process holding `/dev/ttyACM0` had to be stopped to allow flashing (expected: monitor locks the port).

Remaining manual validation (requires physical keypress):

- Run `capture_screenshot_png.py` on the host and press `P` on the device to confirm a valid PNG is produced.
- Press `P` with no host reader to confirm the device remains responsive and does not trigger WDT.

## What to watch out for in the future

### Don’t assume “driver exists” just because hardware exists

Treat USB-Serial/JTAG as an optional subsystem:

- check/install driver (`usb_serial_jtag_is_driver_installed()` / `usb_serial_jtag_driver_install()`), and optionally
- check connection (`usb_serial_jtag_is_connected()`) if you want “fail fast when not connected”.

### Never do infinite retry loops in UI-triggered code paths

Any feature that runs on the main loop (rendering/input) must have:

- bounded time budgets, and/or
- yields, and/or
- be executed in a separate task with its own watchdog policy.

### Large binary transfers must be chunked and/or flow-controlled

If you send large binary payloads:

- chunk writes (ringbuffer items must be small)
- consider flushing (`usb_serial_jtag_wait_tx_done`) if you need stronger delivery semantics
- avoid interleaving with logs; logs can corrupt framed binary protocols unless you use a separate channel.

### Port locking during flash/monitor is normal

`idf.py monitor` holds an exclusive lock on the serial device; it must be stopped before flashing, or you must use a combined `flash monitor` flow (often via tmux).

## Pointers for code review / debugging

- Start: `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/screenshot_png.cpp`
  - `ensure_usb_serial_jtag_driver_ready()`
  - `serial_write_all()`
  - `kTxChunkBytes`
- ESP-IDF reference:
  - `components/esp_driver_usb_serial_jtag/src/usb_serial_jtag.c` (`usb_serial_jtag_write_bytes`)
- Related ticket artifacts:
  - Bug report: `analysis/01-bug-report-usb-serial-jtag-write-bytes-not-initialized-busy-loop-causes-wdt-during-screenshot.md`
  - Ticket diary: `reference/01-diary.md`
