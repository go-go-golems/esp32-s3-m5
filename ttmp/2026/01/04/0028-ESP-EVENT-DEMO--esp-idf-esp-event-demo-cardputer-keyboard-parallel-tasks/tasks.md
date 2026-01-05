# Tasks

## TODO

- [x] Add tasks here

- [x] Survey existing Cardputer keyboard/display firmwares for input + render patterns
- [x] Review ESP-IDF esp_event APIs and internal queue/task model
- [x] Write demo design (event base/ids, handlers, tasks, UI flow)
- [x] Implement firmware project: 0028-cardputer-esp-event-demo (CMakeLists, main/, Kconfig)
- [x] Implement esp_event loop + handlers + UI event log renderer (single UI owner)
- [x] Implement keyboard producer task (cardputer_kb) posting esp_event key events
- [x] Implement 2-3 random/heartbeat producer tasks posting esp_event events
- [x] Write playbook: build/flash + expected on-screen behavior
