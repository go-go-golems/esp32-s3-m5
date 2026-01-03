---
Title: Cardputer LVGL demo firmware
Ticket: 0025-CARDPUTER-LVGL
Status: active
Topics:
    - esp32s3
    - cardputer
    - lvgl
    - ui
    - esp-idf
    - m5gfx
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0022-cardputer-m5gfx-demo-suite/main/app_main.cpp
      Note: Reference Cardputer app setup (display bring-up + main loop)
    - Path: 0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp
      Note: Keyboard events stream to adapt into LVGL indev
    - Path: 0025-cardputer-lvgl-demo/README.md
      Note: Document Files demo and 'keys' command
    - Path: 0025-cardputer-lvgl-demo/main/action_registry.cpp
      Note: ActionRegistry list and CtrlEvent mapping
    - Path: 0025-cardputer-lvgl-demo/main/action_registry.h
      Note: Shared ActionRegistry contract (ActionId/title/keywords)
    - Path: 0025-cardputer-lvgl-demo/main/app_main.cpp
      Note: |-
        Cardputer LVGL demo main loop + UI creation
        Exports loop counter + LVGL handler timing metrics for SysMon
        Close palette before processing Open* CtrlEvents so overlays don’t persist across host-driven screen switches
    - Path: 0025-cardputer-lvgl-demo/main/command_palette.cpp
      Note: |-
        Ctrl+P palette overlay (modal LVGL layer_top UI
        Full-screen palette overlay with dedicated footer hint; scroll selection in filtered results
    - Path: 0025-cardputer-lvgl-demo/main/command_palette.h
      Note: Palette init/toggle API used by app_main
    - Path: 0025-cardputer-lvgl-demo/main/console_repl.cpp
      Note: |-
        esp_console REPL + screenshot command
        Provides  command used by playbook
        Provides the esp_console command 'screenshot' used by the playbook
        esp_console REPL commands including console/menu/basics/pomodoro/setmins/screenshot
        Add 'keys' esp_console command to inject LVGL keycodes remotely
        esp_console commands (saveshot
    - Path: 0025-cardputer-lvgl-demo/main/control_plane.h
      Note: CtrlEvent queue contract between console and UI
    - Path: 0025-cardputer-lvgl-demo/main/demo_file_browser.cpp
      Note: New Files demo (directory browser UI
    - Path: 0025-cardputer-lvgl-demo/main/demo_file_viewer.cpp
      Note: New file preview viewer (read-only textarea
    - Path: 0025-cardputer-lvgl-demo/main/demo_manager.cpp
      Note: Demo catalog screen switching + shared LVGL group
    - Path: 0025-cardputer-lvgl-demo/main/demo_menu.cpp
      Note: Menu row spacing adjustments to fit all items without overlapping hint
    - Path: 0025-cardputer-lvgl-demo/main/demo_pomodoro.cpp
      Note: Pomodoro demo implementation
    - Path: 0025-cardputer-lvgl-demo/main/demo_split_console.cpp
      Note: |-
        SplitConsole LVGL screen (output+input; scrollback/follow-tail)
        Console readability tweaks (text color + hint via output
    - Path: 0025-cardputer-lvgl-demo/main/demo_system_monitor.cpp
      Note: |-
        System Monitor demo screen (sparklines + 250ms sampling)
        SysMon layout fixes (header alignment
    - Path: 0025-cardputer-lvgl-demo/main/lvgl_port_cardputer_kb.cpp
      Note: |-
        LVGL keypad indev adapter for Cardputer keyboard events
        Support synthetic key taps (press+release) for remote injection
    - Path: 0025-cardputer-lvgl-demo/main/lvgl_port_m5gfx.cpp
      Note: LVGL display driver (draw buffers + flush callback)
    - Path: 0025-cardputer-lvgl-demo/main/screenshot_png.cpp
      Note: |-
        Framed PNG capture over USB-Serial/JTAG
        Streaming PNG encoder + MicroSD saveshot support
    - Path: 0025-cardputer-lvgl-demo/main/sdcard_fatfs.cpp
      Note: |-
        MicroSD FATFS mount/unmount + directory listing + file preview helpers (SDSPI)
        SDSPI mount + diagnostics used for task 56 validation
    - Path: 0025-cardputer-lvgl-demo/main/sdcard_fatfs.h
      Note: MicroSD module API and wiring notes
    - Path: 0025-cardputer-lvgl-demo/main/split_console_log.h
      Note: UI-thread console append API (for command responses)
    - Path: 0025-cardputer-lvgl-demo/tools/capture_screenshot_png_from_console.py
      Note: |-
        Host validation script (send command + capture PNG)
        Scripted capture
        Host capture tool supports streamed PNG (len==0)
    - Path: components/cardputer_kb/matrix_scanner.cpp
      Note: Cardputer keyboard matrix scan implementation
    - Path: imports/lv_m5_emulator/src/utility/lvgl_port_m5stack.cpp
      Note: LVGL<->M5GFX port reference (flush/tick/handler patterns)
    - Path: ttmp/2026/01/01/0025-CARDPUTER-LVGL--cardputer-lvgl-demo-firmware/design-doc/02-screen-state-lifecycle-pattern-lvgl-demo-catalog.md
      Note: Explains screen state + lifecycle pattern and robust evolution
    - Path: ttmp/2026/01/01/0025-CARDPUTER-LVGL--cardputer-lvgl-demo-firmware/playbooks/01-screenshot-verification-feedback-loop-cardputer-lvgl.md
      Note: How to capture + OCR-verify screenshots
    - Path: ttmp/2026/01/01/0025-CARDPUTER-LVGL--cardputer-lvgl-demo-firmware/scripts/validate_task_56.sh
      Note: Repeatable validation script for tasks 55/56
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-01T22:48:53.528781539-05:00
WhatFor: ""
WhenToUse: ""
---















# Cardputer LVGL demo firmware

## Overview

This ticket adds a small ESP-IDF firmware for the M5Stack Cardputer that renders a simple LVGL UI and is controllable from the Cardputer keyboard.

The intent is to establish a high-quality “LVGL + M5GFX + Cardputer keyboard” baseline that future UIs can build on (correct display flushing, stable timing, sane memory usage, and usable input mapping).

## Key Links

- Firmware project: `esp32-s3-m5/0025-cardputer-lvgl-demo` (created in this ticket)
- Analysis: `analysis/01-lvgl-on-cardputer-esp-idf-m5gfx-integration-plan.md`
- Project analyses:
  - `analysis/02-project-analysis-split-pane-console-esp-console-scripting.md`
  - `analysis/03-project-analysis-command-palette-overlay-ctrl-p.md`
  - `analysis/04-project-analysis-system-monitor-sparklines-heap-fps-wi-fi.md`
  - `analysis/05-project-analysis-microsd-file-browser-quick-viewer-text-json-log.md`
- Diary: `reference/01-diary.md`
- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- esp32s3
- cardputer
- lvgl
- ui
- esp-idf
- m5gfx

## Tasks

See [tasks.md](./tasks.md) for the current task list.

## Changelog

See [changelog.md](./changelog.md) for recent changes and decisions.

## Structure

- design/ - Architecture and design documents
- reference/ - Prompt packs, API contracts, context summaries
- playbooks/ - Command sequences and test procedures
- scripts/ - Temporary code and tooling
- various/ - Working notes and research
- archive/ - Deprecated or reference-only artifacts
