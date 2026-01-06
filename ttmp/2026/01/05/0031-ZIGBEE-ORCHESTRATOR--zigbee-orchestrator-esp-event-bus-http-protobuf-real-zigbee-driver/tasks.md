# Tasks

## Decisions (locked)

- [x] Confirm 0031 host hardware target (CoreS3 vs Cardputer) and record chosen serial ports + wiring
- [x] Confirm Zigbee split: ESP32‑S3 host + ESP32‑H2 NCP (ZNSP over SLIP/UART) and record exact firmware baselines (IDF/SDK versions)
- [x] Choose initial MVP clusters and flows (suggested: OnOff + LevelControl + basic attr report)

## Phase 0 — New firmware scaffold (host only)

- [x] Create new firmware project directory `0031-zigbee-orchestrator/` by forking `0029-mock-zigbee-http-hub/`
- [ ] Rename `hub_*` modules to `gw_*` (mechanical rename; keep build green at each step)
- [ ] Remove/disable `hub_sim` (virtual device simulator); keep a stub `gw_zigbee_driver` that emits synthetic events for now
- [x] Introduce `gw_types.h` (new event IDs + payloads) and keep the old 0029 types out of 0031
- [ ] Add `Kconfig.projbuild` options for phased bring-up (console-only, wifi, http, ws, ui, zigbee)
- [x] Ensure project builds and flashes (even if it does “nothing” yet)

## Phase 1 — USB‑Serial/JTAG `esp_console` (no Wi‑Fi, no HTTP)

### 1.1 SDK configuration (make JTAG console the default)

- [x] Set `sdkconfig.defaults` for USB‑Serial/JTAG console (and disable UART console)
- [ ] Reduce boot log noise if it interferes with the REPL UX
- [ ] Document the expected serial port(s) for Cardputer and how to pick the right one

### 1.2 Console runtime

- [x] Implement `gw_console_init()` and start the REPL early in `app_main`
- [ ] Ensure line editing works when possible (and degrade gracefully)
- [x] Add a `help` command and a `version`/`build` command (git hash if available)
- [ ] Add `loglevel <tag> <level>` helper (optional) to reduce noisy tags during bring-up
- [x] Validate: can type commands reliably without “writing to serial timing out”

## Phase 2 — `gw_bus` (esp_event loop) + console-driven bus events

### 2.1 Bus core

- [x] Define `GW_EVT` event base and `gw_event_id_t` enum (commands vs notifications vs results)
- [x] Define bounded payload structs with `req_id` correlation (no heap ownership in payloads)
- [x] Create the application loop `gw_loop` via `esp_event_loop_create()` (dedicated task)
- [x] Register baseline handlers (at least one command handler + one “any event” tap)
- [ ] Add bus stats (queue depth/drops counters where feasible) and expose via console

### 2.2 Monitor command (console-side bus tap)

- [x] Add `monitor on` command (enable bus event printing)
- [x] Add `monitor off` command (disable bus event printing)
- [x] Define a stable one-line log format (event id name/number + req_id + key fields)
- [x] Add rate limiting / drop counters for monitor output (avoid watchdog / console lockups)
- [x] Ensure monitor printing does not destabilize the system under high event rates
- [x] Validate: spamming events doesn’t break the REPL

### 2.3 Console commands that drive the bus (still offline)

- [x] Add `gw post ...` commands to emit a few test events (e.g., `GW_CMD_PERMIT_JOIN` stub)
- [x] Add `gw status` to show bus loop health, drops, monitor state
- [x] Add `gw demo` command that starts/stops a background task producing random events (for stress testing)

## Phase 3 — Wi‑Fi STA + Wi‑Fi console config (still no HTTP)

### 3.1 Wi‑Fi core

- [ ] Initialize NVS + netif + default event loop as needed for Wi‑Fi
- [ ] Implement Wi‑Fi STA connect/reconnect flow with clear logging
- [ ] Ensure Wi‑Fi logs don’t trample the REPL (tag filtering if needed)

### 3.2 Wi‑Fi console UX (credentials + scan)

- [ ] Port `wifi scan` command (list SSIDs + RSSI + auth mode)
- [ ] Port `wifi set <ssid> <pass>` command (store credentials to NVS)
- [ ] Port `wifi forget` command (erase credentials from NVS)
- [ ] Port `wifi connect` command (initiate connect using stored credentials)
- [ ] Port `wifi status` command (connected? IP? last error? retries?)
- [ ] Persist credentials (NVS) and auto-connect on boot if present
- [ ] Validate: scan works; connect works; IP printed; REPL remains responsive

## Phase 4 — HTTP API (protobuf) with async `202 Accepted` contract

### 4.1 HTTP server skeleton

- [ ] Start `esp_http_server` only after Wi‑Fi is connected (or tolerate offline start but show state)
- [ ] Implement `GET /v1/health` (200, minimal protobuf or plain text)
- [ ] Implement “root” `/` handler (even if it’s a placeholder) to avoid confusing 404s during testing

### 4.2 Protobuf HTTP bodies (nanopb)

- [ ] Create a new schema component `components/gw_proto/` and fork 0029’s nanopb CMake pattern
- [ ] Define protobuf messages for HTTP request bodies (at minimum: `CmdPermitJoin`, `CmdDeviceInterview`, `CmdZclDevice`)
- [ ] Define protobuf messages for HTTP replies (at minimum: `ReplyAccepted`, `ReplyStatus`, `Device`, `DeviceList`)
- [ ] Define protobuf event envelope + payloads for WS (`GwEvent`, `EventId`, `CmdResult`, `ZbDeviceAnnounce`, `ZbAttrReport`, etc)
- [ ] Generate nanopb code at build time and include headers cleanly

### 4.3 Async command semantics (Option 202)

- [ ] Decide `req_id` generation method (monotonic counter vs random64) and implement it
- [ ] Accept client-supplied `req_id` if present; otherwise generate server-side
- [ ] Return `ReplyAccepted { ok=true, req_id }` for action endpoints (`202 Accepted`)
- [ ] Implement `GW_EVT_CMD_RESULT` emission for each HTTP action command
- [ ] Ensure HTTP handlers never block waiting for Zigbee completion

### 4.4 MVP endpoints (Zigbee driver still stubbed)

- [ ] `POST /v1/zigbee/permit_join` → post `GW_CMD_PERMIT_JOIN` to bus → return `202`
- [ ] `GET /v1/devices` and `GET /v1/devices/{id}` → return registry snapshot (200)
- [ ] Validate with `curl`/scripts: 202 reply returns quickly; result event appears on bus/monitor

## Phase 5 — WebSocket protobuf event stream + tooling

### 5.1 WS server endpoint

- [ ] Add `GET /v1/events/ws` WS handler (binary frames)
- [ ] Implement bounded client tracking and safe broadcast (`httpd_ws_send_frame_async`)
- [ ] Add an “event tap” that converts bus events into protobuf envelopes and pushes them to WS

### 5.2 Validation tooling (non-webapp)

- [ ] Add a Node or Python script under `ttmp/.../0031.../scripts/` to connect to WS and decode events
- [ ] Validate: connect/disconnect resilience; no memory growth; events decode correctly

## Phase 6 — Real Zigbee integration (host↔H2 NCP) + MVP clusters

### 6.1 Bring up the H2 NCP (hardware integration)

- [ ] Document the exact flash/build steps for the H2 NCP firmware (link to ticket 001 playbook)
- [ ] Confirm UART pin mapping between Cardputer and Unit Gateway H2; record wiring and baud
- [ ] Ensure console configuration does not conflict with NCP UART pins (USB‑Serial/JTAG on host)

### 6.2 Host-side zigbee_driver module

- [ ] Implement `gw_zigbee_driver_init()` and a `zigbee_worker` queue/task (single serialization point)
- [ ] Implement translation: `GW_CMD_PERMIT_JOIN` → NCP request → emit `GW_EVT_CMD_RESULT`
- [ ] Add device announce handler: parse announce → post `GW_EVT_ZB_DEVICE_ANNOUNCE`
- [ ] Update registry on announce: ensure stable `device_id` for IEEE; update NWK short
- [ ] Add attribute report handler: parse report → post `GW_EVT_ZB_ATTR_REPORT`
- [ ] Update cached OnOff state from reports (cluster OnOff, attr OnOff)
- [ ] Update cached Level state from reports (cluster LevelControl, attr CurrentLevel)

### 6.3 Interview (minimal)

- [ ] Implement `GW_CMD_DEVICE_INTERVIEW` as “discover endpoints + clusters” (bounded)
- [ ] Store results in registry and emit `GW_EVT_ZB_DEVICE_INTERVIEWED` with a snapshot

### 6.4 Control commands (MVP)

- [ ] Implement `POST /v1/devices/{id}/zcl` request parsing (protobuf) and `GW_CMD_ZCL_CMD_DEVICE` posting
- [ ] Implement OnOff commands (On/Off/Toggle) through NCP
- [ ] Implement LevelControl `MoveToLevel` through NCP
- [ ] Emit `GW_EVT_CMD_RESULT` for each request; correlate via `req_id`

## Phase 7 — Webapp UX (served from the device)

### 7.1 Static UI delivery

- [ ] Choose MVP bundling: embed `index.html` + `app.js` as C strings (copy 0029 pattern)
- [ ] Add a small CMake rule / source file layout for UI assets (so edits are easy)
- [ ] Implement `/` route to serve the UI (avoid 404 on `/`)

### 7.2 Webapp features (MVP)

- [ ] WS connect/disconnect and status indicator
- [ ] Live event log view (decode `GwEvent` protobuf frames)
- [ ] Devices list view (from registry snapshot or from event stream)
- [ ] Action control: “Permit join” (POST `/v1/zigbee/permit_join`, generate req_id client-side or let server assign)
- [ ] Action control: On/Off/Toggle for selected device (POST `/v1/devices/{id}/zcl`)
- [ ] Action control: Level slider for selected device (POST `/v1/devices/{id}/zcl`)
- [ ] Correlate UI actions using `req_id` and show completion from `GW_EVT_CMD_RESULT`

## Phase 8 — Ops polish (playbooks + tmux + validation)

- [ ] Write playbook: build + flash Cardputer host firmware (ports, idf.py commands)
- [ ] Write playbook: build + flash H2 NCP firmware (ports, idf.py commands)
- [ ] Write playbook: tmux session layout + commands (host monitor + H2 monitor + WS decoder)
- [ ] Write playbook: tmux pane 1 (Cardputer monitor over USB‑Serial/JTAG)
- [ ] Write playbook: tmux pane 2 (H2 NCP monitor over USB)
- [ ] Write playbook: tmux pane 3 (WS decode script invocation)
- [ ] Write playbook: common failures + fixes (wrong UART pins, wrong USB port, WS 404, no IP, NCP silent)
- [ ] Add a “demo script” checklist (what to do live in a demo)
- [ ] Experiment 1: erase H2 Zigbee storage (zb_fct/zb_storage) and verify formation channel respects zb ch 25
- [x] Add scripts (tmux + flash/erase helpers) so devs can reproduce channel/commissioning experiments reliably
- [x] Implement NCP: apply Trust Center preconfigured key (TCLK) from ZNSP 0x0028 + expose/get it via 0x0027
- [x] Implement NCP: allow toggling link-key exchange requirement via ZNSP secure-mode (0x002A) for diagnosis
- [x] Implement NCP: expose esp_zb_nvram_erase_at_start via ZNSP (0x002E/0x002F) to force fresh formation/channel
- [x] Host console: add zb commands for tclk get/set, lke on/off, nvram erase_at_start
- [ ] Validate on hardware: set TCLK to ZigBeeAlliance09, observe Device authorized status=0x00, and verify short address stabilizes
- [x] Update tmux dual monitor script to use 'idf.py monitor --no-reset' for both H2+host (avoid H2 reset corrupting ZNSP bus)
- [x] Add pyserial-based serial capture script for H2 (/dev/ttyACM0) so we can log without idf.py monitor TTY/DTR side effects
