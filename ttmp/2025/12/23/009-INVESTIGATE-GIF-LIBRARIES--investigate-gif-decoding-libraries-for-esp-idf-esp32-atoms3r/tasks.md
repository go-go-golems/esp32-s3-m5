# Tasks

## TODO

Note: the “minimal decode+display harness” and the actual port work were moved to ticket **0010-PORT-ANIMATED-GIF** once we decided to standardize on AnimatedGIF.

- [x] Read the “why” and constraints from ticket 008 first (prevents re-deriving AtomS3R display/asset facts)
- [x] Define evaluation criteria + success metrics (CPU, RAM, flash, correctness, integration complexity)
- [x] Identify candidate libraries (short list) and record license + API model
- [x] Build a tiny ESP-IDF harness to decode a known GIF and render to AtomS3R canvas (no serial UI yet) — moved to `0010-PORT-ANIMATED-GIF`
- [x] Decide “default” approach for the tutorial (on-device decode vs pre-converted pack), with rationale
- [x] Write up findings in the 009 analysis doc (tables + recommendation + next steps)
- [x] Ticket bookkeeping: update diary, relate files, update changelog, commit docs

## Detailed checklist (handoff-friendly)

This section is deliberately “procedural” so a new contributor can execute it without context.

### 1) Get oriented (read-only)

- [x] Read ticket `008-ATOMS3R-GIF-CONSOLE` docs first:
  - `analysis/01-brainstorm-architecture-serial-controlled-animation-playback-on-atoms3r.md`
  - `reference/01-asset-pipeline-gif-flash-bundled-animation-playback.md`
- [x] Skim AtomS3R known-good display bring-up:
  - `esp32-s3-m5/0010-atoms3r-m5gfx-canvas-animation/` (stable animation + `waitDMA()`)
  - `esp32-s3-m5/0008-atoms3r-display-animation/` (GC9107 + backlight ground truth)
- [x] Skim the AssetPool “blob on flash partition” pattern:
  - `ATOMS3R-CAM-M12-UserDemo/main/usb_webcam_main.cpp` (partition mmap injection)
  - `ATOMS3R-CAM-M12-UserDemo/main/utils/assets/assets.*` (AssetPool structure + generation)

### 2) Define evaluation criteria (must write down)

- [x] Correctness requirements:
  - [x] Global palette GIFs
  - [x] Local palette GIFs
  - [x] Transparency
  - [x] Disposal modes (important for many GIFs)
  - [x] Frame delay handling + looping behavior
- [x] Performance requirements:
  - [x] Target FPS range (e.g. 10–15fps acceptable vs 30fps “nice”)
  - [x] CPU budget (don’t starve SPI/DMA present loop)
- [x] Resource constraints:
  - [x] RAM: do we require full-canvas RGB buffer?
  - [x] Flash: how many GIFs do we want to ship in ~2MB?
- [x] Integration constraints:
  - [x] Must compile under ESP-IDF 5.4.1
  - [x] Prefer ESP-IDF component packaging (`idf_component_register`) over Arduino-only libraries
  - [x] Avoid requiring LVGL unless the whole UI stack will be LVGL

### 3) Candidate library survey (collect facts, don’t guess)

For each candidate, record in the analysis doc:

- [ ] **Repo URL + license**
- [ ] **API model** (line callback vs full-frame buffer vs LVGL widget)
- [ ] **Storage model** (expects file path, file descriptor, or memory buffer; can it read from flash-mapped data?)
- [ ] **Output model** (paletted indices, RGB888, RGB565 conversion cost)
- [ ] **Known limitations**

Start with these likely candidates (investigated):

- [x] `bitbank2/AnimatedGIF` (widely used, callback-based; note: primarily Arduino, but has ESP32/LovyanGFX examples)
- [x] `UncleRus/esp-idf-libnsgif` (explicit ESP-IDF component wrapper for NetSurf libnsgif)
- [x] `lvgl` GIF support (`lv_gif` / `lv_lib_gif`) (only if we accept LVGL as a dependency)
- [x] `gifdec` (small C decoder; verify correctness/limitations and I/O assumptions)

### 4) Minimal integration harness (recommended path)

Create a small harness project (could be a new chapter under `esp32-s3-m5/`, e.g. `0013` or a scratch dir):

- [x] Display:
  - [x] Use the `0010` canvas + `waitDMA()` present pattern (avoid “flutter”) — moved to `0010-PORT-ANIMATED-GIF`
- [x] Input:
  - [x] Hard-code “play one test GIF” at boot (no console yet) — moved to `0010-PORT-ANIMATED-GIF`
- [x] Decode loop:
  - [x] Decode one frame — moved to `0010-PORT-ANIMATED-GIF`
  - [x] Convert to RGB565 or draw paletted pixels into the canvas buffer — moved to `0010-PORT-ANIMATED-GIF`
  - [x] Present — moved to `0010-PORT-ANIMATED-GIF`
  - [x] Delay according to GIF frame delay — moved to `0010-PORT-ANIMATED-GIF`

Validation:

- [x] Confirm memory usage + heap headroom — moved to `0010-PORT-ANIMATED-GIF`
- [x] Confirm visual correctness on at least one “known tricky” GIF (transparency + disposal) — moved to `0010-PORT-ANIMATED-GIF`

### 5) Recommendation + handoff output

- [x] Pick a default recommendation for ticket 008 (with rationale):
  - [x] “Decode on device” or “pre-converted pack”
  - [x] If decode-on-device: which library and why
- [x] Update ticket 008 tasks with the chosen direction and any new sub-tasks discovered.


