---
Title: Diary
Ticket: MO-001-ATOMS3R-WEBSOCKET-STREAMING
Status: active
Topics:
    - firmware
    - esp32
    - camera
    - wifi
    - analysis
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/main/service/apis/api_camera.cpp
      Note: Camera capture and stream logic analyzed
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/main/service/service_web_server.cpp
      Note: WiFi setup and web server details reviewed
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/main/usb_webcam_main.cpp
      Note: Primary entry point reviewed in Step 1
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/README-GO.md
      Note: Pointed readers to the new firmware implementation
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/README.md
      Note: Noted the firmware path for real hardware
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/README.md
      Note: Firmware build + console usage
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/components/esp32-camera
      Note: Vendored camera component for build
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/components/esp_websocket_client
      Note: Vendored WebSocket client component for build
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/Kconfig.projbuild
      Note: Stream defaults (host/port/interval/quality)
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/console.c
      Note: esp_console WiFi and stream command handlers
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c
      Note: WebSocket streaming task + camera capture implementation
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/wifi_sta.c
      Note: WiFi STA state machine and NVS credential persistence
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/sdkconfig.defaults
      Note: USB Serial/JTAG console + PSRAM defaults
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/server.go
      Note: Go server protocol reviewed in Step 2
    - Path: ttmp/2026/01/14/MO-001-ATOMS3R-WEBSOCKET-STREAMING--atoms3r-websocket-streaming/analysis/01-stock-atoms3r-cam-userdemo-firmware-analysis.md
      Note: Detailed analysis produced in Step 1
    - Path: ttmp/2026/01/14/MO-001-ATOMS3R-WEBSOCKET-STREAMING--atoms3r-websocket-streaming/analysis/02-camera-streaming-protocol-and-console-requirements.md
      Note: Protocol and console requirements captured in Step 2
ExternalSources: []
Summary: Step-by-step diary for analysis and implementation work on MO-001-ATOMS3R-WEBSOCKET-STREAMING.
LastUpdated: 2026-01-14T08:46:59.521535506-05:00
WhatFor: Capture research and implementation steps for the ATOMS3R streaming ticket.
WhenToUse: Update after each analysis or implementation step.
---






# Diary

## Goal

Capture the research and analysis steps used to understand the stock ATOMS3R-CAM UserDemo firmware, with an emphasis on WiFi setup and camera data flow, so future implementation work is grounded in a verified baseline.

## Step 1: Survey stock firmware and document WiFi/camera data flow

This step focused on reading the stock ATOMS3R-CAM UserDemo firmware and extracting a detailed, textbook-style explanation of how the system boots, configures WiFi, and captures camera data. The outcome was a structured analysis document with diagrams, API references, and key file paths.

The analysis also surfaced several concurrency and configuration concerns (e.g., service-mode gating and mutex pairing) that should be kept in mind when designing websocket streaming.

### What I did
- Read the main entry point and service files (`main/usb_webcam_main.cpp`, `main/service/service_web_server.cpp`, `main/service/service_uvc.cpp`)
- Traced camera capture and streaming logic in `main/service/apis/api_camera.cpp` and `main/utils/camera/camera_init.c`
- Reviewed pin definitions and board pinout (`main/utils/camera/camera_pin.h`, `PINOUT.md`)
- Verified configuration defaults in `sdkconfig.old` and `main/Kconfig.projbuild`
- Authored the analysis document: `ttmp/2026/01/14/MO-001-ATOMS3R-WEBSOCKET-STREAMING--atoms3r-websocket-streaming/analysis/01-stock-atoms3r-cam-userdemo-firmware-analysis.md`

### Why
- Build a clear baseline of the stock firmware so websocket streaming work can align with existing WiFi and camera behavior.
- Capture key constraints and entry points to avoid regressions when extending the system.

### What worked
- The codebase is compact and clearly separates UVC, web server, and camera init paths.
- WiFi and camera APIs are centralized in a small set of files, making the data flow traceable.

### What didn't work
- N/A (no command failures or tooling issues during this step).

### What I learned
- WiFi is SoftAP-only with an open SSID (`AtomS3R-CAM-WiFi`) and no station mode.
- Camera streaming exists in two paths: USB UVC and HTTP MJPEG, gated by `SharedData::service_mode`.
- Static web UI content is stored in a custom `assetpool` partition and mapped at runtime.

### What was tricky to build
- Aligning Arduino-based networking (`initArduino`, `WiFi.softAP`) with ESP-IDF camera/UVC code paths.
- Tracing how `SharedData` is used to avoid UVC and web streaming conflicts.

### What warrants a second pair of eyes
- `sendJpg()` uses `SharedData::BorrowData()` without guaranteed `ReturnData()` on early returns; confirm if this can deadlock the mutex-backed `SharedData_StdMutex`.
- The camera power enable sequence does not explicitly set GPIO18 high/low; confirm board-level power behavior.
- The mismatch between UVC and web streaming gating (only MJPEG stream sets service mode) may allow concurrent access.

### What should be done in the future
- Review and correct `BorrowData`/`ReturnData` pairing in camera API handlers if needed.
- Confirm intended WiFi security posture (open AP) and adjust if websocket streaming needs authentication.
- Validate console configuration if USB Serial/JTAG is desired for ESP32-S3 workflows.

### Code review instructions
- Start with `ttmp/2026/01/14/MO-001-ATOMS3R-WEBSOCKET-STREAMING--atoms3r-websocket-streaming/analysis/01-stock-atoms3r-cam-userdemo-firmware-analysis.md`.
- Follow references to `main/usb_webcam_main.cpp`, `main/service/service_web_server.cpp`, and `main/service/apis/api_camera.cpp`.
- No tests or builds were run in this step.

### Technical details
- Commands used:
  - `rg -n "wifi|WiFi|WIFI" main`
  - `sed -n '1,240p' main/usb_webcam_main.cpp`
  - `sed -n '1,240p' main/service/service_web_server.cpp`
  - `sed -n '1,260p' main/service/apis/api_camera.cpp`

## Step 2: Analyze server.go protocol and console requirements

This step analyzed the Go-based streaming server (`server.go`) to identify the exact WebSocket protocol the ESP32 device must speak, including endpoint paths, message framing, and FFmpeg expectations. The output is a dedicated protocol/console analysis document that captures device requirements and highlights port mismatches with older Python docs.

I also surveyed repo playbooks and the `0029` WiFi console implementation to define the baseline `esp_console` requirements for runtime SSID and target host configuration, with USB Serial/JTAG as the preferred console backend.

### What I did
- Read `0040-atoms3r-cam-streaming/esp32-camera-stream/server.go` and `README-GO.md` to derive the wire protocol.
- Cross-checked `ARCHITECTURE.md` for differences in port usage vs the Go server.
- Reviewed `docs/01-websocket-over-wi-fi-esp-idf-playbook.md` and `0029-mock-zigbee-http-hub` console sources for esp_console + WiFi CLI patterns.
- Authored the analysis doc: `ttmp/2026/01/14/MO-001-ATOMS3R-WEBSOCKET-STREAMING--atoms3r-websocket-streaming/analysis/02-camera-streaming-protocol-and-console-requirements.md`.

### Why
- Ensure the ATOMS3R firmware speaks the exact protocol expected by the streaming server.
- Reuse proven `esp_console` patterns for WiFi setup and add target host configuration without hardcoded credentials.

### What worked
- `server.go` provides a clear, minimal protocol: binary WebSocket messages containing raw JPEG frames.
- The `0029` console pattern already supports runtime WiFi setup with NVS persistence.

### What didn't work
- N/A (no command failures or tooling issues during this step).

### What I learned
- The Go server expects camera frames on `ws://HOST:8080/ws/camera` (not 8765/8766).
- Each WebSocket binary message must be a complete JPEG with no metadata.
- The server does not coordinate multiple camera clients; one active camera is assumed.
- `esp_console` USB Serial/JTAG is the repo’s preferred console backend for ESP32-S3 targets.

### What was tricky to build
- Reconciling the Go server port/path details with older Python-oriented docs in `ARCHITECTURE.md`.
- Translating the console patterns from a hub-style demo into camera streaming needs (SSID + host/port).

### What warrants a second pair of eyes
- Confirm that the device-side JPEG framing assumptions match FFmpeg’s `image2pipe` behavior when bytes arrive in discrete WebSocket frames.
- Validate the proposed console command set for stream target configuration against the team’s preferred CLI design.

### What should be done in the future
- Implement the console commands in the ATOMS3R firmware (`wifi ...`, `stream set host/port`, `stream start/stop`).
- Add NVS persistence for stream target configuration (namespace `stream`).
- Decide whether to reject additional camera connections server-side to avoid interleaving frames.

### Code review instructions
- Start at `ttmp/2026/01/14/MO-001-ATOMS3R-WEBSOCKET-STREAMING--atoms3r-websocket-streaming/analysis/02-camera-streaming-protocol-and-console-requirements.md`.
- Follow references to `0040-atoms3r-cam-streaming/esp32-camera-stream/server.go` and `0029-mock-zigbee-http-hub/main/wifi_console.c`.
- No tests or builds were run in this step.

### Technical details
- Commands used:
  - `sed -n '1,260p' 0040-atoms3r-cam-streaming/esp32-camera-stream/server.go`
  - `sed -n '1,260p' 0029-mock-zigbee-http-hub/main/wifi_console.c`
  - `sed -n '1,360p' 0029-mock-zigbee-http-hub/main/wifi_sta.c`

## Step 3: Implement AtomS3R camera streaming firmware + console

This step implemented a real ESP-IDF firmware project inside `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware` to replace the mock camera with an actual AtomS3R-CAM client. The firmware connects to the Go server WebSocket endpoint and sends JPEG frames as binary messages, matching the protocol requirements documented earlier.

The build also includes a USB Serial/JTAG `esp_console` REPL to configure WiFi credentials and stream target host/port at runtime, with NVS persistence for both WiFi and stream settings.

### What I did
- Added a new ESP-IDF project under `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware` with `CMakeLists.txt`, `sdkconfig.defaults`, and `idf_component.yml`.
- Implemented WiFi STA bring-up and console commands (`wifi status/scan/set/connect/clear`) based on the `0029` pattern.
- Implemented a streaming client task that captures frames via `esp_camera`, converts to JPEG as needed, and sends them via `esp_websocket_client` to `/ws/camera`.
- Added stream console commands (`stream status/set/start/stop/clear`) with NVS persistence for host/port.
- Updated `README.md` and `README-GO.md` to point to the real device firmware.

### Why
- Replace the mock Python camera with a real device-side implementation that speaks the exact server protocol.
- Provide runtime configuration over `esp_console` so SSID and destination IP/port do not require recompilation.

### What worked
- The firmware structure mirrors proven patterns from `0029` and respects USB Serial/JTAG console guidance for ESP32-S3.
- The stream client uses binary WebSocket frames containing full JPEG payloads, matching `server.go` expectations.

### What didn't work
- N/A (no build or runtime errors encountered during this step).

### What I learned
- `esp_websocket_client` can be used with a simple event handler to track connection state and send binary frames from a dedicated task.
- Keeping the camera capture loop independent of console handlers avoids blocking the REPL.

### What was tricky to build
- Aligning camera pin mappings and power enable with AtomS3R-CAM hardware while keeping the project minimal.
- Ensuring stream target changes cleanly restart the WebSocket client without running in the event handler context.

### What warrants a second pair of eyes
- Validate camera pin configuration and power GPIO behavior on real AtomS3R-CAM hardware.
- Confirm `esp32-camera` and `esp-protocols` component versions are compatible with the target ESP-IDF version in use.
- Confirm JPEG quality and frame interval defaults are acceptable for WiFi throughput and latency.

### What should be done in the future
- Run on-device validation: confirm `stream start` connects and the Go server displays live video.
- Add backpressure handling if `esp_websocket_client_send_bin` starts failing under load.

### Code review instructions
- Start at `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c`.
- Review `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/console.c` for CLI behavior.
- Verify `sdkconfig.defaults` in `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/sdkconfig.defaults` sets USB Serial/JTAG console.
- No tests or builds were run in this step.

### Technical details
- Commands used:
  - `mkdir -p 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main`
  - `rg --files 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware`

## Step 4: Build firmware and vendor websocket/camera components

This step focused on getting the new firmware to build under ESP-IDF 5.4.1. The initial build failed because `esp_websocket_client` was not resolved via the component manager, so I vendored the needed components locally and re-ran the build with the correct ESP32-S3 target.

The build now completes successfully (with warnings), generating `atoms3r_cam_stream.bin`. There is also a note about the app partition being nearly full, which may warrant adjustments if features are added.

### What I did
- Ran `idf.py build` and `idf.py set-target esp32s3` to configure the project for ESP32-S3.
- Replaced the missing component dependency by copying in local components:
  - `components/esp_websocket_client` from `echo-base--openai-realtime-embedded-sdk`
  - `components/esp32-camera` from `ATOMS3R-CAM-UserDemo`
- Cleaned and re-ran the build until `atoms3r_cam_stream.bin` was produced.

### Why
- The build failed with `Failed to resolve component 'esp_websocket_client'`; vendoring ensures the dependency is available without relying on the component manager.

### What worked
- `idf.py set-target esp32s3` configured the project correctly.
- `idf.py build` completed and generated the firmware binary.

### What didn't work
- Initial build failures:
  - `idf.py build` → `Failed to resolve component 'esp_websocket_client' required by component 'main'`
  - `idf.py set-target esp32s3` failed for the same reason until the component was vendored.

### What I learned
- The local ESP-IDF setup did not resolve `esp_websocket_client` from the component manager; vendoring the component removes that dependency.

### What was tricky to build
- Ensuring the build target was ESP32-S3 (`idf.py set-target esp32s3`) and not the default ESP32.
- Managing local components without conflicting with the component manager workflow.

### What warrants a second pair of eyes
- The build warning: app partition is nearly full (4% free); confirm partition sizing before adding features.
- Camera component warnings (e.g., deprecated GDMA API) are expected but may need future updates.

### What should be done in the future
- Consider increasing the app partition size if additional features are planned.
- Decide whether to keep vendored components or switch back to a component manager dependency flow.

### Code review instructions
- Start at `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/components/esp_websocket_client` and `.../components/esp32-camera` to confirm vendored sources.
- Rebuild: `idf.py -C 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware build`

### Technical details
- Commands used:
  - `idf.py set-target esp32s3`
  - `idf.py build`
  - `cp -a .../esp_websocket_client firmware/components/`
  - `cp -a .../esp32-camera firmware/components/`
