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

