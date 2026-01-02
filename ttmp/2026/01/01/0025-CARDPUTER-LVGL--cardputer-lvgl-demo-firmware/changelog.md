# Changelog

## 2026-01-01

- Initial workspace created


## 2026-01-01

Created LVGL Cardputer demo plan + scaffolded 0025-cardputer-lvgl-demo (LVGL display+keyboard ports, initial textarea/button UI); build succeeds.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0025-cardputer-lvgl-demo/main/app_main.cpp — Initial LVGL demo UI and loop
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0025-cardputer-lvgl-demo/main/lvgl_port_m5gfx.cpp — LVGL flush to M5GFX + tick
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0025-CARDPUTER-LVGL--cardputer-lvgl-demo-firmware/analysis/01-lvgl-on-cardputer-esp-idf-m5gfx-integration-plan.md — Integration plan and API references
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0025-CARDPUTER-LVGL--cardputer-lvgl-demo-firmware/reference/01-diary.md — Research/implementation diary


## 2026-01-01

Flashed 0025-cardputer-lvgl-demo to Cardputer; demo UI appears to work on hardware (monitor not captured due to non-TTY harness).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0025-CARDPUTER-LVGL--cardputer-lvgl-demo-firmware/reference/01-diary.md — Step 3 flash smoke test notes


## 2026-01-01

Added demo menu + Pomodoro screen; refactored Basics into demo catalog; build+flash verified.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0025-cardputer-lvgl-demo/main/demo_manager.cpp — Screen switching + shared LVGL group
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0025-cardputer-lvgl-demo/main/demo_pomodoro.cpp — Pomodoro UI + timer + key controls
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0025-CARDPUTER-LVGL--cardputer-lvgl-demo-firmware/design-doc/01-lvgl-demo-menu-screen-architecture-cardputer.md — Design for menu/screen architecture


## 2026-01-01

Fix Fn+1 (Esc) chord reliability by prioritizing Back binding when keynum 2 is held with Fn.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0025-cardputer-lvgl-demo/main/input_keyboard.cpp — Force Fn+1 to map to Action::Back (Esc)


## 2026-01-01

Fix Back/Esc chord mapping to Fn+` (keynum 29+1) and reduce task_wdt noise by blocking at least 1 tick per loop.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0025-cardputer-lvgl-demo/main/app_main.cpp — Use vTaskDelay(1) to ensure yielding
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/cardputer_kb/include/cardputer_kb/bindings_m5cardputer_captured.h — Back/Esc binding corrected to keynum 1


## 2026-01-01

Fix crash when switching demos by deleting LVGL timers/state on screen delete (prevents use-after-free in demo timers).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0025-cardputer-lvgl-demo/main/demo_basics.cpp — Delete status timer via LV_EVENT_DELETE
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0025-cardputer-lvgl-demo/main/demo_pomodoro.cpp — Delete tick timer via LV_EVENT_DELETE


## 2026-01-02

Added design doc explaining current screen-state pattern (cleanup-on-delete) and a roadmap for a more robust lifecycle framework (async switching, ScreenContext, destroy hooks).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0025-CARDPUTER-LVGL--cardputer-lvgl-demo-firmware/design-doc/02-screen-state-lifecycle-pattern-lvgl-demo-catalog.md — New doc: screen state and lifecycle patterns


## 2026-01-02

Added next tasks: esp_console scripting support; screenshot capture for UI inspection.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0025-CARDPUTER-LVGL--cardputer-lvgl-demo-firmware/tasks.md — New tasks


## 2026-01-02

Added verbose analyses for next demo projects: split-pane console+esp_console, command palette, system monitor, MicroSD file browser.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0025-CARDPUTER-LVGL--cardputer-lvgl-demo-firmware/analysis/02-project-analysis-split-pane-console-esp-console-scripting.md — New analysis
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0025-CARDPUTER-LVGL--cardputer-lvgl-demo-firmware/analysis/03-project-analysis-command-palette-overlay-ctrl-p.md — New analysis
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0025-CARDPUTER-LVGL--cardputer-lvgl-demo-firmware/analysis/04-project-analysis-system-monitor-sparklines-heap-fps-wi-fi.md — New analysis
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0025-CARDPUTER-LVGL--cardputer-lvgl-demo-firmware/analysis/05-project-analysis-microsd-file-browser-quick-viewer-text-json-log.md — New analysis


## 2026-01-02

Updated ticket index with links to new project analysis docs.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0025-CARDPUTER-LVGL--cardputer-lvgl-demo-firmware/index.md — Link new analyses


## 2026-01-02

Expanded tasks into detailed workflows for Console/esp_console, Screenshot, Command Palette, System Monitor, and MicroSD browser; fixed screenshot command task text.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0025-CARDPUTER-LVGL--cardputer-lvgl-demo-firmware/tasks.md — Detailed task breakdown


## 2026-01-02

Minor: normalize screenshot task wording (remove escaped quotes).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0025-CARDPUTER-LVGL--cardputer-lvgl-demo-firmware/tasks.md — Task wording


## 2026-01-02

Implemented esp_console REPL over USB-Serial/JTAG with screenshot capture command; added host capture tools and validated end-to-end (captured 240x135 PNG + OCR verified menu text).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0025-cardputer-lvgl-demo/main/app_main.cpp — UI-thread dispatch for screenshot
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0025-cardputer-lvgl-demo/main/console_repl.cpp — esp_console commands
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0025-cardputer-lvgl-demo/main/screenshot_png.cpp — PNG encode/send + framing
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0025-cardputer-lvgl-demo/tools/capture_screenshot_png_from_console.py — Host validation tool


## 2026-01-02

Related new screenshot/console implementation files to the ticket index.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0025-CARDPUTER-LVGL--cardputer-lvgl-demo-firmware/index.md — RelatedFiles updated


## 2026-01-02

Added playbook for screenshot capture + OCR verification feedback loop.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0025-CARDPUTER-LVGL--cardputer-lvgl-demo-firmware/playbooks/01-screenshot-verification-feedback-loop-cardputer-lvgl.md — New playbook


## 2026-01-02

Diary updated with commit reference for screenshot/OCR workflow. (docs commit fdc2df1)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0025-CARDPUTER-LVGL--cardputer-lvgl-demo-firmware/reference/01-diary.md — Step 7 commit reference


## 2026-01-02

Add v0 esp_console demo-control commands (menu/basics/pomodoro/setmins) and document expected outputs; screenshot OCR validated (commit a623875)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0025-cardputer-lvgl-demo/main/app_main.cpp — UI-thread CtrlEvent dispatch for demo-control commands
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0025-cardputer-lvgl-demo/main/console_repl.cpp — Registers menu/basics/pomodoro/setmins commands that enqueue CtrlEvents
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0025-cardputer-lvgl-demo/main/demo_manager.cpp — Stores/apply Pomodoro minutes default and screen switching
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0025-CARDPUTER-LVGL--cardputer-lvgl-demo-firmware/analysis/02-project-analysis-split-pane-console-esp-console-scripting.md — Defines v0 command set and expected outputs
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0025-CARDPUTER-LVGL--cardputer-lvgl-demo-firmware/reference/01-diary.md — Step 8 diary entry with commands and OCR results


## 2026-01-02

Add SplitConsole LVGL demo screen (output+input, scrollback/follow-tail) and menu entry; add host REPL 'console' command; validated via screenshot OCR (commit 3c6d8ea)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0025-cardputer-lvgl-demo/main/console_repl.cpp — Adds host 'console' command to open SplitConsole via CtrlEvent
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0025-cardputer-lvgl-demo/main/demo_menu.cpp — Menu now lists Console entry
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0025-cardputer-lvgl-demo/main/demo_split_console.cpp — New LVGL split-pane console screen with scrollback and command parsing
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0025-CARDPUTER-LVGL--cardputer-lvgl-demo-firmware/reference/01-diary.md — Step 9 diary entry with validation commands and OCR output

