# Tasks

## TODO

- [x] MVP: create new firmware project `0066-cardputer-adv-ledchain-gfx-sim` (builds, flashes, boots)
- [x] MVP: console REPL over USB Serial/JTAG; `sim` command skeleton (`help`, `status`)
- [x] MVP: integrate canonical pattern engine (`components/ws281x`) without real WS281x output
- [x] MVP: implement virtual strip buffer (50 LEDs) and RGB→RGB565 rendering (no gamma)
- [x] MVP: render 50-LED chain on screen (choose initial topology: 10×5 grid)
- [x] MVP: console commands to set patterns + params (off/rainbow/chase/breathing/sparkle)
- [x] MVP: basic on-screen overlay (pattern name + brightness + frame_ms)
- [x] MVP: smoke test on hardware (Cardputer) and record validation notes in diary

## Later (post-MVP)

- [ ] Optional: keyboard controls for pattern switching (Cardputer keyboard component)
- [ ] Optional: receive MLED/1 over UDP multicast to mirror ESP32-C6 control plane
- [ ] Optional: show-time sync (`execute_at_ms`) + cue semantics for true mirroring
- [ ] Optional: gamma correction toggle to better match perceived LED brightness

## Web Server

- [x] Add Wi-Fi console commands (`wifi ...`) in the same REPL as `sim ...`
- [x] Start `esp_http_server` after Wi-Fi got IP
- [x] Add `GET /api/status` (JSON)
- [x] Add `POST /api/apply` (JSON) to set pattern + params + brightness + frame_ms
- [x] Serve a minimal `/` HTML control page (embedded asset)
- [ ] Smoke test: connect to Wi-Fi from console and hit `/api/status`

## MicroQuickJS

- [x] Integrate `mquickjs` (MicroQuickJS) into 0066 firmware
- [x] Generate a 0066-specific stdlib with a `sim` global object for pattern control
- [x] Add `js` console command for `eval` + `repl` + `stats` + `reset`
- [x] Expose simulator control APIs to JS (`sim.setPattern(...)`, etc.)
- [x] Smoke test on hardware: `js eval sim.status()` and setting patterns/brightness
- [ ] Optional: add `sim.statusJson()` (or seed `JSON.stringify(sim.status())` helper)
- [ ] Optional: add SPIFFS-backed `load(path)` + `:autoload` for JS libraries

## MQJS Timers + GPIO Sequencer

- [ ] Design: setTimeout / interval callbacks + sequencing (this doc)
- [ ] Implement: `setTimeout(fn, ms)` and `clearTimeout(id)`
- [ ] Implement: periodic callback primitive (`setInterval` or `every(ms, fn)`)
- [ ] Implement: GPIO G3/G4 toggle primitives (and explicit pin mapping)
- [ ] Document: example toggle sequences (JS snippets)
