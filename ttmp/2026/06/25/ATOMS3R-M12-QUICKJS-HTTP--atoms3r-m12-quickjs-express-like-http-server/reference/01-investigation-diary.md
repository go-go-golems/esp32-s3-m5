---
Title: Investigation Diary
Ticket: ATOMS3R-M12-QUICKJS-HTTP
Status: active
Topics:
    - atoms3r
    - esp32s3
    - quickjs
    - javascript
    - firmware
    - http
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0103-atoms3r-m12-native-quickjs/main/CMakeLists.txt
      Note: Adds http_server.cpp and esp_http_server dependency (commit 7757d75)
    - Path: 0103-atoms3r-m12-native-quickjs/main/app_main.cpp
      Note: Registers HTTP console commands during firmware startup (commit 7757d75)
    - Path: 0103-atoms3r-m12-native-quickjs/main/http_server.cpp
      Note: Host-owned esp_http_server lifecycle
    - Path: 0103-atoms3r-m12-native-quickjs/main/http_server.h
      Note: Public HTTP host service API and command registration declaration (commit 7757d75)
ExternalSources: []
Summary: Chronological diary for the Express-like HTTP server on AtomS3R M12 QuickJS.
LastUpdated: 2026-06-25T23:59:00-07:00
WhatFor: Use to resume or review HTTP serving, static assets, and JavaScript route registration work.
WhenToUse: Read before adding `esp_http_server`, static file serving, or QuickJS dynamic route dispatch.
---


# Diary

## Goal

Capture the HTTP server work for `0103-atoms3r-m12-native-quickjs`: simple serving, static assets from storage/flash, and an Express-like JavaScript API inspired by `go-go-goja` but sized for ESP32-S3 firmware.

## Step 1: Create the HTTP ticket and intern guide

This step creates a dedicated HTTP implementation track. The design takes inspiration from `go-go-goja`'s Express module and static asset helpers, but intentionally removes auth, sessions, resource policies, and desktop assumptions.

The embedded design separates static serving from dynamic JavaScript route handling. Static files should stream from FatFs outside QuickJS. Dynamic handlers should run through `qjs_service_run()` so HTTP server tasks never call QuickJS directly.

### Prompt Context

**User prompt (verbatim):**

> Create a new ticket for each, and Create  a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and the nupload to remarkable.
>
> Then add tasks and work on them in sequence, keeping a detailed log as you go.

**Assistant interpretation:** Create an HTTP ticket with a detailed guide and task list, using `go-go-goja` as inspiration for a simple embedded Express-like API.

**Inferred user intent:** Add web serving to the AtomS3R QuickJS firmware in a way that is understandable, constrained, and compatible with the existing storage and WiFi work.

**Commit (code):** N/A — design-only step.

### What I did

- Created ticket `ATOMS3R-M12-QUICKJS-HTTP`.
- Added a design document and diary document.
- Replaced the generated task list with phased HTTP tasks.
- Inspected `go-go-goja` references:
  - `modules/express/express.go`
  - `modules/express/typescript.go`
  - `modules/fs/http.go`
- Wrote the HTTP analysis/design/implementation guide.

### Why

- HTTP serving is a separate subsystem from WiFi and storage.
- The intern guide needs to explain ESP-IDF HTTP serving, static assets, QuickJS dispatch, route tables, request/response DTOs, and embedded limits.
- The user asked for Express-like ergonomics without auth complexity.

### What worked

- The guide defines a global `http` API, static file serving, dynamic route dispatch, limits, route lifecycle, and reset behavior.
- The task list sequences host service, static assets, QuickJS route registration, and script workflow.

### What didn't work

- N/A for this documentation step.

### What I learned

- The relevant Goja pattern is host-owned HTTP server lifecycle plus JavaScript route registration. The embedded firmware should keep that idea but avoid desktop-only features such as generic `http.Handler` mounting and auth builders.

### What was tricky to build

- The design needed to balance Express familiarity with QuickJS ownership constraints. The result is not a full Express clone; it is an embedded global `http` object with familiar methods and strict limits.

### What warrants a second pair of eyes

- Review whether dynamic JavaScript routes should be included in the first HTTP implementation or delayed until static serving is validated.
- Review response/body limits before implementation.
- Review runtime-reset behavior for registered JS handlers.

### What should be done in the future

- Implement WiFi first.
- Add `esp_http_server` host service with `/healthz`.
- Add static storage-backed serving before dynamic QuickJS handlers.
- Add route registration only after server and storage serving are stable.

### Code review instructions

- Start with the HTTP guide's architecture and pseudocode.
- Compare against `go-go-goja/modules/express/express.go` for API inspiration, but avoid copying auth machinery.
- Validate static serving before dynamic route dispatch.

### Technical details

- Ticket path: `ttmp/2026/06/25/ATOMS3R-M12-QUICKJS-HTTP--atoms3r-m12-quickjs-express-like-http-server/`.
- Design doc: `design-doc/01-analysis-design-and-implementation-guide.md`.
- Goja references:
  - `/home/manuel/code/wesen/go-go-golems/go-go-goja/modules/express/express.go`
  - `/home/manuel/code/wesen/go-go-golems/go-go-goja/modules/express/typescript.go`
  - `/home/manuel/code/wesen/go-go-golems/go-go-goja/modules/fs/http.go`

## Step 2: Implement and validate the host-owned HTTP health server

This step turns the HTTP design into the first runnable firmware slice. It adds an ESP-IDF `esp_http_server` wrapper owned by the firmware host, plus console lifecycle commands and two built-in routes that do not enter QuickJS.

The milestone proves that WiFi, the HTTP server task, and external client access work together before static assets or JavaScript route registration are added. This keeps the first HTTP slice small: `/healthz` is firmware-owned, deterministic, and safe to test repeatedly over the AtomS3R's WiFi connection.

### Prompt Context

**User prompt (verbatim):** "continue"

**Assistant interpretation:** Resume the AtomS3R HTTP work from the compacted session by committing and documenting the already validated Phase 1 host HTTP server.

**Inferred user intent:** Move the HTTP ticket forward in sequence, preserving a focused commit and a reviewable implementation diary before starting static assets or QuickJS route dispatch.

**Commit (code):** `7757d75` — "0103: add host HTTP health server"

### What I did

- Added `0103-atoms3r-m12-native-quickjs/main/http_server.h`.
- Added `0103-atoms3r-m12-native-quickjs/main/http_server.cpp`.
- Registered `http_server.cpp` and the `esp_http_server` dependency in `0103-atoms3r-m12-native-quickjs/main/CMakeLists.txt`.
- Included `http_server.h` from `0103-atoms3r-m12-native-quickjs/main/app_main.cpp`.
- Registered HTTP console commands during startup with `register_http_commands()`.
- Implemented console commands:
  - `http status`
  - `http start [port]`
  - `http stop`
- Implemented built-in routes:
  - `GET /healthz` -> `ok\n`
  - `GET /` -> small HTML landing page linking to `/healthz`
- Committed the focused firmware change as `7757d75`.
- Marked HTTP ticket tasks H1.1 through H1.4 complete.

### Why

- The HTTP milestone needs a host-owned server before any JavaScript dynamic route registration is safe.
- `/healthz` validates the network path without involving QuickJS, FatFs, route tables, request body parsing, or user scripts.
- Console lifecycle commands keep USB Serial/JTAG as the recovery path: the operator can inspect, start, and stop the server without relying on JavaScript.

### What worked

- The firmware built with `esp_http_server` included; the hardware-smoke binary was reported as `0x14d5f0`, leaving roughly 67% of the app partition free.
- `http start 80` succeeded on the AtomS3R after WiFi was connected.
- External validation over WiFi succeeded:
  - `curl http://192.168.4.22/healthz` returned `ok`.
  - `curl http://192.168.4.22/` returned the built-in HTML landing page.
- The implementation did not require any QuickJS changes, so it keeps Phase 1 independent of runtime ownership and reset handling.

### What didn't work

- N/A for this step. The Phase 1 host server path was already hardware-smoked before this diary update and code commit.

### What I learned

- The smallest useful HTTP milestone is a firmware-owned health route, not an early JavaScript route. It exercises WiFi, `esp_http_server`, external reachability, and console lifecycle control while leaving QuickJS out of the request path.
- `esp_http_server` fits the current firmware size budget alongside WiFi, storage, and QuickJS, but internal RAM remains a constraint for later TLS, body parsing, and dynamic JavaScript handlers.

### What was tricky to build

- The server lifecycle must remain host-owned. The initial implementation deliberately exposes only console commands and built-in handlers; it does not store JavaScript callbacks or allow HTTP tasks to call QuickJS directly.
- The server's control port is derived from the requested data port. Future cleanup should guard edge cases such as `65535`, where `port + 1` wraps, before this becomes a user-facing API.
- `ESP_ERROR_CHECK` is acceptable for this first milestone's handler registration during controlled startup, but later dynamic/static registration paths should return errors instead of aborting the firmware.

### What warrants a second pair of eyes

- Review `http_server_start()` error handling around route registration. If `/healthz` or `/` registration fails after `httpd_start()`, the current code aborts via `ESP_ERROR_CHECK`; production-style code should stop the server and return the error.
- Review the HTTP server stack size and URI handler count before adding static streaming and dynamic QuickJS handlers.
- Review whether `http start <port>` should return `ESP_ERR_INVALID_STATE` when the server is already running on a different port instead of silently returning `ESP_OK`.

### What should be done in the future

- Add Phase 2 static serving from bounded storage virtual roots.
- Add MIME type detection and chunked file streaming before dynamic JavaScript route dispatch.
- Add a QuickJS `http` namespace only after static serving is validated.

### Code review instructions

- Start with `0103-atoms3r-m12-native-quickjs/main/http_server.cpp` and read `http_server_start()`, `http_server_stop()`, `cmd_http()`, `healthz_handler()`, and `root_handler()`.
- Then review `0103-atoms3r-m12-native-quickjs/main/app_main.cpp` to confirm `register_http_commands()` is wired alongside storage, WiFi, and JS console commands.
- Validate with:
  - `idf.py build`
  - Flash/monitor over `/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_B4:3A:45:BE:16:80-if00`
  - `wifi status`
  - `http status`
  - `http start 80`
  - `curl http://<device-ip>/healthz`
  - `curl http://<device-ip>/`

### Technical details

- HTTP server implementation files:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0103-atoms3r-m12-native-quickjs/main/http_server.h`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0103-atoms3r-m12-native-quickjs/main/http_server.cpp`
- Wiring files:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0103-atoms3r-m12-native-quickjs/main/CMakeLists.txt`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0103-atoms3r-m12-native-quickjs/main/app_main.cpp`
- Hardware validation target:
  - AtomS3R M12 ESP32-S3 over USB Serial/JTAG by-id path `/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_B4:3A:45:BE:16:80-if00`.
- Observed HTTP validation IP:
  - `192.168.4.22` on the configured guest WiFi network.
