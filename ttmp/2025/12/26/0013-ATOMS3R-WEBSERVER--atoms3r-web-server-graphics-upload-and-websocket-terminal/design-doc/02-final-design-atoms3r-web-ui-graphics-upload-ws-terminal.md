---
Title: FINAL Design - AtomS3R Web UI (Graphics Upload + WS Terminal)
Ticket: 0013-ATOMS3R-WEBSERVER
Status: active
Topics:
    - webserver
    - websocket
    - graphics
    - atoms3r
    - preact
    - zustand
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../../../esp/esp-idf-5.4.1/components/esp_http_server/include/esp_http_server.h
      Note: Authoritative API signatures for esp_http_server + WebSocket frames
    - Path: ../../../../../../../../../../esp/esp-idf-5.4.1/components/fatfs/vfs/esp_vfs_fat.h
      Note: Authoritative API signatures for FATFS mount_ro vs mount_rw_wl and formatting
    - Path: esp32-s3-m5/0013-atoms3r-gif-console/main/display_hal.cpp
      Note: Repo-verified AtomS3R GC9107 bring-up + present pattern to reuse
    - Path: esp32-s3-m5/components/echo_gif/src/gif_storage.cpp
      Note: Repo-verified FATFS mount semantics; shows mount_ro for prebuilt images and WL caveats
ExternalSources: []
Summary: 'Final design for an ESP-IDF-first AtomS3R device-hosted web UI: PNG upload to FATFS + display render, WebSocket terminal (UART RX/TX), and button events; Preact+Zustand frontend embedded via ESP-IDF CMake.'
LastUpdated: 2025-12-26T09:15:08.759237728-05:00
WhatFor: ""
WhenToUse: ""
---


# FINAL Design - AtomS3R Web UI (Graphics Upload + WS Terminal)

## Executive Summary

This ticket builds a **device-hosted web UI** for the AtomS3R that solves three practical problems at once: (1) upload graphics from your laptop/phone and render them on the 128×128 GC9107 display, (2) stream device events (button presses + UART RX) to the browser in real time, and (3) provide a small “terminal” in the browser to send bytes back to the device over UART.

The key constraint is that we want this to “fit” the existing `esp32-s3-m5` tutorial style: **ESP-IDF-first**, FreeRTOS-centric, and reusing the known-good AtomS3R display HAL and storage patterns already in the repo. Concretely, we’ll implement the server using ESP-IDF’s native **`esp_http_server`** (HTTP handlers + WebSocket support), reuse **M5GFX/LovyanGFX** bring-up from `0013-atoms3r-gif-console`, and store uploaded graphics on a **FATFS partition mounted RW with wear levelling**.

Deliverable-wise, the MVP is: connect to the device (SoftAP), open the web UI, upload a PNG, see it on the display, and watch button presses + UART RX appear live in the browser while being able to type data that the device writes back to UART.

## Problem Statement

Right now, getting pixels onto the AtomS3R display and interacting with the device is mostly a “developer loop” (build/flash/serial monitor). That’s fine for firmware work, but it’s awkward for:

- Quickly iterating on visuals (upload → display) without reflashing.
- Observing physical device inputs (button presses) remotely.
- Using a serial-ish workflow (send bytes to the device, read bytes back) without attaching a USB/serial adapter.

We need a device-local web interface that works over WiFi, is light enough for ESP32-S3 constraints, and is grounded in the existing repo patterns so it can be maintained as another `esp32-s3-m5` tutorial.

## Proposed Solution

### High-level architecture

At runtime the device provides:

- **HTTP** (port 80):
  - Serves the frontend bundle (Preact + Zustand).
  - Provides REST-ish endpoints for listing/downloading/deleting graphics.
  - Provides an upload endpoint that writes directly into FATFS without buffering the whole file.
- **WebSocket** (path `/ws`):
  - **Device → Browser**: button events and UART RX.
  - **Browser → Device**: UART TX (terminal input).

The firmware splits responsibilities into small FreeRTOS tasks/modules to keep timing predictable (display refresh and UART RX shouldn’t be blocked by HTTP request parsing).

### “Fits the repo” implementation strategy

- **Display**: reuse the repo-verified bring-up and present pattern from:
  - `esp32-s3-m5/0013-atoms3r-gif-console/main/display_hal.cpp` (`display_init_m5gfx`, `display_present_canvas`)
- **Storage**:
  - Use a FATFS partition named `storage`, mounted at `/storage`.
  - For **uploads** we mount RW with wear levelling:
    - `esp_vfs_fat_spiflash_mount_rw_wl("/storage", "storage", &cfg, &wl_handle)`
  - This differs intentionally from the `echo_gif` “prebuilt image” approach (which uses `*_mount_ro`) because we need runtime writes.
- **Web server**:
  - Use ESP-IDF `esp_http_server`:
    - URI handlers (`httpd_uri_t` + `httpd_register_uri_handler`)
    - Streaming request bodies (`httpd_req_recv`)
    - WebSocket frames (`httpd_ws_recv_frame`, `httpd_ws_send_frame_async`)

### Data flows (diagrams)

#### Upload → store → display

```
Browser
  PUT /api/graphics/<name>  (raw bytes)
    │
    ▼
esp_http_server URI handler
  - validate name + size
  - fopen("/storage/graphics/<name>", "wb")
  - loop { httpd_req_recv(...) -> fwrite(...) }
  - close file
  - enqueue "display this file" event
    │
    ▼
display task
  - read file
  - decode (MVP: PNG via M5GFX drawPng from memory)
  - present via M5Canvas + display_present_canvas()
```

#### UART RX/TX via WebSocket

```
UART RX task                      Browser WS client
  uart_read_bytes()  ───────────►  WS binary frame (terminal_rx)

Browser terminal input            WS handler / queue worker         UART TX
  WS binary frame (terminal_tx) ─► drain bytes -> uart_write_bytes() ─► device
```

### HTTP API (MVP)

We intentionally avoid `multipart/form-data` parsing for the device. Instead we use “raw body” endpoints that are trivial to implement with `httpd_req_recv`.

- **`GET /`**: frontend `index.html`
- **`GET /assets/<file>`**: frontend static assets (JS/CSS)
- **`GET /api/status`**: JSON status (heap, uptime, wifi mode, etc.)
- **`GET /api/graphics`**: JSON list of available graphics (name, bytes, mtime if available)
- **`PUT /api/graphics/<name>`**: upload file as raw body
  - Content length required (`req->content_len`)
  - Enforce max size (configurable)
- **`GET /api/graphics/<name>`**: download raw file
- **`DELETE /api/graphics/<name>`**: delete file
- **(Optional)** `POST /api/display/<name>`: select which file to display (if upload shouldn’t auto-display)

### WebSocket API (MVP)

We use one WebSocket endpoint (`/ws`). Frames are either JSON text (control) or raw bytes (terminal data).

- **Text frames (JSON)**
  - **button event**:
    - `{"type":"button","pressed":true,"ts_ms":123456}`
  - **info/telemetry** (optional):
    - `{"type":"status","free_heap":12345,...}`
- **Binary frames**
  - **terminal RX**: raw bytes from UART → browser
  - **terminal TX**: raw bytes from browser → UART

On the ESP-IDF side, WebSocket support is explicitly modeled in the API:

```c
// (ESP-IDF 5.4.1) WebSocket receive/send APIs:
esp_err_t httpd_ws_recv_frame(httpd_req_t *req, httpd_ws_frame_t *pkt, size_t max_len);
esp_err_t httpd_ws_send_frame_async(httpd_handle_t hd, int fd, httpd_ws_frame_t *frame);
```

To send frames from tasks, we keep the `httpd_handle_t` server handle and the client socket fds (for connected WS clients) and use `httpd_ws_send_frame_async`.

### Frontend (Preact + Zustand)

The frontend is a small single-page UI:

- **Graphics panel**:
  - upload file (uses `fetch(PUT ...)` with `file.stream()` or `arrayBuffer()`)
  - list files, click to display, delete
- **Terminal panel**:
  - connects to `/ws`
  - displays UART RX (binary) as text/hex (toggle)
  - sends UART TX (binary) on Enter
- **Status bar**:
  - connection state (WS up/down)
  - device IP / AP SSID instructions

To make firmware serving stable, we configure Vite to emit deterministic filenames (no hashes), e.g. `assets/app.js`, `assets/app.css`, so we don’t need to edit firmware on every rebuild.

### Asset bundling / serving strategy

We embed the built frontend artifacts into the firmware image using ESP-IDF CMake `EMBED_FILES`/`EMBED_TXTFILES` (repo has examples of `EMBED_TXTFILES` usage in vendored components). The handlers serve these blobs with correct `Content-Type` and optional gzip `Content-Encoding`.

For example, `main/CMakeLists.txt` for the new tutorial can embed:

- `web/dist/index.html`
- `web/dist/assets/app.js`
- `web/dist/assets/app.css`

and the HTTP handlers map:

- `/` → `index.html`
- `/assets/app.js` → JS blob
- `/assets/app.css` → CSS blob

If we gzip the JS/CSS at build time, we must also set:

- `httpd_resp_set_hdr(req, "Content-Encoding", "gzip")`

## Design Decisions

### 1) ESP-IDF `esp_http_server` (not ESPAsyncWebServer) for the `esp32-s3-m5` tutorial

- **Decision**: Use `esp_http_server` as the server implementation.
- **Why**: It matches the ESP-IDF-first style of `esp32-s3-m5` tutorials and doesn’t require Arduino integration.
- **Evidence**:
  - `ATOMS3R-CAM-M12-UserDemo` uses ESPAsyncWebServer but also calls `initArduino()` and uses Arduino WiFi APIs (`WiFi.softAP`), so adopting it would implicitly adopt Arduino as a core dependency.

### 2) Raw-body upload endpoint (no multipart parsing)

- **Decision**: Use `PUT /api/graphics/<name>` with raw body.
- **Why**: `multipart/form-data` parsing adds complexity, memory pressure, and test surface area. `httpd_req_recv` makes streaming raw bodies straightforward and robust.

### 3) FATFS RW with wear levelling for uploads

- **Decision**: Use `esp_vfs_fat_spiflash_mount_rw_wl` and allow formatting on first boot (`format_if_mount_failed = true`).
- **Why**: We need runtime writes. The repo’s prebuilt-image strategy (`*_mount_ro`) is great for “flash once, read forever” assets but is not the right default for user uploads.

### 4) WebSocket: binary frames for terminal, JSON text for control

- **Decision**: Binary for UART bytes, JSON text for button/control.
- **Why**: Avoid base64/escaping for arbitrary bytes; keep button messages human-readable for debugging.

## Alternatives Considered

### Arduino + ESPAsyncWebServer

- **Pros**: Convenient APIs; proven in `ATOMS3R-CAM-M12-UserDemo`.
- **Cons**: Pulls Arduino into an ESP-IDF tutorial; mixes Arduino types (`String`, `File`, etc.) into `esp32-s3-m5` which has been intentionally ESP-IDF-first so far.

### FATFS prebuilt image (fatfsgen + parttool) mounted RO

- **Pros**: Great for curated asset packs (like GIFs in `0013-atoms3r-gif-console`).
- **Cons**: Doesn’t support runtime uploads unless we re-flash the partition from the host.

### SPIFFS / LittleFS for uploads

- **Pros**: Common in Arduino world; sometimes simpler APIs.
- **Cons**: ESP-IDF has stronger first-class support for FATFS + WL and we already have FATFS tooling/patterns in-repo.

## Implementation Plan

### 1) Create new tutorial project directory

- [ ] Create `esp32-s3-m5/0017-atoms3r-web-ui/` (name can be adjusted) as an ESP-IDF project
- [ ] Reuse M5GFX component wiring pattern (EXTRA_COMPONENT_DIRS) consistent with `0013-atoms3r-gif-console`
- [ ] Add `partitions.csv` with a `storage` FATFS partition sized for uploads (start with 2–6MB)

### 2) WiFi bring-up

- [ ] Implement SoftAP mode first (SSID like `AtomS3R-WebUI-XXXX`)
- [ ] Make STA mode optional via menuconfig (SSID/password)
- [ ] Expose IP + instructions on serial log at boot

### 3) Storage (FATFS RW)

- [ ] Mount `/storage` using `esp_vfs_fat_spiflash_mount_rw_wl`
- [ ] Ensure `/storage/graphics` exists (create on first boot)
- [ ] Add “factory reset storage” helper (optional: button hold at boot or a `DELETE /api/storage/reset`)

### 4) Display subsystem

- [ ] Reuse `display_hal.cpp` pattern and a `M5Canvas` present loop
- [ ] Implement `display_png_from_file(path)` (MVP: read entire file into memory, enforce size limit)
- [ ] Add a “status overlay” option (small text: WiFi state / last upload)

### 5) HTTP server

- [ ] Start `httpd_start` with reasonable config (stack size, max handlers)
- [ ] Implement static asset handlers (`/`, `/assets/app.js`, `/assets/app.css`)
- [ ] Implement `/api/status`
- [ ] Implement `/api/graphics` list + `GET/PUT/DELETE /api/graphics/<name>`

### 6) WebSocket

- [ ] Add `httpd_uri_t` handler for `/ws` with `is_websocket = true`
- [ ] Track connected WS clients (fd list), drop dead fds via `httpd_ws_get_fd_info`
- [ ] Implement binary terminal frames and JSON button frames

### 7) UART terminal

- [ ] Configure UART pins via menuconfig (default GROVE: TX=GPIO1, RX=GPIO2, but allow overrides)
- [ ] UART RX task reads bytes and pushes to WS broadcast
- [ ] WS handler receives binary frames and writes bytes to UART TX

### 8) Frontend (Preact + Zustand)

- [ ] Create `web/` folder (Vite + Preact) with deterministic output filenames
- [ ] Implement Zustand store for WS connection + message dispatch
- [ ] Implement graphics upload UI using `PUT` raw body
- [ ] Implement terminal UI that handles binary WS frames
- [ ] Add build step to embed the built assets into firmware

### 9) Acceptance criteria / demo script

- [ ] From a clean flash, device starts AP and logs SSID + IP
- [ ] Visiting `/` loads the UI from the device
- [ ] Uploading a PNG results in: file present in `/storage/graphics`, and the display updates
- [ ] Pressing the device button produces an event in the browser within ~100ms
- [ ] UART RX bytes show in browser terminal; typing in browser sends bytes back to UART

## Open Questions

- **GROVE pin mapping on your AtomS3R revision**: Kconfig defaults indicate G1=GPIO1, G2=GPIO2, but there has been investigation about swapping in other tickets. We should validate on hardware early and keep pins configurable.
- **Graphics formats**: MVP should be PNG (best supported by M5GFX). JPEG/GIF can be added later.
- **Auto-display behavior**: Should upload always display? Or should it just store and require manual selection?
- **Max upload size**: pick a safe default (e.g. 256KB) and enforce it.

## References

- **Repo (display bring-up)**: `esp32-s3-m5/0013-atoms3r-gif-console/main/display_hal.cpp`
- **Repo (FATFS mount semantics)**: `esp32-s3-m5/components/echo_gif/src/gif_storage.cpp`
- **ESP-IDF (WebSocket APIs)**: `/home/manuel/esp/esp-idf-5.4.1/components/esp_http_server/include/esp_http_server.h` (see `httpd_ws_recv_frame`, `httpd_ws_send_frame_async`)
- **ESP-IDF (FATFS mount APIs)**: `/home/manuel/esp/esp-idf-5.4.1/components/fatfs/vfs/esp_vfs_fat.h` (`esp_vfs_fat_spiflash_mount_rw_wl`, `esp_vfs_fat_spiflash_mount_ro`)

## Design Decisions

<!-- Document key design decisions and rationale -->

## Alternatives Considered

<!-- List alternative approaches that were considered and why they were rejected -->

## Implementation Plan

<!-- Outline the steps to implement this design -->

## Open Questions

<!-- List any unresolved questions or concerns -->

## References

<!-- Link to related documents, RFCs, or external resources -->
