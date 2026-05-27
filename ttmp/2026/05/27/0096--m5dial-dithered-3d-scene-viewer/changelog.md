# Changelog

## 2026-05-27

- Initial workspace created


## 2026-05-27

Created ticket, design doc (27KB), and investigation diary. Added 16 tasks across 5 phases. Copied m5dial.jsx reference, related 5 source files.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/05/27/0096--m5dial-dithered-3d-scene-viewer/design-doc/01-dithered-3d-scene-viewer-design-and-implementation-guide.md — Design doc with memory budget


## 2026-05-27

Fixed 0096 build: removed invalid COMPONENT_DIRS override, copied missing Button vendor files, corrected console/scene compile errors, and verified idf.py build with ESP-IDF 5.4.2.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0096-m5dial-dithered-3d/CMakeLists.txt — Root CMake fix; do not override ESP-IDF component discovery
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0096-m5dial-dithered-3d/main/CMakeLists.txt — Added missing Button vendor source and include path
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0096-m5dial-dithered-3d/main/app_main.cpp — Switched console startup to ESP-IDF USB Serial/JTAG REPL helper


## 2026-05-27

Hardware iteration: added dedicated target-image TERRAIN poster renderer, dirty redraw, encoder-visible terrain motion, RGB565 byte-swap fix, and restored button palette cycling.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0096-m5dial-dithered-3d/main/app_main.cpp — Dirty redraw loop
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0096-m5dial-dithered-3d/main/terrain_poster.cpp — Fast target-style TERRAIN renderer based on provided screenshot


## 2026-05-27

Tuned encoder sensitivity: one full camera orbit per roughly 12 M5Dial clicks (2π/12 rad per click), matching the device's tactile detents.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0096-m5dial-dithered-3d/main/app_main.cpp — Encoder delta mapping changed to 0.5236 rad per click


## 2026-05-27

Implemented five poster-style scenes, runtime pixel-size control, console redraw invalidation, and published/pushed the Obsidian deep-dive report (vault commit bd7c310).

### Related Files

- /home/manuel/code/wesen/go-go-golems/go-go-parc/Projects/2026/05/27/ARTICLE - M5Dial Dithered 3D Scene Viewer - Software Rendering on ESP32-S3.md — Committed/pushed deep-dive project report
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0096-m5dial-dithered-3d/main/console_commands.cpp — Pixel command and render invalidation for console state changes
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0096-m5dial-dithered-3d/main/scene.h — Render parameters now include pixel_size and revision
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0096-m5dial-dithered-3d/main/terrain_poster.cpp — Five-scene poster renderer


## 2026-05-27

Added poster FPS stats, runtime encoder sensitivity/debug UI commands, serial dumpfb screenshot export, and a dependency-free dumpfb-to-PNG host script; validated a reconstructed TERRAIN PNG.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0096-m5dial-dithered-3d/main/app_main.cpp — Framebuffer registration
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0096-m5dial-dithered-3d/main/console_commands.cpp — dumpfb
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/05/27/0096--m5dial-dithered-3d-scene-viewer/artifacts/latest-dumpfb.png — Validated reconstructed framebuffer screenshot artifact
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/05/27/0096--m5dial-dithered-3d-scene-viewer/scripts/02-dumpfb-to-png.py — Host-side PNG reconstruction from dumpfb transcript


## 2026-05-27

Added pyserial screenshot capture helper, captured/read all five scenes, added UI safe areas, and made text solid instead of dithered/pixel-blocked.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0096-m5dial-dithered-3d/main/terrain_poster.cpp — UI safe areas and solid
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/05/27/0096--m5dial-dithered-3d-scene-viewer/artifacts/scene-captures-v2/terrain.png — Post-fix terrain screenshot inspected with read
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/05/27/0096--m5dial-dithered-3d-scene-viewer/scripts/03-capture-dumpfb.py — Automated serial dumpfb capture to PNG

