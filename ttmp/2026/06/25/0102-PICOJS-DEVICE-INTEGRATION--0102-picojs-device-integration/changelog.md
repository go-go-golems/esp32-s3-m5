# Changelog

## 2026-06-25

- Initial workspace created


## 2026-06-25

Created device integration ticket, implementation guide, initial tasks, and diary for PicoJS firmware work

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/0102-PICOJS-DEVICE-INTEGRATION--0102-picojs-device-integration/design-doc/01-device-implementation-guide.md — Design plan for merge


## 2026-06-25

Merged feature/0102-js-scripts into main, validated host-side JS/native tests, and split firmware work into child tickets

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0102-esp32-p4-visual-quickjs-repl/js/README.md — Host-side PicoJS workflow merged into main
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/0102-PICOJS-DEVICE-INTEGRATION--0102-picojs-device-integration/design-doc/01-device-implementation-guide.md — Umbrella guide now lists child tickets and phase scopes


## 2026-06-25

Uploaded umbrella and child ticket implementation guides to reMarkable at /ai/2026/06/25/0102-PICOJS-DEVICE-INTEGRATION

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/0102-PICOJS-DEVICE-INTEGRATION--0102-picojs-device-integration/design-doc/01-device-implementation-guide.md — Guide bundle source


## 2026-06-25

Completed child ticket 0102-PICOJS-CONSOLE-FEEDBACK: js smoke, screen dump, by-id UART probe, build/flash/probe validation, and reMarkable upload

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp — Console feedback commands implemented
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/0102-PICOJS-CONSOLE-FEEDBACK--0102-picojs-console-feedback-loop/reference/01-implementation-diary.md — Validation evidence for completed child ticket


## 2026-06-25

Completed child ticket 0102-PICOJS-MINIMAL-DSL: native OS/App/Panel/Text bindings, hello load/frame/dump flow, reset reinstall handling, and reMarkable upload

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/picojs_runtime/picojs_runtime.cpp — Minimal native DSL implementation now lives in picojs_runtime
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/0102-PICOJS-MINIMAL-DSL--0102-picojs-minimal-native-dsl-slice/reference/01-implementation-diary.md — Validation evidence for completed child ticket


## 2026-06-25

Completed child ticket 0102-PICOJS-LAYOUT-WIDGETS: native layout row/col builder, panel region binding, gauges, dashboard validation, and reMarkable upload

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/picojs_runtime/picojs_runtime.cpp — Layout and gauge DSL milestone implementation
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/0102-PICOJS-LAYOUT-WIDGETS--0102-picojs-layout-and-widgets/reference/01-implementation-diary.md — Validation evidence for completed child ticket


## 2026-06-25

Completed PicoJS frame/timer callbacks, app-mode keyboard routing, LCD rendering, and hardware validation (code commit 263323f; docs commit 3362e47)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/picocalc_lcd/picocalc_lcd.c — 40 MHz LCD SPI stabilization
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/picojs_runtime/picojs_runtime.cpp — Callbacks
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/visual_repl/visual_repl.cpp — LCD frame renderer and screen-dump parity


## 2026-06-25

Imported original picoOS devkit JSX into ticket sources for future DSL/app parity reference

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/0102-PICOJS-DEVICE-INTEGRATION--0102-picojs-device-integration/sources/picoos-devkit.jsx — Source reference


## 2026-06-25

Added picoOS devkit app parity assessment, estimating current firmware app-source compatibility at roughly 15-25% and prioritizing missing widgets/OS APIs before host SDL extraction

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/0102-PICOJS-DEVICE-INTEGRATION--0102-picojs-device-integration/analysis/01-picoos-devkit-app-parity-assessment.md — Parity assessment
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/0102-PICOJS-DEVICE-INTEGRATION--0102-picojs-device-integration/sources/picoos-devkit.jsx — Imported reference source

