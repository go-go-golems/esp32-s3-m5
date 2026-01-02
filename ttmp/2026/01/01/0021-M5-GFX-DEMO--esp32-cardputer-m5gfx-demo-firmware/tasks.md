# Tasks

## TODO

- [x] Finish exhaustive M5GFX feature inventory (API + files + demos)
- [x] Define demo-suite firmware scope and UX (menu/navigation, keyboard controls)
- [ ] Decide asset strategy (embedded arrays vs SD card; formats: BMP/JPG/PNG/QOI)
- [x] Create new ESP-IDF project under `esp32-s3-m5/` for the demo suite
- [x] Implement shared infrastructure: display init, frame loop, input, scene switcher
- [ ] Implement demo modules (primitives, text/fonts, sprites/transforms, images, QR, widgets, perf)
- [x] Add build/flash/run playbook and expected outputs

## Starter Scenarios (implement first)

- [x] A2 List view + selection (menu/settings/file picker pattern)
- [x] A1 Status bar + HUD overlay (persistent chrome + small dirty redraws)
- [x] B2 Frame-time + perf overlay (instrumentation for all other demos)
- [x] E1 Terminal/log console (scrollback + append performance)
- [x] B3 Screenshot to serial (createPng + USB-Serial-JTAG framing)
