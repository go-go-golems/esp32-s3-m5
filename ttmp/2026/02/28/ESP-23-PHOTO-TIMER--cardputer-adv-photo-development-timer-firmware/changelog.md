# Changelog

## 2026-02-28

- Initial workspace created


## 2026-02-28

Completed architecture analysis of existing firmwares and wrote detailed implementation plan + execution tasks for photo timer firmware.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/28/ESP-23-PHOTO-TIMER--cardputer-adv-photo-development-timer-firmware/design-doc/01-cardputer-adv-photo-development-timer-implementation-plan.md — Primary implementation plan
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/28/ESP-23-PHOTO-TIMER--cardputer-adv-photo-development-timer-firmware/reference/01-investigation-diary.md — Chronological work log
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/28/ESP-23-PHOTO-TIMER--cardputer-adv-photo-development-timer-firmware/tasks.md — Detailed task checklist


## 2026-02-28

Implemented 0071 scaffold commit f401c35 (project wiring, config defaults, embedded web skeleton).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0071-cardputer-adv-photo-timer/CMakeLists.txt — Project scaffold
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0071-cardputer-adv-photo-timer/main/CMakeLists.txt — Main component wiring


## 2026-02-28

Implemented core preset storage + timer engine commit d662c86.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0071-cardputer-adv-photo-timer/main/app_state.cpp — Shared runtime state facade
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0071-cardputer-adv-photo-timer/main/preset_store.cpp — JSON storage and validation
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0071-cardputer-adv-photo-timer/main/timer_engine.cpp — Timer state machine


## 2026-02-28

Integrated LVGL encoder UI + HTTP API commit d50bdef; build blocked by missing ESP-IDF Python env in current shell.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0071-cardputer-adv-photo-timer/main/app_main.cpp — UI and runtime loop integration
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0071-cardputer-adv-photo-timer/main/chain_encoder_uart.cpp — Encoder UART transport
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0071-cardputer-adv-photo-timer/main/http_server.cpp — REST and preset upload endpoints

