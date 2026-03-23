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
    - "/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0078-papers3-gnosis-layout/main/app_main.cpp:Working reference - gnosis 0078 does M5.begin on core 1 and works fine"
ExternalSources: []
Summary: "Investigation diary for the null EPD framebuffer crash in the 0080 e-reader firmware. Panel_EPD._buf is 0x0 when FullRefresh calls fillScreen."
LastUpdated: 2026-03-23
WhatFor: "Handoff document for embedded engineer to diagnose and fix the Panel_EPD framebuffer allocation issue."
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
