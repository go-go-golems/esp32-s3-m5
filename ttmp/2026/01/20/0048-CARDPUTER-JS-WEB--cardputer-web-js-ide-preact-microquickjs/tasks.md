# Tasks

## Documentation (this ticket)

- [x] Create ticket workspace
- [x] Create diary and keep high-frequency entries
- [x] Create prior art + reading list with filenames/symbols
- [x] Draft Phase 1 design doc (embedded UI + REST eval)
- [x] Draft Phase 2 design doc (encoder telemetry over WS)
- [x] Draft Phase 1 smoke-test playbook
- [x] Draft Phase 2 smoke-test playbook

## Implementation (future work)

- [ ] Create firmware project (recommended: `esp32-s3-m5/0048-cardputer-js-web/`)
- [ ] Implement deterministic asset pipeline (Vite → `main/assets/`)
- [ ] Implement `esp_http_server` routes (`/`, `/assets/*`, `/api/status`, `/api/js/eval`)
- [ ] Integrate MicroQuickJS runner (based on `imports/esp32-mqjs-repl/.../JsEvaluator`)
- [ ] Implement timeouts (`JS_SetInterruptHandler`) and request size limits
- [ ] Implement `/ws` endpoint and encoder streaming (Phase 2)
- [ ] Implement browser UI (Preact/Zustand + CodeMirror) and WS telemetry display
