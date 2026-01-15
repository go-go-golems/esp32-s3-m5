# Changelog

## 2026-01-14

- Initial workspace created


## 2026-01-14

Step 1: Gate UVC component and code behind CONFIG_USB_WEBCAM_ENABLE_UVC

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo/components/usb_device_uvc/CMakeLists.txt — Skip UVC component registration when UVC is disabled.
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo/main/service/service_uvc.cpp — Guard UVC service compilation.
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo/main/utils/camera/camera_init.c — Remove UVC header dependency.


## 2026-01-14

Step 2: Exclude TinyUSB in CMake when UVC disabled; build attempt shows tinyusb removed

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo/CMakeLists.txt — Exclude TinyUSB/UVC components when UVC disabled.
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo/main/CMakeLists.txt — Optional gen_single_bin include.


## 2026-01-14

Step 3: Investigate esp_insights build failure after dependency updates

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo/dependencies.lock — Lockfile updates accompany TinyUSB removal.
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo/managed_components/espressif__esp_insights/src/esp_insights_cbor_encoder.c — SHA_SIZE undefined build error.


## 2026-01-14

Step 4: Import esp_insights SHA_SIZE research and summarize fixes

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/14/MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO--atoms3r-cam-userdemo-usb-serial-jtag-console/sources/local/Research — esp_insights SHA_SIZE.md:Dependency drift analysis and fix options.


## 2026-01-14

Step 5: Add esp_insights pin and rebuild; solver resolved to 1.3.1

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo/dependencies.lock — Updated managed component versions.
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo/idf_component.yml — Project manifest for esp_insights.


## 2026-01-14

Step 5: Bump esp_insights to restore UserDemo build.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo/dependencies.lock — Managed component versions updated after dependency refresh.
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-UserDemo/idf_component.yml — Updated esp_insights version to align managed dependencies.


## 2026-01-14

Step 6: Add 0041 minimal USB Serial/JTAG test firmware (commit f573748).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0041-atoms3r-cam-jtag-serial-test/main/main.c — Minimal USB Serial/JTAG tick loop.
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0041-atoms3r-cam-jtag-serial-test/sdkconfig.defaults — Console defaults for USB Serial/JTAG.


## 2026-01-14

Step 7: Flash attempt failed due to /dev/ttyACM0 busy/not present.


## 2026-01-14

Step 8: Add camera-only init/capture firmware in 0041 (commit efc7afc).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0041-atoms3r-cam-jtag-serial-test/main/Kconfig.projbuild — Camera pin/power Kconfig options.
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0041-atoms3r-cam-jtag-serial-test/main/main.c — Camera init
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0041-atoms3r-cam-jtag-serial-test/sdkconfig.defaults — Camera/PSRAM/SCCB defaults.


## 2026-01-14

Step 9: Align 0041 camera init flow with UserDemo (commit 5e8f1f4).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0041-atoms3r-cam-jtag-serial-test/main/main.c — SCCB scan and sensor tuning aligned to UserDemo.


## 2026-01-14

Step 10: Flash attempt blocked by busy /dev/ttyACM0.


## 2026-01-14

Step 11: Flash 0041 and capture camera init failure logs (SCCB scan empty, ESP_ERR_NOT_FOUND).


## 2026-01-14

Added camera init comparison analysis and a new analysis diary for the UserDemo vs 0041 review.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/14/MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO--atoms3r-cam-userdemo-usb-serial-jtag-console/analysis/01-camera-init-analysis-userdemo-vs-0041.md — Detailed camera init comparison
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/14/MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO--atoms3r-cam-userdemo-usb-serial-jtag-console/reference/02-diary-camera-init-analysis.md — Diary of analysis steps


## 2026-01-14

Expanded camera init analysis with background explanation; recorded diary Step 2.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/14/MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO--atoms3r-cam-userdemo-usb-serial-jtag-console/analysis/01-camera-init-analysis-userdemo-vs-0041.md — Added background on camera pipeline and PSRAM
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/14/MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO--atoms3r-cam-userdemo-usb-serial-jtag-console/reference/02-diary-camera-init-analysis.md — Logged Step 2


## 2026-01-14

Added step-by-step UserDemo vs 0041 comparison and remediation guidance.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/14/MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO--atoms3r-cam-userdemo-usb-serial-jtag-console/analysis/01-camera-init-analysis-userdemo-vs-0041.md — Phase-by-phase comparison and action items
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/14/MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO--atoms3r-cam-userdemo-usb-serial-jtag-console/reference/02-diary-camera-init-analysis.md — Diary Step 3


## 2026-01-14

Fleshed out the comparison with pseudo-code, expected observations, and a deeper remediation checklist.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/14/MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO--atoms3r-cam-userdemo-usb-serial-jtag-console/analysis/01-camera-init-analysis-userdemo-vs-0041.md — Expanded diagnostic guidance
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/14/MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO--atoms3r-cam-userdemo-usb-serial-jtag-console/reference/02-diary-camera-init-analysis.md — Diary Step 4


## 2026-01-14

Added 0041 debugging guidebook and created a dedicated debugging diary.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/14/MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO--atoms3r-cam-userdemo-usb-serial-jtag-console/analysis/01-camera-init-analysis-userdemo-vs-0041.md — Debugging plan/guidebook
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/14/MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO--atoms3r-cam-userdemo-usb-serial-jtag-console/reference/03-diary-0041-debugging.md — New debugging diary


## 2026-01-14

Ran debug Step 1 (power polarity sweep) in tmux, captured logs, and initialized sqlite debug tracking.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0041-atoms3r-cam-jtag-serial-test/main/main.c — Step marker and power sweep instrumentation
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/14/MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO--atoms3r-cam-userdemo-usb-serial-jtag-console/scripts/debug_db/debug.sqlite3 — Debug run data store
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/14/MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO--atoms3r-cam-userdemo-usb-serial-jtag-console/various/debug-logs/step-01-power-sweep-flash.log — Step 1 log output


## 2026-01-14

Expanded debug dashboard verbosity, added metrics tables, and backfilled sqlite metrics.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/14/MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO--atoms3r-cam-userdemo-usb-serial-jtag-console/scripts/debug_db/backfill_metrics.py — Metrics backfill script
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/14/MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO--atoms3r-cam-userdemo-usb-serial-jtag-console/scripts/debug_db/dashboard.py — Verbose dashboard output
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/14/MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO--atoms3r-cam-userdemo-usb-serial-jtag-console/scripts/debug_db/schema.sql — Metrics tables


## 2026-01-14

Imported user-provided IDF 5.1.4 monitor log; confirmed PSRAM allocation failure during camera init.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/14/MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO--atoms3r-cam-userdemo-usb-serial-jtag-console/scripts/debug_db/debug.sqlite3 — Run data for IDF 5.1.4 log
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/14/MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO--atoms3r-cam-userdemo-usb-serial-jtag-console/various/debug-logs/step-01-power-sweep-monitor-idf5.1.4-user.log — Step 1 runtime log


## 2026-01-14

Enabled CONFIG_SPIRAM in 0041 sdkconfig and added SCCB scan investigation task.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0041-atoms3r-cam-jtag-serial-test/sdkconfig — Enable PSRAM for camera buffer allocation
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/14/MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO--atoms3r-cam-userdemo-usb-serial-jtag-console/tasks.md — New SCCB scan task


## 2026-01-14

Updated .envrc to clear IDF_PYTHON_ENV_PATH before sourcing IDF 5.1.4.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0041-atoms3r-cam-jtag-serial-test/.envrc — Prevent IDF env mismatch


## 2026-01-14

Noted attempted Step 3 tmux rebuild/flash; no logs captured, rerun pending.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/14/MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO--atoms3r-cam-userdemo-usb-serial-jtag-console/reference/03-diary-0041-debugging.md — Diary Step 9


## 2026-01-14

Step 10: add esp_psram component requirement (commit ad7e772)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0041-atoms3r-cam-jtag-serial-test/main/CMakeLists.txt — Fix build error for esp_psram.h include

