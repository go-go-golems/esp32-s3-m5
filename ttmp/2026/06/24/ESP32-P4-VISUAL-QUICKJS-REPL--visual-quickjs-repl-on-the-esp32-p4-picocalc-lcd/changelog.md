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


## 2026-06-24

Fixed visual font clipping with a host-side SVG renderer preview and firmware x/y scaling split; added Phase 4 keyboard editor code with input-row repaint, line submission without eval, and rate-limited keyboard I2C backoff. Hardware input smoke remains blocked by current keyboard I2C ESP_ERR_INVALID_STATE until a PicoCalc keyboard/southbridge reset or power-cycle is tested.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp — Keyboard editor implementation and I2C polling backoff
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/visual_repl/tools/render_preview.py — Host-side renderer experiment showing 2x horizontal glyph overflow
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/visual_repl/visual_repl.cpp — Firmware font geometry fix and input-row renderer
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/24/ESP32-P4-VISUAL-QUICKJS-REPL--visual-quickjs-repl-on-the-esp32-p4-picocalc-lcd/reference/01-investigation-diary.md — Step 5 records font diagnosis


## 2026-06-24

Fixed visual_repl rendering past NUL terminators: row rendering now turns all cells after the first string terminator into spaces, and row/prompt buffers are zero-filled to prevent stale bytes from appearing on the LCD input line.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/visual_repl/visual_repl.cpp — Terminator-aware row rendering and deterministic zero-filled row buffers
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/24/ESP32-P4-VISUAL-QUICKJS-REPL--visual-quickjs-repl-on-the-esp32-p4-picocalc-lcd/reference/01-investigation-diary.md — Step 6 records the uninitialized/stale-byte display bug and fix


## 2026-06-24

Added 0102 LCD color diagnostics: lcd rect and lcd swatches commands for operator-reported RGB565 color-order/inversion investigation; swatch chart flashed and rendered successfully.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp — LCD rect/swatch UART diagnostics
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/24/ESP32-P4-VISUAL-QUICKJS-REPL--visual-quickjs-repl-on-the-esp32-p4-picocalc-lcd/reference/01-investigation-diary.md — Step 7 records color diagnostic implementation and swatch layout

