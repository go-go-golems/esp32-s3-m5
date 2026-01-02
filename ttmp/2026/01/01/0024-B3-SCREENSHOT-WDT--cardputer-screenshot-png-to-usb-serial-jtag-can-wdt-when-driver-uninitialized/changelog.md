# Changelog

## 2026-01-01

- Initial workspace created


## 2026-01-01

Created bug report for screenshot-to-serial WDT (driver not initialized + busy-loop) and linked relevant implementation files/diary.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/screenshot_png.cpp — Retry loop and usb_serial_jtag_write_bytes usage
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0024-B3-SCREENSHOT-WDT--cardputer-screenshot-png-to-usb-serial-jtag-can-wdt-when-driver-uninitialized/analysis/01-bug-report-usb-serial-jtag-write-bytes-not-initialized-busy-loop-causes-wdt-during-screenshot.md — Bug report details


## 2026-01-01

Added additional logs showing the issue correlates with entering the B3 Screenshot scene (then triggering screenshot).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0024-B3-SCREENSHOT-WDT--cardputer-screenshot-png-to-usb-serial-jtag-can-wdt-when-driver-uninitialized/analysis/01-bug-report-usb-serial-jtag-write-bytes-not-initialized-busy-loop-causes-wdt-during-screenshot.md — Added navigation log snippet + interpretation


## 2026-01-01

Fixed screenshot-to-serial to avoid WDT (install USJ driver, chunk writes, bounded retries). Commit da2f85f.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/screenshot_png.cpp — ensure_usb_serial_jtag_driver_ready + chunked serial_write_all


## 2026-01-01

Added postmortem/root-cause report for the screenshot-to-serial WDT issue (includes context, failure mode, fix, and future cautions).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0024-B3-SCREENSHOT-WDT--cardputer-screenshot-png-to-usb-serial-jtag-can-wdt-when-driver-uninitialized/analysis/02-postmortem-screenshot-png-over-usb-serial-jtag-caused-wdt-root-cause-fix.md — New developer-facing explanation


## 2026-01-01

Postmortem now explicitly links to the original captured backtrace/stacktrace and explains how to decode future traces.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0024-B3-SCREENSHOT-WDT--cardputer-screenshot-png-to-usb-serial-jtag-can-wdt-when-driver-uninitialized/analysis/02-postmortem-screenshot-png-over-usb-serial-jtag-caused-wdt-root-cause-fix.md — Added crash artifacts section


## 2026-01-01

Closed: root cause identified and fixed (commit da2f85f) + postmortem written. Remaining: end-to-end validation of PNG capture and fail-fast behavior (tasks 4,7).


## 2026-01-01

Noted follow-up crash during screenshot send: stack overflow in main; tracked separately in 0026

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0024-B3-SCREENSHOT-WDT--cardputer-screenshot-png-to-usb-serial-jtag-can-wdt-when-driver-uninitialized/reference/01-diary.md — Added Step 3 follow-up crash context

