---
Title: Diary
Ticket: 0013-ATOMS3R-WEBSERVER
Status: active
Topics:
    - webserver
    - websocket
    - graphics
    - atoms3r
    - preact
    - zustand
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: Step-by-step narrative of research and implementation work for AtomS3R web server with graphics upload and WebSocket terminal
LastUpdated: 2026-01-05
WhatFor: ""
WhenToUse: ""
---

# Diary

## Goal

This diary captures the step-by-step research, analysis, and implementation journey for building a web server on the AtomS3R that enables graphics upload, WebSocket communication for button presses and terminal RX, and a frontend terminal interface. The goal is to document what changed, why it changed, what happened (including failures), and what we learned.

## Step 1: Initial Codebase Exploration and Ticket Creation

This step established the foundation for the project by creating the ticket workspace and performing initial reconnaissance of the existing codebases to understand the hardware capabilities, display system, button handling, serial communication patterns, and existing web server implementations.

**Commit (code):** N/A — Research phase

### What I did

- Created ticket `0013-ATOMS3R-WEBSERVER` using docmgr with topics: webserver, websocket, graphics, atoms3r, preact, zustand
- Explored `M5AtomS3-UserDemo` codebase structure:
  - Read `README.md` and `platformio.ini` to understand build system (PlatformIO, Arduino framework)
  - Analyzed `src/main.cpp` (740 lines) to understand function-based menu system
  - Identified display system using M5GFX/M5Unified with 128×128 GC9107 panel
  - Found button handling via `M5.BtnA.wasHold()` and `M5.BtnA.wasClicked()` methods
  - Discovered UART bridge functionality in `func_uart_t` class (Grove UART ↔ USB serial)
- Explored `esp32-s3-m5/0013-atoms3r-gif-console` project:
  - Found ESP-IDF based implementation with GIF playback
  - Analyzed control plane architecture using FreeRTOS queues (`CtrlEvent`)
  - Reviewed button ISR implementation (`control_plane.cpp`)
  - Examined console REPL using `esp_console` over UART/USB Serial JTAG
  - Found UART RX heartbeat debugging helper
- Explored `ATOMS3R-CAM-M12-UserDemo` for web server patterns:
  - Found `ESPAsyncWebServer` implementation in `main/service/service_web_server.cpp`
  - Discovered asset bundling pattern using `AssetPool` with flash partition memory-mapping
  - Analyzed `asset_pool_gen/main.cpp` for host-side asset generation
  - Reviewed WebSocket support in `ESPAsyncWebServer` library
- Performed semantic searches for:
  - Graphics display patterns (M5GFX canvas, sprite rendering)
  - Button press handling (GPIO ISR, M5Unified abstractions)
  - Serial UART communication (esp_console, UART driver APIs)
  - Web server implementations (ESPAsyncWebServer, HTTP server)

### Why

Understanding the existing codebase patterns is critical before designing a new system. The AtomS3R has multiple codebases using different frameworks (Arduino/PlatformIO vs ESP-IDF), and we need to choose the right foundation. Additionally, understanding how assets are bundled and served will inform our graphics upload strategy.

### What worked

- Semantic search effectively surfaced relevant code patterns across multiple projects
- The `M5AtomS3-UserDemo` provides clear examples of:
  - Display rendering (canvas vs direct)
  - Button state machine patterns
  - UART bridging for terminal communication
- The `ATOMS3R-CAM-M12-UserDemo` shows a working web server with:
  - AsyncWebServer setup
  - Asset bundling via flash partition memory-mapping
  - WiFi AP mode configuration
- The `0013-atoms3r-gif-console` demonstrates:
  - Control plane architecture with FreeRTOS queues
  - Button ISR patterns
  - Console REPL integration

### What didn't work

- Initial search for "web server" returned many results but needed filtering to find the actual implementation
- Some codebase references point to symlinked components that require understanding the build system

### What I learned

**Display System Architecture:**
- AtomS3R uses GC9107 panel with 128×128 visible area (128×160 memory panel, offset_y=32)
- Two rendering approaches exist:
  - Canvas-based (M5Canvas sprite): double-buffered, prevents flicker, ~25KB RAM
  - Direct display: simpler, potential flicker, full screen access
- Canvas initialization: `canvas.setColorDepth(16)` (RGB565), `canvas.createSprite(width, height)`

**Button Handling Patterns:**
- M5Unified provides high-level API: `M5.BtnA.wasHold()`, `M5.BtnA.wasClicked()`
- ESP-IDF approach uses GPIO ISR with FreeRTOS queues for thread-safe event delivery
- Button debouncing handled by library (M5Unified) or manual (ESP-IDF)

**Serial Communication:**
- Arduino: `Serial.begin()` for Grove UART, `USBSerial` for USB
- ESP-IDF: `uart_driver_install()`, `uart_read_bytes()`, `uart_write_bytes()`
- Console REPL: `esp_console` provides command registration and UART/USB Serial JTAG bindings

**Web Server Patterns:**
- `ESPAsyncWebServer` is the standard library (async, non-blocking)
- Asset bundling: Flash partition memory-mapping (`esp_partition_mmap`) avoids RAM overhead
- WiFi AP mode: `WiFi.softAP(ssid, password)` creates access point
- WebSocket support: `AsyncWebSocket` class in ESPAsyncWebServer library

**Asset Bundling Strategy:**
- AssetPool pattern: C struct with fixed-size byte arrays, memory-mapped from flash partition
- Custom partition type: `233/0x23` for asset pool
- Host-side generation: C++ tool creates binary blob from source files
- Device-side: Memory-map partition, cast to struct, access fields directly

### What was tricky to build

- Understanding the relationship between Arduino/PlatformIO and ESP-IDF codebases
- Tracing asset bundling pipeline from host tool to device memory-mapping
- Distinguishing between different display rendering approaches and when to use each

### What warrants a second pair of eyes

- Choice between Arduino/PlatformIO (`M5AtomS3-UserDemo`) vs ESP-IDF (`0013-atoms3r-gif-console`) framework
- Asset storage strategy: Flash partition memory-mapping vs FATFS filesystem vs SPIFFS
- WebSocket message protocol design for button presses and terminal data

### What should be done in the future

- Document the decision rationale for framework choice (Arduino vs ESP-IDF)
- Create a reference document mapping common operations (display, buttons, UART) between frameworks
- Research WebSocket message framing and binary vs text protocol trade-offs
- Investigate graphics format support (PNG, JPEG, GIF, raw RGB565) for upload feature

### Code review instructions

- Start with `M5AtomS3-UserDemo/src/main.cpp` lines 249-320 (`func_uart_t`) for UART bridge pattern
- Review `ATOMS3R-CAM-M12-UserDemo/main/service/service_web_server.cpp` for web server setup
- Check `esp32-s3-m5/0013-atoms3r-gif-console/main/control_plane.cpp` for button ISR pattern
- Examine `ATOMS3R-CAM-M12-UserDemo/main/utils/assets/assets.h` for asset bundling interface

### Technical details

**Key Files Discovered:**
- `M5AtomS3-UserDemo/src/main.cpp`: Main application with function menu system
- `M5AtomS3-UserDemo/platformio.ini`: Build configuration (Arduino framework, ESP32-S3)
- `esp32-s3-m5/0013-atoms3r-gif-console/main/control_plane.cpp`: Button ISR and control queue
- `esp32-s3-m5/0013-atoms3r-gif-console/main/console_repl.cpp`: Console command registration
- `ATOMS3R-CAM-M12-UserDemo/main/service/service_web_server.cpp`: Web server implementation
- `ATOMS3R-CAM-M12-UserDemo/main/utils/assets/assets.h`: Asset pool interface

**Display API Signatures:**
```cpp
// M5GFX Canvas
M5Canvas canvas(&M5.Display);
canvas.setColorDepth(16);
canvas.createSprite(width, height);
canvas.fillRect(x, y, w, h, color);
canvas.pushSprite(x, y);
canvas.drawPng(data, len, x, y);
```

**Button API Signatures:**
```cpp
// M5Unified (Arduino)
M5.update();
bool clicked = M5.BtnA.wasClicked();
bool held = M5.BtnA.wasHold();

// ESP-IDF GPIO ISR
gpio_set_intr_type(pin, GPIO_INTR_NEGEDGE);
gpio_isr_handler_add(pin, isr_handler, arg);
```

**UART API Signatures:**
```cpp
// Arduino
Serial.begin(baud, config, tx_pin, rx_pin);
size_t avail = Serial.available();
uint8_t c = Serial.read();
Serial.write(data, len);

// ESP-IDF
uart_driver_install(uart_num, rx_buf, tx_buf, queue_size, &queue, flags);
uart_read_bytes(uart_num, buf, len, timeout);
uart_write_bytes(uart_num, data, len);
```

**Web Server API Signatures:**
```cpp
// ESPAsyncWebServer
AsyncWebServer server(80);
server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/html", html_content);
});
AsyncWebSocket ws("/ws");
ws.onEvent(onWebSocketEvent);
server.addHandler(&ws);
server.begin();
```

### What I'd do differently next time

- Start with a focused question: "Which framework should we use?" rather than exploring everything
- Create a comparison matrix early: Arduino vs ESP-IDF for each required feature
- Document file paths and line numbers immediately while reading code

## Step 2: Doc Audit + Repo-Verified Corrections (Display/FATFS/Web Server Stack)

This step was a “trust but verify” pass over the documents we had generated in Step 1. The goal was to catch places where we drifted into plausible-but-wrong pseudocode and replace it with repo-verified facts (actual symbols, files, and config defaults). This is especially important here because this repo contains both Arduino-centric and ESP-IDF-centric projects, and mixing assumptions between them can produce a design that’s hard to implement cleanly.

**Commit (code):** N/A — documentation edits only

### What I did
- Re-checked the display bring-up implementation actually used by `esp32-s3-m5/0013-atoms3r-gif-console`:
  - `esp32-s3-m5/0013-atoms3r-gif-console/main/display_hal.cpp` uses `lgfx::Bus_SPI`, `lgfx::Panel_GC9107`, and `m5gfx::M5GFX::init(&panel)` with `SPI3_HOST` and `spi_3wire = true` (not the simplified “set pin_* on M5GFX config” pseudocode we initially wrote).
- Re-checked how FATFS is mounted for flash-bundled assets:
  - `esp32-s3-m5/components/echo_gif/src/gif_storage.cpp` uses `esp_vfs_fat_spiflash_mount_ro(...)` explicitly and documents why WL-backed mounts break prebuilt fatfsgen images (`FR_NO_FILESYSTEM` / “f_mount failed (13)”).
- Re-checked console/UART defaults and button defaults:
  - `esp32-s3-m5/0013-atoms3r-gif-console/main/Kconfig.projbuild` documents GROVE defaults (`G1=GPIO1`, `G2=GPIO2`) and sets default button GPIO to 41 with revision ambiguity notes.
- Started moving the design away from “ESPAsyncWebServer everywhere” assumptions:
  - In this repo, ESPAsyncWebServer usage is clearly Arduino-integrated (e.g. `ATOMS3R-CAM-M12-UserDemo/main/service/service_web_server.cpp` calling `initArduino()` and using `WiFi.softAP(...)`).
  - For a new `esp32-s3-m5` tutorial, an ESP-IDF-native stack (`esp_http_server` + WebSocket support) is a better default unless we intentionally accept Arduino as a dependency.

### Why
- The earlier drafts mixed Arduino-isms (`File`, `SPIFFS`, `String`) with ESP-IDF expectations (FATFS partitions, ESP-IDF tasks, `sdkconfig`/menuconfig). That’s a classic source of “design doc looks fine but implementation is painful.”
- We want future readers to be able to jump from the doc to the code and see the exact same shapes.

### What worked
- The repo already contains “golden path” implementations we can reuse:
  - Display HAL + present pattern in `0013-atoms3r-gif-console`
  - Flash-bundled FATFS pipeline (`make_storage_fatfs.sh`, `flash_storage.sh`)
  - Mount semantics documented in `echo_gif_storage_mount()`

### What didn’t work
- Generic web research was less useful than directly reading the repo and the ESP-IDF headers installed locally.

### What I learned
- **Display init is more concrete than expected**: the GC9107 path is expressed via explicit LovyanGFX bus/panel objects, not just a convenience M5Unified config struct.
- **FATFS mount mode matters a lot**: prebuilt images want `*_mount_ro`; runtime uploads want `*_mount_rw_wl` (and thus a different provisioning story).
- **“ESPAsyncWebServer + ESP-IDF” is a loaded statement**: it’s feasible, but only if we accept Arduino integration (as demonstrated by ATOMS3R-CAM-M12).

### What was tricky to build
- Keeping the doc accurate while still presenting a clean “recommended path” (because multiple stacks are plausible).

### What warrants a second pair of eyes
- Choosing the exact “storage story” for uploads:
  - Do we want runtime-writable FATFS (WL-backed) for uploads, or “flash images from host” like the GIF console?
  - This choice affects partition mounting APIs, tooling, and UX.

### What should be done in the future
- Produce a “final design doc” that cleanly commits to one stack (likely `esp_http_server` + FATFS RW WL for uploads) and updates the analysis accordingly.
- Replace remaining Arduino-flavored pseudocode with ESP-IDF-native API signatures where the final design uses ESP-IDF.

## Step 3: Repo-Verified “Golden Path” Building Blocks (Display + Button ISR + FATFS Mount Modes)

This step confirmed the concrete, repo-existing implementation patterns we’ll reuse for the actual MVP firmware project. The key outcome was de-risking the “hardware bring-up” parts (display + button) and clarifying the correct FATFS API split between read-only prebuilt images and runtime writable uploads.

**Commit (code):** N/A — research/verification phase

### What I did
- Verified AtomS3R GC9107 display bring-up + present pattern:
  - Read `esp32-s3-m5/0013-atoms3r-gif-console/main/display_hal.cpp` (SPI3_HOST, 3wire SPI, GC9107 panel config, `waitDMA()` present pattern)
- Verified the existing repo guidance for FATFS mounts:
  - Read `esp32-s3-m5/components/echo_gif/src/gif_storage.cpp`
  - Noted the explicit rationale for RO mount without WL when flashing a prebuilt FATFS image (WL expects metadata; prebuilt images can show FR_NO_FILESYSTEM if mounted via WL)
- Verified a safe ISR → queue pattern for button events (no heavy work in ISR):
  - Read `esp32-s3-m5/0013-atoms3r-gif-console/main/control_plane.cpp`
  - Confirmed it uses `xQueueSendFromISR` + `portYIELD_FROM_ISR()` and avoids logging/allocations in ISR
- Verified repo defaults and configurability for GROVE UART and button GPIO:
  - Read `esp32-s3-m5/0013-atoms3r-gif-console/main/Kconfig.projbuild` (defaults: GROVE TX=GPIO1, RX=GPIO2; button default 41; debounce setting)

### Why
- This project mixes subsystems where “almost correct” assumptions burn time (display offsets, SPI host/pins, WL mount semantics). Locking in repo-verified patterns reduces implementation risk and helps keep the new tutorial project consistent with existing ones in `esp32-s3-m5`.

### What worked
- The repo already includes a known-good AtomS3R display init sequence with explicit bus/panel objects (no guesswork).
- The FATFS storage module includes a clear explanation of why mounting mode matters depending on how the partition is provisioned (prebuilt image vs runtime writes).

### What didn’t work
- Hardware-specific validation (“your revision’s GROVE pins and button GPIO”) still requires running on the actual device; code review alone can’t guarantee wiring correctness.

### What I learned
- **Display init is explicit and stable**: SPI3_HOST + 3-wire SPI + GC9107 panel with y-offset=32 is a proven combination in this repo.
- **FATFS provisioning dictates mount API**: prebuilt images → mount RO without WL; runtime writable uploads → mount RW with WL (`*_mount_rw_wl`).
- **ISR discipline matters**: the existing queue-based pattern is the right baseline for button → WebSocket notifications.

### What was tricky to build
- Reconciling “we want writable uploads” with the repo’s prior FATFS usage being prebuilt-image + RO mount. This implies we need to introduce a new RW mount module and a different provisioning story than the GIF console uses.

### What warrants a second pair of eyes
- The storage story decision (RW WL for uploads) affects partition sizing, wear-leveling assumptions, and failure modes when users flash/erase.
- Button GPIO and GROVE mapping are revision-dependent; we need a clear verification playbook so folks don’t silently misconfigure pins.

### What should be done in the future
- Add a short “hardware verification” section to the new project README/playbook: confirm GROVE UART loopback and confirm button GPIO level changes before relying on those signals.

### Code review instructions
- Display: start at `esp32-s3-m5/0013-atoms3r-gif-console/main/display_hal.cpp` (`display_init_m5gfx`, `display_present_canvas`)
- FATFS mount semantics: `esp32-s3-m5/components/echo_gif/src/gif_storage.cpp` (`echo_gif_storage_mount`)
- Button ISR queue pattern: `esp32-s3-m5/0013-atoms3r-gif-console/main/control_plane.cpp` (`button_isr`, queue send)

## Step 4: Scaffold Tutorial 0017 Project + First Clean Build (ESP32-S3 target pinned)

This step created a new ESP-IDF-first tutorial project (`0017-atoms3r-web-ui`) to host the eventual WiFi + HTTP + WebSocket + upload implementation. The key outcome is a **known-good baseline**: display/backlight bring-up works and the project builds cleanly for the **correct target** (`esp32s3`), so future work can be incremental and testable.

**Commit (code):** 64af1c707e937df49da18dceba6d703ca6a830fd — "Tutorial 0017: scaffold AtomS3R web UI project"

### What I did
- Created new project `esp32-s3-m5/0017-atoms3r-web-ui/` with:
  - `CMakeLists.txt` using the same `EXTRA_COMPONENT_DIRS` strategy as other tutorials (reusing vendored `M5GFX`)
  - `main/` with `display_hal.*` + `backlight.*` copied/adapted from the known-good AtomS3R tutorials
  - `main/Kconfig.projbuild` + `sdkconfig.defaults` with sensible AtomS3R defaults (LCD offsets, backlight wiring, UART/button defaults, storage sizing knobs for later)
  - `partitions.csv` including a large `storage` FATFS partition (for future uploads)
- Ran a clean build under ESP-IDF 5.4.1 after explicitly setting the project target to `esp32s3`.

### Why
- We need an ESP-IDF-native, repo-consistent foundation to implement `esp_http_server` + FATFS RW + WebSockets without mixing Arduino-specific assumptions.
- Having a minimal “display proves hardware is alive” baseline makes later webserver debugging much easier.

### What worked
- `idf.py set-target esp32s3` + `idf.py build` produces an `esp32s3` image and links successfully.
- Display/backlight modules compile cleanly with the existing M5GFX component wiring.

### What didn't work
- The very first build attempt defaulted to the `esp32` target (because `set-target` hadn’t been run yet). This was caught by inspecting `build/config/sdkconfig.h` and corrected by re-running the build for `esp32s3`.

### What I learned
- For new tutorial projects, it’s worth validating `CONFIG_IDF_TARGET` immediately after the first build; otherwise it’s easy to accidentally compile for the wrong chip and misinterpret subsequent errors.

### What was tricky to build
- Ensuring the project target is pinned to `esp32s3` early enough to avoid silently generating an `esp32` binary.

### What warrants a second pair of eyes
- The decision to use USB Serial/JTAG as the primary console output (freeing GROVE UART for the WebSocket terminal feature) should be verified against how you flash/monitor the AtomS3R in your setup.

### What should be done in the future
- Add a short “first-run” section to `0017-atoms3r-web-ui/README.md` clarifying console transport expectations (USB Serial/JTAG) and how to override if needed.

### Code review instructions
- Start at `esp32-s3-m5/0017-atoms3r-web-ui/main/hello_world_main.cpp` (boot + display smoke test).
- Review `esp32-s3-m5/0017-atoms3r-web-ui/main/display_hal.cpp` + `backlight.cpp` for AtomS3R wiring assumptions.

## Step 5: Add WiFi SoftAP Bring-up (Boot Logs + Menuconfig Defaults)

This step added a minimal but complete WiFi SoftAP bring-up to the new `0017-atoms3r-web-ui` firmware. The goal was to establish the networking baseline early: the device should boot, start an AP, and print connection instructions before we start layering in `esp_http_server` and WebSockets.

**Commit (code):** bc4d09db056806ebbf914626471cbbe4530a89d1 — "Tutorial 0017: add WiFi SoftAP bring-up"

### What I did
- Added a `wifi_softap` module:
  - `esp32-s3-m5/0017-atoms3r-web-ui/main/wifi_softap.cpp`
  - `esp32-s3-m5/0017-atoms3r-web-ui/main/wifi_softap.h`
- Wired it into boot:
  - `esp32-s3-m5/0017-atoms3r-web-ui/main/hello_world_main.cpp` now calls `wifi_softap_start()` after the display smoke test.
- Added the required component deps in `main/CMakeLists.txt` (`nvs_flash`, `esp_event`, `esp_netif`, `esp_wifi`).
- Implemented boot logs:
  - SSID, channel, max connections, auth mode
  - SoftAP IP (via `esp_netif_get_ip_info`) and a “browse `http://192.168.4.1/`” hint

### Why
- We want a tight incremental loop: WiFi first, then HTTP routes, then uploads, then WebSocket terminal.
- Printing the connection instructions at boot avoids “it works but I don’t know where to connect” friction during manual testing.

### What worked
- Clean build after adding WiFi dependencies and module.
- SoftAP configuration supports both open and WPA2 (password empty → open).

### What didn't work
- Initial build failed due to `-Werror` complaining about `strnlen()` being called with a bound larger than the known string-literal size. This was fixed by computing `ssid_len` from the destination buffer after copying (avoiding GCC’s overread heuristic).

### What I learned
- When compiling with `-Werror`, some “safe” libc patterns can still be flagged if the compiler can reason about the string object size. In those cases, measuring length from a local fixed-size buffer is a robust workaround.

### What was tricky to build
- Getting the “just works” sequence right without dragging in extra complexity:
  - NVS init (including erase-on-version mismatch)
  - netif/event loop init
  - AP netif creation + WiFi init + set mode/config + start

### What warrants a second pair of eyes
- The assumption that SoftAP default IP is always `192.168.4.1` is true for the default ESP-IDF AP netif, but we should confirm it remains correct if we later customize netif config.

### What should be done in the future
- (Optional) Add STA mode behind menuconfig as planned (without complicating the MVP path).
- When `esp_http_server` is added, print the actual URL based on `esp_netif_get_ip_info` rather than relying on the default.

### Code review instructions
- Start at `esp32-s3-m5/0017-atoms3r-web-ui/main/wifi_softap.cpp` (`wifi_softap_start`).
- Confirm `main/CMakeLists.txt` includes the WiFi + NVS dependencies.

## Step 6: Mount FATFS RW (Wear-Levelled) and Prepare `/storage/graphics` for Uploads

This step added the “runtime writable storage” foundation the MVP needs for graphics uploads. Unlike the GIF console tutorial (which mounts a prebuilt FATFS image read-only), the web UI needs a writable partition so uploads can persist across reboots.

**Commit (code):** 4adadf62f36b7c38ea8e356de85d384dc45988fb — "Tutorial 0017: add FATFS RW storage mount"

### What I did
- Added a storage module:
  - `esp32-s3-m5/0017-atoms3r-web-ui/main/storage_fatfs.cpp`
  - `esp32-s3-m5/0017-atoms3r-web-ui/main/storage_fatfs.h`
- Mounted the `storage` partition at `/storage` using:
  - `esp_vfs_fat_spiflash_mount_rw_wl(...)` with `format_if_mount_failed=true`
- Ensured the uploads directory exists on boot:
  - `mkdir("/storage/graphics")` (ignore `EEXIST`)
- Implemented filename validation for future `/api/graphics/<name>` routes:
  - rejects `/` and `\\`
  - rejects `..`
  - enforces a max length
- Wired storage init into boot (`hello_world_main.cpp`) right after SoftAP start.

### Why
- The MVP requires “upload → persist → render after reboot”. That means a writable filesystem partition and a stable directory layout.
- We want to keep the upload handler streaming-to-disk later; having mount + directory creation handled early simplifies the HTTP code.

### What worked
- Clean build with FATFS + WL components added to `main/CMakeLists.txt`.
- Mount success path logs clearly (“mount ok”) and errors are logged with `esp_err_to_name`.

### What didn't work
- N/A (this step built cleanly).

### What I learned
- For this repo, it’s useful to treat FATFS as two distinct “modes”:
  - prebuilt-image + RO mount (no WL) for bundled assets
  - runtime-writable + WL mount for user uploads

### What was tricky to build
- Deciding on `format_if_mount_failed=true`: it’s great for first boot on a blank partition, but it can also hide corruption by reformatting. For the tutorial MVP that trade-off is acceptable, but worth calling out.

### What warrants a second pair of eyes
- Confirm the “format on mount failure” behavior is desired for your workflow (especially if you expect uploads to survive across reflashes that don’t erase the storage partition).

### What should be done in the future
- When implementing the upload endpoint, enforce `CONFIG_TUTORIAL_0017_MAX_UPLOAD_BYTES` while streaming (stop early, delete partial file).

### Code review instructions
- Start at `esp32-s3-m5/0017-atoms3r-web-ui/main/storage_fatfs.cpp` (`storage_fatfs_mount`, `storage_fatfs_ensure_graphics_dir`, `storage_fatfs_is_valid_filename`).

## Step 7: Display “App Layer” (Persistent Canvas + PNG-from-File Rendering)

This step refactored the early display smoke-test code into a small display “app layer” that owns a persistent `M5Canvas` and exposes a simple API: show a boot screen, present, and render a PNG from a filesystem path. This is the shape we’ll need once HTTP uploads trigger display updates from non-main code paths.

**Commit (code):** b5ebdb85311c0f50ad4ae745afd2a00dee83c25b — "Tutorial 0017: display canvas + PNG render"

### What I did
- Added `display_app` wrapper:
  - `esp32-s3-m5/0017-atoms3r-web-ui/main/display_app.cpp`
  - `esp32-s3-m5/0017-atoms3r-web-ui/main/display_app.h`
- Moved display/backlight init + sprite allocation into `display_app_init()`.
- Added `display_app_png_from_file(path)` which:
  - draws the PNG onto the offscreen canvas using `drawPngFile(...)` (stdio/VFS-backed)
  - presents the canvas using the existing DMA wait pattern
  - frees PNG decoder scratch memory via `releasePngMemory()`
- Updated `hello_world_main.cpp` to use the new `display_app_*` API (less top-level clutter, easier future integration).

### Why
- Once the HTTP server is added, “render newest upload” won’t live in `app_main`; it will be triggered by request handlers/tasks. A module-owned canvas avoids passing pointers around or duplicating display init logic.
- Rendering PNGs directly from VFS avoids reading whole images into RAM (better aligned with the “no giant buffers” constraint).

### What worked
- Clean build after refactor; the boot flow remains: display init → SoftAP → storage mount.

### What didn't work
- N/A.

### What I learned
- LovyanGFX’s default `DataWrapperT<void>` supports `FILE*` reads under ESP-IDF, so `drawPngFile("/storage/graphics/foo.png")` can stream decode directly from FATFS.

### What was tricky to build
- Avoiding static init order pitfalls while still keeping the canvas global. The approach here is: global `M5Canvas` object exists, but sprite allocation happens explicitly inside `display_app_init()` after the panel is initialized.

### What warrants a second pair of eyes
- Confirm that calling `releasePngMemory()` on the display/canvas after drawing is the right lifecycle for repeated draws (we don’t want slow memory growth across many uploads).

### What should be done in the future
- Define and document the “display policy”:
  - default to auto-render newest upload
  - optionally add an API to select a specific file

### Code review instructions
- Start at `esp32-s3-m5/0017-atoms3r-web-ui/main/display_app.cpp` (`display_app_init`, `display_app_png_from_file`).

## Step 8: Add `esp_http_server` + Streaming Upload API (Upload → FATFS → Auto-Display)

This step introduced the first “real” web server capabilities in the ESP-IDF tutorial: a running `esp_http_server` instance with JSON status + a minimal graphics CRUD API. The most important property is the upload path: it writes the request body to FATFS **incrementally** (no whole-file RAM buffer), then immediately renders the uploaded PNG on the screen to prove the end-to-end loop.

**Commit (code):** be7fb78770de05cc617513aec216369a741c64b4 — "Tutorial 0017: add HTTP server + graphics API"

### What I did
- Added `esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp` + `.h`
- Started the server with `HTTPD_DEFAULT_CONFIG()` and enabled wildcard URI matching (`httpd_uri_match_wildcard`) for `/api/graphics/*`.
- Implemented routes:
  - `GET /`: small HTML page indicating server is up (temporary placeholder until embedded frontend assets exist)
  - `GET /api/status`: minimal JSON status
  - `GET /api/graphics`: JSON list from `/storage/graphics`
  - `PUT /api/graphics/<name>`: **stream** request body to `/storage/graphics/<name>` using repeated `httpd_req_recv` → `fwrite`
    - validates filename (no slashes, no `..`, max length)
    - enforces `CONFIG_TUTORIAL_0017_MAX_UPLOAD_BYTES`
    - deletes partial files on error
    - auto-renders the newest upload via `display_app_png_from_file(path)` (MVP display policy)
  - `GET /api/graphics/<name>`: download with chunked responses
  - `DELETE /api/graphics/<name>`: delete file
- Wired server startup into `hello_world_main.cpp` after SoftAP + storage init.

### Why
- This moves the project from “device boots on the bench” to “device provides a usable API surface” — which is the foundation for the frontend UI, WebSocket terminal, and upload workflow.
- Implementing streaming upload early enforces the “no giant RAM buffers” constraint up-front, not as a late refactor.

### What worked
- Clean build after adding the HTTP server component dependency.
- Upload handler streams correctly using request `content_len` and a small fixed buffer.

### What didn't work
- Build initially failed because the constant name for HTTP 413 differs in ESP-IDF (`HTTPD_413_CONTENT_TOO_LARGE` vs the guessed `HTTPD_413_PAYLOAD_TOO_LARGE`). Fixed by switching to the correct IDF constant.

### What I learned
- `esp_http_server` wildcard routing (`/api/graphics/*`) is a good fit for “file-like” endpoints without adding a full router.

### What was tricky to build
- Getting the failure cases right for uploads:
  - reject too-large uploads early (413)
  - ensure partial files are cleaned up on any recv/write error

### What warrants a second pair of eyes
- The status endpoint currently prints a default AP IP string (`192.168.4.1`) instead of deriving it from the actual netif. This is OK for MVP, but should be tightened.
- Auto-display on upload runs inside the HTTP handler path; we should confirm this doesn’t starve the server under rapid uploads (it’s fine for MVP, but worth reviewing).

### What should be done in the future
- Replace `GET /` placeholder string with embedded frontend assets (index.html + JS + CSS) and correct `Content-Type` headers.
- Improve `/api/status` to report the actual IP address via `esp_netif_get_ip_info`.

### Code review instructions
- Start at `esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp`:
  - `http_server_start`
  - `graphics_put` (streaming upload)
  - `graphics_list_get`

## Step 9: Enable `/ws` WebSocket + UART Terminal Bridge + Button Events

This step turned the web server into an interactive control surface by adding a WebSocket endpoint (`/ws`) and wiring it to two real-time streams: UART bytes and button press events. The browser can now send binary frames to transmit UART bytes, and it receives UART RX bytes (binary frames) plus button events (JSON text frames).

**Commit (code):** 2b35945badfd22b4c9ddc47bfd0d63732af8560d — "Tutorial 0017: add WebSocket UART terminal + button events"

### What I did
- Enabled WebSocket support in the ESP-IDF HTTP server config:
  - `CONFIG_HTTPD_WS_SUPPORT=y` (required for `httpd_uri_t.is_websocket` and WS APIs)
- Implemented `/ws` endpoint inside `http_server`:
  - Registers a WebSocket URI with `is_websocket=true`
  - Tracks connected client socket fds for broadcast
  - Receives binary frames and forwards them to a callback (`http_server_ws_set_binary_rx_cb`)
  - Provides broadcast helpers:
    - `http_server_ws_broadcast_binary(...)` for UART RX bytes
    - `http_server_ws_broadcast_text(...)` for JSON events
- Added UART terminal bridge:
  - `esp32-s3-m5/0017-atoms3r-web-ui/main/uart_terminal.cpp`
  - RX task: `uart_read_bytes` → `http_server_ws_broadcast_binary`
  - WS binary RX callback: `uart_write_bytes`
- Added button input module:
  - `esp32-s3-m5/0017-atoms3r-web-ui/main/button_input.cpp`
  - GPIO ISR → FreeRTOS queue → task with debounce → `http_server_ws_broadcast_text("{...}")`
- Wired both modules into boot after the HTTP server starts (`hello_world_main.cpp`).

### Why
- The MVP explicitly requires “WebSocket terminal” and “button events streamed to browser” — this step establishes the bidirectional real-time channel the frontend will consume.
- Keeping the UART stream binary avoids JSON overhead and preserves raw bytes.

### What worked
- Clean build with WebSocket support enabled.
- Uses `httpd_ws_send_data_async(...)` so sends are queued onto the HTTP server work queue (safe from non-server tasks).

### What didn't work
- Setting `CONFIG_HTTPD_WS_SUPPORT=y` in `sdkconfig.defaults` alone didn’t take effect because the tracked `sdkconfig` had the option explicitly “not set”. We fixed this by updating the tracked `sdkconfig` to enable WS support.

### What I learned
- For this repo, `sdkconfig.defaults` is not an override layer; if an option is already set (or “not set”) in the tracked `sdkconfig`, defaults won’t flip it. For feature gates like WebSockets, update the tracked `sdkconfig` too.

### What was tricky to build
- Avoiding blocking sends from non-HTTP-server tasks: the UART RX task and button task both need to enqueue work rather than writing directly to the socket.
- Getting the ISR path right: ISR does only `xQueueSendFromISR` and yields; formatting and broadcasting happens in a task.

### What warrants a second pair of eyes
- Confirm the WS handler’s handshake/receive behavior matches ESP-IDF’s contract (handshake call is `HTTP_GET`, subsequent frames invoke the handler with a different method).
- Confirm the chosen buffer sizes (UART RX buffer/task stack and WS frame max 1024) are appropriate for your intended terminal usage.

### What should be done in the future
- Define the browser-side WS protocol more formally (message types, framing expectations) once the Preact frontend is implemented.
- Add a “pin conflict verification” playbook step to ensure the chosen button GPIO and GROVE UART pins don’t collide on your AtomS3R revision.

### Code review instructions
- Start at `esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp` (WS pieces + client tracking + broadcast helpers).
- Then review `uart_terminal.cpp` and `button_input.cpp` for the task/ISR patterns.

## Step 10: Frontend (Vite + Preact + Zustand) + Embed Built Assets + One-Command Build Script

This step adds the browser UI and wires it into the device APIs. It also makes the “build + embed” loop deterministic and low-friction by producing stable asset names and embedding the build outputs directly into the firmware image.

**Commits (code):**
- 2126d49 — "Tutorial 0017: embed web UI assets"
- 7dbb0af — "Tutorial 0017: add one-command build script"
- 0891b70 — "Tutorial 0017: web build outputs directly into embedded assets"

### What I did
- Added `web/` (Vite + Preact + Zustand) with:
  - `PUT /api/graphics/<name>` upload (raw body)
  - WebSocket client to `/ws`:
    - binary frames → terminal RX display
    - JSON text frames → button event list
    - typed line (ASCII) → binary TX to UART
- Made build outputs deterministic and firmware-friendly:
  - Vite builds a single JS bundle and a single CSS file at fixed paths:
    - `/assets/app.js`
    - `/assets/app.css`
  - Vite outputs directly into `main/assets/` (so `npm run build` updates the embedded files).
- Embedded the built assets in firmware via `main/CMakeLists.txt` (`EMBED_TXTFILES`) and served them:
  - `GET /` → embedded `index.html`
  - `GET /assets/app.js`
  - `GET /assets/app.css`
- Added `esp32-s3-m5/0017-atoms3r-web-ui/build.sh`:
  - sources ESP-IDF only when needed (checks `ESP_IDF_VERSION`)
  - runs `idf.py ...` so building is one command (`./build.sh build`)

### Why
- The MVP needs a real browser UI, not just curl-able endpoints.
- Stable filenames avoid needing a manifest on-device and keep firmware routes fixed.
- Writing the Vite build directly into `main/assets/` removes “copy artifacts” steps and reduces chances of embedding stale assets.

## Step 11: Verification playbook (flash + connect + UI + upload + WS + persistence)

Created an end-to-end verification playbook for running the MVP on real AtomS3R hardware.

### What I did
- Wrote the step-by-step test procedure:
  - `ttmp/2025/12/26/0013-ATOMS3R-WEBSERVER--atoms3r-web-server-graphics-upload-and-websocket-terminal/playbooks/01-verification-playbook-atoms3r-web-ui.md`

### Why
- This ticket’s acceptance criteria are inherently “system-level” (WiFi + HTTP + FATFS + display + WS + UART + button). A written playbook reduces guesswork and makes regressions repeatable.

## Step 12: WiFi mode selection (SoftAP vs STA DHCP vs AP+STA)

Added a menuconfig-selectable WiFi bring-up that can either:

- host a **SoftAP** (existing behavior),
- **join an existing WiFi network** as STA and acquire an IP via **DHCP**, or
- do **AP+STA** (keep SoftAP while also joining an upstream network).

**Commit (code):** 4148abd — "Tutorial 0017: add WiFi STA DHCP mode (menuconfig selectable)"

### What I did
- Implemented `wifi_app_start()` (`main/wifi_app.cpp`) with three modes:
  - SoftAP (`WIFI_MODE_AP`)
  - STA (`WIFI_MODE_STA`, DHCP)
  - AP+STA (`WIFI_MODE_APSTA`)
- Added `menuconfig` options under `Tutorial 0017: WiFi`:
  - WiFi mode choice
  - STA SSID/password/hostname + connect timeout
  - STA-only fallback option to bring up SoftAP if DHCP fails
- Updated `/api/status` to report:
  - current mode (`softap|sta|apsta`)
  - actual IPs derived from `esp_netif_get_ip_info` (not hardcoded `192.168.4.1`)

### Why
- It’s common to want the AtomS3R reachable on an existing LAN (DHCP) for easier development and integration, while still keeping SoftAP available as a “recovery” path.

## Step 13: Update verification playbook for STA/AP+STA + write WiFi guide

This step expands the system documentation now that the firmware supports both “device-hosted network” and “join existing network” workflows.

### What I did
- Updated the verification playbook to be **mode-agnostic** by introducing a `DEVICE_URL` variable and documenting how to discover the device URL for:
  - SoftAP
  - STA (DHCP)
  - AP+STA
- Wrote a long-form WiFi guide aimed at senior embedded developers new to ESP-IDF networking:
  - `reference/02-esp-idf-wifi-softap-sta-apsta-guide.md`

### Why
- Without STA/AP+STA guidance, “verification” becomes brittle and assumes `192.168.4.1` even when the device is running on DHCP.
- The WiFi guide bridges ESP-IDF concepts (`esp_wifi`, `esp_netif`, `esp_event`) into an actionable mental model for extending and debugging the project.

## Step 14: JS-focused playbook for bundling + embedding Preact/Zustand web assets

This step documents the reusable frontend pattern behind the AtomS3R device-hosted web UI: how to structure a small Preact + Zustand app, how to configure Vite for deterministic “firmware routes”, and how to embed/serve the resulting artifacts from ESP-IDF. The goal is to make the “JS side” repeatable for future firmware projects without rediscovering the bundling details.

**Commit (code):** N/A — Documentation only

### What I did
- Studied the concrete implementation in Tutorial `esp32-s3-m5/0017-atoms3r-web-ui/`, focusing on:
  - Vite bundling rules for deterministic outputs (`web/vite.config.ts`)
  - Asset embedding via ESP-IDF CMake (`main/CMakeLists.txt`, `EMBED_TXTFILES`)
  - Preact app structure + Zustand WebSocket store (`web/src/*`)
  - Firmware handlers serving embedded assets (`main/http_server.cpp`)
- Wrote a new JS-focused playbook at `docs/playbook-embedded-preact-zustand-webui.md`.

### Why
- The ticket docs cover the full system design, but future work often starts by “I need to embed a small web UI into firmware” and needs a single, focused reference for the frontend build/structure decisions.
- Deterministic file naming (no hashes) is a recurring constraint for embedded asset serving and should be documented as a first-class pattern, not an incidental Vite configuration detail.

### What worked
- The `0017` tutorial provides an end-to-end reference where the outputs are small (~30KB JS) and the build pipeline is straightforward: `npm run build` emits directly into `main/assets/`, and ESP-IDF embeds the results.

### What didn't work
- N/A (no debugging or implementation failures in this step).

### What I learned
- The most important firmware-facing Vite knobs are:
  - stable filenames (`assets/app.js`, `assets/app.css`)
  - a single JS chunk (`inlineDynamicImports: true`)
  - no accidental extra files (`publicDir: false`, `emptyOutDir: true`)
- Using deterministic filenames shifts the problem from “cache busting via hashes” to “being intentional about Cache-Control”, which should be addressed explicitly in docs for device-hosted UIs.

### What was tricky to build
- Explaining (for future maintainers) why the Vite config looks “odd” compared to typical web apps: disabling hashed filenames and chunking is the opposite of common best practices, but it is the right tradeoff for firmware embedding.

### What warrants a second pair of eyes
- N/A (documentation-only change).

### What should be done in the future
- Consider adding a small “manifest-driven” alternative in the playbook (hashed filenames + generated route table) if/when embedded UIs in this repo grow beyond “one JS + one CSS”.

### Code review instructions
- Start with `docs/playbook-embedded-preact-zustand-webui.md` and verify the referenced examples match the repo:
  - `esp32-s3-m5/0017-atoms3r-web-ui/web/vite.config.ts`
  - `esp32-s3-m5/0017-atoms3r-web-ui/main/CMakeLists.txt`
  - `esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp`
  - `esp32-s3-m5/0017-atoms3r-web-ui/web/src/store/ws.ts`

### Technical details
- Commands used to locate relevant sources:
  - `docmgr doc list --ticket 0013-ATOMS3R-WEBSERVER`
  - `rg -n "preact|zustand|vite" -S .`
