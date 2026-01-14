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

