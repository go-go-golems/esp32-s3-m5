# Changelog

## 2026-01-04

- Initial workspace created


## 2026-01-04

Step 1: create ticket workspace + scaffold docs (diary + design-doc)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/04/0028-ESP-EVENT-DEMO--esp-idf-esp-event-demo-cardputer-keyboard-parallel-tasks/design-doc/01-esp-event-demo-architecture-event-taxonomy.md — Design doc scaffold
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/04/0028-ESP-EVENT-DEMO--esp-idf-esp-event-demo-cardputer-keyboard-parallel-tasks/reference/01-diary.md — Diary scaffold + Step 1 entry


## 2026-01-04

Step 2: surveyed Cardputer keyboard + UI patterns (cardputer_kb, typewriter UI, demo-suite console)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/04/0028-ESP-EVENT-DEMO--esp-idf-esp-event-demo-cardputer-keyboard-parallel-tasks/reference/01-diary.md — Recorded findings and candidate baselines


## 2026-01-04

Step 3: reviewed esp_event API + internals (loop task vs manual esp_event_loop_run, event_data heap copy semantics)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/04/0028-ESP-EVENT-DEMO--esp-idf-esp-event-demo-cardputer-keyboard-parallel-tasks/reference/01-diary.md — Captured esp_event dispatch model + constraints


## 2026-01-04

Step 4: drafted esp_event demo design (event base/IDs, payloads, UI-dispatch loop, producer tasks)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/04/0028-ESP-EVENT-DEMO--esp-idf-esp-event-demo-cardputer-keyboard-parallel-tasks/design-doc/01-esp-event-demo-architecture-event-taxonomy.md — Filled in architecture + event taxonomy
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/04/0028-ESP-EVENT-DEMO--esp-idf-esp-event-demo-cardputer-keyboard-parallel-tasks/reference/01-diary.md — Recorded design drafting step


## 2026-01-04

Updated ticket index with overview + links to design doc and diary

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/04/0028-ESP-EVENT-DEMO--esp-idf-esp-event-demo-cardputer-keyboard-parallel-tasks/index.md — Added overview text and key doc links


## 2026-01-04

Design doc: add note about C/C++ linkage for ESP_EVENT_DECLARE_BASE/DEFINE_BASE

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/04/0028-ESP-EVENT-DEMO--esp-idf-esp-event-demo-cardputer-keyboard-parallel-tasks/design-doc/01-esp-event-demo-architecture-event-taxonomy.md — Clarified base declaration/definition when mixing C/C++


## 2026-01-05

Implemented firmware project 0028-cardputer-esp-event-demo and added build/flash playbook

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0028-cardputer-esp-event-demo/README.md — Usage instructions
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0028-cardputer-esp-event-demo/main/app_main.cpp — esp_event loop + producers + UI renderer
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/04/0028-ESP-EVENT-DEMO--esp-idf-esp-event-demo-cardputer-keyboard-parallel-tasks/playbook/01-build-flash-demo-walkthrough.md — Build/flash walkthrough
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/04/0028-ESP-EVENT-DEMO--esp-idf-esp-event-demo-cardputer-keyboard-parallel-tasks/reference/01-diary.md — Step 5 implementation notes


## 2026-01-05

Firmware: fix UI sprite parent wiring (M5Canvas initialized with &display)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0028-cardputer-esp-event-demo/main/app_main.cpp — UiState now constructs M5Canvas with parent display
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/04/0028-ESP-EVENT-DEMO--esp-idf-esp-event-demo-cardputer-keyboard-parallel-tasks/reference/01-diary.md — Recorded the fix


## 2026-01-05

Ticket closed

