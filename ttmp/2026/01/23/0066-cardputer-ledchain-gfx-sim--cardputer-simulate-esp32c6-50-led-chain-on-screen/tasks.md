# Tasks

## TODO

- [ ] MVP: create new firmware project `0066-cardputer-adv-ledchain-gfx-sim` (builds, flashes, boots)
- [ ] MVP: console REPL over USB Serial/JTAG; `sim` command skeleton (`help`, `status`)
- [ ] MVP: integrate canonical pattern engine (`components/ws281x`) without real WS281x output
- [ ] MVP: implement virtual strip buffer (50 LEDs) and RGB→RGB565 rendering (no gamma)
- [ ] MVP: render 50-LED chain on screen (choose initial topology: 10×5 grid)
- [ ] MVP: console commands to set patterns + params (off/rainbow/chase/breathing/sparkle)
- [ ] MVP: basic on-screen overlay (pattern name + brightness + frame_ms)
- [ ] MVP: smoke test on hardware (Cardputer) and record validation notes in diary

## Later (post-MVP)

- [ ] Optional: keyboard controls for pattern switching (Cardputer keyboard component)
- [ ] Optional: receive MLED/1 over UDP multicast to mirror ESP32-C6 control plane
- [ ] Optional: show-time sync (`execute_at_ms`) + cue semantics for true mirroring
- [ ] Optional: gamma correction toggle to better match perceived LED brightness
