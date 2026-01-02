# Tasks

## TODO

- [x] Finish exhaustive M5GFX feature inventory (API + files + demos)
- [x] Define demo-suite firmware scope and UX (menu/navigation, keyboard controls)
- [x] Decide asset strategy (embedded arrays vs SD card; formats: BMP/JPG/PNG/QOI)
- [x] Create new ESP-IDF project under `esp32-s3-m5/` for the demo suite
- [x] Implement shared infrastructure: display init, frame loop, input, scene switcher
- [ ] Implement demo modules (primitives, text/fonts, sprites/transforms, images, QR, widgets, perf)
- [x] Implement C1 Plasma effect demo (port from `0011-cardputer-m5gfx-plasma-animation`)
- [x] Implement C2 Primitives demo (lines/rects/circles/triangles)
- [x] Add build/flash/run playbook and expected outputs
- [x] Harden B3 screenshot-to-serial (driver init guard + bounded retries; see `0024-B3-SCREENSHOT-WDT`)
- [x] Replace placeholder bodies for A1/B2/B3 scenes

## Demo modules backlog (decomposed)

These are the remaining chunks under “Implement demo modules …”, broken down so each item can be implemented/tested/committed independently.

### Text / fonts

- [ ] D1 Text: builtin fonts showcase + text metrics (`textWidth`, datum/alignment)
- [ ] D2 Text: wrapping/ellipsis demo (reusing `ui_list_view` truncation behavior)
- [ ] D3 Text: UTF-8 / symbol rendering sanity (what works, what doesn’t)

### Sprites / transforms

- [ ] D4 Sprites: sprite compose demo (multiple sprites, transparency/colorkey)
- [ ] D5 Transforms: rotate/scale demo (pivot, `pushRotateZoom`, clipping)

### Images / assets

- [ ] D6 Assets: add tiny embedded image set (BMP/JPG/PNG/QOI) + loader helpers
- [ ] D7 Images: decode/render demo (draw each format + show timing + memory)
- [ ] D8 Images: scaling/cropping demo (fit/fill/center-crop patterns)

### QR / widgets

- [ ] D9 QR: QR code demo (vary size/error correction; render text payload)
- [ ] D10 Widgets: basic “UI kit” demo (buttons, progress bars, toggles, focus state)

### Perf / stress

- [ ] D11 Stress: fill-rate / primitives throughput micro-bench (with B2 overlay visible)

## Starter Scenarios (implement first)

- [x] A2 List view + selection (menu/settings/file picker pattern)
- [x] A1 Status bar + HUD overlay (persistent chrome + small dirty redraws)
- [x] B2 Frame-time + perf overlay (instrumentation for all other demos)
- [x] E1 Terminal/log console (scrollback + append performance)
- [x] B3 Screenshot to serial (createPng + USB-Serial-JTAG framing)
