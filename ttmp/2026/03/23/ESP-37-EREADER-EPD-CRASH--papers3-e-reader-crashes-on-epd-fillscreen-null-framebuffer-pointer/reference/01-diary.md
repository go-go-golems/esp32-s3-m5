---
Title: Diary
Ticket: ESP-37-EREADER-EPD-CRASH
Status: active
Topics:
    - papers3
    - esp-idf
    - esp32s3
    - e-paper
    - m5gfx
    - bugfix
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - "/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0080-papers3-ereader/main/app_main.cpp:Entry point - tried both core 0 init and core 1 init"
    - "/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0080-papers3-ereader/main/ereader_app.cpp:FullRefresh at line 267 triggers the crash via fillScreen"
    - "/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp:EPD panel driver - _buf allocated at 249, null deref at 436"
    - "/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/components/M5GFX/src/M5GFX.cpp:PaperS3 board autodetect now sets default rotation before M5Unified clear_display"
    - "/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0078-papers3-gnosis-layout/main/app_main.cpp:Working reference - gnosis 0078 does M5.begin on core 1 and works fine"
ExternalSources: []
Summary: "Investigation diary for the PaperS3 e-reader EPD boot crash. The real failure was a 43,520-byte LUT allocation forced into DMA-capable internal RAM, followed by a separate PaperS3 default-rotation bug during M5Unified clear_display."
LastUpdated: 2026-03-23
WhatFor: "Handoff document for embedded engineer to understand the real EPD init failure, the PaperS3 boot-time rotation issue, and the validated hardware fix."
WhenToUse: "When investigating or fixing the EPD crash in the e-reader firmware."
---

# Diary

## Goal

Track the investigation of a LoadProhibited crash in the PaperS3 e-reader firmware (0080). The EPD panel driver's internal framebuffer pointer (`_buf`) is null when `fillScreen` is called during the first `FullRefresh()`, causing a null dereference in `Panel_EPD::writeFillRectPreclipped`.

## The Bug

The e-reader firmware boots, mounts SPIFFS, loads the book index, then calls `OpenBook(0)` -> `SwitchScreen(READING)` -> `FullRefresh()` -> `M5.Display.fillScreen(0xFFFFFF)` -> crash.

**Crash signature:**
```
I (1783) lgfx_epd_dbg: fillrect xs=0 ys=0 xe=959 ye=539 w=960 h=540 raw=65535 mode=1 buf=0x0 ext=0 stride=480
Guru Meditation Error: Core  1 panic'ed (LoadProhibited). Exception was unhandled.
PC: Panel_EPD::writeFillRectPreclipped at Panel_EPD.cpp:436
EXCVADDR: 0x00000000
```

The key evidence is `buf=0x0` in the debug log. The EPD driver's `_buf` pointer is null.

## Step 1: Initial encounter - stack overflow masked the real bug

The very first boot attempt crashed with a FreeRTOS stack overflow in the `ereader_ui` task (8 KB stack). The paginator had two `char[8192]` buffers on the stack in `EnsurePage()` and `GetPageText()`.

### What I did
- Made both buffers `static` to move them off the stack
- Increased task stack from 8192 to 16384 bytes

**Commit:** `d0d557a` — "fix(ereader): resolve stack overflow by making read buffers static"

### Result
Stack overflow fixed. Revealed the real bug underneath: the null framebuffer crash.

## Step 2: Attempted fix - move M5.begin to core 0

**Hypothesis:** The EPD framebuffer (PSRAM) allocation inside `M5.begin()` fails or is incomplete when called from a non-main-task on core 1.

### What I did
- Split `Run()` into `Init()` (called from `app_main` on core 0) and `RunLoop()` (called from core 1 task)
- `Init()` calls `M5.begin()` + `MountStorage()` on the main task
- `RunLoop()` only does `OpenBook` + main loop on core 1

**Commit:** `5b1e146` — "fix(ereader): init display on main task to avoid null framebuffer crash"

### Result
**Did NOT fix the crash.** Identical `buf=0x0` error. Hypothesis was wrong.

## Step 3: Analysis - what we know and don't know

### Confirmed facts

1. **`buf=0x0` in the debug log** — this is `Panel_EPD::_buf`, printed at Panel_EPD.cpp line ~430 just before the crash.

2. **`_buf` is allocated in `Panel_EPD::init()` at line 249:**
   ```cpp
   _buf = (uint8_t *)heap_caps_aligned_alloc(16, (panel_w * panel_h) / 2, MALLOC_CAP_SPIRAM);
   // For 960x540: (960 * 540) / 2 = 259,200 bytes (~253 KB from PSRAM)
   ```

3. **If the allocation at line 249 fails, `_buf` is set to null at line 257** and `init()` bails:
   ```cpp
   if (!_step_framebuf || !_buf || !_dma_bufs[0] || !_dma_bufs[1] || !_lut_2pixel) {
       if (_buf) { heap_caps_free(_buf); _buf = nullptr; }
       // ...
   }
   ```

4. **M5.begin with `clear_display = true` does its own fill** — and it does NOT crash. So either `M5.begin` uses a code path that bypasses `_buf`, or `_buf` is valid at that point and gets freed/nulled later.

5. **0078-papers3-gnosis-layout uses the identical pattern** (`M5.begin` on core 1 -> `FullRefresh` with `fillScreen` + `epd_quality`) and works perfectly. Same M5GFX component, same hardware, same board detection.

6. **The EPD mode in the crash is `mode=1` (epd_quality).** The `clear_display = true` initial fill uses `mode=0` (default). Might be relevant.

### The puzzle: why does 0078 work but 0080 doesn't?

The call sequence differs:

**0078 (works):**
```
M5.begin(cfg)          // clear_display = true, fills screen
setRotation(1)
BuildCurrentScreen()   // ~50 nodes, fast
LayoutScreen()         // microseconds
FullRefresh()          // fillScreen -> WORKS
```

**0080 (crashes):**
```
M5.begin(cfg)          // clear_display = true, fills screen
setRotation(1)
MountStorage()         // SPIFFS mount + index parse (~100ms)
OpenBook(0)
  ComputeTotalPages()  // paginate entire file (~50ms)
  SwitchScreen(READING)
    BuildReadingScreen()
    LayoutScreen()
    LoadCurrentPage()  // SPIFFS read + paginate
    FullRefresh()      // fillScreen -> CRASH (buf=0x0)
```

The gap between `M5.begin()` and the first real draw is much larger in 0080. Something happens in that gap.

### Leads to pursue (ranked by likelihood)

1. **Lazy/deferred buffer allocation.** `Panel_EPD::_buf` might not be allocated in `init()` but in `post_init()` or on the first `startWrite()`/`endWrite()` cycle. If `clear_display = true` does a write cycle, that would trigger the allocation. But if something about our longer init gap means the deferred init hasn't run yet, or the buffer was freed after clear_display's fill, `_buf` would be null.
   - **Action:** Read `Panel_EPD::init()`, `post_init()`, and the `startWrite()` path carefully. Check if `_buf` allocation is conditional on state that changes between the clear_display fill and our FullRefresh.

2. **Buffer freed after clear_display fill.** Maybe `clear_display` allocates a temporary buffer for the fill and frees it afterward, expecting the application to trigger a "real" allocation later via a different code path.
   - **Action:** Search Panel_EPD.cpp for `heap_caps_free(_buf)` calls. Check if there's a cleanup path after the initial fill.

3. **`setRotation(1)` reinitializes the panel.** Rotation changes might destroy and recreate internal state. If `_buf` is freed during rotation change and only re-allocated on the next write cycle...
   - **Action:** Check `Panel_EPD::setRotation()` or `Panel_EPD::setWindow()` for buffer reallocation.

4. **SPIFFS I/O corrupts the framebuffer pointer.** Unlikely but worth ruling out — the SPIFFS mount at 4769/474641 bytes uses the same SPI bus infrastructure. A DMA conflict could theoretically corrupt memory.
   - **Action:** Try removing all SPIFFS code and see if a plain `M5.begin()` -> delay(2000) -> `fillScreen()` works.

5. **`ComputeTotalPages()` exhausts PSRAM.** The paginator uses 8 KB static buffers, the page offset table is ~8 KB. Total PSRAM usage is low (~270 KB for framebuffer + 16 KB for our data out of 8 MB). Very unlikely.
   - **Action:** Check `heap_caps_get_free_size(MALLOC_CAP_SPIRAM)` before the crash.

### Minimal reproducer to try

```cpp
extern "C" void app_main(void) {
    auto cfg = M5.config();
    cfg.clear_display = true;
    M5.begin(cfg);
    M5.Display.setRotation(1);

    vTaskDelay(pdMS_TO_TICKS(2000));  // simulate the SPIFFS init gap

    M5.Display.setEpdMode(epd_mode_t::epd_quality);
    M5.Display.startWrite();
    M5.Display.fillScreen(0xFFFFFF);   // does this crash?
    M5.Display.endWrite();
    M5.Display.waitDisplay();
}
```

If this crashes, the bug is in the delay/timing. If it works, the bug is in something our init code does (SPIFFS, paginator, etc).

## Reproduction

```bash
cd 0080-papers3-ereader
bash -l -c 'unset IDF_PYTHON_ENV_PATH; export IDF_PATH=~/esp/esp-idf-5.3.4; source $IDF_PATH/export.sh; idf.py build'
idf.py -p /dev/ttyACM0 flash monitor
```

Crashes on every boot at the first `FullRefresh()`.

## Key Files

| File | Line | What |
|------|------|------|
| `Panel_EPD.cpp` | 249 | `_buf` allocation: `heap_caps_aligned_alloc(16, 259200, MALLOC_CAP_SPIRAM)` |
| `Panel_EPD.cpp` | 256-257 | Null check + cleanup if alloc fails |
| `Panel_EPD.cpp` | 430 | Debug log printing `buf=0x0` |
| `Panel_EPD.cpp` | 436 | Crash site: `auto buf = &_buf[y * stride]` |
| `ereader_app.cpp` | 267 | Our `FullRefresh()` calling `fillScreen` |
| `ereader_app.cpp` | 42-48 | `InitBoard()` calling `M5.begin()` |
| `app_main.cpp` | 19 | `Init()` called on core 0 |
| `app_main.cpp` | 22 | `RunLoop()` launched on core 1 |
| `0078/gnosis_app.cpp` | 22-30 | Working reference: same M5.begin + FullRefresh pattern |

## Step 4: Live console probe proved the framebuffer theory was wrong

The live USB Serial/JTAG probe changed the diagnosis materially. The crash signature still showed `buf=0x0` during `fillScreen`, but once the panel driver was instrumented at allocation time it became clear that `_buf` was not the primary failing allocation. `_buf` and `_step_framebuf` were both successfully allocated in PSRAM, and the two DMA line buffers were also present.

The actual failure was the PaperS3 EPD driver's `_lut_2pixel` allocation, which requested 43,520 bytes from DMA-capable internal RAM. On this board, after the rest of early boot finished, the largest DMA-capable free block was only 31,744 bytes, so `Panel_EPD::init_intenal()` failed as a whole. `M5.begin()` then continued running anyway, which left the app alive with no registered display and later produced the observed null-buffer crash when app code touched `M5.Display`.

### Prompt Context

**User prompt (verbatim):** "Continue working on ESP-37-EREADER-EPD-CRASH which a colleague has left for you, since you are an expert at embedded work. The PaperS3 is attached over the console. Feel free to look at the scripts in 0082 (i think) or so where a lot of serial based crash debugging took place"

**Assistant interpretation:** Continue the existing PaperS3 crash investigation on real hardware, use the attached console, and reuse prior serial-debugging patterns where useful.

**Inferred user intent:** Find the real cause of the EPD boot crash and get the e-reader firmware into a usable state on the attached board.

**Commit (code):** not committed in this session

### What I did
- Added allocation-failure logging to `Panel_EPD::init_intenal()` with exact pointer values, requested sizes, free heap sizes, and largest-block metrics.
- Added `display_ready_` gating in the e-reader app so the console stays alive even if display init fails.
- Flashed the firmware and attached a single non-reset serial probe to `/dev/ttyACM0`.
- Captured the key failure line:

```text
E (973) lgfx_epd_dbg: init alloc failed panel=960x540 memory=960x540 lut=0x0(43520) step=0x3c070ba0(1036800) buf=0x3c16ddc0(259200) dma0=0x3fce1c74(248) dma1=0x3fce1d70(248) free_dma=116523 free_internal=124311 free_spiram=7089628 largest_dma=31744 largest_spiram=7077888
```

### Why
- The original hypothesis focused too much on `_buf == nullptr` at the crash site instead of on why panel init failed earlier.
- The only defensible way to sort that out was to instrument the allocator and capture real hardware numbers on the attached board.

### What worked
- The added init logging immediately showed that the failing allocation was `_lut_2pixel`, not `_buf`.
- The app-side `display_ready_` guard prevented repeated crashes and kept the console available for follow-up validation.

### What didn't work
- The earlier theory that `_buf` was lazily allocated, freed later, or corrupted by SPIFFS activity did not hold up once the init path was instrumented.
- The earlier core-affinity fix (`M5.begin()` on core 0) did not address the real problem.

### What I learned
- `M5.begin(cfg)` does not fail loudly enough for this case; display init can fail and the application can still proceed unless it explicitly checks whether a display was registered.
- The PaperS3 EPD driver was over-constraining `_lut_2pixel` into DMA-capable memory even though the LUT is CPU-read lookup data, not a DMA payload.

### What was tricky to build
- The board is on USB Serial/JTAG, so probe discipline mattered. I had to keep `/dev/ttyACM0` single-owner and avoid overlapping `idf.py`, monitor, and custom serial readers.
- The failure signature was misleading because the later crash dereferenced `_buf`, but the real bug happened much earlier during an all-or-nothing init path.

### What warrants a second pair of eyes
- `M5Unified` continuing after display init failure is still a footgun and may deserve a broader library-level contract review.
- The EPD driver still has temporary debug logging budgets enabled in this workspace; they were helpful here, but they are not polished production logging.

### What should be done in the future
- Consider tightening the app or library contract so display-dependent firmware cannot silently continue after a failed display init.

### Code review instructions
- Start in `/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp`, specifically `Panel_EPD::init_intenal()`.
- Then review `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0080-papers3-ereader/main/ereader_app.cpp` for the app-side `display_ready_` guard and the boot diagnostics.
- Validate by building/flashing `0080-papers3-ereader` and capturing boot logs over `/dev/ttyACM0`.

### Technical details
- `_lut_2pixel` size: 43,520 bytes.
- Largest observed DMA-capable free block at failure time: 31,744 bytes.
- `_step_framebuf` PSRAM allocation succeeded at 1,036,800 bytes.
- `_buf` PSRAM allocation succeeded at 259,200 bytes.

## Step 5: Two targeted fixes made the reader usable again

After the allocator evidence was clear, the fix split naturally into two parts. First, `_lut_2pixel` was moved off the constrained DMA heap and onto generic 8-bit-capable memory so it can live in PSRAM when internal RAM is too fragmented. That removed the null-framebuffer crash and allowed the e-reader to boot normally.

Once that was fixed, a second boot-time issue surfaced: `M5Unified` performs `clear_display` during `M5.begin()`, before app code calls `M5.Display.setRotation(1)`. On PaperS3 the default panel rotation plus `offset_rotation = 3` produced a logical `540x960` clear against a physical `960x540` framebuffer layout. Setting the PaperS3 panel's default rotation to `1` inside board autodetect fixed those early out-of-range clears for both `0080` and the similar `0078` app pattern.

### Prompt Context

**User prompt (verbatim):** (same as Step 4)

**Assistant interpretation:** Finish the real fix on hardware, not just the diagnosis, and leave the board in a usable state.

**Inferred user intent:** Make the PaperS3 e-reader boot and render reliably on the attached hardware.

**Commit (code):** not committed in this session

### What I did
- Changed `_lut_2pixel` allocation in `Panel_EPD::init_intenal()` from `MALLOC_CAP_DMA` to `MALLOC_CAP_8BIT`.
- Kept the e-reader's `display_ready_` logging and guards so failed init remains debuggable.
- Added `p->setRotation(1);` in the PaperS3 board autodetect path in `M5GFX.cpp` so `M5Unified` boot-time clears use the correct landscape orientation.
- Rebuilt, flashed, and validated on the attached PaperS3 over `/dev/ttyACM0`.

### Why
- The allocator fix addressed the hard init failure.
- The PaperS3 default-rotation fix addressed the residual boot-time out-of-range clears that happened before application code could set the intended rotation.

### What worked
- The board now boots with `display_count=1 display_ready=yes`.
- The early `fillrect` calls are now `960x540` from the start.
- The out-of-range warnings disappeared.
- The app reaches the console prompt and opens the first book without crashing.

### What didn't work
- An intermediate attempt to keep `_lut_2pixel` in internal 8-bit RAM was still too restrictive for this board state; allowing generic 8-bit memory was the effective fix.

### What I learned
- There were two independent problems layered together: a hard init failure from the LUT allocation and a softer bring-up bug in the PaperS3 default orientation.
- The similar `0078` app structure was useful evidence that the library bring-up path, not just the e-reader app, needed attention.

### What was tricky to build
- Fixing the allocator issue exposed a second bug immediately. It would have been easy to stop after the crash disappeared and miss the incorrect early `540x960` clears.
- Validation had to stay disciplined around a single serial owner so the captured boot logs remained trustworthy.

### What warrants a second pair of eyes
- The PaperS3 default rotation choice now matches the working host-side board definition and the app expectations in `0078`/`0080`, but any firmware that intentionally relied on the old default would need review.

### What should be done in the future
- Trim or convert the temporary EPD debug logs once ESP-37 is formally closed.
- Consider moving the `display_ready_` pattern into a reusable helper if more PaperS3 apps are expected.

### Code review instructions
- Start with `/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp` for the `_lut_2pixel` allocation change.
- Then inspect `/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/components/M5GFX/src/M5GFX.cpp` for the PaperS3 default rotation fix.
- Finally inspect `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0080-papers3-ereader/main/ereader_app.cpp` and `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0080-papers3-ereader/main/ereader_app.h` for the app-side diagnostics and guard.
- Validate by flashing `0080-papers3-ereader` and confirming the boot log contains only `960x540` early fills plus `display_ready=yes`.

### Technical details
- Validated image: `f92aa00` built on `2026-03-23 16:56:29`.
- Representative successful boot lines:

```text
I (993) lgfx_epd_dbg: init panel=960x540 memory=960x540 buf=0x3c1787e0 ext=1 step=0x3c07b5c0 ext_step=1 dma0=0x3fce1c74 dma1=0x3fce1d70
I (1063) lgfx_epd_dbg: fillrect xs=0 ys=0 xe=959 ye=539 w=960 h=540 raw=0 mode=1 buf=0x3c1787e0 ext=1 stride=480 first=0 last=259199 len=259200 busy=0
ereader: board=19 display_count=1 display_ready=yes free_internal=119131 free_spiram=7046104
opened [0] The Deliverator (page 1/3)
```
