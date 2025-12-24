# GIF decoder survey (ESP-IDF): candidates, constraints, and recommendation

## Executive summary

We investigated four GIF-decoding options for ESP32-S3 / ESP-IDF 5.4.1, with an explicit focus on: (1) correctness (disposal modes, transparency, local palettes), (2) **flash-mapped / memory buffer input**, and (3) a practical path to **RGB565** for an M5GFX 128×128 canvas.

**Recommendation:** use **AnimatedGIF** as the primary choice for the AtomS3R serial-controlled GIF player because it is explicitly designed for microcontrollers, supports *memory-backed sources* and a *scanline draw callback*, and can expose **RGB565 palette entries directly** (or even “cooked” pixels when you allocate a frame buffer) for fast M5GFX blits. :contentReference[oaicite:0]{index=0}  
**Fallback:** if you want a “more conventional” C library with strong decoding semantics and are OK with converting from RGBA8888 → RGB565, use **libnsgif (esp-idf-libnsgif)**. It’s already packaged as an ESP-IDF component and is MIT licensed. :contentReference[oaicite:1]{index=1}

## Evaluation criteria

Constraints (from ticket context):

- **Target**: ESP32-S3, ESP-IDF 5.4.1
- **Display**: 128×128 RGB565 canvas (M5GFX)
- **Asset I/O**: prefer **flash-mapped blob** (e.g., `esp_partition_mmap`) over filesystem
- **Correctness**: disposal modes + transparency + local palettes should work (or we must clearly scope limits)
- **Integration**: reasonable effort to integrate into an ESP-IDF (CMake) project
- **Performance/RAM**: avoid unnecessary copies; prefer streaming/scanline decode where possible

## Candidate comparison table

| Library | License | ESP-IDF Support | Input Model | Output Model | Correctness notes | Integration Effort | Recommendation |
|---|---|---:|---|---|---|---:|---|
| **AnimatedGIF** | Apache-2.0 :contentReference[oaicite:2]{index=2} | ⚠️ Arduino-first, but portable C/C++ core | **Memory buffer** open + optional file callbacks :contentReference[oaicite:3]{index=3} | Scanline callback with **RGB565 palette**; optional full-frame buffer via `allocFrameBuf()` :contentReference[oaicite:4]{index=4} | Disposal method surfaced to callback; can be handled by library when using frame buffer :contentReference[oaicite:5]{index=5} | **Medium** | ⭐ **Primary** |
| **esp-idf-libnsgif** (NetSurf libnsgif) | MIT :contentReference[oaicite:6]{index=6} | ✅ ESP-IDF component repo w/ CMakeLists :contentReference[oaicite:7]{index=7} | **Memory buffer** scanning/decoding; client holds source data :contentReference[oaicite:8]{index=8} | Decodes to **32bpp RGBA**; client controls channel order :contentReference[oaicite:9]{index=9} | Designed for robust decoding; only one frame fully decoded at a time :contentReference[oaicite:10]{index=10} | **Low–Medium** | ✅ **Fallback** |
| **gifdec** (lecram) | Public domain :contentReference[oaicite:11]{index=11} | ⚠️ “plain C” (no ESP-IDF packaging) | `gd_open_gif(const char *fname)` (file-path centric) :contentReference[oaicite:12]{index=12} | Canvas state in indexed bytes; `gd_render_frame()` outputs RGB888 :contentReference[oaicite:13]{index=13} | Requires **global color table**; supports local palettes but needs `gd_render_frame()` for correctness :contentReference[oaicite:14]{index=14} | **Medium–High** | ⚠️ Only if you accept constraints |
| **LVGL GIF** | LVGL core is MIT; extra deps depend on version | ✅ if you already use LVGL | LVGL image src: file or variable :contentReference[oaicite:15]{index=15} | LVGL widget renders into LVGL draw buffers | RAM cost noted (e.g. depth 16 ⇒ 4×W×H) :contentReference[oaicite:16]{index=16} | **High** (adds LVGL) | ❌ Only if you’re already on LVGL |

## Detailed findings

### AnimatedGIF (bitbank2)

**Repository**: https://github.com/bitbank2/AnimatedGIF :contentReference[oaicite:17]{index=17}

**License**: Apache-2.0 :contentReference[oaicite:18]{index=18}

**API model**
- Two `open()` paths:
  - **From memory**: `open(uint8_t *pData, int iDataSize, GIF_DRAW_CALLBACK *pfnDraw)` :contentReference[oaicite:19]{index=19}
  - **From “file”**: `open(char *szFilename, ... open/close/read/seek callbacks ...)` :contentReference[oaicite:20]{index=20}
- Frame progression is forward-only via `playFrame(bool bSync, int *delayMilliseconds)` (returns 0 at end). :contentReference[oaicite:21]{index=21}
- Provides canvas size getters and an info scan API (`getInfo(GIFINFO*)`) for frame count/duration. :contentReference[oaicite:22]{index=22}

**Input**
- Best fit for us: **memory open** + data coming from `esp_partition_mmap` (direct pointer + length). This matches the library’s explicit “play from memory” path. :contentReference[oaicite:23]{index=23}

**Output**
- Core rendering contract is a **scanline draw callback**: `typedef void (GIF_DRAW_CALLBACK)(GIFDRAW *pDraw);` :contentReference[oaicite:24]{index=24}
- `GIFDRAW` provides:
  - (x,y) offsets, line index, width
  - `pPixels` = 8-bit palette indices for the line
  - `pPalette` = **RGB565 palette entries** (little/big endian chosen via `begin(iEndian)`) :contentReference[oaicite:25]{index=25}
  - transparency + disposal fields (`ucTransparent`, `ucHasTransparency`, `ucDisposalMethod`, etc.) :contentReference[oaicite:26]{index=26}

**Correctness**
- Disposal information is exposed per line via `ucDisposalMethod` and transparency fields. :contentReference[oaicite:27]{index=27}
- If you can afford a full canvas buffer, `allocFrameBuf()` lets the library handle disposal + transparency internally, so your draw callback can become “just blit pixels.” :contentReference[oaicite:28]{index=28}  
- Additionally, `setDrawType(GIF_DRAW_COOKED)` can request palette translation inside the library (requires the frame buffer). :contentReference[oaicite:29]{index=29}

**ESP-IDF integration**
- Not an ESP-IDF component out of the box, but the core is a C/C++ library with explicit callbacks; practical path is to vendor it as `components/animatedgif` with a CMakeLists and compile the sources.
- You must decide how to implement timing (`bSync`)—likely set `bSync=false` and handle delays in your animation loop.

**Flash-mapped input effort**
- **Low**: use the memory `open()` signature with the `mmap` pointer and size. :contentReference[oaicite:30]{index=30}

**RGB565 output effort**
- **Low**: palette is already RGB565 and the draw callback is scanline-based. :contentReference[oaicite:31]{index=31}  
- With `allocFrameBuf()` + cooked mode, you can get “ready-to-send” pixel runs rather than translating indices yourself. :contentReference[oaicite:32]{index=32}

**Verdict**
- Best overall fit for “serial-controlled playback to M5GFX canvas” given our constraints.

---

### esp-idf-libnsgif (NetSurf libnsgif port)

**Repository**: https://github.com/UncleRus/esp-idf-libnsgif :contentReference[oaicite:33]{index=33}  
**Upstream model reference**: NetSurf libnsgif README/API (example: netsurf-plan9 mirror) :contentReference[oaicite:34]{index=34}

**License**: MIT :contentReference[oaicite:35]{index=35}

**API model (upstream characteristics)**
- Modern libnsgif exposes a “scan/prepare/decode” flow where you feed the source data and decode frames on demand. It explicitly notes:
  - source data is scanned before decoding
  - **only one frame is fully decoded to a bitmap at a time**
  - client provides bitmap callbacks and receives **32bpp** output (RGBA ordering configurable) :contentReference[oaicite:36]{index=36}

**Input**
- Library expects the GIF bytes in memory and requires the client to keep them until destroy (important for flash-mapped usage: mapping must remain active for the life of the animation). :contentReference[oaicite:37]{index=37}

**Output**
- 32bpp RGBA bitmap; for our display we must convert to RGB565 each frame (or per updated region). :contentReference[oaicite:38]{index=38}

**Correctness**
- Designed as a robust decoder; in practice this tends to be “safer” than microcontroller-focused players when you care about corner cases. (Still: verify with test GIFs that stress disposal/local palettes.)

**ESP-IDF integration**
- The port repo is already structured as an ESP-IDF component (CMakeLists + clone-into-components instructions). :contentReference[oaicite:39]{index=39}  
- You’ll still need to bridge decoded RGBA → M5GFX canvas and handle timing/looping using the library’s frame metadata.

**Flash-mapped input**
- **Low**: fits well with `esp_partition_mmap` so long as you keep the mapping alive.

**RGB565 output**
- **Medium**: add a fast RGBA8888→RGB565 converter and decide whether you redraw the full canvas each frame or only dirty rects.

**Verdict**
- Great “MIT + robust decoding + memory input” option, but slightly more work (and bandwidth) than AnimatedGIF for RGB565.

---

### gifdec (lecram)

**Repository**: https://github.com/lecram/gifdec :contentReference[oaicite:40]{index=40}

**License**: Public domain :contentReference[oaicite:41]{index=41}

**API model**
- Open/close:
  - `gd_open_gif(const char *fname)` / `gd_close_gif(gd_GIF *gif)` :contentReference[oaicite:42]{index=42}
- Frame advance:
  - `gd_get_frame(gd_GIF *gif)` reads the next frame into an internal indexed canvas buffer :contentReference[oaicite:43]{index=43}
- Full-canvas RGB output:
  - `gd_render_frame(gd_GIF *gif, uint8_t *buffer)` writes **RGB888** for the whole canvas (buffer size `W*H*3`). :contentReference[oaicite:44]{index=44}
- Delay/looping exposed via `gif->gce.delay` and `gif->loop_count`. :contentReference[oaicite:45]{index=45}

**Correctness**
- The README explicitly calls out how local palettes work: if local palettes are used, indices alone aren’t sufficient; you must call `gd_render_frame()` to get correct full-canvas RGB state. :contentReference[oaicite:46]{index=46}
- Hard limitation: **no support for GIF files without a global color table**. :contentReference[oaicite:47]{index=47}

**ESP-IDF integration**
- No ESP-IDF packaging; you’d vendor the C files and build as a component.
- The bigger mismatch is the input model: the documented `gd_open_gif()` takes a filename; our preferred model is a flash-mapped blob. :contentReference[oaicite:48]{index=48}

**Flash-mapped input effort**
- **High** unless you either:
  - add an alternate “open-from-memory” path, or
  - build a VFS shim that presents the blob as a file.

**RGB565 output effort**
- **Medium**: you will convert RGB888 to RGB565 (plus you may want to do this per-line to avoid an extra full RGB888 frame buffer).

**Verdict**
- Viable only if we constrain the asset pipeline (must include a global palette) and accept extra integration work for non-filesystem input.

---

### LVGL GIF support

**Docs**: LVGL “GIF decoder” page :contentReference[oaicite:49]{index=49}  
**Legacy repo** (archived; merged into LVGL): lvgl/lv_lib_gif :contentReference[oaicite:50]{index=50}

**Model**
- `lv_gif_create()` + `lv_gif_set_src(obj, src)` where `src` may be a file path or a variable (C array). :contentReference[oaicite:51]{index=51}
- Memory requirements are explicitly documented (for LV_COLOR_DEPTH 16: **4×W×H** bytes). :contentReference[oaicite:52]{index=52}

**Verdict**
- Only reasonable if your UI stack is already LVGL. For an M5GFX canvas app, adding LVGL just for GIF is likely unjustified.

## Recommendation

### Recommended approach: **AnimatedGIF** (primary)

**Why**
- Best alignment with our constraints:
  - **memory-backed open()** is first-class :contentReference[oaicite:53]{index=53}
  - produces data in a form that maps cleanly to M5GFX: scanlines + **RGB565 palette** :contentReference[oaicite:54]{index=54}
  - disposal/transparency are either (a) exposed for you to handle in draw callback, or (b) handled internally if you allocate a frame buffer :contentReference[oaicite:55]{index=55}

**How we’d use it on AtomS3R**
1. Map the GIF asset partition with `esp_partition_mmap` and obtain `(uint8_t*)ptr` + `size`.
2. `gif.begin(/* endian for RGB565 */)` :contentReference[oaicite:56]{index=56}
3. Optional but recommended for correctness/simplicity on ESP32-S3: `gif.allocFrameBuf(...)` then `gif.setDrawType(GIF_DRAW_COOKED)` to simplify the draw callback. :contentReference[oaicite:57]{index=57}
4. `gif.open(ptr, size, GIFDraw)` and then repeatedly call `playFrame(false, &delay_ms)`; blit to M5GFX canvas and `waitDMA()` between frames.

### Fallback: **esp-idf-libnsgif**

Use this if you want MIT + robust decoding and don’t mind adding an RGBA8888→RGB565 conversion step (and potentially redrawing per frame/dirty rect). :contentReference[oaicite:58]{index=58}

### Do not recommend (for our current constraints)
- **gifdec** unless we accept “global color table required” and do extra work to support flash-mapped inputs. :contentReference[oaicite:59]{index=59}
- **LVGL GIF** unless LVGL is already part of the product. :contentReference[oaicite:60]{index=60}

## Next steps (if we pick AnimatedGIF)

1. Vendor AnimatedGIF into `components/animatedgif/` with a minimal ESP-IDF `CMakeLists.txt`.
2. Implement `GIFDraw` that writes RGB565 scanlines into the M5GFX canvas (or blits cooked runs).
3. Build a tiny “known-bad” test set of GIFs to validate:
   - local palettes
   - transparency
   - disposal method 2/3
4. Measure:
   - peak RAM (with and without `allocFrameBuf`)
   - frame decode time vs target FPS
   - serial command latency impact while decoding
