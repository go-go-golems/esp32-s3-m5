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

### Repo wiring + skeleton

- [ ] Create firmware project dir `esp32-s3-m5/0048-cardputer-js-web/` (CMakeLists + README + partitions)
- [ ] Add `sdkconfig.defaults` for Cardputer console defaults (USB Serial/JTAG)
- [ ] Add `main/Kconfig.projbuild` for Wi‑Fi mode + credentials + JS limits
- [ ] Add deterministic embedded asset pipeline (`web/` Vite outDir → `main/assets/`)

### Firmware: Wi‑Fi bring-up

- [ ] Implement Wi‑Fi bring-up module (choose: SoftAP, STA, or APSTA fallback)
- [ ] Print browse URL and serve `/api/status` with mode + IP(s)

### Firmware: HTTP server + static assets

- [ ] Add embedded asset symbols (`_binary_index_html_start`, `_binary_app_js_start`, `_binary_app_css_start`)
- [ ] Implement routes:
  - [ ] `GET /` (index)
  - [ ] `GET /assets/app.js`
  - [ ] `GET /assets/app.css`
  - [ ] `GET /api/status`
- [ ] Add cache headers strategy (avoid stale UI while keeping bundle small)

### Firmware: MicroQuickJS runner (`/api/js/eval`)

- [ ] Decide stdlib strategy:
  - [ ] reuse `esp32_stdlib_runtime.c` + generated stdlib (imports), or
  - [ ] generate a minimal stdlib for web IDE
- [ ] Implement `js_runner` module:
  - [ ] fixed memory pool (configurable size)
  - [ ] mutex or dedicated task ownership
  - [ ] `eval(code) -> { ok, output, error }` contract
- [ ] Implement `POST /api/js/eval`:
  - [ ] bounded body read (e.g. 16 KiB max)
  - [ ] return JSON result (and proper HTTP error codes on bad requests)
- [ ] Add safety:
  - [ ] timeout via `JS_SetInterruptHandler` + deadline
  - [ ] optional `/api/js/reset`

### Frontend: Preact/Zustand + editor

- [ ] Create Preact app shell
- [ ] Add Zustand store for:
  - [ ] editor text
  - [ ] running/last response
  - [ ] REST call wrapper with errors
- [ ] Integrate editor (CodeMirror 6 preferred):
  - [ ] JS syntax highlighting
  - [ ] minimal keymap
  - [ ] avoid rerender/recreate editor on keystrokes
- [ ] Render output panel (stdout-like + error)

### Phase 2: encoder telemetry over WebSocket

- [ ] Implement `/ws` endpoint (WS upgrade + client tracking)
- [ ] Implement encoder driver interface (`encoder_init`, `encoder_read`)
- [ ] Implement sample/coalesce/broadcast loop (bounded rate + seq)
- [ ] Implement browser WS store + UI rendering of `pos` and `pressed`

### Validation

- [ ] Run Phase 1 playbook and record results
- [ ] Run Phase 2 playbook and record results
