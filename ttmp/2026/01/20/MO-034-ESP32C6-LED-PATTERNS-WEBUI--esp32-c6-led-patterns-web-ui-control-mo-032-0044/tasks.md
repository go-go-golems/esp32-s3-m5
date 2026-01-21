# Tasks

## TODO

- [ ] Define scope + acceptance criteria (network mode: STA via console like 0045 vs SoftAP)
- [ ] Align 0046 for XIAO ESP32C6 flash/partitions (4MB flash; larger app partition; release/size optimization for non-debug builds)
- [ ] Implement HTTP API mapping to `led_task_send` (minimal set first):
  - [ ] `POST /api/led/pause|resume|clear`
  - [ ] `POST /api/led/brightness` + `POST /api/led/frame`
  - [ ] `POST /api/led/pattern` (type)
  - [ ] WS staging/apply endpoints (`/api/led/ws/stage`, `/api/led/ws/apply`)
- [ ] Implement Preact+Zustand UI:
  - [ ] Status panel (polling)
  - [ ] Global controls (pause/resume/clear, brightness, frame_ms, log toggle if exposed)
  - [ ] Driver config editor + apply confirmation
- [ ] Bundle pipeline:
  - [ ] Deterministic filenames (`/assets/app.js`, `/assets/app.css`)
  - [ ] NUL-trim for `EMBED_TXTFILES` served assets
  - [ ] Optional sourcemaps served at `/assets/app.js.map` (gzip + `EMBED_FILES`)
- [ ] Hardware validation:
  - [ ] Flash, connect, open UI, verify controls map 1:1 with console behavior
  - [ ] Verify `led ws apply` via UI (reinit safe) and heap stability
  - [ ] Add a short smoke-test playbook and command list

## Done

- [x] Create new tutorial project `0046-xiao-esp32c6-led-patterns-webui/` combining 0044 patterns + 0045 embedded web UI pattern
- [x] Add minimal LED control HTTP API (`/api/led/status`, `/api/led/pattern`, `/api/led/brightness`, `/api/led/frame`, `/api/led/pause|resume|clear`)
- [x] Add per-pattern config endpoints (`/api/led/rainbow`, `/api/led/chase`, `/api/led/breathing`, `/api/led/sparkle`) matching console parameter names/ranges
- [x] Expand `GET /api/led/status` to include active-pattern parameters (console naming)
- [x] Add Preact+Zustand UI with global controls + per-pattern forms (console naming/units)
- [x] Fix `frame_ms` < RTOS tick causing `xTaskDelayUntil` assert (clamp/normalize)
- [x] Write firmware flash/partition/size optimization walkthrough for XIAO ESP32C6

<!-- Notes: keep TODO as "needs validation" items; move here once implemented. -->
