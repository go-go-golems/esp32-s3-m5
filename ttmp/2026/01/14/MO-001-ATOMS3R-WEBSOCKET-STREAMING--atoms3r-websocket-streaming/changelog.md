# Changelog

## 2026-01-14

- Initial workspace created


## 2026-01-14

Step 1: document stock firmware WiFi and camera data flow

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/14/MO-001-ATOMS3R-WEBSOCKET-STREAMING--atoms3r-websocket-streaming/analysis/01-stock-atoms3r-cam-userdemo-firmware-analysis.md — Captured detailed analysis for baseline behavior


## 2026-01-14

Step 2: analyze Go server protocol and console requirements

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/14/MO-001-ATOMS3R-WEBSOCKET-STREAMING--atoms3r-websocket-streaming/analysis/02-camera-streaming-protocol-and-console-requirements.md — Captured websocket protocol and esp_console requirements


## 2026-01-14

Step 3: add AtomS3R camera streaming firmware + esp_console config

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c — Implements WebSocket JPEG streaming to server


## 2026-01-14

Step 4: build firmware (vendor esp_websocket_client + esp32-camera)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/components/esp_websocket_client — Vendored component to resolve build


## 2026-01-14

Step 5: Documented PSRAM vs SPIRAM and camera buffer usage for ESP32-S3 streaming.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/14/MO-001-ATOMS3R-WEBSOCKET-STREAMING--atoms3r-websocket-streaming/analysis/03-psram-vs-spiram-for-esp32-s3-camera-streaming.md — New analysis doc.


## 2026-01-14

Step 6: Uploaded PSRAM analysis, enabled PSRAM config, added psram status console command.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/console.c — psram status command.
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/sdkconfig — PSRAM config aligned.


## 2026-01-14

Step 7: Added esp_psram dependency to fix include error; build succeeds.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/CMakeLists.txt — esp_psram dependency.


## 2026-01-14

Step 8-10: Added server bind address flag; mapped WiFi reason 201; investigated camera probe failures.

### Related Files

- /home/manuel/esp/esp-idf-5.4.1/components/esp_wifi/include/esp_wifi_types_generic.h — Reason code mapping.
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo/main/usb_webcam_main.cpp — Camera power enable reference.
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0040-atoms3r-cam-streaming/esp32-camera-stream/server.go — Bind address + URL logging.


## 2026-01-14

Step 11: Aligned camera power + JPEG settings with UserDemo; added camera Kconfig options; published alignment audit; rebuilt firmware.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/Kconfig.projbuild — Added camera module/XCLK configs.
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c — Power sequencing and JPEG quality changes.
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/14/MO-001-ATOMS3R-WEBSOCKET-STREAMING--atoms3r-websocket-streaming/analysis/04-atoms3r-cam-userdemo-alignment-audit.md — Alignment report.


## 2026-01-14

Step 12: Added camera power debug command + sensor tuning; rebuilt firmware.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/console.c — cam power/dump commands.
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c — Power helpers + sensor tuning.


## 2026-01-14

Step 13: Persisted camera power level across init for polarity testing; rebuilt firmware.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c — Power level persistence.


## 2026-01-14

Step 14: add debug logging for WiFi/WebSocket/camera pipeline and rebuild

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c — Stream stats and camera logging
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/wifi_sta.c — WiFi reason logging
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0040-atoms3r-cam-streaming/esp32-camera-stream/server.go — WebSocket connection logging


## 2026-01-14

Step 15: map cam power to active-low behavior and rebuild

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/Kconfig.projbuild — New camera power Kconfig
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/console.c — Usage clarified for cam power
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c — Active-low mapping and input-output GPIO readback


## 2026-01-14

Step 17: add UVC stats + PSRAM logging to UserDemo for comparison

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo/main/service/service_uvc.cpp — UVC capture stats logging
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo/main/utils/camera/camera_init.c — PSRAM readiness logging


## 2026-01-14

Step 18: add SCCB/I2C scan to UserDemo boot

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo/main/usb_webcam_main.cpp — SCCB scan helper and boot hook


## 2026-01-14

Step 19: add menuconfig toggle to disable UVC in UserDemo

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo/main/Kconfig.projbuild — New UVC enable config
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo/main/usb_webcam_main.cpp — UVC startup guard


## 2026-01-14

Step 20: surface esp32-camera tuning options in AtomS3R menu

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/Kconfig.projbuild — Added camera component options submenu


## 2026-01-14

Step 21: log camera component options and analyze sdkconfig diff

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/Kconfig.projbuild — Camera menu additions
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c — Camera config logging
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/14/MO-001-ATOMS3R-WEBSOCKET-STREAMING--atoms3r-websocket-streaming/analysis/05-sdkconfig-diff-demo-vs-stream.md — Diff analysis report


## 2026-01-14

Step 22: confirm diary completeness

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/14/MO-001-ATOMS3R-WEBSOCKET-STREAMING--atoms3r-websocket-streaming/reference/01-diary.md — Updated diary timeline and metadata


## 2026-01-14

Step 23: expand diary with variable-level details

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/14/MO-001-ATOMS3R-WEBSOCKET-STREAMING--atoms3r-websocket-streaming/reference/01-diary.md — Added variable-level documentation


## 2026-01-14

Step 24: Add USB Serial/JTAG console test loop in UserDemo to debug enumeration

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo/main/Kconfig.projbuild — Config toggle for console test loop.
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo/main/usb_webcam_main.cpp — Console test task and USB Serial/JTAG setup.


## 2026-01-14

Step 25: Add USB Serial/JTAG VFS include fallback for UserDemo build

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo/main/usb_webcam_main.cpp — Conditional include to avoid missing usb_serial_jtag_vfs.h build failure.


## 2026-01-14

Step 26: Remove USB Serial/JTAG VFS include to avoid Arduino termios conflict

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo/main/usb_webcam_main.cpp — Console test now relies on system console and usb_serial_jtag_is_connected.


## 2026-01-14

Step 27: Re-check sdkconfig console settings via diff script

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo/sdkconfig — USB Serial/JTAG console is primary in current config.
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo/sdkconfig.defaults — Defaults still use UART0 as primary console.


## 2026-01-14

Step 28: Align UserDemo console defaults with USB Serial/JTAG

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo/sdkconfig — Align console settings with defaults.
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo/sdkconfig.defaults — USB Serial/JTAG primary console defaults.


## 2026-01-15

Add 0040 camera init alignment analysis (post MO-002)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/14/MO-001-ATOMS3R-WEBSOCKET-STREAMING--atoms3r-websocket-streaming/analysis/06-0040-camera-init-alignment-post-mo-002.md — Analysis of app_main + sdkconfig divergences


## 2026-01-15

Align 0040 camera init: warmup delay, post-XCLK SCCB probe, PSRAM-only

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c — Camera init timing and SCCB diagnostics


## 2026-01-15

Step 31: stabilize websocket send/start (commit 3a40f51)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c — Backpressure timeout + start gating


## 2026-01-15

Step 32: fix websocket error logging fields for IDF 5.4 (commit 1f387df)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c — Use esp_tls_stack_err and cert flags


## 2026-01-15

Step 33: fix viewer websocket URL (commit 25b3a6c)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0040-atoms3r-cam-streaming/esp32-camera-stream/client.html — Use page host for /ws/viewer

