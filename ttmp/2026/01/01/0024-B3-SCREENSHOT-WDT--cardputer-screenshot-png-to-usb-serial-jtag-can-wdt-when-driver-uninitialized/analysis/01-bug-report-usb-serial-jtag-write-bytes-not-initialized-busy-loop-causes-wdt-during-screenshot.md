---
Title: 'Bug report: usb_serial_jtag_write_bytes not initialized + busy-loop causes WDT during screenshot'
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
    - Path: esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/app_main.cpp
      Note: Binds screenshot action (P) and calls screenshot_png_to_usb_serial_jtag()
    - Path: esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/screenshot_png.cpp
      Note: Contains serial_write_all() busy-loop and screenshot_png_to_usb_serial_jtag()
    - Path: esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/sdkconfig
      Note: Console/USB Serial JTAG configuration that may affect driver init
    - Path: esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/tools/capture_screenshot_png.py
      Note: Host-side framing parser used to capture screenshots
    - Path: esp32-s3-m5/ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/reference/02-implementation-diary.md
      Note: Diary entry Step 8 implemented B3 and notes logging/interleaving risks
ExternalSources: []
Summary: Screenshot-to-serial can busy-loop on `usb_serial_jtag_write_bytes` failure and trigger the Task WDT when the USB-Serial/JTAG driver is not initialized.
LastUpdated: 2026-01-01T21:21:48.087007881-05:00
WhatFor: ""
WhenToUse: ""
---


# Bug report: `usb_serial_jtag_write_bytes` not initialized + busy-loop causes WDT during screenshot

## Summary

Triggering the B3 screenshot path (`P` in the demo-suite) can wedge the device and eventually trip the Task WDT. The immediate symptom is repeated:

- `usb_serial_jtag_write_bytes(...): The driver hasn't been initialized`

The backtrace shows the main task stuck inside `serial_write_all()` in `screenshot_png.cpp`, which is currently an infinite “retry on <= 0 bytes written” loop with no backoff and no “bail out” condition.

## Observed logs (user report)

```
E (41293) usb_serial_jtag: usb_serial_jtag_write_bytes(269): The driver hasn't been initialized
E (41303) usb_serial_jtag: usb_serial_jtag_write_bytes(269): The driver hasn't been initialized
E (41313) usb_serial_jtag: usb_serial_jtag_write_bytes(269): The driver hasn't been initialized
E (41323) task_wdt: Task watchdog got triggered. The following tasks/users did not reset the watchdog in time:
E (41323) task_wdt:  - IDLE0 (CPU 0)
E (41323) task_wdt: Tasks currently running:
E (41323) task_wdt: CPU 0: main
E (41323) task_wdt: CPU 1: IDLE1
E (41323) task_wdt: Print CPU 0 (current core) backtrace

Backtrace: ...
--- 0x42009661: usb_serial_jtag_write_bytes at .../components/esp_driver_usb_serial_jtag/src/usb_serial_jtag.c:269
--- 0x4200d27d: serial_write_all(void const*, unsigned int) at .../esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/screenshot_png.cpp:13
--- 0x4200d2c7: screenshot_png_to_usb_serial_jtag(m5gfx::M5GFX&) at .../esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/screenshot_png.cpp:31
--- 0x4200c6ae: app_main at .../esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/app_main.cpp:331
```

## Additional observed logs (navigating to B3 Screenshot scene)

The error also correlates with opening the “B3 Screenshot” scene in the demo-suite UI:

```
I (8443) cardputer_m5gfx_demo_suite: open: idx=1 scene=A1 HUD overlay
I (15243) cardputer_m5gfx_demo_suite: open: idx=2 scene=B2 Perf overlay
I (33663) cardputer_m5gfx_demo_suite: open: idx=4 scene=B3 Screenshot
E (36443) usb_serial_jtag: usb_serial_jtag_write_bytes(269): The driver hasn't been initialized
E (36453) usb_serial_jtag: usb_serial_jtag_write_bytes(269): The driver hasn't been initialized
...
```

Note: in current firmware, the screenshot send is triggered by a global “screenshot requested” flag (key `P`), not by scene-open itself. So the most likely interpretation is “entered B3 scene, then triggered screenshot, then got stuck”.

## Expected behavior

- If USB-Serial/JTAG is available, a screenshot should be transmitted framed as:
  - `PNG_BEGIN <len>\n` + raw bytes + `\nPNG_END\n`
- If USB-Serial/JTAG is not available/initialized, the screenshot action should fail fast:
  - show a short on-screen error/toast and return to the app loop without hanging

## Actual behavior

- The firmware spams error logs and can trip the watchdog while trying to transmit the screenshot.

## Likely root cause

There are two compounding issues:

1. **No driver-init guard**: `screenshot_png.cpp` calls `usb_serial_jtag_write_bytes()` directly, but the driver may not be installed when the app runs (or may be unavailable in a given console configuration).
2. **Busy-loop on error**: `serial_write_all()` currently does:
   - `int n = usb_serial_jtag_write_bytes(...);`
   - `if (n <= 0) continue;`

If the driver is not installed, this becomes an infinite tight loop. Since `usb_serial_jtag_write_bytes()` itself logs an error in that case, the loop also floods logging and can starve IDLE0, leading to Task WDT.

### Relevant configuration clue (demo-suite)

`esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/sdkconfig` sets the console to UART by default:

- `CONFIG_ESP_CONSOLE_UART_DEFAULT=y`
- `CONFIG_ESP_CONSOLE_SECONDARY_USB_SERIAL_JTAG=y`
- `# CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG is not set`

This makes it plausible that USB-Serial/JTAG is *not* always installed/ready for direct use by the app (even if the port exists physically and/or is used for flashing).

## Where to look (files + symbols)

### Demo-suite firmware

- `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/screenshot_png.cpp`
  - `serial_write_all()`
  - `screenshot_png_to_usb_serial_jtag()`
- `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/app_main.cpp`
  - global key binding for screenshot trigger (search for `KeyCode::P` / `screenshot_png_to_usb_serial_jtag`)
- `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/sdkconfig`
  - console selection settings (`CONFIG_ESP_CONSOLE_*`)

### ESP-IDF driver references (v5.4.1)

- `components/esp_driver_usb_serial_jtag/include/driver/usb_serial_jtag.h`
  - `usb_serial_jtag_driver_install()`
  - `usb_serial_jtag_is_driver_installed()`
  - `usb_serial_jtag_is_connected()`
  - `usb_serial_jtag_write_bytes()`

## Reproduction (best-known)

1. Build + flash `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite` to a Cardputer.
2. Trigger screenshot (currently global key `P`).
3. Observe repeated `usb_serial_jtag_write_bytes(...): The driver hasn't been initialized` and eventual watchdog reset.

Notes:
- The report says “after a while”, so there may also be a time/disconnect component (USB host sleep / unplug / port closed). Even in that scenario, we should still fail fast rather than spinning forever.

## Suggested fix direction

Minimum safety fix (prevents WDT even if USB is unavailable):

- In `serial_write_all()`:
  - treat `n < 0` as a fatal error and return `false` (no infinite retry)
  - treat `n == 0` as “would block”; add `vTaskDelay(1)` backoff and also add a bounded retry counter / time budget
  - avoid `portMAX_DELAY` for the write timeout; prefer a small bounded wait and a total time budget for the whole send

Make USB-Serial/JTAG availability explicit:

- Before sending any bytes:
  - if `!usb_serial_jtag_is_driver_installed()`: attempt `usb_serial_jtag_driver_install(USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT())` (or return an error)
  - optionally check `usb_serial_jtag_is_connected()` and fail fast if not connected

Optional robustness improvements:

- Chunk writes to a reasonable size (e.g. 256–1024 bytes) and yield (`vTaskDelay(0/1)`) between chunks so IDLE can run.
- Consider temporarily suppressing logging while sending raw PNG (log interleaving can corrupt the host capture).

## Validation checklist (post-fix)

- Trigger screenshot with host capture script running:
  - `python3 esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/tools/capture_screenshot_png.py <port> out.png`
  - press `P`
  - verify file opens and `PNG_END` framing completes
- Disconnect host / close port / unplug USB and press `P`:
  - device remains responsive and shows an on-screen “not connected”/error toast
- Confirm no watchdog triggers during repeated screenshot sends.

## Links

- Design notes for B3: `esp32-s3-m5/ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/design/06-implementation-guide-screenshot-to-serial-b3.md`
- Diary context where B3 was implemented (Step 8): `esp32-s3-m5/ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/reference/02-implementation-diary.md`
