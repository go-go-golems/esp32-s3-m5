---
Title: Diary
Ticket: 0010-PORT-ANIMATED-GIF
Status: active
Topics:
    - esp32s3
    - esp-idf
    - atoms3r
    - display
    - animation
    - gif
    - m5gfx
    - assets
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ATOMS3R-CAM-M12-UserDemo/main/usb_webcam_main.cpp
      Note: Reference for future mmap-based GIF input
    - Path: M5Cardputer-UserDemo/components/M5GFX
      Note: Shared M5GFX dependency used by harness
    - Path: esp32-s3-m5/0010-atoms3r-m5gfx-canvas-animation/CMakeLists.txt
      Note: Key reference for how tutorial projects pull M5GFX
    - Path: esp32-s3-m5/0010-atoms3r-m5gfx-canvas-animation/main/hello_world_main.cpp
      Note: Key reference for display init + present loop mentioned in diary
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-23T17:32:08.399280692-05:00
WhatFor: ""
WhenToUse: ""
---


# Diary

## Goal

Keep a step-by-step, handoff-friendly diary of the work done for ticket `0010-PORT-ANIMATED-GIF`: packaging `bitbank2/AnimatedGIF` as an ESP-IDF component and building a minimal AtomS3R harness that plays a single GIF (no GIF-CONSOLE integration yet).

## Step 1: Create a starting map (docs + repo orientation)

This step established the “starting map” for the ticket so the implementation work doesn’t thrash. The focus was to identify the repo’s known-good AtomS3R display/present pattern, the existing ticket constraints/decision record, and the likely integration points (CMake, component layout, draw callback shape).

No code was changed in this step; this is documentation + orientation work only.

### What I did
- Read the ticket `0010` checklist and existing plan doc to confirm scope (“single GIF harness first; no console integration yet”).
- Re-read the upstream decision record from ticket `009` to lock in the expected AnimatedGIF API surface (`open(ptr,size,cb)`, `playFrame`, `GIFDRAW`, optional cooked mode).
- Re-read ticket `008` architecture + asset pipeline docs to keep the constraints in view (AtomS3R display facts + eventual mmap input).
- Inspected existing AtomS3R tutorial projects:
  - `esp32-s3-m5/0010-atoms3r-m5gfx-canvas-animation/` for stable present + backlight init patterns.
  - `esp32-s3-m5/0013-atoms3r-gif-console/` and confirmed it’s currently a scaffold/mock, not yet the integration target.
- Verified docmgr root configuration (top-level `.ttmp.yaml` points to `echo-base--openai-realtime-embedded-sdk/ttmp`).
- Created two docs for ticket `0010`:
  - `analysis/02-starting-map-files-symbols-and-integration-points.md`
  - `reference/01-diary.md` (this file)

### Why
- The “port AnimatedGIF” work spans docs + an ESP-IDF tutorial app + a new component; a clear map reduces wasted time and prevents accidentally integrating into `0013` too early.
- AtomS3R display and backlight behavior is easy to regress; anchoring on `0010` keeps the harness grounded in known-good patterns.

### What worked
- Confirmed the repo’s existing M5GFX usage model: tutorial apps pull `M5GFX` via `EXTRA_COMPONENT_DIRS` pointing at `M5Cardputer-UserDemo/components/M5GFX`.
- Confirmed docmgr is configured for this ticket workspace via the repo root `.ttmp.yaml`.

### What didn't work
- N/A (no implementation work yet).

### What I learned
- `esp32-s3-m5/0013-atoms3r-gif-console` currently reuses the `0010` canvas demo code and should be treated as “reserved for later integration” per ticket instruction.
- The M5GFX component is shared from `M5Cardputer-UserDemo/components/M5GFX`, so the new “single GIF” harness should follow the same pattern to stay consistent and avoid duplicating large dependencies.

### What was tricky to build
- Docmgr workspace roots: this mono-repo contains multiple `.ttmp.yaml` files; the top-level one is authoritative for the `echo-base--openai-realtime-embedded-sdk/ttmp` ticket set.

### What warrants a second pair of eyes
- Confirm we should keep the new harness under `esp32-s3-m5/` (tutorial-style) while tracking docs under the `echo-base--openai-realtime-embedded-sdk/ttmp` ticket workspace. This is likely correct, but it’s a “repo organization” decision that benefits from a quick sanity check.

### What should be done in the future
- Decide and document the exact new tutorial folder name for the standalone harness (proposed: `esp32-s3-m5/0014-atoms3r-animatedgif-single/`) and then start implementation there.

### Code review instructions
- Start with `analysis/02-starting-map-files-symbols-and-integration-points.md` to see the intended file touch points and symbol map.

### Technical details
- Docmgr commands executed (from repo root):
  - `docmgr doc add --ticket 0010-PORT-ANIMATED-GIF --doc-type analysis --title "Starting map: files, symbols, and integration points"`
  - `docmgr doc add --ticket 0010-PORT-ANIMATED-GIF --doc-type reference --title "Diary"`

## Step 2: Create a standalone AtomS3R project skeleton (no GIF playback yet)

This step created a brand-new ESP-IDF tutorial project that will become the “truth harness” for the AnimatedGIF port. The project is intentionally separate from the `0013-atoms3r-gif-console` work so we can iterate on the decoder/component without pulling in console UX or asset directory concerns.

This step *did* change code (new project files), but it still does not include the AnimatedGIF dependency yet; the app just brings up the display, allocates an `M5Canvas`, and shows a stable placeholder frame.

**Commit (code, esp32-s3-m5 repo):** `56d2f164d01323e50f60494c5d14cb8141aae7f6` — "atoms3r: add 0014 AnimatedGIF single harness scaffold"

### What I did
- Created the new standalone project directory:
  - `esp32-s3-m5/0014-atoms3r-animatedgif-single/`
- Added ESP-IDF project scaffolding mirroring the known-good `0010` approach:
  - `CMakeLists.txt` uses `EXTRA_COMPONENT_DIRS` to reuse `M5Cardputer-UserDemo/components/M5GFX`
  - `dependencies.lock` pins ESP-IDF `5.4.1` like the other tutorials
  - `partitions.csv` + `sdkconfig.defaults` set AtomS3R-appropriate defaults
- Added `main/hello_world_main.cpp` that:
  - reuses the AtomS3R GC9107 pinout
  - retains the “legacy I2C driver” backlight logic (to avoid ESP-IDF 5.x legacy/new I2C driver conflicts)
  - creates an `M5Canvas` sprite and presents a placeholder string using `pushSprite()` and `waitDMA()` (configurable)
- Renamed Kconfig symbols from `TUTORIAL_0010_*` to `TUTORIAL_0014_*` so the new project doesn’t inherit confusing config names.

### Why
- Keep the decoder/component work isolated: “single GIF playback” is the minimal proof point for the port.
- Preserve known-good AtomS3R display behavior (backlight init + DMA present sequencing) by following the `0010` pattern.

### What worked
- The new `0014` project mirrors the existing tutorial structure cleanly and keeps M5GFX reuse consistent with the repo.

### What didn't work
- N/A (no build attempted yet in this step).

### What I learned
- The `0010` display+backlight code is modular enough to reuse as-is in a new tutorial without pulling in the plasma animation logic.

### What was tricky to build
- Avoiding “Kconfig symbol drift”: copying `0010` verbatim would compile, but the `CONFIG_TUTORIAL_0010_*` names would be misleading in `0014`. Renaming early keeps later debugging less confusing.

### What warrants a second pair of eyes
- Confirm that keeping `M5GFX` as an `EXTRA_COMPONENT_DIRS` dependency (vs copying M5GFX into `0014/components/`) remains the preferred pattern once we add `components/animatedgif/`.

### What should be done in the future
- Vendor/pin AnimatedGIF and add it as `components/animatedgif/` under `0014`, then replace the placeholder loop with real `gif.open()` + `playFrame()` rendering.

### Code review instructions
- Start at `esp32-s3-m5/0014-atoms3r-animatedgif-single/main/hello_world_main.cpp` to see the display init + canvas present contract this harness will rely on.

### Technical details
- Files added:
  - `esp32-s3-m5/0014-atoms3r-animatedgif-single/CMakeLists.txt`
  - `esp32-s3-m5/0014-atoms3r-animatedgif-single/dependencies.lock`
  - `esp32-s3-m5/0014-atoms3r-animatedgif-single/partitions.csv`
  - `esp32-s3-m5/0014-atoms3r-animatedgif-single/sdkconfig.defaults`
  - `esp32-s3-m5/0014-atoms3r-animatedgif-single/README.md`
  - `esp32-s3-m5/0014-atoms3r-animatedgif-single/main/CMakeLists.txt`
  - `esp32-s3-m5/0014-atoms3r-animatedgif-single/main/Kconfig.projbuild`
  - `esp32-s3-m5/0014-atoms3r-animatedgif-single/main/hello_world_main.cpp`

## Step 3: Vendor AnimatedGIF as an ESP-IDF component + play one embedded GIF

This step turned the `0014` harness into an actual “single GIF player” by vendoring upstream **bitbank2/AnimatedGIF** into `components/animatedgif` and wiring a minimal `GIFDraw` callback that writes into the M5GFX `M5Canvas` RGB565 buffer.

The resulting harness compiles under **ESP-IDF 5.4.1** and uses the library’s **RAW** draw mode (palettized indices + RGB565 palette) to render a tiny embedded test GIF.

**Commit (code, esp32-s3-m5 repo):** `91a3eacd972b220774e2b9f45eec4472dd5c9c2d` — "atoms3r: vendor AnimatedGIF component + play single GIF (0014)"

### What I did
- Vendored upstream AnimatedGIF into a project-local ESP-IDF component:
  - `esp32-s3-m5/0014-atoms3r-animatedgif-single/components/animatedgif/`
  - Upstream pinned revision: `bc8c48cb9d749f2e1d9eed951256e97314c7c3e7`
- Added minimal ESP-IDF glue for Arduino timing helpers used by `AnimatedGIF::playFrame(bSync=true, ...)`:
  - implemented `millis()` and `delay(ms)` via `esp_timer_get_time()` and `vTaskDelay()`
  - declared component dependencies (`esp_timer`, `freertos`) so headers resolve cleanly
- Added a tiny test GIF from upstream as a C header (so we don’t commit binary blobs):
  - `main/assets/green.h` (63 bytes of GIF87a data)
- Implemented:
  - `GIFDraw(GIFDRAW*)`: indexed → RGB565 scanline writes into `canvas.getBuffer()`
  - Playback loop: `gif.open(green, sizeof(green), GIFDraw)` then `playFrame(false, &delay_ms, &ctx)` and `pushSprite()` + `waitDMA()`

### Why
- This is the minimal “truth harness” proving the port works end-to-end (memory buffer → decode → RGB565 canvas → present), without dragging in serial console UX or asset-directory work.

### What worked
- `idf.py build` succeeds under ESP-IDF 5.4.1 after declaring the right component dependencies.
- The harness now exercises the exact API shape we expect to reuse later:
  - `begin(GIF_PALETTE_RGB565_LE)`
  - `open(ptr,size,GIFDraw)`
  - `playFrame(false, &delay_ms, user_ctx)`

### What didn't work
- Initial build failed because upstream’s non-Arduino path still referenced Arduino timing helpers in `playFrame()`:
  - errors: `millis` / `delay` not declared
- Follow-up build failed because the new ESP-IDF header includes required declaring `esp_timer`/`freertos` in the component `REQUIRES`.

### What I learned
- AnimatedGIF’s **RAW** draw path already provides `pPalette` as RGB565 entries, so a simple “index → RGB565” write into the sprite buffer is enough for a first working player.

### What was tricky to build
- ESP-IDF component dependency wiring: if a public header includes `esp_timer.h`, the component must declare `REQUIRES esp_timer` (ESP-IDF helpfully points this out, but it’s easy to forget while porting Arduino-ish code).

### What warrants a second pair of eyes
- The `GIFDraw` clipping/math around `iX` offsets: it’s simple, but any off-by-one here becomes “random memory scribble” on embedded.
- Whether we should move the `millis()/delay()` shims out of the public header to reduce dependency surface (nice-to-have cleanup once correctness is proven).

### What should be done in the future
- Add at least one transparency/disposal stress GIF (not just `green.h`) and confirm correctness on real hardware.
- Consider enabling “COOKED + framebuffer” mode (`allocFrameBuf` + `setDrawType(GIF_DRAW_COOKED)`) as a correctness-first option for tricky GIFs.

### Code review instructions
- Start at:
  - `esp32-s3-m5/0014-atoms3r-animatedgif-single/main/hello_world_main.cpp` (`GIFDraw`, `playFrame` loop)
  - `esp32-s3-m5/0014-atoms3r-animatedgif-single/components/animatedgif/src/AnimatedGIF.h` (ESP-IDF timing shims)

### Technical details
- Build command used:
  - `source /home/manuel/esp/esp-idf-5.4.1/export.sh && cd .../0014-atoms3r-animatedgif-single && idf.py build`

## Step 4: Fix `open()` return-value handling (device log showed a false failure)

This step was driven by a real device run. The firmware booted, the display init and canvas allocation succeeded, but it logged `gif open failed: rc=1` and returned from `app_main()`. That looked like a decoder error at first, but it was actually a harness bug: **AnimatedGIF::open() returns 1 on success and 0 on failure** (not a `GIF_*` error code).

After fixing the harness to treat `open_ok==0` as failure (and to log `getLastError()` only in that case), the “open failed rc=1” false alarm goes away and the player can proceed into the frame loop.

**Commit (code, esp32-s3-m5 repo):** `65c6cd6cbcfd3262a1ab9a44e356645d1e3012c2` — "atoms3r: fix AnimatedGIF open() return handling (0014)"

### What I did
- Updated `esp32-s3-m5/0014-atoms3r-animatedgif-single/main/hello_world_main.cpp`:
  - Treat `AnimatedGIF::open(...)` return value as a boolean (1=success, 0=failure).
  - On failure, log `getLastError()` to get a real `GIF_*` error code.
  - Improved `playFrame()` handling by distinguishing:
    - `>0` (frame played, more frames likely)
    - `0` (EOF / restart)
    - `<0` (error, log `getLastError()`, reset)

### Why
- The runtime log showed a mismatch between what we *thought* the return contract was and what the library actually does.
- Without this fix, we bail out even when the decoder accepted the buffer.

### What worked
- The bug is fully explained by the library code: `AnimatedGIF::open()` returns the result of `GIFInit()` which is documented as `1` success / `0` failure.

### What didn't work
- The initial log message strongly implied a decoder failure; the real issue was our incorrect assumption about `open()`'s return semantics.

### What I learned
- For AnimatedGIF, treat these as boolean-like:
  - `open()`: 1 success / 0 failure
  - `playFrame()`: `>0` keep going, `0` done, `<0` error

### What was tricky to build
- The library uses both “boolean returns” and “error codes” in different places; it’s easy to mix them up if you only look at the enum list and not the function docs/implementation.

### What warrants a second pair of eyes
- Sanity-check the on-device behavior after this change (it should no longer exit `app_main()` right after open).

### Code review instructions
- Start at `esp32-s3-m5/0014-atoms3r-animatedgif-single/main/hello_world_main.cpp`, search for `open_ok` and `playFrame`.

## Step 5: Fix EOF/present semantics + yield to avoid watchdog reset

This step was driven by another on-device run: after the `open()` return-value fix, the device ran for a while but eventually hit the **task watchdog** with a backtrace inside `DrawNewPixels()` during `playFrame()`. In parallel, nothing new appeared on the screen beyond the initial solid-color sanity test.

The underlying issue is the library’s contract: `playFrame()` can return `0` (EOF) **even if it successfully decoded a frame**, and our harness only called `present_canvas()` when `prc > 0`. For a single-frame GIF, that meant we could decode a frame and then immediately treat it as EOF without ever presenting it. Also, in the EOF path we didn’t delay/yield, which can starve `IDLE0` and trip the watchdog over time.

**Commit (code, esp32-s3-m5 repo):** `515a6cc0a0bd3cb8ca2956e7d7e9a0bd8bc3e565` — "atoms3r: present frame on EOF + yield to avoid WDT (0014)"

### What I did
- Updated `0014` playback loop to:
  - call `present_canvas()` when `last_err == GIF_SUCCESS`, even if `playFrame()` returned `0`
  - add an explicit `vTaskDelay(1)` on the EOF/reset path to ensure `IDLE0` can run
  - keep robust error logging/reset for the `prc < 0` case

### Why
- Prevent “decoded but never shown” frames for single-frame and short GIFs.
- Prevent tight loops that starve the idle task and trigger task WDT.

### What warrants a second pair of eyes
- Confirm on real hardware that the screen now shows the decoded GIF frame (not just the initial solid colors) and that the watchdog no longer triggers over longer runs.

## Step 6: Switch to a multi-frame test GIF + log GIFINFO (verify real animation)

This step addresses the “is it moving?” confusion: our first embedded test asset (`green.h`) is effectively a static/single-frame GIF, so the expected behavior is a stationary square. To validate *actual animation*, we switched the harness to use the upstream `homer_tiny` test GIF (multi-frame) and added explicit logging of `GIFINFO` (frame count, duration, delays, and loop count) so we can confirm the decoder’s view of the asset on-device.

**Commit (code, esp32-s3-m5 repo):** `360f3770cf493f203c60cf1b0dbfc92754161561` — "atoms3r: use homer_tiny test gif + log GIFINFO (0014)"

### What I did
- Added `main/assets/homer_tiny.h` (upstream AnimatedGIF test header) to the `0014` harness.
- Updated the harness to open `homer_tiny` by default instead of `green`.
- Logged `GIFINFO` via `getInfo(&info)` and `getLoopCount()` after a successful `open()`.

### Why
- A static/single-frame GIF is a poor visual test; a multi-frame asset immediately proves the playback loop and present path.
- `GIFINFO` logging makes it obvious whether the decoder sees frames and what delays it is producing (helps debug “too fast/too slow/not moving” issues).

### What warrants a second pair of eyes
- Confirm that the large header (`homer_tiny.h`) is acceptable to keep in-repo for the tutorial harness, or if we should later replace it with a smaller custom multi-frame test GIF.

## Step 7: Fix color byte order + scale GIF to full 128x128 canvas (harness UX)

This step was driven by a visual mismatch on hardware: the animation played, but colors were clearly wrong (e.g. greens looked bluish/purple), and the decoded GIF only occupied a smaller region of the screen. Both issues are common “plumbing” problems:

- **RGB565 byte order**: some pipelines produce RGB565 values in MSB-first byte order (suitable for directly streaming to SPI) while our `M5Canvas` buffer expects CPU-native `uint16_t` ordering.
- **Canvas scaling**: `homer_tiny` is a 64×64 GIF, so without scaling it won’t fill the AtomS3R’s 128×128 visible area.

To make the harness easier to validate visually, we added two Kconfig-controlled features:

- `CONFIG_TUTORIAL_0014_GIF_SWAP_BYTES`: byte-swap RGB565 pixels before writing into the canvas buffer
- `CONFIG_TUTORIAL_0014_GIF_SCALE_TO_FULL_SCREEN`: nearest-neighbor integer scaling to fill the 128×128 canvas

Both are enabled by default in `sdkconfig.defaults` for tutorial `0014`.

**Commit (code, esp32-s3-m5 repo):** `994d5afbe13a818726de21be3aaaccbbb1a16b2e` — "atoms3r: fix GIF colors (swap bytes) + scale to 128x128 (0014)"

### What I did
- Updated `GIFDraw` to:
  - optionally `__builtin_bswap16()` RGB565 pixels (configurable)
  - optionally scale each decoded pixel into an \(N \times M\) block (configurable; for 64→128 this is 2×2)
  - compute scale factors/offsets based on `gif.getCanvasWidth/Height()` vs the 128×128 screen canvas
- Added Kconfig options in `main/Kconfig.projbuild` + enabled defaults in `sdkconfig.defaults`
- Added a one-line log reporting the active render settings (swap/scale factors, offsets)

### Why
- Color correctness and full-screen occupancy are the two fastest “human sanity checks” while iterating on a decoder port.

### What warrants a second pair of eyes
- Confirm the byte-swap assumption is correct across different GIFs (if a future asset already produces CPU-native RGB565, swap would break it — the Kconfig toggle should remain available).

## Step 8: Switch harness to AnimatedGIF COOKED mode (framebuffer-assisted) to reduce artifacts

This step targets the “DMA flutter” *look* that can happen even when the DMA synchronization is correct: many real GIFs rely on disposal methods + transparency, and if you do a naive RAW draw (just blit the current frame’s pixels) you can get residual pixels / partial-frame artifacts that look like flicker or flutter.

AnimatedGIF provides a correctness-first path: allocate its internal frame buffer and use **COOKED** draw mode. In this mode, the library merges pixels into its 8-bit canvas (handling disposal), then converts each scanline into ready-to-blit RGB565 pixels and calls `GIFDraw` with `pPixels` pointing to that cooked scanline.

**Commit (code, esp32-s3-m5 repo):** `d166462dae3b596b8f1e27d9e0661480780b4728` — "atoms3r: use AnimatedGIF COOKED mode to reduce artifacts (0014)"

### What I did
- Added `CONFIG_TUTORIAL_0014_GIF_USE_COOKED` (default **y**) and enabled it in `sdkconfig.defaults`.
- After `open()`, called:
  - `gif.allocFrameBuf(gif_alloc)` and
  - `gif.setDrawType(GIF_DRAW_COOKED)`
- Updated `GIFDraw` to support both cases:
  - RAW: palette indices → RGB565 through `pPalette`
  - COOKED: consume `pPixels` as an RGB565 scanline (already composited by the library)

### Why
- “Flutter” can be caused by correctness issues (disposal/transparency) not just transfer overlap.
- COOKED mode is an easy, high-leverage way to validate correctness before attempting micro-optimizations.

### What warrants a second pair of eyes
- Confirm the visual improvement is real on hardware (especially on GIFs with transparency/disposal), and that memory overhead is acceptable for the eventual GIF-console integration.

## Step 9: Correction — “DMA flutter” report was a false alarm; revert COOKED-mode change

This step corrects the narrative based on your follow-up: the apparent “DMA flutter” was misdiagnosed, and the RAW mode playback was already behaving correctly. In other words, COOKED mode was a valid experiment to improve GIF correctness for tricky assets, but it was **not required** to address the reported issue.

To keep the diary honest and useful for future readers, we keep the Step 8 record (what we tried and why) and then explicitly record that it was reverted after reassessing the evidence.

**Commit (code, esp32-s3-m5 repo):** `d3ee436` — Revert "atoms3r: use AnimatedGIF COOKED mode to reduce artifacts (0014)"

### What I did
- Reverted the COOKED-mode change in the `0014` harness to return to the previously-working RAW mode behavior.

### Why
- Avoid unnecessary framebuffer allocation/memory overhead when the reported issue wasn’t real.

### What I learned
- It’s easy to conflate different artifacts:
  - DMA overlap/tearing (true “transfer issue”)
  - disposal/transparency artifacts (decoder correctness)
  - frame pacing / delay handling (timing)
  Keeping the “symptom → hypothesis → validation” chain explicit in logs/diary prevents cargo-cult fixes.

## Step 10: Prepare AnimatedGIF for reuse by 0013 (choose symlinked shared component path)

This step begins the “Phase B” integration work: we want the existing console+button firmware (`0013`) to play **real GIFs**. Before touching playback logic, we need a stable way for `0013` (and other tutorials) to depend on the AnimatedGIF component without copying it around.

The initial instinct was to `git mv` the component out of the `0014` harness into a shared folder, but we decided against that churn and instead created a **symlink-based shared entry point**.

### What I did
- Kept the canonical AnimatedGIF component location unchanged:
  - `esp32-s3-m5/0014-atoms3r-animatedgif-single/components/animatedgif/`
- Added a shared “pointer path” for reuse:
  - `esp32-s3-m5/components/animatedgif` (symlink) → `../0014-atoms3r-animatedgif-single/components/animatedgif`
- Updated `0013` to see shared components by adding an extra component dir in:
  - `esp32-s3-m5/0013-atoms3r-gif-console/CMakeLists.txt`

### Why
- Avoid a noisy repo-wide move while still giving `0013` an easy, stable include path.
- Make it trivial for future tutorials to reuse AnimatedGIF without duplicating source or maintaining multiple copies.

### What worked
- The symlink makes AnimatedGIF look like a normal ESP-IDF component folder to projects using `EXTRA_COMPONENT_DIRS`.

### What should be done next
- In `0013`, switch `main/CMakeLists.txt` to depend on `animatedgif` and replace the mock animation rendering loop with a GIF player that:
  - enumerates `/storage/gifs/*.gif`
  - opens selected GIF and decodes via `AnimatedGIF::open(...)` + `playFrame(...)`
  - presents via `pushSprite()` + `waitDMA()`

## Step 11: Integrate real GIF playback into 0013 (FATFS `storage` + AnimatedGIF + console/button)

This step implements the core “Phase B” goal: the AtomS3R console+button firmware (`0013`) now plays **real GIFs**, selected via `esp_console` commands and cycled via a hardware button. GIF files are loaded from a flash-bundled **FATFS** partition named `storage`.

### What I did
- Wired `0013` to reuse AnimatedGIF as a normal ESP-IDF component:
  - Added `esp32-s3-m5/components/animatedgif` as a **symlink** pointing at the canonical component in the `0014` harness.
  - Added `../components` to `EXTRA_COMPONENT_DIRS` in `esp32-s3-m5/0013-atoms3r-gif-console/CMakeLists.txt`.
  - Updated `0013/main/CMakeLists.txt` to `REQUIRES animatedgif` and added `gif_storage.cpp`.
- Implemented FATFS mount + directory scan + file loading:
  - New module: `esp32-s3-m5/0013-atoms3r-gif-console/main/gif_storage.{h,cpp}`
  - Mounts `storage` at `/storage` via `esp_vfs_fat_spiflash_mount_rw_wl`
  - Lists GIFs from `/storage/gifs` (preferred) with fallback to `/storage` (flat images)
  - Reads the selected `.gif` into RAM and calls `AnimatedGIF::open(buf, len, GIFDraw)`
- Replaced the mock animation registry + render loop with real AnimatedGIF playback:
  - `list` now prints GIF assets found on the storage partition
  - `play <id|name>` opens a GIF and starts decoding
  - `next` cycles to the next GIF and reopens it
  - `stop` pauses decode loop
  - `info` prints current playback state + GIF geometry
- Reused the known-good `GIFDraw` scanline renderer from the `0014` harness (RGB565 palette + optional byte-swap + optional integer scaling).
- Added/updated Kconfig knobs for GIF playback:
  - min frame delay floor (avoid tight loops)
  - max asset count and max path length
  - swap-bytes and scale-to-full-screen toggles
- Updated `0013/README.md` to reflect “real GIFs” and document mount/scan paths.

### Why
- This connects the already-proven pieces:
  - `0014` proves AnimatedGIF decode → `GIFDraw` → M5Canvas present works
  - `0013` proves the control plane (console + button + state machine) works
  Now `0013` is a single end-to-end “pick GIF → play GIF” firmware.

### What worked
- `idf.py build` succeeds for `0013` with the AnimatedGIF component pulled via the shared symlink path.
- The storage loader pattern is straightforward and keeps the console UX responsive (control-plane events are still queue-driven).

### What didn't work
- Initial build failures were caused by incorrect component names in `main/CMakeLists.txt` (`esp_vfs` / `esp_vfs_fat`), which don’t exist as standalone components in ESP-IDF 5.4.1. Removing those fixed the build.

### What should be done next
- Finish the host-side bundling workflow:
  - build a FATFS image (`storage.bin`) containing GIF files (using ESP-IDF `fatfsgen.py`)
  - flash it via `parttool.py write_partition --partition-name=storage`
- Validate on real hardware (console commands + button cycling + a few representative GIFs).

## Step 12: Add reproducible host workflow for bundling + flashing GIF files (fatfsgen + parttool)

This step makes the “flash-bundled GIFs” workflow concrete and reproducible. Instead of hand-running `fatfsgen.py` or hunting down `parttool.py` invocations, `0013` now includes small helper scripts that encode a directory of GIFs into a FATFS image and flash it into the `storage` partition.

### What I did
- Added `esp32-s3-m5/0013-atoms3r-gif-console/make_storage_fatfs.sh`:
  - uses ESP-IDF `components/fatfs/fatfsgen.py`
  - defaults to encoding `./assets` into a `storage.bin` image sized to 1MiB (matching `partitions.csv`)
  - requires `IDF_PATH` set (ESP-IDF environment already active)
- Added `esp32-s3-m5/0013-atoms3r-gif-console/flash_storage.sh`:
  - runs `parttool.py write_partition --partition-name=storage --input storage.bin`
  - takes `PORT` as the first arg (defaults to `/dev/ttyACM0`)
- Added `assets/.gitignore` + `assets/README.md` so developers can drop local `*.gif` files in `assets/` without accidentally committing binaries.
- Updated the firmware scan logic to support both layouts:
  - preferred: `/storage/gifs`
  - fallback: `/storage` (flat FATFS image)

### Why
- End-to-end iteration speed: updating GIF files should not require rebuilding firmware; reflashing a single partition is fast.
- “Works on my machine” avoidance: explicit scripts document the exact tool invocation and partition name.

### What should be done next
- Run the scripts on a real device:
  - `./make_storage_fatfs.sh && ./flash_storage.sh /dev/ttyACM0`
  - then flash the app (`idf.py flash`) if needed and validate the console commands.

## Step 13: Add a conversion script: crop arbitrary GIFs to 128×128 for AtomS3R

This step adds a small host-side helper to normalize source GIFs to the AtomS3R-friendly **128×128** format by **center-cropping** (not padding). This is useful because many internet GIFs aren’t square; padding would waste pixels and look bad on the 128×128 display.

### What I did
- Added `esp32-s3-m5/0013-atoms3r-gif-console/convert_assets_to_128x128_crop.sh`:
  - converts all `assets/*.gif` to `*_128x128.gif`
  - uses ImageMagick (`magick` or `convert`) when available (better preservation of GIF timing/looping)
  - falls back to `ffmpeg` (fixed FPS; configurable via `--fps`)
- Documented the script in `assets/README.md`.

### Why
- Consistent “asset contract” for the on-device player: 128×128 frames mean predictable scaling and less per-frame work.
- Cropping (instead of padding) uses the limited screen real estate effectively.

## Step 14: Attempt to bundle 4× 128×128 GIFs into FATFS `storage` (hit size limit) + decide to increase partition

This step attempted to package and flash the four new `*_128x128.gif` outputs into the FATFS `storage` partition image (`storage.bin`). The attempt failed because the current `storage` partition size (1MiB) is too small for these assets.

### What I did
- Verified the four converted assets exist:
  - `assets/*_128x128.gif`
- Attempted to generate a FATFS image sized to match `partitions.csv` (1MiB) using ESP-IDF `fatfsgen.py`.

### What happened / what didn't work
- `fatfsgen.py` failed with:
  - `fatfs_utils.exceptions.NoFreeClusterException: No free cluster available!`
- This indicates the FATFS image is out of space (free clusters exhausted) before all 4 GIF files could be written.

### Why
- We need an end-to-end “bundle + flash + play” loop with real 128×128 GIFs; bundling into `storage.bin` is the intended workflow.

### What we decided
- Increase the `storage` partition size. The firmware already assumes an 8MB flash configuration (common on AtomS3R), and the current partition table uses a 4MB app partition, so we should have room to grow `storage` substantially.

### What should be done next
- Double-check effective flash size (and resulting max available space for `storage`) using the project config.
- Increase `storage` size in `esp32-s3-m5/0013-atoms3r-gif-console/partitions.csv`.
- Rebuild (partition table changes) and regenerate `storage.bin` with the new size.
- Flash firmware + partition table (`idf.py flash`) and then flash the `storage` partition via `parttool.py`.

### Additional detail (why 1MiB was hopeless)
- One of the converted GIFs (`*_128x128.gif`) is ~4.7MiB by itself, so it cannot fit in a 1MiB FATFS image.
- With an 8MB flash target and a 4MB app partition, the *maximum* storage partition would only be ~3.9MB, which still wouldn’t fit that single 4.7MB asset.
- Therefore, the correct fix is not “bump storage from 1MiB to 2MiB”, but rather to **reduce the app partition size** (the firmware is currently far smaller than 2MB) and give the space to `storage`.

## Step 15: Confirm flash size + shrink app partition + expand storage (to fit 4.7MiB GIF)

This step confirms the effective flash size and adjusts the partition layout so we can actually store the converted GIF assets on-device.

### What I did
- Confirmed the project is configured for **8MB flash**:
  - `CONFIG_ESPTOOLPY_FLASHSIZE_8MB=y`
- Checked the app partition utilization:
  - firmware binary is ~`0x6a980` bytes and fits comfortably in a **2MiB** app partition.
- Updated the partition table to allocate more space to `storage`:
  - `factory` app partition: **4M → 2M**
  - `storage` FATFS partition: **1M → 0x5F0000 (~6.0MiB)**
- Rebuilt to regenerate the partition table and confirm the app still fits:
  - build shows `storage` at `0x210000` with size `6080K`

### Why
- One asset is ~4.7MiB; we need a `storage` partition larger than that plus FAT overhead.
- 8MB flash gives us enough headroom if we don’t waste 4MB on the app partition.

## Step 16: Trim the “too big” GIF to 30 frames, bundle 4 GIFs, and flash device

This step completes the first real “assets-in-flash” loop on hardware: we trimmed the problematic giant GIF down to a small 30-frame loop, created a FATFS `storage.bin` containing 4 GIFs, and flashed both the firmware (with the updated partition table) and the storage partition to the device.

### What I did
- Investigated why the large GIF was huge:
  - It had **626 frames** at roughly 350×350, so even after cropping it remained large.
- Created a 30-frame 128×128 version:
  - Script: `esp32-s3-m5/0013-atoms3r-gif-console/trim_gif_128x128_frames.sh`
  - Output: `*_128x128_30f.gif` (30 frames, ~224KiB)
- Built a FATFS image containing the 3 small 128×128 GIFs plus the new 30-frame trimmed GIF:
  - `./make_storage_fatfs.sh <tmpdir-with-4-gifs> ./storage.bin`
- Flashed firmware + partition table:
  - `idf.py -p /dev/ttyACM0 flash`
- Flashed the `storage` partition (FATFS image) via parttool:
  - `./flash_storage.sh /dev/ttyACM0 storage.bin`

### What worked
- Device flash succeeded and confirmed:
  - ESP32-S3-PICO-1 with **8MB flash** and **8MB PSRAM**
- Storage partition write succeeded:
  - wrote `storage.bin` at offset `0x210000`

### Why
- 30 frames is enough to get a satisfying loop while staying within flash storage constraints.
- This gives us a practical “artist/dev workflow”: normalize GIF → bundle → flash partition → select via console.

## Step 17: Fix “malloc failed” when opening larger GIFs (stream from FATFS instead of reading whole file)

After flashing and listing assets, the device could only play the smallest GIF. Attempts to play the ~200–600KB GIFs failed with logs like:

- `gif_storage: malloc failed for 633484 bytes`
- `failed to read gif ... (ESP_ERR_NO_MEM)`

Root cause: the current implementation reads the entire file into RAM (`malloc(size)`), but internal heap free on-device is only ~300KB. Also, in this project config PSRAM is currently disabled (`CONFIG_SPIRAM is not set`), so we can’t rely on large allocations succeeding.

### What I did
- Switched AnimatedGIF open path in `0013` to use its **file callback API** instead of a full-buffer RAM open:
  - `AnimatedGIF::open(path, open_cb, close_cb, read_cb, seek_cb, GIFDraw)`
- Implemented callbacks using `FILE*` from FATFS (`fopen`/`fread`/`fseek`/`fclose`), and ensured the callbacks update `GIFFILE::iPos`.
- Removed the “read whole file into malloc buffer” path from the player.

### Why
- This makes GIF size much less coupled to internal heap availability (we only need small working buffers; the decoder already has its own buffers).
- It also keeps the door open to enabling PSRAM later, but it’s no longer required for “moderately sized” assets.
