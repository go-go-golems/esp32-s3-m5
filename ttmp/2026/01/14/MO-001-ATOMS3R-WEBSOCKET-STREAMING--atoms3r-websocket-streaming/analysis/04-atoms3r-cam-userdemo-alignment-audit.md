---
Title: ATOMS3R-CAM UserDemo Alignment Audit
Ticket: MO-001-ATOMS3R-WEBSOCKET-STREAMING
Status: active
Topics:
    - firmware
    - esp32
    - camera
    - wifi
    - analysis
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/main/service/apis/api_camera.cpp
      Note: JPEG conversion quality in demo.
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/main/usb_webcam_main.cpp
      Note: Camera power enable sequence reference.
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/main/utils/camera/camera_init.c
      Note: Camera init parameters in demo.
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/main/utils/camera/camera_pin.h
      Note: AtomS3R-CAM pin map reference.
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/sdkconfig
      Note: UserDemo camera module
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/Kconfig.projbuild
      Note: Firmware JPEG quality defaults.
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c
      Note: Firmware camera config and power sequencing.
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/sdkconfig
      Note: Firmware camera module/XCLK/PSRAM settings.
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/sdkconfig.defaults
      Note: Default camera/PSRAM settings aligned to demo.
ExternalSources: []
Summary: Audit of the ATOMS3R-CAM UserDemo vs the websocket streaming firmware; highlights mismatches, applied fixes, and remaining risks for camera bring-up.
LastUpdated: 2026-01-14T11:49:25-05:00
WhatFor: Ensure the new firmware matches the demo’s hardware bring-up behavior and camera configuration.
WhenToUse: When debugging camera probe failures or aligning firmware behavior with the stock demo.
---


# ATOMS3R-CAM UserDemo Alignment Audit

## Scope

This report compares the **ATOMS3R-CAM UserDemo** firmware in `ATOMS3R-CAM-UserDemo/` against the websocket streaming firmware in `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/`. The focus is on **hardware bring-up and camera configuration**: power sequencing, pin mapping, SCCB/I2C, XCLK, pixel format, PSRAM usage, and JPEG conversion behavior.

The Wi-Fi networking model intentionally differs (UserDemo uses SoftAP + web UI; streaming firmware uses STA + WebSocket). Those differences are called out as **intentional**.

## High-level alignment summary

- **Pins**: Camera pin map matches the UserDemo’s AtomS3R-CAM mapping.
- **SCCB**: Both use SCCB hardware I2C port 1 @ 100 kHz.
- **XCLK**: Aligned to 20 MHz to match UserDemo.
- **Power enable**: Aligned to UserDemo’s GPIO18 pulldown-only power enable sequence.
- **PSRAM**: Aligned to UserDemo’s OPI/80 MHz PSRAM configuration.
- **JPEG conversion**: Aligned to UserDemo’s `frame2jpg` quality ~80 and sensor JPEG quality 14.

Remaining issue: **camera probe failures (ESP_ERR_NOT_FOUND)** still occur on the device; likely still power or board variant related.

## Alignment matrix (UserDemo vs streaming firmware)

| Area | UserDemo (source) | Streaming firmware (status) | Alignment status |
| --- | --- | --- | --- |
| Camera module selection | `CONFIG_CAMERA_MODULE_M5STACK_ATOMS3R_CAM=y` (`ATOMS3R-CAM-UserDemo/sdkconfig`) | Added to `firmware/sdkconfig` and `sdkconfig.defaults` | Aligned |
| XCLK frequency | `CONFIG_CAMERA_XCLK_FREQ=20000000` | Set in `sdkconfig` and used in `stream_client.c` | Aligned |
| SCCB/I2C port | `CONFIG_SCCB_HARDWARE_I2C_PORT1=y`, 100 kHz | Same | Aligned |
| Pin map | `camera_pin.h` AtomS3R-CAM definitions | Hardcoded to same values in `stream_client.c` | Aligned |
| Camera power enable | GPIO18 pulldown-only + 200 ms delay | Updated to pulldown-only + delay | Aligned |
| Pixel format | `PIXFORMAT_RGB565` | `PIXFORMAT_RGB565` | Aligned |
| Frame size | `FRAMESIZE_QVGA` | `FRAMESIZE_QVGA` | Aligned |
| Sensor JPEG quality | `jpeg_quality=14` | `CONFIG_ATOMS3R_SENSOR_JPEG_QUALITY=14` | Aligned |
| RGB->JPEG conversion | `frame2jpg(..., 80, ...)` | `CONFIG_ATOMS3R_CONVERT_JPEG_QUALITY=80` | Aligned |
| PSRAM usage | `CAMERA_FB_IN_PSRAM`, PSRAM enabled | Same (PSRAM enabled, FB in PSRAM) | Aligned |
| Camera converter | `CONFIG_CAMERA_CONVERTER_ENABLED=y` | Set to `y` | Aligned |
| Wi-Fi mode | SoftAP + HTTP/MJPEG | STA + WebSocket | Intentional difference |

## Detailed findings and changes applied

### 1) Power enable sequencing (GPIO18)

**UserDemo reference**: `ATOMS3R-CAM-UserDemo/main/usb_webcam_main.cpp`

```cpp
gpio_reset_pin((gpio_num_t)18);
gpio_set_direction((gpio_num_t)18, GPIO_MODE_OUTPUT);
gpio_set_pull_mode((gpio_num_t)18, GPIO_PULLDOWN_ONLY);
vTaskDelay(pdMS_TO_TICKS(200));
```

**Streaming firmware**: `firmware/main/stream_client.c`
- Updated to match pulldown-only + delay.

**Why it matters**: Logs showed GPIO18 configured as input with pull-up, indicating camera power may not be asserted. Aligning the pulldown-only configuration reduces the chance of the sensor being unpowered during SCCB probe.

### 2) XCLK and module configuration

**UserDemo** uses `CONFIG_CAMERA_XCLK_FREQ=20000000` and the module selector `CONFIG_CAMERA_MODULE_M5STACK_ATOMS3R_CAM=y` in `ATOMS3R-CAM-UserDemo/sdkconfig`.

**Streaming firmware** now mirrors these:
- `CONFIG_CAMERA_XCLK_FREQ=20000000`
- `CONFIG_CAMERA_MODULE_M5STACK_ATOMS3R_CAM=y`
 - Kconfig definitions for `CAMERA_XCLK_FREQ` and `CAMERA_MODULE_*` added to `firmware/main/Kconfig.projbuild`

**Why it matters**: The esp32-camera driver’s mode and timing are sensitive to XCLK; matching the demo eliminates timing mismatch risk.

### 3) JPEG conversion quality vs sensor JPEG quality

**UserDemo** uses `frame2jpg(..., 80, ...)` in the web stream path and sets sensor `jpeg_quality=14`.

**Streaming firmware** now separates the two:
- `CONFIG_ATOMS3R_SENSOR_JPEG_QUALITY=14`
- `CONFIG_ATOMS3R_CONVERT_JPEG_QUALITY=80`

**Why it matters**: The previous firmware used a 0–63 quality value for `frame2jpg`, which does not match the demo’s 80 and can drastically change frame size and CPU load.

### 4) PSRAM configuration

**UserDemo** enables OPI PSRAM at 80 MHz and boots it early. The streaming firmware now mirrors this in both `sdkconfig` and `sdkconfig.defaults`.

**Why it matters**: The camera driver allocates frame buffers in PSRAM; without it, the sensor probe may fail or streaming will stall due to memory pressure.

## Remaining risks / open issues

1) **Camera probe still failing (ESP_ERR_NOT_FOUND)**
   - This indicates the sensor is not responding on SCCB.
   - Likely causes: power rail not enabled, wrong board revision (M12 vs non-M12), or hardware fault.
   - Next step: validate GPIO18 power rail polarity on the specific board revision; confirm SCCB pins with hardware.

2) **Wi-Fi mode mismatch is intentional**
   - UserDemo uses SoftAP with HTTP MJPEG streaming.
   - The new firmware uses STA to reach the Go WebSocket server.
   - This is expected and not a hardware bring-up mismatch.

## Files audited

- UserDemo
  - `ATOMS3R-CAM-UserDemo/sdkconfig`
  - `ATOMS3R-CAM-UserDemo/main/usb_webcam_main.cpp`
  - `ATOMS3R-CAM-UserDemo/main/utils/camera/camera_pin.h`
  - `ATOMS3R-CAM-UserDemo/main/utils/camera/camera_init.c`
  - `ATOMS3R-CAM-UserDemo/main/service/apis/api_camera.cpp`

- Streaming firmware
  - `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c`
  - `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/Kconfig.projbuild`
  - `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/sdkconfig`
  - `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/sdkconfig.defaults`

## Action checklist (if probe still fails)

- Confirm the board variant (M12 vs non-M12) and ensure the matching pin map.
- Verify GPIO18 power rail polarity (active low vs active high).
- Add a one-time SCCB scan (I2C probe) on port 1 before `esp_camera_init()` to confirm the sensor address.
- If needed, toggle GPIO18 high for a short pulse before pulling low, in case the enable pin is edge-sensitive.
