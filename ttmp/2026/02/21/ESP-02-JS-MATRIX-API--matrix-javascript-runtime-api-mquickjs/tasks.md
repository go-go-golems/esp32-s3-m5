# Tasks

## TODO

- [x] Add tasks here

- [x] Audit existing 0067 matrix firmware and prior mqjs infrastructure
- [x] Design integration architecture for mquickjs in 0067 (tasks, queues, memory, deadlines)
- [x] Design JavaScript matrix API from low-level pixels/framebuffer to high-level animations
- [x] Design REST script submission/runtime model with safety and real-time constraints
- [x] Produce 8+ page textbook-style analysis document in ticket
- [x] Create detailed implementation diary entry with commands/findings
- [x] Upload bundled ticket docs to reMarkable
- [x] Implementation phase: add CMake wiring for mqjs_service and imported mquickjs components in 0067
- [x] Implementation phase: add Kconfig knobs for JS memory/body/timeout/timers
- [x] Implementation phase: extend matrix_engine with script framebuffer APIs (clear/fill/set/get/present/geometry)
- [x] Implementation phase: add 0067 JS timers module (setTimeout/clearTimeout scheduler task)
- [x] Implementation phase: add 0067 JS stdlib runtime bindings for matrix primitives and timing helpers
- [x] Implementation phase: add 0067 js_service wrapper (start/reset/eval/mem/status/stop) over mqjs_service
- [x] Implementation phase: integrate JS service lifecycle into app_main boot flow
- [x] Implementation phase: add REST endpoints /api/js/eval|reset|mem|status|stop
- [x] Implementation phase: add esp_console js command parser with examples
- [x] Implementation phase: ensure existing matrix REST/console behavior remains functional
- [x] Validation phase: run idf.py build in tmux and fix compile/runtime integration errors
- [x] Validation phase: run targeted CLI/API smoke tests locally and document limits
- [x] Documentation phase: update ticket diary with step-by-step implementation + failures
- [x] Documentation phase: relate changed source files and refresh changelog entries
- [x] Release hygiene: create focused commits at opportune milestones
- [x] Follow-up: add complex JS matrix animation examples in 0067 examples folder
- [x] Follow-up: add tracked script to play JS examples over /api/js/eval
- [x] Follow-up: validate example playback on device and document commands
