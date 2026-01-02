---
Title: 'Project analysis: MicroSD file browser + quick viewer (text/JSON/log)'
Ticket: 0025-CARDPUTER-LVGL
Status: active
Topics:
    - esp32s3
    - cardputer
    - lvgl
    - ui
    - esp-idf
    - m5gfx
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0017-atoms3r-web-ui/main/storage_fatfs.cpp
      Note: Repo’s canonical “mount FATFS + ensure dir + validate filename” patterns (flash FATFS)
    - Path: 0017-atoms3r-web-ui/main/http_server.cpp
      Note: File listing and streaming read/write patterns (opendir/readdir/fread/fwrite)
    - Path: 0013-atoms3r-gif-console/make_storage_fatfs.sh
      Note: FATFS image creation tooling and mount path conventions (/storage)
    - Path: 0022-cardputer-m5gfx-demo-suite/main/screenshot_png.cpp
      Note: Screenshot PNG generation + chunked send; can be adapted to “save to file”
    - Path: 0022-cardputer-m5gfx-demo-suite/main/ui_list_view.cpp
      Note: List selection + scrolling behavior to port conceptually into LVGL file list
    - Path: 0022-cardputer-m5gfx-demo-suite/main/ui_console.cpp
      Note: Text wrapping + scrollback model useful for a quick text/log viewer
    - Path: ttmp/2026/01/01/0024-B3-SCREENSHOT-WDT--cardputer-screenshot-png-to-usb-serial-jtag-can-wdt-when-driver-uninitialized/analysis/02-postmortem-screenshot-png-over-usb-serial-jtag-caused-wdt-root-cause-fix.md
      Note: Why screenshot send must be chunked and bounded (avoid WDT wedges)
ExternalSources: []
Summary: Analyzes how to add a MicroSD-backed file browser and quick text/JSON/log viewer to the Cardputer LVGL demo, reusing FATFS/VFS patterns from the repo and integrating with screenshot capture workflows.
LastUpdated: 2026-01-02T09:39:42.442447162-05:00
WhatFor: ""
WhenToUse: ""
---

# Project analysis: MicroSD file browser + quick viewer (text/JSON/log)

This document analyzes how to build a MicroSD-backed file browser and a “quick viewer” for text/log/JSON files on the Cardputer LVGL demo. The core idea is “make the device self-debuggable”:

- browse `/sd` (or similar mount) directly on-device,
- open files without pulling out a laptop,
- view logs/configs quickly,
- optionally **save screenshots** to MicroSD and then open them later (or enumerate them for host transfer).

This is moderately challenging because it exercises:

- FATFS mounting on external storage (MicroSD)
- directory enumeration
- file streaming and incremental UI updates
- selection + scrolling UX on 240×135
- correctness around blocking IO (don’t stall the UI)

But it’s extremely reusable: once you have file browsing and viewing, many future demos become easier to build and debug.

---

## What already exists in the repo (relevant code + patterns)

### 1) FATFS mounting and filesystem utilities (flash FATFS, but patterns carry over)

`0017-atoms3r-web-ui/main/storage_fatfs.cpp` is a canonical “how we do storage” module:

- `storage_fatfs_mount()` mounts a FATFS partition and caches `s_mounted`
- `storage_fatfs_ensure_graphics_dir()` creates a subdirectory (with good errno logging)
- `storage_fatfs_is_valid_filename()` rejects traversal and separators

While this mounts **SPI flash FATFS** (via `esp_vfs_fat_spiflash_mount_rw_wl`), the design ideas still apply directly:

- mount-once and cache “mounted” state
- provide “ensure directory” helpers
- validate user-supplied filenames aggressively

### 2) Directory listing and streaming reads/writes (VFS + stdio)

`0017-atoms3r-web-ui/main/http_server.cpp` contains small, real-world filesystem patterns:

- enumerate a directory:
  - `opendir()`, `readdir()`, `closedir()`
- stream file upload to disk incrementally:
  - `fopen("wb")` + repeated `fwrite(buf, n)`
- stream file download incrementally:
  - `fopen("rb")` + repeated `fread(buf)`

This code is valuable because it’s already written with:

- bounded buffers (`char buf[1024]`)
- explicit error handling (`errno`, `unlink` on partial failure)

We can reuse the *structure* for file browsing and viewing.

### 3) List selection and scroll behavior (non-LVGL)

For the file list UI we need:

- selection movement (Up/Down)
- directory scroll when list > visible rows
- label truncation (“…”) for long filenames

`0022-cardputer-m5gfx-demo-suite/main/ui_list_view.cpp` provides a minimal, tested behavior model we can port into LVGL.

### 4) Text wrapping + scrollback model (non-LVGL)

The quick viewer will need to show text with wrapping and scrolling.

`0022-cardputer-m5gfx-demo-suite/main/ui_console.cpp` already implements:

- a deque of wrapped lines
- a scrollback model (“follow tail” vs user scrolled up)
- a wrapping algorithm based on font metrics

In LVGL, we can implement the same model but render it using:

- `lv_textarea` (read-only) for the viewport, or
- a scrollable container with labels (more control, more work)

### 5) Screenshot PNG generation (great complement to file browser)

`0022-cardputer-m5gfx-demo-suite/main/screenshot_png.cpp` generates a PNG on-device:

- `display.createPng(&len, x, y, w, h)` returns an in-memory buffer
- it streams the bytes over USB-Serial/JTAG using a framed protocol
- it uses a dedicated task to avoid main task stack overflow

For the file browser, we can adapt the same `createPng` buffer to:

- write the PNG bytes to a file on MicroSD (`fopen("/sd/shot.png","wb")` + `fwrite`)

Known pitfalls are documented in ticket 0024 (USB driver not installed; busy-loop WDT) and 0026 (stack overflow if PNG encode happens on main task). Even if we switch from “send to serial” to “save to file”, PNG encoding still wants a safe stack budget (task approach still applies).

---

## Storage architecture: mounting MicroSD via ESP-IDF VFS

This repo currently mounts FATFS on flash partitions, but MicroSD mounting uses a different ESP-IDF helper. The exact choice depends on how Cardputer’s MicroSD is wired:

- **SDMMC (4-bit)**: faster, more pins
- **SDSPI (SPI)**: simpler wiring, more common on small boards

In ESP-IDF, the conceptual APIs are:

```c
// SDMMC host:
esp_err_t esp_vfs_fat_sdmmc_mount(
  const char* base_path,
  const sdmmc_host_t* host_config,
  const sdmmc_slot_config_t* slot_config,
  const esp_vfs_fat_mount_config_t* mount_config,
  sdmmc_card_t** out_card
);

// SDSPI host:
esp_err_t esp_vfs_fat_sdspi_mount(
  const char* base_path,
  const sdmmc_host_t* host_config,
  const sdspi_device_config_t* slot_config,
  const esp_vfs_fat_mount_config_t* mount_config,
  sdmmc_card_t** out_card
);
```

Suggested “house style” module for 0025:

- `sdcard_fatfs.h/.cpp`:
  - `esp_err_t sdcard_mount(void);`
  - `bool sdcard_is_mounted(void);`
  - `const char* sdcard_mount_path(void); // "/sd"`
  - `esp_err_t sdcard_ensure_dir(const char* rel);`

And cache `sdmmc_card_t*` for card info (optional).

### Mount configuration choices

`esp_vfs_fat_mount_config_t` fields that matter:

- `.format_if_mount_failed`:
  - **false** for MicroSD (never auto-format user’s card)
- `.max_files`:
  - 4–8 is fine
- long file names:
  - ensure FATFS LFN is enabled in sdkconfig if you want real filenames

---

## UI architecture: file browser + quick viewer screens

Treat file browsing and viewing as separate “screens” in the demo catalog:

- `FileBrowser` screen:
  - list directories/files
  - handles navigation (enter directory, go up, open file)
- `FileViewer` screen:
  - shows file contents
  - handles scrolling
  - supports “back to browser”

This is a good fit for the existing `DemoManager` pattern:

- screens are created by functions
- screen-local timers and state are cleaned on `LV_EVENT_DELETE`
- global `Esc` returns to menu (already handled)

### UI layout suggestion for FileBrowser (240×135)

```
┌──────────────────────────────┐
│ /sd/logs                     │  ← path header (breadcrumb-ish)
│──────────────────────────────│
│  config.json                 │
│  run.log                     │  ← selection highlight
│  screenshots/                │
│  ...                         │
│──────────────────────────────│
│ Up/Down select  Enter open   │  ← hint bar
│ Back: parent  Fn+` menu      │
└──────────────────────────────┘
```

LVGL widget options:

- use `lv_list` for simplicity
- or emulate `demo_menu.cpp` with your own rows and style (predictable)

### UI layout suggestion for FileViewer

```
┌──────────────────────────────┐
│ run.log                      │
│──────────────────────────────│
│ [scrollable text viewport]   │
│                              │
│                              │
│──────────────────────────────│
│ Up/Down scroll  Back return  │
└──────────────────────────────┘
```

For text viewport:

- simplest: read-only multiline `lv_textarea`
- more control: scrollable container with line labels

Given file sizes might be large, you’ll likely want:

- a limit (“preview first N bytes/lines”)
- incremental loading

---

## Non-blocking IO: the main engineering constraint

Reading an entire file into memory and then rendering it may:

- blow RAM (especially if you’re also keeping LVGL draw buffers)
- stall the UI loop long enough to feel laggy

Two safe patterns:

### Pattern A (simpler): size-limited preview

- `stat(path)` and reject > X KB, or only show first X KB
- read in chunks and append to the textarea

This is usually enough for “quick log viewer”.

### Pattern B (more robust): background read task + UI queue

If we want smooth scrolling and large files:

1. spawn a `file_read_task` that reads the file line-by-line (or chunk-by-chunk)
2. push chunks into a queue
3. UI thread drains the queue and updates the textarea/viewport

This is the same “control plane boundary” idea used for `esp_console` commands: *all LVGL updates happen on the UI thread*.

---

## Integration with screenshot capture (why the file browser complements it)

Once you have MicroSD storage, screenshot workflows can improve:

- `screenshot` command/action can:
  - send PNG to serial (existing protocol), and/or
  - save PNG to `/sd/screenshots/<timestamp>.png`

If you save screenshots to SD:

- file browser can list them
- you can implement “export last screenshot” later via serial framing or Wi‑Fi upload

### Pseudocode: “save screenshot to SD”

```c++
bool screenshot_png_to_file(m5gfx::M5GFX& display, const char* path) {
  size_t len = 0;
  void* png = display.createPng(&len, 0, 0, display.width(), display.height());
  if (!png || len == 0) { free(png); return false; }
  FILE* f = fopen(path, "wb");
  if (!f) { free(png); return false; }
  fwrite(png, 1, len, f);
  fclose(f);
  free(png);
  return true;
}
```

But: per ticket 0026, PNG encoding can be stack-hungry, so this should likely run in a dedicated task just like the serial sender does.

---

## For a new developer: how to approach this project safely

1. Start by reusing proven filesystem patterns from `0017-atoms3r-web-ui`:
   - use small fixed buffers
   - log errno on failure
   - avoid path traversal if any user input is involved
2. Make the UI feel responsive:
   - never block the LVGL loop for long reads
   - prefer previews, chunking, or background tasks
3. Keep LVGL lifecycle disciplined:
   - if a screen uses timers/tasks, ensure cleanup happens on `LV_EVENT_DELETE`
4. Be conservative about formatting the SD card:
   - never auto-format MicroSD (`format_if_mount_failed=false`)
5. Test with “bad” inputs:
   - empty directories
   - huge files
   - weird filenames (long names, spaces)

---

## Implementation checklist (pragmatic)

1. Add MicroSD mount module (`sdcard_fatfs.*`) with `sdcard_mount()` and mount path `"/sd"`.
2. Add `DemoId::FileBrowser` screen:
   - list current directory entries
   - selection and scrolling
   - Enter opens directory or opens file viewer
3. Add `DemoId::FileViewer` screen:
   - read-only viewport
   - show first N KB/lines
   - Up/Down scroll; Back returns
4. Add optional integration:
   - screenshot save-to-sd action/command
   - file browser shows screenshot directory by default

