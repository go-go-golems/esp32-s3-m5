# Changelog

## 2026-01-01

- Initial workspace created

## 2026-01-02

Created the initial demo-suite firmware scaffold as a new ESP-IDF project and implemented the first starter scenario (A2): a reusable ListView with keyboard-driven selection.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/app_main.cpp — Demo-suite home/menu scaffold
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/ui_list_view.cpp — ListView widget implementation
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp — Cardputer keyboard matrix scan + key events
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/reference/02-implementation-diary.md — New implementation diary

## 2026-01-02

Flashed the demo-suite firmware to a real Cardputer and documented the build/flash/monitor/validate workflow (including tmux constraints and serial-port locking gotchas).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/build.sh — Build/flash helper with a tmux entry point
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/design/07-playbook-build-flash-monitor-validate.md — Playbook for device validation loop

## 2026-01-02

Implemented shared demo-suite infrastructure (scene switching + layered sprites) and completed starter scenarios A1 (HUD header) and B2 (perf footer overlay).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/app_main.cpp — Scene switcher + layered present pipeline (body/header/footer)
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/ui_hud.cpp — HUD + perf overlay drawing and perf EMA tracking

## 2026-01-02

Implemented starter scenario E1: a reusable terminal/log console scene (scrollback, input line, wrapping) and flashed it to the Cardputer for validation.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/ui_console.cpp — Console buffer + wrapping + rendering
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/app_main.cpp — Console scene integration and key bindings

## 2026-01-02

Implemented starter scenario B3 (screenshot-to-serial) using `createPng()` and USB-Serial/JTAG framing, plus a host capture script; flashed to device (host capture validation pending).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/screenshot_png.cpp — PNG capture + serial framing
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/tools/capture_screenshot_png.py — Host capture helper

## 2026-01-02

Decided the asset strategy for the demo suite: use the existing flash-backed `storage` FATFS partition for large assets (images/gifs), and embed only tiny UI icons in firmware.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/design/08-asset-strategy-storage-partition.md — Asset strategy decision


## 2026-01-01

Created M5GFX inventory + demo-plan analysis and a research diary; documented Cardputer autodetect, core APIs, and proposed demo-suite coverage matrix.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/analysis/01-m5gfx-functionality-inventory-demo-plan.md — Exhaustive capability inventory and demo plan
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/reference/01-diary.md — Research diary with step-by-step log


## 2026-01-01

Uploaded analysis + diary PDFs to reMarkable (ai/2026/01/01/0021-M5-GFX-DEMO/...) and adjusted the coverage matrix markers to ASCII to avoid missing-glyph issues in DejaVu fonts.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/analysis/01-m5gfx-functionality-inventory-demo-plan.md — Coverage matrix now uses ASCII markers for PDF conversion
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/reference/01-diary.md — Diary step documenting validation and reMarkable upload


## 2026-01-01

Added scenario-driven demo catalog design doc focused on real-world UI patterns and developer workflows (menus, terminal, charts, overlays, image viewer, diagnostics).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/design/01-demo-firmware-real-world-scenario-catalog.md — Design doc for demo-suite content and UX


## 2026-01-01

Uploaded scenario-driven demo catalog design doc PDF to reMarkable (ai/2026/01/01/0021-M5-GFX-DEMO/design/...).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/design/01-demo-firmware-real-world-scenario-catalog.md — Published to reMarkable as PDF


## 2026-01-01

Added starter-scenario tasks and wrote five intern-friendly implementation guides (list view, status HUD, perf overlay, terminal console, screenshot-to-serial).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/design/02-implementation-guide-list-view-selection-a2.md — Intern implementation guide
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/design/03-implementation-guide-status-bar-hud-overlay-a1.md — Intern implementation guide
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/design/04-implementation-guide-frame-time-perf-overlay-b2.md — Intern implementation guide
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/design/05-implementation-guide-terminal-log-console-e1.md — Intern implementation guide
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/design/06-implementation-guide-screenshot-to-serial-b3.md — Intern implementation guide
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/tasks.md — Starter scenario task breakdown


## 2026-01-01

Uploaded five starter-scenario implementation guide PDFs to reMarkable under ai/2026/01/01/0021-M5-GFX-DEMO/design/.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/design/02-implementation-guide-list-view-selection-a2.md — Published to reMarkable
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/design/03-implementation-guide-status-bar-hud-overlay-a1.md — Published to reMarkable
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/design/04-implementation-guide-frame-time-perf-overlay-b2.md — Published to reMarkable
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/design/05-implementation-guide-terminal-log-console-e1.md — Published to reMarkable
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/design/06-implementation-guide-screenshot-to-serial-b3.md — Published to reMarkable

## 2026-01-01

Opened bug ticket 0024 for B3 screenshot-to-serial WDT (USB-Serial/JTAG driver not initialized + busy-loop).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/screenshot_png.cpp — Screenshot send path implicated in watchdog
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0024-B3-SCREENSHOT-WDT--cardputer-screenshot-png-to-usb-serial-jtag-can-wdt-when-driver-uninitialized/analysis/01-bug-report-usb-serial-jtag-write-bytes-not-initialized-busy-loop-causes-wdt-during-screenshot.md — Bug report with backtrace and fix direction


## 2026-01-01

Fixed B3 screenshot-to-serial WDT by guarding/initializing USB-Serial/JTAG and chunking writes (commit da2f85f).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/screenshot_png.cpp — Fixed serial_write_all and driver init


## 2026-01-01

Switched demo-suite input to cardputer_kb; adjusted esc/del bindings; began fixing screenshot stack overflow (tracked in 0026)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp — Use cardputer_kb scanner+bindings for nav/esc/del
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/screenshot_png.cpp — Task-based PNG encode/send to avoid main stack overflow
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/reference/02-implementation-diary.md — Added steps 12-13

