# Tasks

## TODO

<!-- Managed via `docmgr task add/check/list` -->

- [x] Scaffold new ESP32-C6 WS281x console firmware project (0044)
- [x] Port WS281x RMT driver + encoder into reusable module
- [x] Implement pattern configs + algorithms: rainbow/chase/breathing/sparkle
- [x] Implement animation task + FreeRTOS queue control protocol
- [x] Implement esp_console REPL over USB Serial/JTAG (led verbs + ws staging/apply)
- [x] idf.py build (esp32c6) + record build output
- [x] Write minimal run/usage notes + smoke checklist
- [x] Make periodic loop/status logs opt-in (default off) + add REPL toggle
- [x] Add led help + pattern-aware led status; widen pattern speed validation to 1..255
- [x] Add tmux flash/monitor workflow + curated smoke command list
- [x] Update design doc for latest REPL semantics and upload updated PDF to reMarkable
- [ ] Move tmux/smoke scripts into ticket scripts/ folder and update playbook references
- [ ] Revise pattern speed scaling: rainbow 0..20, chase meaningful >30, breathing slower/smoother, sparkle slower
- [ ] Extend chase pattern: configurable train spacing and multiple trains
- [ ] Write analysis doc answering speed mapping + brightness/power questions and upload to reMarkable
