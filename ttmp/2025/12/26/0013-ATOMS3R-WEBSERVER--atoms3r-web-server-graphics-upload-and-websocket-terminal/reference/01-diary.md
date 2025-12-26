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
LastUpdated: 2025-12-26T08:54:18.638857926-05:00
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
