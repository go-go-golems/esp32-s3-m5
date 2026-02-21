# Changelog

## 2026-02-21

- Initial workspace created


## 2026-02-21

Completed deep source analysis for 0036/0065/0066 plus shared wifi components; authored 7+ page 0067 architecture+intern guide and populated multi-step diary with findings/errors/decisions.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0036-cardputer-adv-led-matrix-console/main/matrix_console.c — Primary source analyzed for bounce text migration
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/wifi_console/wifi_console.c — Primary source analyzed for Wi-Fi esp_console integration
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/21/ESP-01-STAMP-MATRIX--esp32-c3-stamp-matrix-http-firmware/design/01-0067-esp-c3-led-matrix-http-firmware-architecture-and-intern-guide.md — Primary technical deliverable for implementation and onboarding
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/21/ESP-01-STAMP-MATRIX--esp32-c3-stamp-matrix-http-firmware/reference/01-diary.md — Frequent work diary with prompt context and execution trail


## 2026-02-21

Uploaded bundled ticket package to reMarkable: 'ESP-01-STAMP-MATRIX - 0067 STAMP C3 Matrix HTTP Plan' at /ai/2026/02/21/ESP-01-STAMP-MATRIX.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/21/ESP-01-STAMP-MATRIX--esp32-c3-stamp-matrix-http-firmware/changelog.md — Included in uploaded bundle for historical trace
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/21/ESP-01-STAMP-MATRIX--esp32-c3-stamp-matrix-http-firmware/design/01-0067-esp-c3-led-matrix-http-firmware-architecture-and-intern-guide.md — Included in uploaded bundle as primary deliverable
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/21/ESP-01-STAMP-MATRIX--esp32-c3-stamp-matrix-http-firmware/index.md — Included in uploaded bundle as ticket landing context
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/21/ESP-01-STAMP-MATRIX--esp32-c3-stamp-matrix-http-firmware/reference/01-diary.md — Included in uploaded bundle as frequent execution diary
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/21/ESP-01-STAMP-MATRIX--esp32-c3-stamp-matrix-http-firmware/tasks.md — Included in uploaded bundle to show completion state


## 2026-02-21

Added second design document for C++ component extraction of matrix+MAX7219+animations, including built-in optional MatrixConsoleParser for easy esp_console registration.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/21/ESP-01-STAMP-MATRIX--esp32-c3-stamp-matrix-http-firmware/design/02-reusable-c-matrix-max7219-component-extraction-plan.md — New parser-inclusive extraction architecture and migration plan


## 2026-02-21

Uploaded second bundle to reMarkable: 'ESP-01-STAMP-MATRIX - C++ Matrix Component + Parser' and confirmed parser-inclusive C++ extraction plan delivery.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/21/ESP-01-STAMP-MATRIX--esp32-c3-stamp-matrix-http-firmware/design/02-reusable-c-matrix-max7219-component-extraction-plan.md — Uploaded parser-inclusive second design doc
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/21/ESP-01-STAMP-MATRIX--esp32-c3-stamp-matrix-http-firmware/reference/01-diary.md — Diary updated with upload/confirmation step


## 2026-02-21

Implemented and validated firmware 0067 on STAMP C3: matrix engine + console parser + REST, flashed via /dev/serial/by-id, joined CLUB:LINK, verified HTTP controls; switched console defaults to UART for this hardware path (commit dfeb7ad).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0067-esp-c3-led-matrix-http/main/app_main.c — Wires wifi_mgr got-IP callback
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0067-esp-c3-led-matrix-http/main/http_server.c — Implements matrix REST API endpoints
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0067-esp-c3-led-matrix-http/main/matrix_engine.c — Implements framebuffer + scroll/drop animation runtime and synchronization
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0067-esp-c3-led-matrix-http/sdkconfig.defaults — Switches console backend to UART for working REPL on by-id serial path
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/21/ESP-01-STAMP-MATRIX--esp32-c3-stamp-matrix-http-firmware/design/01-0067-esp-c3-led-matrix-http-firmware-architecture-and-intern-guide.md — Implementation addendum with verified runtime outcomes
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/21/ESP-01-STAMP-MATRIX--esp32-c3-stamp-matrix-http-firmware/reference/01-diary.md — Detailed build/flash/test diary for tasks 4-11


## 2026-02-21

Uploaded refreshed implementation bundle to reMarkable: ESP-01-STAMP-MATRIX - 0067 Firmware Implementation Validation.pdf in /ai/2026/02/21/ESP-01-STAMP-MATRIX.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/21/ESP-01-STAMP-MATRIX--esp32-c3-stamp-matrix-http-firmware/changelog.md — Changelog records final reMarkable delivery
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/21/ESP-01-STAMP-MATRIX--esp32-c3-stamp-matrix-http-firmware/reference/01-diary.md — Diary updated with final upload step


## 2026-02-21

Added broad printable punctuation glyph support to matrix engine, removed animation sanitizer that dropped symbols, rebuilt firmware, and completed task 12.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0067-esp-c3-led-matrix-http/main/matrix_engine.c — Expanded 5x7 font map and widened glyph acceptance to printable ASCII
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/21/ESP-01-STAMP-MATRIX--esp32-c3-stamp-matrix-http-firmware/tasks.md — Checked off task 12


## 2026-02-21

Implemented per-animation `repeat_count` (default `0 = infinity`) end-to-end across matrix engine, console parser, and REST API; rebuilt/flashed on STAMP C3 and validated stop-on-repeat and infinite-loop behavior via HTTP status transitions; completed task 13.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0067-esp-c3-led-matrix-http/main/matrix_engine.h — Adds `repeat_count` to status and animation API signatures
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0067-esp-c3-led-matrix-http/main/matrix_engine.c — Tracks cycles and stops animation after configured repeat count
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0067-esp-c3-led-matrix-http/main/matrix_console.c — Adds optional repeat arg, examples, and status/reporting updates
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0067-esp-c3-led-matrix-http/main/http_server.c — Parses/returns `repeat_count` in matrix REST endpoints
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/21/ESP-01-STAMP-MATRIX--esp32-c3-stamp-matrix-http-firmware/tasks.md — Checked off task 13


## 2026-02-21

Uploaded refreshed ticket bundle to reMarkable as `ESP-01-STAMP-MATRIX - 0067 Firmware Repeat Count Update.pdf` in `/ai/2026/02/21/ESP-01-STAMP-MATRIX`.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/21/ESP-01-STAMP-MATRIX--esp32-c3-stamp-matrix-http-firmware/index.md — Included in uploaded bundle
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/21/ESP-01-STAMP-MATRIX--esp32-c3-stamp-matrix-http-firmware/reference/01-diary.md — Included in uploaded bundle with repeat-count validation steps
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/21/ESP-01-STAMP-MATRIX--esp32-c3-stamp-matrix-http-firmware/tasks.md — Included in uploaded bundle showing all tasks complete
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/21/ESP-01-STAMP-MATRIX--esp32-c3-stamp-matrix-http-firmware/changelog.md — Includes upload history entry
