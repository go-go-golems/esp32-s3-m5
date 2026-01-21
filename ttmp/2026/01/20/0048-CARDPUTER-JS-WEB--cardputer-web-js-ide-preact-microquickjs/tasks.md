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

- [x] Create firmware project dir `esp32-s3-m5/0048-cardputer-js-web/` (CMakeLists + README + partitions)
- [x] Add `sdkconfig.defaults` for Cardputer console defaults (USB Serial/JTAG)
- [x] Add `main/Kconfig.projbuild` for Wi‑Fi mode + credentials + JS limits
- [x] Add deterministic embedded asset pipeline (`web/` Vite outDir → `main/assets/`)

### Firmware: Wi‑Fi bring-up

- [x] Implement Wi‑Fi bring-up module (STA + esp_console, like 0046)
- [x] Print browse URL on got-IP and serve `/api/status` with STA state + IP

### Firmware: HTTP server + static assets

- [x] Add embedded asset symbols (`_binary_index_html_start`, `_binary_app_js_start`, `_binary_app_css_start`)
- [x] Implement routes:
  - [x] `GET /` (index)
  - [x] `GET /assets/app.js`
  - [x] `GET /assets/app.css`
  - [x] `GET /api/status`
- [x] Add cache headers strategy (no-store MVP)

### Firmware: MicroQuickJS runner (`/api/js/eval`)

- [x] Decide stdlib strategy (MVP: reuse imports stdlib wiring)
  - [x] reuse `esp32_stdlib_runtime.c` + generated stdlib (imports)
  - [ ] generate a minimal stdlib for web IDE
- [x] Implement `js_runner` module:
  - [x] fixed memory pool (configurable size)
  - [x] mutex ownership (MVP)
  - [x] `eval(code) -> { ok, output, error }` contract
- [x] Implement `POST /api/js/eval`:
  - [x] bounded body read (default 16 KiB max)
  - [x] return JSON result (and proper HTTP error codes on bad requests)
- [ ] Add safety:
  - [x] timeout via `JS_SetInterruptHandler` + deadline
  - [ ] optional `/api/js/reset`

### Frontend: Preact/Zustand + editor

- [x] Create Preact app shell
- [x] Add Zustand store for:
  - [x] editor text
  - [x] running/last response
  - [x] REST call wrapper with errors
- [ ] Integrate editor (CodeMirror 6 preferred):
  - [x] JS syntax highlighting
  - [x] minimal keymap
  - [x] avoid rerender/recreate editor on keystrokes
- [x] Render output panel (stdout-like + error)

### Frontend: CodeMirror 6 integration (done)

- [x] Add CodeMirror deps (codemirror 6 packages) with deterministic Vite output preserved
- [x] Implement a `CodeEditor` component (mount/unmount lifecycle, no re-init per keystroke)
- [x] Wire editor value to Zustand (debounced or on-change, but avoid expensive rerenders)
- [x] Add JS language + basic keymap (Ctrl/Cmd-Enter to run, Tab indent)
- [x] Add minimal editor theming (dark-ish, readable on phone)

### Phase 2: encoder telemetry over WebSocket

- [ ] Firmware: WebSocket plumbing
  - [x] Implement `/ws` endpoint (upgrade + client tracking)
  - [x] Define a minimal text-frame schema for telemetry (`type:"encoder"`, `seq`, `ts_ms`, `pos`, `delta`, `pressed`)
  - [x] Add a small broadcast helper (async send, backpressure-safe, client removal on send failure)
  - [ ] Add a periodic “ping/keepalive” plan (or document why we don’t need it for MVP)
- [ ] Firmware: encoder input
  - [ ] Decide hardware backend (built-in GPIO rotary vs M5 Chain Encoder UART)
  - [ ] Implement encoder driver interface (`encoder_init`, `encoder_read`)
  - [x] Add config (Kconfig) for driver selection + pins/UART config (don’t hardcode)
  - [x] Implement sample/coalesce/broadcast loop (bounded rate + seq; immediate send on click edge)
  - [ ] Add `/api/status` fields or log lines that make it obvious when encoder init fails
- [ ] Frontend: WebSocket client + UI
  - [x] Add a small WS client module (connect/reconnect with backoff; “connected” indicator)
  - [x] Add Zustand store slice for encoder telemetry (`pos`, `delta`, `pressed`, `seq`, `ts_ms`)
  - [x] Render encoder telemetry in the UI (pos + pressed; optional debug fields)
  - [x] Handle parse errors / unknown `type` defensively (ignore, don’t crash)

### Validation

- [x] Ensure ESP-IDF tooling is available (`idf.py` on PATH or sourced environment)
- [x] `idf.py build` succeeds for esp32s3 (local build dir via `-B build_esp32s3_v2`)
- [ ] Run Phase 1 playbook and record results
- [ ] Run Phase 2 playbook and record results
- [x] Phase 2C: Write JS→WS event stream design doc
- [x] Phase 2C: Upload design doc 04 to reMarkable
- [x] Phase 2C: UI bounded WS event history panel
- [ ] Phase 2C: Firmware JS emit accumulator + flush + WS broadcast (MVP)
- [ ] Phase 2C: Update JS help panel with emit() docs
- [x] Phase 2B+C: Design JS service task + inbound/outbound queue structures
- [x] Phase 2B+C: Upload JS service design doc 05 to reMarkable
