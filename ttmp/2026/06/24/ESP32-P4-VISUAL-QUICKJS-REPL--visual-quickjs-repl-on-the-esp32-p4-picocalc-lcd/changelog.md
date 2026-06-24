# Changelog

## 2026-06-24

- Initial workspace created


## 2026-06-24

Created ticket, wrote the visual QuickJS REPL design guide, created the investigation diary, expanded implementation tasks, and related key 0099/0101 evidence files.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/24/ESP32-P4-VISUAL-QUICKJS-REPL--visual-quickjs-repl-on-the-esp32-p4-picocalc-lcd/design-doc/01-visual-quickjs-repl-analysis-design-and-implementation-guide.md — Initial intern-facing design and implementation guide
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/24/ESP32-P4-VISUAL-QUICKJS-REPL--visual-quickjs-repl-on-the-esp32-p4-picocalc-lcd/reference/01-investigation-diary.md — Step 1 design/evidence diary
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/24/ESP32-P4-VISUAL-QUICKJS-REPL--visual-quickjs-repl-on-the-esp32-p4-picocalc-lcd/tasks.md — Implementation-grade phase/task checklist


## 2026-06-24

Initial ticket validation and reMarkable upload complete: docmgr doctor passed and initial visual QuickJS REPL guide bundle uploaded to /ai/2026/06/24/ESP32-P4-VISUAL-QUICKJS-REPL.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/24/ESP32-P4-VISUAL-QUICKJS-REPL--visual-quickjs-repl-on-the-esp32-p4-picocalc-lcd/reference/01-investigation-diary.md — Step 2 records doctor and upload evidence
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/24/ESP32-P4-VISUAL-QUICKJS-REPL--visual-quickjs-repl-on-the-esp32-p4-picocalc-lcd/tasks.md — T0.6 and T0.7 marked complete


## 2026-06-24

Completed Phase 1 and Phase 2 skeleton: extracted reusable PicoCalc LCD/keyboard components, created 0102 firmware skeleton, built with 4 MB app partition, flashed to ESP32-P4, and verified LCD fill, keyboard event polling, and QuickJS eval over debug console.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp — 0102 skeleton and debug command validation
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/picocalc_keyboard/picocalc_keyboard.c — Keyboard component extraction and hardware-verified event polling
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/picocalc_lcd/picocalc_lcd.c — LCD component extraction and hardware-verified fill path
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/24/ESP32-P4-VISUAL-QUICKJS-REPL--visual-quickjs-repl-on-the-esp32-p4-picocalc-lcd/reference/01-investigation-diary.md — Step 3 records build


## 2026-06-24

Added first visual_repl renderer component: 40x20 fixed-cell terminal model, semantic row styles, built-in bitmap glyphs, startup demo, screen demo UART command, and full-screen redraw measurement around 31 ms on hardware.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp — Startup demo and screen demo debug command
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/visual_repl/include/visual_repl.h — Renderer public API and fixed-cell geometry
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/visual_repl/visual_repl.cpp — Renderer implementation and measured full-screen redraw path
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/24/ESP32-P4-VISUAL-QUICKJS-REPL--visual-quickjs-repl-on-the-esp32-p4-picocalc-lcd/reference/01-investigation-diary.md — Step 4 records renderer implementation

