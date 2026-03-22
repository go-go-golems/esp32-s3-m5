# Changelog

## 2026-03-22

- Initial workspace created


## 2026-03-22

Created comprehensive implementation guide for Gnosis layout engine on M5Paper S3, covering data structures, layout algorithm, widget rendering, dirty-rect tracking, EPD refresh, touch input, screen definitions, build system, and step-by-step implementation plan


## 2026-03-22

Implemented complete Gnosis layout engine firmware (0078-papers3-gnosis-layout). Builds cleanly with ESP-IDF 5.4.1. Features: tree-based layout (VBOX/HBOX/FIXED), 16 widget types, dirty-rect tracking with EPD partial refresh, 7 screen presets (dashboard/calendar/boot/gallery/telemetry/reader/minimal), esp_console REPL for live preset switching, bitmap 5x7 font. 657KB binary.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0078-papers3-gnosis-layout/main/gnosis_console.cpp — esp_console REPL for switching presets
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0078-papers3-gnosis-layout/main/gnosis_types.h — Core data structures (Rect
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0078-papers3-gnosis-layout/main/layout_engine.cpp — Layout algorithm (VBOX/HBOX/FIXED recursive)
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0078-papers3-gnosis-layout/main/screens.cpp — 7 screen presets (dashboard
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0078-papers3-gnosis-layout/main/widget_renderer.cpp — Widget rendering for all 16 node types

