# Tasks

## TODO

- [x] Write/confirm the overall integration plan (decoder + console + assets) in:
  - `analysis/03-animatedgif-support-plan-runtime-integration-flash-asset-bundling-parttool.md`
- [x] Consolidate the AnimatedGIF ESP-IDF component so it’s reusable (not trapped inside a single harness project)
- [x] Integrate AnimatedGIF playback into the existing AtomS3R console+button MVP (`esp32-s3-m5/0013-atoms3r-gif-console`)
- [x] Implement flash-bundled GIF assets using a `parttool.py`-flashed partition (start with FATFS `storage`, then consider raw `gifpack`)
- [ ] Validate correctness + performance on real hardware and document trade-offs
- [ ] Ticket bookkeeping: relate references, update diary/changelog, commit docs

## Detailed tasks (handoff-friendly)

This ticket is intentionally procedural. The goal is to make AnimatedGIF usable as a normal ESP-IDF component, integrate it into the existing control-plane firmware, and define a practical “bundle GIFs into flash” workflow flashed via `parttool.py`.

### 0) Read these first (context + prior research)

- [ ] Read the AnimatedGIF recommendation write-up:
  - `009-INVESTIGATE-GIF-LIBRARIES/sources/research-gif-decoder.md`
- [ ] Read the parent architecture constraints:
  - `008-ATOMS3R-GIF-CONSOLE/analysis/01-brainstorm-architecture-serial-controlled-animation-playback-on-atoms3r.md`
  - `008-ATOMS3R-GIF-CONSOLE/reference/01-asset-pipeline-gif-flash-bundled-animation-playback.md`
- [ ] Identify the known-good display present pattern we will reuse:
  - `esp32-s3-m5/0010-atoms3r-m5gfx-canvas-animation/main/hello_world_main.cpp`
  - `esp32-s3-m5/0014-atoms3r-animatedgif-single/main/hello_world_main.cpp` (AnimatedGIF draw/present harness)
  - `esp32-s3-m5/0013-atoms3r-gif-console/main/hello_world_main.cpp` (console+button playback controller)

### 1) Decide “where AnimatedGIF lives” in the repo (component packaging)

Right now, AnimatedGIF is vendored in the harness:

- `esp32-s3-m5/0014-atoms3r-animatedgif-single/components/animatedgif/`

We need to decide whether to:

- keep it there and reference via `EXTRA_COMPONENT_DIRS` from other chapters, or
- move/copy it into a shared location (recommended), e.g. `esp32-s3-m5/components/animatedgif/`

Tasks:

- [x] Choose a single canonical component path and document it in the analysis doc.
- [x] Ensure the component includes:
  - [ ] upstream license file (Apache-2.0)
  - [ ] a pinned upstream revision note (`README.md`)
  - [ ] ESP-IDF build glue in `CMakeLists.txt`

### 2) Verify the ESP-IDF “platform glue” is correct

AnimatedGIF is Arduino-first. Our vendored header already contains minimal ESP-IDF timing shims:

- `AnimatedGIF.h` defines `PROGMEM`, aliases `memcpy_P`, and provides `millis()`/`delay()` for `ESP_PLATFORM`.

Tasks:

- [ ] Confirm the shims compile cleanly under ESP-IDF 5.4.1 and do not conflict with other headers.
- [ ] Confirm no additional Arduino-only dependencies remain (search upstream sources for `Arduino.h`, `yield()`, etc.).

### 3) AnimatedGIF playback integration (use existing, known-good symbols)

Use the harness implementation as the canonical reference:

- `esp32-s3-m5/0014-atoms3r-animatedgif-single/main/hello_world_main.cpp`
  - `present_canvas(M5Canvas&)`
  - `GifRenderCtx`
  - `GIFDraw(GIFDRAW*)`
  - `s_gif.begin(...)`, `open(...)`, `playFrame(...)`, `getLastError()`, `reset()`

Tasks:

- [x] Extract the “GIF rendering plumbing” into a reusable module for 0013 (or copy it first, then refactor):
  - [ ] `gif_player.h/.cpp` (decoder lifecycle + playback loop)
  - [ ] `gif_draw_m5gfx.h/.cpp` (GIFDraw + canvas scaling/offset + optional swap)
- [x] Replace the mock animation registry in `0013` with a GIF-backed registry:
  - [ ] `list` prints available GIFs
  - [ ] `play <id|name>` opens selected GIF and starts playback
  - [ ] `next` cycles selection and reopens
  - [ ] `stop` halts playback (but keeps console alive)
  - [ ] `info` prints gif canvas/frame size and last delay

### 4) Flash asset bundling (parttool-flashed “storage” partition first)

We want a workflow similar in spirit to existing repo patterns:

- `M5Cardputer-UserDemo/partitions.csv` includes `storage, data, fat, , 1M`
- `ATOMS3R-CAM-M12-UserDemo/upload_asset_pool.sh` demonstrates `parttool.py write_partition`

Recommended MVP: FATFS `storage` partition containing `*.gif` files.

Tasks (host side):

- [x] Add a `storage` partition to the AtomS3R GIF console project’s `partitions.csv` (or reuse existing storage partition) and size it appropriately (start at 1–2MB).
- [x] Create a reproducible host build step that turns a folder of GIFs into a FATFS image:
  - [x] Script location: `esp32-s3-m5/0013-atoms3r-gif-console/make_storage_fatfs.sh`
  - [x] Use ESP-IDF’s `fatfsgen.py` (version-appropriate) to generate `storage.bin`
- [x] Create a flashing helper that uses `parttool.py`:
  - [x] `parttool.py --port <PORT> write_partition --partition-name=storage --input storage.bin`

Tasks (device side):

- [x] Mount FATFS at `/storage` and enumerate `/storage/gifs/`:
  - [x] Provide `list` output based on directory scan
- [x] Implement the “open selected GIF” path:
  - [x] simplest: read file into memory buffer then `gif.open(buf, len, GIFDraw)`
  - [ ] optional: implement AnimatedGIF file callbacks (`GIF_OPEN_CALLBACK`, `GIF_READ_CALLBACK`, `GIF_SEEK_CALLBACK`, `GIF_CLOSE_CALLBACK`) using `FILE*`

### 5) Later: raw `gifpack` partition (memory-mapped directory)

If we later want a pure mmap/no-filesystem model:

- [ ] Define a minimal `GifPack` binary format (header + directory + bytes)
- [ ] Write a host packer tool that produces `GifPack.bin`
- [ ] Flash with `parttool.py write_partition --partition-name=gifpack --input GifPack.bin`
- [ ] On device: `esp_partition_mmap` + parse directory + call memory-backed `open()`

### 6) Correctness + performance validation

Use a “known tricky” GIF set and document results for both modes:

- [ ] Collect at least 4 test GIFs:
  - [ ] local palette usage
  - [ ] transparency
  - [ ] disposal method 2 (“restore to background”)
  - [ ] disposal method 3 (“restore to previous”)
- [ ] Measure:
  - [ ] effective FPS for a representative GIF on AtomS3R
  - [ ] peak heap usage (especially if reading GIF file into RAM)
- [ ] Decide default configuration for the tutorial (RAW vs COOKED, scaling, byte-swap policy) and document it.

### 7) Deliverables

- [ ] AnimatedGIF component is in a shared, reusable location and builds under ESP-IDF 5.4.1
- [ ] `0013` can play real GIFs selected via console + button
- [ ] “bundle GIFs into flash” workflow exists and is reproducible (`make_storage_fatfs` + `parttool.py write_partition`)
- [ ] Documented constraints/trade-offs + recommended defaults


