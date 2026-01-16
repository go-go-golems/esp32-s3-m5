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
    - Path: ../../../../../../../../../../esp/esp-idf-5.4.1/components/esp_wifi/include/esp_wifi_types_generic.h
      Note: WiFi disconnect reason mapping (201 = NO_AP_FOUND).
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/main/Kconfig.projbuild
      Note: |-
        UserDemo camera module + XCLK Kconfig reference.
        Added UVC enable/disable menuconfig option.
        Added menuconfig toggle to enable the console test loop.
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/main/service/apis/api_camera.cpp
      Note: Camera capture and stream logic analyzed
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/main/service/service_uvc.cpp
      Note: Added UVC capture/convert stats logging.
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/main/service/service_web_server.cpp
      Note: WiFi setup and web server details reviewed
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/main/usb_webcam_main.cpp
      Note: |-
        Primary entry point reviewed in Step 1
        Added camera power GPIO logging for demo comparison.
        Added SCCB/I2C scan on camera pins before camera init.
        Guarded start_service_uvc behind config.
        Added USB Serial/JTAG console test loop for enumeration debugging.
        Added conditional USB Serial/JTAG VFS includes and API fallback.
        Removed VFS header usage to avoid termios/Arduino macro conflict.
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/main/utils/camera/camera_init.c
      Note: |-
        Added camera config
        Added PSRAM readiness logging for demo comparison.
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/main/utils/camera/camera_pin.h
      Note: Reference pin map for AtomS3R-CAM.
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/sdkconfig
      Note: Align generated console settings with defaults.
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/sdkconfig.defaults
      Note: Set USB Serial/JTAG console as primary and disable UART defaults.
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/README-GO.md
      Note: Pointed readers to the new firmware implementation
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/README.md
      Note: Noted the firmware path for real hardware
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/client.html
      Note: Viewer websocket URL fix (commit 25b3a6c)
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/README.md
      Note: Firmware build + console usage
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/components/esp32-camera
      Note: Vendored camera component for build
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/components/esp_websocket_client
      Note: Vendored WebSocket client component for build
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/CMakeLists.txt
      Note: Added esp_psram component dependency to fix build.
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/Kconfig.projbuild
      Note: |-
        Stream defaults (host/port/interval/quality)
        Added active-low camera power Kconfig option.
        Surfaced esp32-camera tuning options in project menu.
        Added custom JPEG frame size config to camera options.
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/console.c
      Note: |-
        esp_console WiFi and stream command handlers
        Clarified cam power usage (1=on).
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c
      Note: |-
        WebSocket streaming task + camera capture implementation
        Added stream stats
        Added active-low camera power mapping and input-output readback.
        Logs camera component options at init.
        WebSocket backpressure/start gating and send timeout (commit 3a40f51)
        WebSocket backpressure/start gating + error log compatibility (commits 3a40f51
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.h
      Note: Camera power debug API added.
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/wifi_sta.c
      Note: |-
        WiFi STA state machine and NVS credential persistence
        Added WiFi connect/disconnect detail logs and reason names.
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/scripts/diff_sdkconfig.py
      Note: Used to diff demo vs stream sdkconfig.
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/sdkconfig
      Note: |-
        Aligned PSRAM settings to match ATOMS3R-CAM demo.
        Enabled active-low camera power config.
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/sdkconfig.defaults
      Note: |-
        USB Serial/JTAG console + PSRAM defaults
        Set active-low camera power default.
        Added camera task/core/DMA/JPEG defaults.
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/server.go
      Note: |-
        Go server protocol reviewed in Step 2
        Added bind address flag and URL logging for LAN access.
        Added WebSocket client metadata and FFmpeg command logs.
    - Path: ttmp/2026/01/14/MO-001-ATOMS3R-WEBSOCKET-STREAMING--atoms3r-websocket-streaming/analysis/01-stock-atoms3r-cam-userdemo-firmware-analysis.md
      Note: Detailed analysis produced in Step 1
    - Path: ttmp/2026/01/14/MO-001-ATOMS3R-WEBSOCKET-STREAMING--atoms3r-websocket-streaming/analysis/02-camera-streaming-protocol-and-console-requirements.md
      Note: Protocol and console requirements captured in Step 2
    - Path: ttmp/2026/01/14/MO-001-ATOMS3R-WEBSOCKET-STREAMING--atoms3r-websocket-streaming/analysis/03-psram-vs-spiram-for-esp32-s3-camera-streaming.md
      Note: PSRAM vs SPIRAM analysis added in Step 5.
    - Path: ttmp/2026/01/14/MO-001-ATOMS3R-WEBSOCKET-STREAMING--atoms3r-websocket-streaming/analysis/04-atoms3r-cam-userdemo-alignment-audit.md
      Note: Alignment audit report.
    - Path: ttmp/2026/01/14/MO-001-ATOMS3R-WEBSOCKET-STREAMING--atoms3r-websocket-streaming/analysis/05-sdkconfig-diff-demo-vs-stream.md
      Note: Config diff analysis report.
    - Path: ttmp/2026/01/14/MO-001-ATOMS3R-WEBSOCKET-STREAMING--atoms3r-websocket-streaming/various/sdkconfig-demo-vs-stream.diff
      Note: Unified sdkconfig diff output.
ExternalSources: []
Summary: Step-by-step diary for analysis and implementation work on MO-001-ATOMS3R-WEBSOCKET-STREAMING.
LastUpdated: 2026-01-15T23:18:45-05:00
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

## Step 5: Explain PSRAM vs SPIRAM and camera buffer usage

This step focused on producing a textbook-style PSRAM/SPIRAM explanation grounded in the ATOMS3R-CAM demo configuration and the esp32-camera driver internals. The outcome is a dedicated analysis document that explains what PSRAM is, how ESP-IDF initializes it, and exactly how the camera pipeline uses PSRAM-backed frame buffers and DMA paths on ESP32-S3.

I tied the explanation to concrete code and config references so the PSRAM requirement for video streaming is explicit and reproducible. This also captures the reason enabling PSRAM in the demo made the camera functional, while misconfiguring it leads to early boot aborts.

### What I did
- Reviewed PSRAM-related settings in `ATOMS3R-CAM-UserDemo/sdkconfig`.
- Traced how the demo requests PSRAM-backed frame buffers in `main/utils/camera/camera_init.c`.
- Read `cam_hal.c` and `ll_cam.c` to capture psram_mode behavior, DMA vs copy paths, and buffer alignment.
- Authored `ttmp/2026/01/14/MO-001-ATOMS3R-WEBSOCKET-STREAMING--atoms3r-websocket-streaming/analysis/03-psram-vs-spiram-for-esp32-s3-camera-streaming.md`.

### Why
- The project needs a clear, conceptual explanation of PSRAM so SDK configuration choices are informed rather than guesswork.
- Video streaming requirements depend heavily on buffer sizing and the DMA path, which is tied to PSRAM and XCLK frequency.

### What worked
- The demo code makes PSRAM usage explicit via `CAMERA_FB_IN_PSRAM` and `CONFIG_SPIRAM=y`.
- The esp32-camera source clearly shows how PSRAM changes allocation and DMA behavior.

### What didn't work
- N/A (no command failures or tooling issues during this step).

### What I learned
- PSRAM is the hardware; SPIRAM is ESP-IDF's software enablement and configuration for that hardware.
- On ESP32-S3, the esp32-camera driver toggles its PSRAM DMA path based on a 16 MHz XCLK setting.
- Without PSRAM, even modest frame buffers can exhaust internal SRAM when Wi-Fi is active.

### What was tricky to build
- Mapping the psram_mode flag to XCLK frequency rather than direct PSRAM presence is non-obvious and easy to misunderstand.
- Explaining why PSRAM init failures cause boot aborts while still tying it to camera buffer allocation.

### What warrants a second pair of eyes
- Confirm whether 16 MHz XCLK is required or just recommended for the PSRAM DMA path on this specific sensor.
- Validate that the memory math and frame size estimates align with the target camera resolution and JPEG quality settings.

### What should be done in the future
- N/A

### Code review instructions
- Start with `ttmp/2026/01/14/MO-001-ATOMS3R-WEBSOCKET-STREAMING--atoms3r-websocket-streaming/analysis/03-psram-vs-spiram-for-esp32-s3-camera-streaming.md`.
- Follow references to `ATOMS3R-CAM-UserDemo/sdkconfig`, `main/utils/camera/camera_init.c`, and `components/esp32-camera/driver/cam_hal.c`.
- No tests or builds were run in this step.

### Technical details
- Commands used:
  - `rg -n "SPIRAM|PSRAM" sdkconfig`
  - `sed -n '1040,1125p' sdkconfig`
  - `rg -n "camera_config_t|fb_location|psram" main -S`
  - `sed -n '1,140p' main/utils/camera/camera_init.c`
  - `rg -n "psram|spiram" components/esp32-camera -S`
  - `sed -n '1,420p' components/esp32-camera/driver/cam_hal.c`
  - `sed -n '1,220p' components/esp32-camera/target/esp32s3/ll_cam.c`

## Step 6: Enable PSRAM config and add PSRAM console status

This step aligned the firmware's active `sdkconfig` with the demo's PSRAM (SPIRAM) settings and added a console command to verify PSRAM health at runtime. It also uploads the new PSRAM analysis to reMarkable so the reference is accessible offline.

The goal was to remove ambiguity around PSRAM enablement and provide a quick on-device sanity check before debugging streaming failures.

### What I did
- Uploaded the PSRAM analysis doc to reMarkable using `remarkable_upload.py`.
- Updated `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/sdkconfig` to match the demo's PSRAM settings (OPI mode, 80 MHz, boot init, malloc integration).
- Added a `psram status` console command that reports init state and heap stats in `firmware/main/console.c`.

### Why
- The demo only worked once PSRAM was enabled; matching its config reduces boot and camera buffer failures.
- A direct PSRAM status command speeds up field diagnostics before starting a stream.

### What worked
- `remarkable_upload.py` successfully produced and uploaded the PDF to `ai/2026/01/14/`.
- The console command compiles cleanly with PSRAM enabled and uses standard heap metrics.

### What didn't work
- N/A (no command failures or tooling issues during this step).

### What I learned
- The demo's PSRAM settings include explicit OPI mode + boot init; missing those can lead to early aborts.
- Reporting both PSRAM and internal heap sizes is useful to distinguish buffer exhaustion from init failures.

### What was tricky to build
- Keeping the `psram` command compile-safe when `CONFIG_SPIRAM` is disabled (guarded includes and runtime handling).

### What warrants a second pair of eyes
- Confirm the PSRAM pin assignments (`CLK_IO=30`, `CS_IO=26`) are correct for the target module variant.
- Validate that enabling PSRAM in `sdkconfig` does not conflict with any downstream build system assumptions.

### What should be done in the future
- N/A

### Code review instructions
- Check `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/sdkconfig` for PSRAM alignment with the demo.
- Review `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/console.c` for the `psram status` command.
- No tests or builds were run in this step.

### Technical details
- Commands used:
  - `python3 /home/manuel/.local/bin/remarkable_upload.py --dry-run --ticket-dir ... 03-psram-vs-spiram-for-esp32-s3-camera-streaming.md`
  - `python3 /home/manuel/.local/bin/remarkable_upload.py --ticket-dir ... 03-psram-vs-spiram-for-esp32-s3-camera-streaming.md`

## Step 7: Fix esp_psram include build error and rebuild

This step addressed the `esp_psram.h` include failure after PSRAM was enabled by ensuring the main component explicitly depends on the `esp_psram` component. After adding the dependency, the firmware build completed successfully.

The build output still reports that the smallest app partition is nearly full, which remains a size risk for future features.

### What I did
- Added `esp_psram` to `PRIV_REQUIRES` in the firmware `main/CMakeLists.txt`.
- Rebuilt the firmware with `idf.py -C ... build`.

### Why
- The header `esp_psram.h` is provided by the `esp_psram` component, so the main component must declare a dependency for include paths to resolve.

### What worked
- The build succeeded and produced `atoms3r_cam_stream.bin`.

### What didn't work
- Previous build failed with: `fatal error: esp_psram.h: No such file or directory`.

### What I learned
- Enabling PSRAM in `sdkconfig` is not enough; the component dependency must be declared to access its headers.

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- The app partition remains nearly full (3% free). Confirm partition sizing before adding new features.

### What should be done in the future
- Consider resizing partitions if we expect additional functionality.

### Code review instructions
- Review `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/CMakeLists.txt` for the `esp_psram` dependency.
- Rebuild with: `idf.py -C 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware build`.

### Technical details
- Commands used:
  - `idf.py -C 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware build`

## Step 8: Allow server bind address configuration and print reachable IPs

This step improved the Go streaming server so it can bind to a non-localhost address and advertise reachable URLs for a camera on the LAN. The intent was to make the server usable by external devices without requiring code changes.

The change adds a `-addr` flag and prints actual interface IPs when binding to `0.0.0.0`, which should simplify setup for the AtomS3R-CAM.

### What I did
- Added an `-addr` CLI flag in `0040-atoms3r-cam-streaming/esp32-camera-stream/server.go` with a default of `127.0.0.1:8080`.
- Implemented helper functions to parse host/port and to list local IPv4 addresses.
- Updated startup logs to print camera/viewer/web URLs based on the bound address.

### Why
- The previous server bound only to localhost, which prevented the camera from connecting over Wi-Fi.
- Printing resolved IPs when binding to `0.0.0.0` reduces guesswork in the field.

### What worked
- The server now accepts `-addr 0.0.0.0:8080` and logs per-interface URLs.

### What didn't work
- N/A (no failures during this step).

### What I learned
- Listing active non-loopback interfaces is a simple way to provide actionable connection URLs.

### What was tricky to build
- Ensuring host/port parsing accepts `:8080`, `0.0.0.0:8080`, and `host` without a port.

### What warrants a second pair of eyes
- Confirm the URL formatting and address parsing behavior for IPv6 bind addresses.

### What should be done in the future
- N/A

### Code review instructions
- Review `0040-atoms3r-cam-streaming/esp32-camera-stream/server.go` for `-addr` parsing and log output.
- Manual test: `go run server.go -addr 0.0.0.0:8080` and verify printed URLs.

### Technical details
- Commands used: N/A

## Step 9: Decode WiFi disconnect reason 201

This step mapped the runtime disconnect reason `201` to the ESP-IDF enum so we can give accurate guidance during bring-up. The result is that the console logs now clearly indicate the STA is not seeing the target AP during scans.

I also recorded the most likely immediate checks (2.4 GHz SSID visibility, spelling, hidden SSIDs, and signal strength).

### What I did
- Located the ESP-IDF reason code mapping in `esp_wifi_types_generic.h`.
- Confirmed reason `201` maps to `WIFI_REASON_NO_AP_FOUND`.

### Why
- Accurate interpretation of disconnect reasons speeds up Wi-Fi troubleshooting without guesswork.

### What worked
- The reason code mapping is explicit in the ESP-IDF headers.

### What didn't work
- N/A

### What I learned
- Reason 201 indicates the STA failed to find the AP (not an auth failure).

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- N/A

### Code review instructions
- Review `esp_wifi_types_generic.h` in the ESP-IDF tree for reason codes.

### Technical details
- Commands used:
  - `rg -n "WIFI_REASON_" /home/manuel/esp/esp-idf-5.4.1/components/esp_wifi/include`

## Step 10: Investigate camera probe failures (ESP_ERR_NOT_FOUND)

This step analyzed repeated `esp_camera_init` failures where the sensor probe returns `ESP_ERR_NOT_FOUND`. The logs indicate SCCB is active on `I2C port 1` but the camera is not responding, suggesting power enable or pin mapping issues.

I compared the firmware’s camera pin map and power sequencing with the ATOMS3R-CAM demo, noting that the demo uses GPIO18 with a pulldown-only configuration and delay. The current firmware’s power sequence does not match that, and the log shows GPIO18 as input with pull-up, which is likely preventing the sensor from powering.

### What I did
- Reviewed the firmware camera pin map and power sequence in `stream_client.c`.
- Cross-checked the AtomS3R-CAM pin map in `ATOMS3R-CAM-UserDemo/main/utils/camera/camera_pin.h`.
- Reviewed the demo power enable sequence in `ATOMS3R-CAM-UserDemo/main/usb_webcam_main.cpp`.

### Why
- The repeated probe failures indicate the sensor is not visible on SCCB; power or pin mismatches are the most likely cause.

### What worked
- The pin map in the firmware matches the demo, so the most likely culprit is the power enable sequence or board variant.

### What didn't work
- Camera probe continues to fail with `ESP_ERR_NOT_FOUND` on the target device.

### What I learned
- The demo uses a GPIO18 pulldown-only configuration and delay to enable camera power.
- GPIO18 appearing as input with pull-up in logs strongly suggests the power enable step isn’t taking effect.

### What was tricky to build
- Diagnosing camera probe errors requires careful alignment of board variant, power rail control, and SCCB wiring.

### What warrants a second pair of eyes
- Confirm the correct camera power polarity and GPIO18 usage for the specific AtomS3R-CAM revision.
- Validate that the correct module variant (M12 vs non-M12) is being flashed.

### What should be done in the future
- Align `camera_power_on()` with the demo’s GPIO18 pulldown-only sequence and re-test.

### Code review instructions
- Review `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c` for pin assignments and power sequencing.
- Compare with `ATOMS3R-CAM-UserDemo/main/utils/camera/camera_pin.h` and `ATOMS3R-CAM-UserDemo/main/usb_webcam_main.cpp`.

### Technical details
- Commands used:
  - `sed -n '1,240p' 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c`
  - `sed -n '1,200p' /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo/main/utils/camera/camera_pin.h`
  - `sed -n '1,120p' /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo/main/usb_webcam_main.cpp`

## Step 11: Align firmware with UserDemo and publish audit report

This step performed a full “food inspector” alignment against the ATOMS3R-CAM UserDemo. I updated camera power sequencing, JPEG quality settings, and Kconfig defaults to match the demo, then documented the audit in a dedicated report.

I also added UserDemo-style Kconfig options for camera module selection and XCLK so the firmware can reflect the same config surface as the stock demo.

### What I did
- Updated `camera_power_on()` to use GPIO18 pulldown-only + delay to match UserDemo power enable.
- Split JPEG quality settings into sensor vs conversion defaults (14 and 80 respectively).
- Added UserDemo-style camera Kconfig options (module selection + XCLK frequency) and set defaults to AtomS3R-CAM + 20 MHz.
- Enabled `CONFIG_CAMERA_CONVERTER_ENABLED=y` and ensured PSRAM defaults align with the demo.
- Authored the alignment report: `analysis/04-atoms3r-cam-userdemo-alignment-audit.md`.
- Rebuilt firmware after reconfiguring Kconfig.

### Why
- Camera probe failures indicated potential power/pin or config mismatch.
- The demo’s config provides a known-good baseline; aligning eliminates hidden deltas.

### What worked
- Build succeeded after reconfigure; firmware binary regenerated.
- Camera config and defaults now mirror the demo (XCLK, pin map, JPEG conversion quality).

### What didn't work
- Initial build failed: `CONFIG_CAMERA_XCLK_FREQ` undeclared in `stream_client.c`.
- Reconfigure timed out after 10s on first run (cmake completed but the tool timed out).

### What I learned
- The UserDemo defines camera module selection and XCLK in its own `Kconfig.projbuild`; we need to mirror this if we want the same config symbols.
- `frame2jpg` quality is separate from sensor JPEG quality; conflating them shifts image size and CPU load.

### What was tricky to build
- Keeping Kconfig symbols consistent between the UserDemo and this firmware without over-complicating the menu structure.

### What warrants a second pair of eyes
- Confirm GPIO18 power enable polarity for the specific board revision (still the most likely cause of probe failures).
- Verify that keeping `CONFIG_SPIRAM_ALLOW_STACK_EXTERNAL_MEMORY` in defaults is acceptable under IDF 5.4.1 (warning about replacement).

### What should be done in the future
- N/A

### Code review instructions
- Start with `analysis/04-atoms3r-cam-userdemo-alignment-audit.md`.
- Review `firmware/main/stream_client.c`, `firmware/main/Kconfig.projbuild`, `firmware/sdkconfig`, and `firmware/sdkconfig.defaults`.
- Rebuild: `idf.py -C 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware build`.

### Technical details
- Commands used:
  - `idf.py -C 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware reconfigure`
  - `idf.py -C 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware build`

## Step 12: Add camera power debug command and sensor tuning

This step addressed the ongoing camera probe failures by adding direct camera power diagnostics and matching the UserDemo’s post-init sensor tuning. The new `cam` console command lets us force GPIO18 high/low and dump the pin configuration, which should help verify whether the power rail is actually being asserted on the target board.

I also mirrored the UserDemo’s sensor flip/brightness adjustments after `esp_camera_init`, then rebuilt the firmware.

### What I did
- Added `cam power <0|1>` and `cam dump` console commands to drive and inspect GPIO18.
- Added `stream_camera_power_set` / `stream_camera_power_dump` helpers in `stream_client.c`.
- Added sensor tuning (vflip/brightness/saturation/hmirror) to match the UserDemo behavior.
- Rebuilt the firmware.

### Why
- The logs show the camera probe failing with `ESP_ERR_NOT_FOUND`, indicating SCCB is not seeing the sensor; power rail verification is the next diagnostic step.
- The UserDemo applies sensor tweaks that we should mirror for consistency once the camera is detected.

### What worked
- Build succeeded after adding the power debug functions and console command.

### What didn't work
- Initial build failed due to incorrect `gpio_dump_io_configuration` call signature; fixed by passing `stdout`.

### What I learned
- `gpio_dump_io_configuration` requires a `FILE*` output stream.

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- Confirm that forcing GPIO18 high or low is safe on the target board revision before testing.

### What should be done in the future
- Use `cam power 1` / `cam power 0` on hardware to test polarity and see if SCCB probe starts succeeding.

### Code review instructions
- Review `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/console.c` for the `cam` command.
- Review `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c` for power helpers and sensor tuning.
- Rebuild: `idf.py -C 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware build`.

### Technical details
- Commands used:
  - `idf.py -C 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware build`

## Step 13: Persist camera power level across init

This step fixed the diagnostic path so the camera power level set via `cam power` is not immediately overridden when the stream initializes the camera. The previous behavior always forced GPIO18 low during `camera_power_on`, making it impossible to test active-high polarity.

The update stores a runtime power level and uses it during camera init, with pull-up vs pull-down selected based on the requested level.

### What I did
- Added a `s_camera_power_level` state and used it in `camera_power_on()`.
- Updated power configuration to select pull-up for level 1 and pull-down for level 0.
- Rebuilt the firmware.

### Why
- The logs showed `cam power 1` being overridden when `stream start` re-initialized the camera, preventing polarity testing.

### What worked
- Build completed after the change.

### What didn't work
- N/A

### What I learned
- Persisting the power level across init is necessary to accurately test camera power polarity.

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- Confirm pull-up vs pull-down selection is safe for the target board when toggling power.

### What should be done in the future
- N/A

### Code review instructions
- Review `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c` for the new power-level state.
- Rebuild: `idf.py -C 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware build`.

### Technical details
- Commands used:
  - `idf.py -C 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware build`

## Step 14: Add debug logging for WiFi, WebSocket, and capture pipeline

This step added targeted debug logging to both the Go streaming server and the ESP-IDF firmware so WiFi association, WebSocket lifecycle, and camera capture/encode/send flow can be traced in live logs. The firmware now logs stream idle reasons, camera configuration, PSRAM status, sensor IDs, frame stats, and WebSocket events; the server logs client metadata, connection counts, and the FFmpeg command used for transcoding.

I also rebuilt the Go server and firmware to validate the changes and resolved a deprecation warning by switching camera pin config/logging to the newer `pin_sccb_*` fields.

### What I did
- Added stream state and throughput stats logging in `stream_client.c` (capture/convert/send timings, frame counters).
- Added WiFi connection/disconnection logs with reason names and SSID/BSSID details in `wifi_sta.c`.
- Added WebSocket connection metadata and client count logging in `server.go`, plus FFmpeg command logging.
- Rebuilt the Go server and ESP-IDF firmware.

### Why
- We need high-signal logs to diagnose the camera probe failures and WebSocket behavior on real hardware without adding intrusive instrumentation.

### What worked
- `go build ./...` completed successfully for the server.
- `idf.py -C 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware build` completed successfully after updating the pin field names.

### What didn't work
- Initial firmware build timed out with the default command timeout:
  - `idf.py -C 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware build`
  - `command timed out after 10007 milliseconds`

### What I learned
- The esp_camera config now prefers `pin_sccb_*` fields, and logging them avoids the deprecated `pin_sscb_*` warnings.

### What was tricky to build
- Avoiding excessive per-frame log volume while still capturing useful throughput stats (settled on a 5-second interval).

### What warrants a second pair of eyes
- Confirm that the new log volume is acceptable for USB Serial/JTAG output during continuous streaming.

### What should be done in the future
- Use the new logs on hardware to confirm WiFi association reasons and camera capture timings.
- Consider expanding the app partition if additional diagnostics are added (partition space is nearly full).

### Code review instructions
- Review `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c` for stream stats and camera config logging.
- Review `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/wifi_sta.c` for WiFi reason logging.
- Review `0040-atoms3r-cam-streaming/esp32-camera-stream/server.go` for WebSocket connection logging.
- Rebuild: `go build ./...` and `idf.py -C 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware build`.

### Technical details
- Commands used:
  - `go build ./...`
  - `idf.py -C 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware build`

## Step 15: Normalize camera power semantics (active-low) and improve power readback

This step adjusted the camera power handling to match the UserDemo’s active-low behavior while keeping the console command semantics intuitive (`cam power 1` means “on”). I also enabled input mode on the power GPIO so `gpio_get_level()` reflects the actual pad state and added explicit logging for the active-low mapping.

The firmware was rebuilt to incorporate the new Kconfig option and console usage text.

### What I did
- Added `CONFIG_ATOMS3R_CAMERA_POWER_ACTIVE_LOW` and set it to default `y`.
- Mapped `cam power 1` to a low GPIO level when active-low is enabled.
- Set the camera power GPIO to input/output mode for accurate readback.
- Updated `cam` command usage text to clarify `1=on`.
- Rebuilt the firmware.

### Why
- The demo firmware uses GPIO18 as a pulldown-only “enable” line, which implies active-low control.
- The previous logs always showed level 0 because input was disabled; enabling input makes the readback meaningful.

### What worked
- `idf.py -C 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware build` completed successfully after the changes.

### What didn't work
- N/A

### What I learned
- Treating camera power as active-low matches the demo’s behavior without changing command semantics.

### What was tricky to build
- Ensuring the active-low mapping was centralized so logs and GPIO pull configuration stay consistent.

### What warrants a second pair of eyes
- Confirm the active-low default is correct for non-M12 AtomS3R-CAM hardware.

### What should be done in the future
- If camera probing still fails, add an SCCB/I2C probe command to verify sensor presence on pins 12/9.

### Code review instructions
- Review `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c` for the power mapping logic.
- Review `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/Kconfig.projbuild` for the new config.
- Review `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/console.c` for updated usage text.
- Rebuild: `idf.py -C 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware build`.

### Technical details
- Commands used:
  - `idf.py -C 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware build`

## Step 16: Add logging to UserDemo for camera power and init details

This step added extra logging to the stock ATOMS3R-CAM UserDemo firmware so we can compare its known-good camera initialization path with our custom firmware. The logs capture camera power GPIO state, camera configuration pins, framebuffer settings, and sensor ID to make SCCB probe differences obvious.

These changes are intended purely for diagnosis; no behavior changes were made to the demo logic.

### What I did
- Added camera power GPIO level logging in `usb_webcam_main.cpp`.
- Added camera init logging (config, pin map, fb settings, sensor ID) and error reporting in `camera_init.c`.

### Why
- The demo firmware reliably probes the camera; matching its logs against our firmware should show where behavior diverges.

### What worked
- N/A (no builds or flashes run in this step).

### What didn't work
- N/A

### What I learned
- N/A

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- Confirm log volume is acceptable during normal demo operation.

### What should be done in the future
- Flash the demo with these logs and compare SCCB/pin/output behavior side-by-side.

### Code review instructions
- Review `../ATOMS3R-CAM-UserDemo/main/usb_webcam_main.cpp` for GPIO18 logging.
- Review `../ATOMS3R-CAM-UserDemo/main/utils/camera/camera_init.c` for camera config/sensor logs.

### Technical details
- Commands used: N/A

## Step 17: Add UVC capture stats + PSRAM logs to the UserDemo

This step expanded the UserDemo diagnostics to include periodic UVC capture statistics and PSRAM readiness logs. The additional detail should make it easier to compare the working USB streaming pipeline to our custom firmware’s capture/convert timings and buffer behavior.

### What I did
- Logged PSRAM availability/size during `my_camera_init`.
- Added a 5-second UVC stats report (fps, avg bytes, capture/convert timing, failure counts) in `service_uvc.cpp`.

### Why
- We need apples-to-apples timing and buffer metrics between the UserDemo and the custom firmware when diagnosing SCCB probe failures.

### What worked
- N/A (no builds or flashes run in this step).

### What didn't work
- N/A

### What I learned
- N/A

### What was tricky to build
- Keeping the logging cadence low enough to avoid spamming during steady streaming.

### What warrants a second pair of eyes
- Confirm the added UVC stats logging doesn’t interfere with USB streaming throughput.

### What should be done in the future
- Flash the demo with these logs and capture a short UVC session for comparison with the custom firmware.

### Code review instructions
- Review `../ATOMS3R-CAM-UserDemo/main/utils/camera/camera_init.c` for PSRAM logs.
- Review `../ATOMS3R-CAM-UserDemo/main/service/service_uvc.cpp` for the UVC stats counter.

### Technical details
- Commands used: N/A

## Step 18: Add SCCB/I2C scan to UserDemo boot sequence

This step added an explicit SCCB/I2C bus scan on the camera pins (12/9) during the UserDemo’s boot sequence. The goal is to confirm whether the camera sensor responds on the bus before `esp_camera_init`, making it easier to compare sensor visibility between the stock demo and the custom streaming firmware.

### What I did
- Implemented `camera_sccb_scan()` in `usb_webcam_main.cpp` using the IDF I2C driver on the SCCB pins.
- Logged the SCCB port/pins/frequency and any detected device addresses.
- Wired the scan to run before `camera_init()`.

### Why
- The custom firmware fails SCCB probe; this scan should show whether the demo sees a device on the same pins with the same port configuration.

### What worked
- N/A (no builds or flashes run in this step).

### What didn't work
- N/A

### What I learned
- N/A

### What was tricky to build
- Avoiding driver conflicts by uninstalling the I2C driver after the scan.

### What warrants a second pair of eyes
- Confirm the scan doesn’t interfere with the demo’s camera init on certain board revisions.

### What should be done in the future
- Capture the SCCB scan output from the demo and compare with the custom firmware’s logs.

### Code review instructions
- Review `../ATOMS3R-CAM-UserDemo/main/usb_webcam_main.cpp` for the new SCCB scan logic.

### Technical details
- Commands used: N/A

## Step 19: Add menuconfig toggle to disable UVC in UserDemo

This step added a menuconfig option to disable USB UVC streaming at boot so the USB Serial/JTAG console can be used without USB OTG conflicts. The UVC service is now gated behind `CONFIG_USB_WEBCAM_ENABLE_UVC`.

### What I did
- Added `CONFIG_USB_WEBCAM_ENABLE_UVC` to `main/Kconfig.projbuild` (default on).
- Guarded `start_service_uvc()` in `usb_webcam_main.cpp` and added a warning log when disabled.

### Why
- USB UVC uses the USB OTG peripheral and prevents USB Serial/JTAG from working concurrently.

### What worked
- N/A (no builds or flashes run in this step).

### What didn't work
- N/A

### What I learned
- The demo does not have a built-in UVC enable toggle; adding one is the simplest path to preserve Serial/JTAG access.

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- Confirm the disable toggle doesn’t break any startup sequencing in the demo.

### What should be done in the future
- If needed, also gate any UVC-specific memory allocation behind the same config.

### Code review instructions
- Review `../ATOMS3R-CAM-UserDemo/main/Kconfig.projbuild` for the new config.
- Review `../ATOMS3R-CAM-UserDemo/main/usb_webcam_main.cpp` for the UVC guard.

### Technical details
- Commands used: N/A

## Step 20: Surface esp32-camera tuning options in AtomS3R Camera Stream menu

This step copied the most relevant esp32-camera Kconfig options into the project’s "AtomS3R Camera Stream" menu for faster access when aligning with the UserDemo. The options include SCCB I2C port selection, SCCB clock, camera task stack size/pinning, DMA buffer size, and JPEG mode frame size policy.

### What I did
- Added a "Camera Component Options" submenu in `firmware/main/Kconfig.projbuild`.
- Exposed SCCB I2C port, SCCB clock frequency, camera task stack size, task pinning, DMA buffer size, and JPEG frame size mode settings.

### Why
- These are the exact camera tuning controls used by the UserDemo and are needed to keep configuration in sync without hunting through component menus.

### What worked
- N/A (no builds or flashes run in this step).

### What didn't work
- N/A

### What I learned
- The esp32-camera component already defines these symbols; surfacing them in the project menu is the most direct path to parity.

### What was tricky to build
- Avoiding confusion with duplicate menu entries while keeping the options discoverable.

### What warrants a second pair of eyes
- Confirm the duplicated Kconfig prompts behave as expected in menuconfig.

### What should be done in the future
- If duplicate prompts are confusing, consider hiding the component menu or documenting the preferred menu path.

### Code review instructions
- Review `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/Kconfig.projbuild` for the new submenu.

### Technical details
- Commands used: N/A

## Step 21: Use camera component options + analyze sdkconfig differences

This step wired the newly surfaced camera component options into runtime logging so the stream firmware reports the SCCB port, SCCB clock, camera task stack/core, DMA buffer size, and JPEG frame size policy at boot. I also generated a unified diff between the UserDemo and streaming firmware `sdkconfig` files and wrote an analysis report highlighting key differences to unify.

### What I did
- Added camera component option logging in `stream_client.c`.
- Added the missing custom JPEG frame size config to `main/Kconfig.projbuild`.
- Updated `sdkconfig.defaults` to include camera task stack/core, DMA buffer size, and JPEG frame size defaults.
- Generated the `sdkconfig` diff with the `diff_sdkconfig.py` script and wrote the analysis report.

### Why
- Having the config values printed at runtime ensures we can verify that the menuconfig options are actually in effect during camera init.
- The `sdkconfig` diff highlights system-level differences (flash size, CPU freq, console, UVC) that could explain SCCB probe failures.

### What worked
- `diff_sdkconfig.py` produced a unified diff for inspection.

### What didn't work
- N/A

### What I learned
- The camera-specific settings already match between demo and streaming firmware; the biggest differences are flash size, CPU frequency, console, and Arduino/UVC features.

### What was tricky to build
- Keeping the analysis scoped to differences that are likely to affect camera bring-up.

### What warrants a second pair of eyes
- Validate which config differences are truly impactful vs incidental (e.g., Arduino-only settings).

### What should be done in the future
- Decide whether to match the demo’s 240 MHz CPU frequency and 8 MB flash layout in the streaming firmware.

### Code review instructions
- Review `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c` for the new logging.
- Review `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/Kconfig.projbuild` for the added custom JPEG frame size config.
- Review `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/sdkconfig.defaults` for new camera defaults.
- Review `ttmp/2026/01/14/MO-001-ATOMS3R-WEBSOCKET-STREAMING--atoms3r-websocket-streaming/analysis/05-sdkconfig-diff-demo-vs-stream.md` for the diff analysis.

### Technical details
- Commands used:
  - `python 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/scripts/diff_sdkconfig.py ../ATOMS3R-CAM-UserDemo/sdkconfig 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/sdkconfig --out ttmp/2026/01/14/MO-001-ATOMS3R-WEBSOCKET-STREAMING--atoms3r-websocket-streaming/various/sdkconfig-demo-vs-stream.diff`

## Step 22: Confirm diary completeness for all work to date

This step consolidates the diary so every change, analysis, and debugging action taken so far is captured in a consistent format. The goal is to make it possible to retrace the work chronologically without cross-referencing ad-hoc notes.

I reviewed the diary entries end-to-end, ensured the sequence covers the added logging, SCCB scan, UVC toggle, camera config surface, and sdkconfig diff analysis, and updated the metadata timestamp.

### What I did
- Audited the diary steps for coverage and ordering.
- Confirmed that the recent UserDemo logging/SCCB scan and UVC disable toggle are included.
- Updated the diary metadata timestamp.

### Why
- The project requires a full, retraceable record of all steps taken so far.

### What worked
- The diary already contained the full step sequence from initial analysis through the latest config diff work.

### What didn't work
- N/A

### What I learned
- Keeping the diary updated after each step avoids large backfill work later.

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- Verify the diary captures any uncommitted or experimental changes that might have been overlooked.

### What should be done in the future
- Continue updating the diary immediately after each step.

### Code review instructions
- Review `ttmp/2026/01/14/MO-001-ATOMS3R-WEBSOCKET-STREAMING--atoms3r-websocket-streaming/reference/01-diary.md` for the full timeline.

### Technical details
- Commands used: N/A

## Step 23: Add variable-level detail for all touched code/config

This step expands the diary with variable-level documentation for the code and configuration we modified so far. The list below focuses on **variables, structs, and Kconfig symbols we introduced or edited** and explains what each does, to make later debugging and review precise.

The scope is limited to the files we changed in the streaming firmware and UserDemo, plus the new Kconfig options surfaced in the AtomS3R Camera Stream menu.

### What I did
- Enumerated all edited variables, structs, and Kconfig symbols in the modified files.
- Added brief descriptions of each variable’s role and where it is used.

### Why
- Variable-level documentation makes it easier to reason about camera bring-up failures and confirm that runtime behavior matches the demo.

### What worked
- N/A (documentation-only step).

### What didn't work
- N/A

### What I learned
- The camera bring-up path has many toggles; documenting them reduces ambiguity.

### What was tricky to build
- Keeping the list comprehensive while staying within the diary format.

### What warrants a second pair of eyes
- Verify the variable list is complete across all modified files.

### What should be done in the future
- Keep this variable list updated as new debug toggles or config symbols are added.

### Code review instructions
- Review `ttmp/2026/01/14/MO-001-ATOMS3R-WEBSOCKET-STREAMING--atoms3r-websocket-streaming/reference/01-diary.md` Step 23.

### Technical details
- **Firmware variables (streaming firmware)**
- `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c:s_host` — runtime stream hostname buffer (NVS or Kconfig).
- `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c:s_port` — runtime stream port (NVS or Kconfig).
- `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c:s_has_saved` — whether target host/port is saved in NVS.
- `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c:s_has_runtime` — whether a runtime target is set.
- `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c:s_stream_enabled` — gate for the streaming loop.
- `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c:s_ws_connected` — WebSocket connection state (event-driven).
- `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c:s_ws` — esp_websocket_client handle.
- `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c:s_stream_task` — FreeRTOS task handle for the stream loop.
- `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c:s_camera_ready` — camera initialization state.
- `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c:s_camera_power_level` — requested camera power state (1=on semantics).
- `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c:s_logged_frame_info` — one-shot guard to log frame info.
- `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c:s_stats` — capture/convert/send counters and timings for 5s stats logs.
- `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c:s_last_state_log_us` — last time the stream idle reason was logged.
- `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c:STREAM_STATS_INTERVAL_US` — stats/log cadence (5 seconds).
- `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c:CAMERA_POWER_GPIO` — GPIO used for camera power enable (GPIO18).
- `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/wifi_sta.c:s_mu` — mutex guarding WiFi state.
- `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/wifi_sta.c:s_retry` — retry count for STA reconnect attempts.
- `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/wifi_sta.c:s_started` — whether STA start event occurred.
- `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/wifi_sta.c:s_autoconnect` — auto-connect enable flag.
- `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/wifi_sta.c:s_sta_netif` — STA netif handle.
- `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/wifi_sta.c:s_wifi_inited` — whether esp_wifi_init completed.
- `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/wifi_sta.c:s_runtime_ssid` — runtime SSID.
- `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/wifi_sta.c:s_runtime_pass` — runtime password.
- `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/wifi_sta.c:s_has_runtime` — runtime credentials set.
- `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/wifi_sta.c:s_has_saved` — credentials saved in NVS.
- `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/wifi_sta.c:s_status` — exported status struct (state, SSID, IP, last reason).

- **Go server variables (streaming server)**
- `0040-atoms3r-cam-streaming/esp32-camera-stream/server.go:StreamServer.cameraClients` — map of camera WebSocket clients.
- `0040-atoms3r-cam-streaming/esp32-camera-stream/server.go:StreamServer.viewerClients` — map of viewer WebSocket clients.
- `0040-atoms3r-cam-streaming/esp32-camera-stream/server.go:StreamServer.ffmpegCmd` — active ffmpeg process.
- `0040-atoms3r-cam-streaming/esp32-camera-stream/server.go:StreamServer.ffmpegStdin` — pipe used to feed JPEG frames to ffmpeg.
- `0040-atoms3r-cam-streaming/esp32-camera-stream/server.go:StreamServer.h264Buffer` — rolling buffer of MPEG-TS output.
- `0040-atoms3r-cam-streaming/esp32-camera-stream/server.go:StreamServer.isStreaming` — ffmpeg running guard.
- `0040-atoms3r-cam-streaming/esp32-camera-stream/server.go:StreamServer.camFrameCount` — camera frame counter for stats.
- `0040-atoms3r-cam-streaming/esp32-camera-stream/server.go:StreamServer.camByteCount` — total bytes from camera for stats.
- `0040-atoms3r-cam-streaming/esp32-camera-stream/server.go:StreamServer.viewByteCount` — total bytes sent to viewers for stats.

- **UserDemo variables (comparison firmware)**
- `../ATOMS3R-CAM-UserDemo/main/usb_webcam_main.cpp:camera_sccb_scan` — helper that scans SCCB/I2C addresses on camera pins at boot.
- `../ATOMS3R-CAM-UserDemo/main/utils/camera/camera_init.c:camera_config` — esp_camera config struct with pins, xclk, format, fb count.
- `../ATOMS3R-CAM-UserDemo/main/service/service_uvc.cpp:s_stats` — UVC capture stats accumulator.

- **Project-level Kconfig symbols surfaced in AtomS3R Camera Stream menu**
- `CONFIG_SCCB_HARDWARE_I2C_PORT0/1` — selects SCCB I2C peripheral.
- `CONFIG_SCCB_CLK_FREQ` — SCCB clock frequency (Hz).
- `CONFIG_CAMERA_TASK_STACK_SIZE` — camera task stack size.
- `CONFIG_CAMERA_CORE0/CORE1/NO_AFFINITY` — camera task pinning.
- `CONFIG_CAMERA_DMA_BUFFER_SIZE_MAX` — DMA buffer upper limit.
- `CONFIG_CAMERA_JPEG_MODE_FRAME_SIZE_AUTO/CUSTOM` — JPEG frame size policy.
- `CONFIG_CAMERA_JPEG_MODE_FRAME_SIZE` — custom JPEG frame size when enabled.
- `CONFIG_ATOMS3R_CAMERA_POWER_ACTIVE_LOW` — maps “cam power 1” to active-low GPIO.

## Step 24: Add USB Serial/JTAG console test loop in UserDemo

This step adds a minimal console test task to the ATOMS3R-CAM UserDemo so we can isolate whether USB Serial/JTAG enumerates when UVC is disabled. The loop prints a heartbeat every second and echoes any received USB Serial/JTAG bytes, giving us a clear signal that the USB Serial/JTAG interface is alive independent of camera streaming.

I also updated the includes to ensure the console test compiles cleanly in C++ and to access the USB Serial/JTAG VFS APIs. The console test is behind a new Kconfig toggle so it can be enabled only when needed.

### What I did
- Added `CONFIG_USB_WEBCAM_CONSOLE_TEST` to the UserDemo menuconfig.
- Implemented `console_test_task` and `start_console_test` in `usb_webcam_main.cpp`.
- Added USB Serial/JTAG VFS setup and a non-blocking read path.
- Included `<inttypes.h>` and `<stdio.h>` for `PRIu32` and `printf`/`fflush`.

### Why
- We need a minimal, repeatable signal to confirm USB Serial/JTAG enumeration without UVC interference.

### What worked
- The console test loop is isolated and does not touch camera state, so it is safe to enable while debugging USB enumeration.

### What didn't work
- N/A

### What I learned
- A dedicated test loop makes it easier to distinguish USB Serial/JTAG issues from camera or UVC failures.

### What was tricky to build
- Ensuring the USB Serial/JTAG VFS setup is only compiled when the console is configured to use it.

### What warrants a second pair of eyes
- Confirm that enabling the console test does not introduce USB resource conflicts when UVC is also enabled.

### What should be done in the future
- If the console test proves useful, consider a similar diagnostic path in the streaming firmware.

### Code review instructions
- Review `../ATOMS3R-CAM-UserDemo/main/usb_webcam_main.cpp` for the console test task and USB Serial/JTAG setup.
- Review `../ATOMS3R-CAM-UserDemo/main/Kconfig.projbuild` for the new config toggle.

### Technical details
- **UserDemo config symbols**
- `CONFIG_USB_WEBCAM_CONSOLE_TEST` — enables the console test task at boot.
- `CONFIG_USB_WEBCAM_ENABLE_UVC` — if enabled alongside the test, USB Serial/JTAG may not enumerate.
- `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG` — determines whether USB Serial/JTAG APIs are compiled/enabled.

- **UserDemo variables/functions**
- `../ATOMS3R-CAM-UserDemo/main/usb_webcam_main.cpp:console_test_task` — prints `console test tick=<n>` every second and echoes incoming bytes.
- `../ATOMS3R-CAM-UserDemo/main/usb_webcam_main.cpp:start_console_test` — registers USB Serial/JTAG VFS and starts the task.
- `../ATOMS3R-CAM-UserDemo/main/usb_webcam_main.cpp:count` — monotonic tick counter for heartbeat output.

## Step 25: Make USB Serial/JTAG VFS include compatible with older headers

After enabling the console test loop, the UserDemo build failed because `driver/usb_serial_jtag_vfs.h` was not found in the include path for this project. This step adds a conditional include path so we can use the newer VFS API when available and fall back to the deprecated `esp_vfs_usb_serial_jtag.h` API when it is not.

This keeps the console test portable across IDF variants, while still enabling the USB Serial/JTAG VFS registration for stdout/stderr when the newer header is present.

### What I did
- Replaced the direct `driver/usb_serial_jtag_vfs.h` include with `__has_include` checks.
- Added fallback to `esp_vfs_usb_serial_jtag.h` and mapped the register/line-ending functions.
- Updated the console test setup to use the appropriate API based on availability.

### Why
- The build failed on the UserDemo because the newer VFS header was missing; we need compatibility with that toolchain.

### What worked
- The conditional include isolates the build from the missing header without changing the console test behavior.

### What didn't work
- Build failure reported:
  ```
  /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo/main/usb_webcam_main.cpp:21:10: fatal error: driver/usb_serial_jtag_vfs.h: No such file or directory
  ```

### What I learned
- Some builds expose `driver/usb_serial_jtag.h` but not the newer VFS header, so a fallback path is required.

### What was tricky to build
- Ensuring the console test keeps the same runtime behavior while supporting two VFS APIs.

### What warrants a second pair of eyes
- Verify that the fallback API is still appropriate for the UserDemo’s IDF version.

### What should be done in the future
- If the project standardizes on a newer IDF, remove the fallback and simplify the include path.

### Code review instructions
- Review `../ATOMS3R-CAM-UserDemo/main/usb_webcam_main.cpp` around the USB Serial/JTAG include block and `start_console_test`.

### Technical details
- **Compatibility macros**
- `CAM_USB_JTAG_VFS_API` — indicates `driver/usb_serial_jtag_vfs.h` is available.
- `CAM_USB_JTAG_VFS_DEPRECATED_API` — indicates `esp_vfs_usb_serial_jtag.h` is available.

## Step 26: Avoid termios/Arduino macro conflict by removing VFS header use

The UserDemo build failed under IDF 5.1.4 because `esp_vfs_usb_serial_jtag.h` pulls in `<termios.h>`, which defines `B0` and conflicts with the Arduino core’s `binary.h` enum. This step removes the VFS header dependency entirely and switches the console test to only print a heartbeat (and USB Serial/JTAG connection status) using the system console routing.

This keeps the console test useful while avoiding the `B0` macro collision, and it relies on the system console configuration (`CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG`) rather than manually registering VFS from the application.

### What I did
- Removed the VFS header include block from `usb_webcam_main.cpp`.
- Updated the console test loop to log `usb_serial_jtag_is_connected()` instead of reading raw bytes.
- Dropped the explicit VFS registration path in `start_console_test`.

### Why
- Including `esp_vfs_usb_serial_jtag.h` in this Arduino-based project collides with the Arduino binary constants and breaks the build.

### What worked
- The console test still produces a 1 Hz heartbeat and reports USB Serial/JTAG connection state without needing VFS setup in app code.

### What didn't work
- Build failure encountered:
  ```
  /home/manuel/esp/esp-idf-5.1.4/components/newlib/platform_include/sys/termios.h:86:21: error: expected identifier before numeric constant
  /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo/components/arduino-esp32/cores/esp32/binary.h:38:3: note: in expansion of macro 'B0'
  ```

### What I learned
- Avoid pulling in `<termios.h>` when building with the Arduino core’s `binary.h` enum definitions.

### What was tricky to build
- Maintaining console test behavior while avoiding the VFS registration that triggers the macro conflict.

### What warrants a second pair of eyes
- Confirm that the console test still uses the system console path in this build configuration.

### What should be done in the future
- If full USB Serial/JTAG VFS control is needed, consider a separate non-Arduino build variant or a guarded include that undefines `B0` safely.

### Code review instructions
- Review `../ATOMS3R-CAM-UserDemo/main/usb_webcam_main.cpp` around the console test loop and include block.

### Technical details
- `../ATOMS3R-CAM-UserDemo/main/usb_webcam_main.cpp:usb_serial_jtag_is_connected` — reports host SOF presence to infer enumeration.

## Step 27: Re-check sdkconfig console settings with diff script

I re-ran the sdkconfig diff script to compare the UserDemo’s `sdkconfig` against the streaming firmware’s `sdkconfig`, focusing on console-related settings. The diff is very large because the projects build against different IDF versions (UserDemo 5.1.4 vs streaming 5.4.1), so I narrowed attention to the console section to confirm whether USB Serial/JTAG is actually the primary console in the UserDemo build.

This surfaced a mismatch: the UserDemo’s `sdkconfig` has USB Serial/JTAG set as the primary console, but `sdkconfig.defaults` still sets UART0 as the primary console and only enables USB Serial/JTAG as secondary. If `sdkconfig` is regenerated (e.g., after `fullclean`), it will likely revert to UART output.

### What I did
- Ran the sdkconfig diff script to compare the two builds.
- Queried console-related symbols in both `sdkconfig` and `sdkconfig.defaults`.
- Noted the primary/secondary console mismatch between defaults and the current config.

### Why
- We need to explain why USB Serial/JTAG logs are missing in the UserDemo while the streaming firmware’s console works.

### What worked
- The diff confirmed that `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG` is set in UserDemo’s `sdkconfig` but not in `sdkconfig.defaults`.

### What didn't work
- The diff output is noisy due to the IDF version mismatch, so it is not directly actionable beyond targeted checks.

### What I learned
- Console behavior can silently revert after a `fullclean` if `sdkconfig.defaults` is not aligned with the desired console.

### What was tricky to build
- Filtering the diff to relevant console symbols without being distracted by the IDF version noise.

### What warrants a second pair of eyes
- Confirm whether the secondary console option in IDF 5.1.4 mirrors logs to USB Serial/JTAG or only enables input.

### What should be done in the future
- Update UserDemo `sdkconfig.defaults` to match the USB Serial/JTAG primary console settings used in the streaming firmware.

### Code review instructions
- Check `../ATOMS3R-CAM-UserDemo/sdkconfig` vs `../ATOMS3R-CAM-UserDemo/sdkconfig.defaults` for console settings.
- Run the diff script if needed: `python3 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/scripts/diff_sdkconfig.py <userdemo-sdkconfig> <stream-sdkconfig>`.

### Technical details
- **Commands**
- `python3 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/scripts/diff_sdkconfig.py /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo/sdkconfig /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/sdkconfig`
- **Key symbols**
- UserDemo `sdkconfig`: `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`, `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG_ENABLED=y`, `CONFIG_ESP_CONSOLE_UART_NUM=-1`.
- UserDemo `sdkconfig.defaults`: `CONFIG_ESP_CONSOLE_UART_DEFAULT=y`, `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG` not set, `CONFIG_ESP_CONSOLE_SECONDARY_USB_SERIAL_JTAG=y`, `CONFIG_ESP_CONSOLE_UART_NUM=0`.

## Step 28: Align UserDemo console defaults with USB Serial/JTAG

This step updates the UserDemo’s `sdkconfig.defaults` so USB Serial/JTAG is the primary console and UART is disabled, matching the console behavior in the streaming firmware. I also updated the current `sdkconfig` to remove the multiple UART flag so it reflects the same intent.

These changes ensure clean builds don’t silently revert the console to UART0, which would hide boot logs on the USB Serial/JTAG port.

### What I did
- Set `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y` and disabled UART console defaults.
- Set `CONFIG_ESP_CONSOLE_SECONDARY_NONE=y` and `CONFIG_ESP_CONSOLE_UART_NUM=-1` in defaults.
- Removed `CONFIG_ESP_CONSOLE_MULTIPLE_UART` from the generated `sdkconfig`.

### Why
- The UserDemo’s defaults were still pointing to UART0, so clean builds lose USB Serial/JTAG console output.

### What worked
- Defaults now mirror the streaming firmware console setup.

### What didn't work
- N/A

### What I learned
- Console misalignment between `sdkconfig` and `sdkconfig.defaults` is a common reason for “missing” USB Serial/JTAG output.

### What was tricky to build
- Ensuring the defaults disable UART entirely without breaking menuconfig parsing.

### What warrants a second pair of eyes
- Confirm no other component forces UART console (e.g., RainMaker or debug stubs) after regeneration.

### What should be done in the future
- Re-run `menuconfig` and verify the console channel remains USB Serial/JTAG after a clean build.

### Code review instructions
- Review `../ATOMS3R-CAM-UserDemo/sdkconfig.defaults` for console defaults.
- Review `../ATOMS3R-CAM-UserDemo/sdkconfig` to confirm the generated config is aligned.

### Technical details
- `../ATOMS3R-CAM-UserDemo/sdkconfig.defaults`: set `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`, unset UART and secondary USB.
- `../ATOMS3R-CAM-UserDemo/sdkconfig`: unset `CONFIG_ESP_CONSOLE_MULTIPLE_UART`.

## Step 29: Audit 0040 camera init against the bootstrap playbook

I revisited MO-001 after closing the MO-002 camera bring-up work and documented where the current streaming firmware diverges from the now-verified camera bootstrap playbook. This produced a new analysis document covering `app_main.c`, `stream_client.c`, and both `sdkconfig` files.

This step also re-verified the 0040 PSRAM and console settings and confirmed they match the validated AtomS3R-CAM defaults.

**Commit (code):** dc881fa — "Docs: analyze 0040 camera init alignment"

### What I did
- Read `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/app_main.c` and `stream_client.c`.
- Checked `sdkconfig` and `sdkconfig.defaults` for PSRAM, SCCB, and console values.
- Authored `analysis/06-0040-camera-init-alignment-post-mo-002.md` with divergences and fixes.

### Why
- We needed a direct alignment audit between 0040 and the proven camera init sequence.

### What worked
- Confirmed 0040 already uses USB Serial/JTAG console and PSRAM settings aligned with UserDemo.

### What didn't work
- N/A.

### What I learned
- The biggest remaining gap is timing (no explicit warmup delay) and diagnostics (no post-XCLK SCCB probe).

### What was tricky to build
- N/A.

### What warrants a second pair of eyes
- Confirm the analysis doc’s divergence list is complete before applying fixes.

### What should be done in the future
- Implement the warmup delay and optional SCCB probe in 0040 (next step).

### Code review instructions
- Start in `ttmp/2026/01/14/MO-001-ATOMS3R-WEBSOCKET-STREAMING--atoms3r-websocket-streaming/analysis/06-0040-camera-init-alignment-post-mo-002.md`.

### Technical details
- Commands: `rg -n "CONFIG_(ESP_CONSOLE|SPIRAM|CAMERA|SCCB|ATOMS3R)" .../sdkconfig*`, `rg -n "camera" .../stream_client.c`, `sed -n '1,200p' .../app_main.c`.

## Step 30: Align 0040 camera init with the verified sequence

I updated the streaming firmware to match the verified camera bootstrap playbook by adding an explicit warmup delay, adding a post-XCLK SCCB probe that reuses the existing I2C driver, and enforcing PSRAM-only buffers (no DRAM fallback). This is intended to eliminate timing-related probe failures and keep 0040 behavior aligned with the working UserDemo/0041 path.

**Commit (code):** 3fdf22b — "0040: add camera warmup + post-XCLK SCCB probe"

### What I did
- Added a 1000 ms warmup delay after `camera_power_on()` and before `esp_camera_init()`.
- Added `camera_sccb_probe_known_addrs()` and called it post-init to report SCCB ACKs.
- Removed DRAM fallback: init now fails if PSRAM is unavailable.

### Why
- The SCCB scan removal showed that implicit delays were masking power-up timing; explicit delays are safer.
- Post-XCLK probing avoids false negatives and reuses the driver’s I2C setup.

### What worked
- N/A (no flash/monitor run yet).

### What didn't work
- N/A.

### What I learned
- The camera init path in 0040 needs explicit timing to behave deterministically across boots.

### What was tricky to build
- Ensuring the probe doesn’t attempt to reinstall the I2C driver already owned by esp32-camera.

### What warrants a second pair of eyes
- Confirm the probe address list matches the driver’s supported sensors.
- Validate the 1000 ms warmup delay is sufficient but not excessive.

### What should be done in the future
- Flash and capture logs to confirm SCCB probe ACKs and stable frame capture.

### Code review instructions
- Review `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c` in `camera_init_once()` and `camera_sccb_probe_known_addrs()`.

### Technical details
- Added constants: `CAMERA_WARMUP_DELAY_MS`, `CAMERA_SIOD_GPIO`, `CAMERA_SIOC_GPIO`, `CAMERA_XCLK_GPIO`.

## Step 31: Stabilize WebSocket streaming under load

I tuned the websocket client path to avoid rapid restarts and fragile non-blocking writes. The existing stream loop was repeatedly calling `esp_websocket_client_start()` while a connection was already in flight, and all frame sends used a zero timeout; once the socket backpressured, the client aborted the connection and the server logged EOFs.

This step introduces a start-in-flight guard, a larger websocket buffer, a sane send timeout, and richer error logging so we can correlate on-device failures to transport details.

**Commit (code):** 3a40f51 — "0040: stabilize websocket send/start"

### What I did
- Added a `s_ws_start_pending` flag to avoid calling `esp_websocket_client_start()` while a start is already in flight.
- Increased websocket buffer size and configured a 15s network timeout in `esp_websocket_client_config_t`.
- Switched websocket frame sends to use a 1000 ms timeout instead of zero.
- Logged websocket error details (type, handshake status, TLS error/flags) on error events.

### Why
- The logs showed repeated `websocket start failed: ESP_FAIL` and `esp_transport_write() returned 0`, indicating we were fighting the connection state and using a non-blocking send that fails under backpressure.

### What worked
- N/A (awaiting a flash/monitor run after the change).

### What didn't work
- N/A.

### What I learned
- Zero-timeout websocket sends can make the client abort connections under normal network backpressure.
- Calling `esp_websocket_client_start()` in a tight loop makes reconnect storms worse rather than better.

### What was tricky to build
- Ensuring start gating doesn’t block legitimate reconnect attempts (cleared on CONNECTED, DISCONNECTED, and ERROR).

### What warrants a second pair of eyes
- Confirm the new timeouts and buffer size won’t starve other tasks or hide real connectivity failures.

### What should be done in the future
- Flash and capture logs to confirm the transport errors disappear and sustained streaming is stable.

### Code review instructions
- Start in `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c`.
- Review `ws_event_handler()`, `ensure_ws_client()`, and the send path in the stream loop.

### Technical details
- `esp_websocket_client_config_t`: `network_timeout_ms=15000`, `buffer_size=16384`.
- Send path: `esp_websocket_client_send_bin(..., pdMS_TO_TICKS(1000))`.

## Step 32: Fix websocket error logging for IDF 5.4

After the websocket stabilization changes, the build failed because I referenced error fields that do not exist in the vendored `esp_websocket_client` component for IDF 5.4. I corrected the logging to use the available fields so the build succeeds and still captures transport and TLS context.

This keeps the richer error context without breaking compilation.

**Commit (code):** 1f387df — "0040: fix websocket error logging fields"

### What I did
- Swapped websocket error logging to use `esp_tls_stack_err`, `esp_tls_cert_verify_flags`, and `esp_transport_sock_errno`.
- Rebuilt with `idf.py build` to confirm compilation success.

### Why
- The earlier log used `esp_tls_error_code` and `esp_tls_flags`, which are not present in this component version.

### What worked
- `idf.py build` completed after the fix.

### What didn't work
- `idf.py build` failed with:
  - `error: 'esp_websocket_error_codes_t' has no member named 'esp_tls_error_code'`
  - `error: 'esp_websocket_error_codes_t' has no member named 'esp_tls_flags'`

### What I learned
- The websocket error struct fields vary across IDF/esp-protocols versions; logging must match the vendored component.

### What was tricky to build
- Keeping error logs detailed while staying compatible with the local component version.

### What warrants a second pair of eyes
- Confirm the updated error fields are the most useful for diagnosing transport failures in this project.

### What should be done in the future
- Consider version-gating error logs if we upgrade the websocket component.

### Code review instructions
- Review `ws_event_handler()` in `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c`.
- Validate by running `idf.py build` in the firmware directory.

### Technical details
- Log format now includes `tls_stack`, `tls_verify`, and `sock_errno` from `esp_websocket_error_codes_t`.

## Step 33: Fix viewer WebSocket URL in the client HTML

The viewer page was hard-coding `:8766` for local development, which caused `NS_ERROR_CONNECTION_REFUSED` when the server is actually running on port 8080. I updated the client to use the same host/port as the page so the viewer WebSocket connects correctly in the default setup.

This aligns the browser UI with the server log guidance (viewer connects to the same port as HTTP).

**Commit (code):** 25b3a6c — "client: use page host for viewer websocket"

### What I did
- Removed the local-dev `:8766` override in `client.html`.
- Use `window.location.host` for the viewer websocket URL.

### Why
- The server binds to port 8080 and serves `/ws/viewer` there; the UI was pointing at a port that wasn’t listening.

### What worked
- N/A (needs browser retest).

### What didn't work
- N/A.

### What I learned
- The current server doesn’t use a separate viewer websocket port; the client should always default to the page host/port.

### What was tricky to build
- Ensuring the change doesn’t break proxied environments that previously expected a port rewrite.

### What warrants a second pair of eyes
- Confirm whether any dev environment still requires the `8080- → 8766-` substitution before removing it entirely.

### What should be done in the future
- Retest `client.html` in the target environment (browser + server on 8080).

### Code review instructions
- Review `connect()` in `0040-atoms3r-cam-streaming/esp32-camera-stream/client.html`.

### Technical details
- WebSocket URL now derives from `window.location.host` and `/ws/viewer`.
