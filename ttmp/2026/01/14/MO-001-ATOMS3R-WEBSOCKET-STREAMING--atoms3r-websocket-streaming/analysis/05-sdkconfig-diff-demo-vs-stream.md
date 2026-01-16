---
Title: SDKConfig Diff (UserDemo vs Camera Stream)
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
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/sdkconfig
      Note: UserDemo reference config (known-good camera init).
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/sdkconfig
      Note: Streaming firmware reference config.
    - Path: ../various/sdkconfig-demo-vs-stream.diff
      Note: Unified diff produced by diff_sdkconfig.py.
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/scripts/diff_sdkconfig.py
      Note: Script used to generate the diff.
ExternalSources: []
Summary: Comparison of UserDemo vs streaming firmware sdkconfig values, with a focus on camera bring-up and system differences that may affect camera probe and streaming stability.
LastUpdated: 2026-01-14T14:21:19-05:00
WhatFor: Identify sdkconfig differences that could explain camera probe failures or runtime mismatches.
WhenToUse: When aligning firmware behavior with the working UserDemo.
---

# SDKConfig Diff (UserDemo vs Camera Stream)

## Overview

This report compares the **UserDemo** `sdkconfig` with the **streaming firmware** `sdkconfig` using the unified diff in `ttmp/2026/01/14/MO-001-ATOMS3R-WEBSOCKET-STREAMING--atoms3r-websocket-streaming/various/sdkconfig-demo-vs-stream.diff`. The focus is on camera bring-up, USB/console usage, memory, and system settings that could affect SCCB probe success.

Command used:

```bash
python 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/scripts/diff_sdkconfig.py \
  ../ATOMS3R-CAM-UserDemo/sdkconfig \
  0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/sdkconfig \
  --out ttmp/2026/01/14/MO-001-ATOMS3R-WEBSOCKET-STREAMING--atoms3r-websocket-streaming/various/sdkconfig-demo-vs-stream.diff
```

## Camera configuration parity (already aligned)

These items **match** between the two configs and are unlikely to explain the camera probe failure:

- `CONFIG_CAMERA_MODULE_M5STACK_ATOMS3R_CAM=y`
- `CONFIG_CAMERA_XCLK_FREQ=20000000`
- `CONFIG_SCCB_HARDWARE_I2C_PORT1=y`
- `CONFIG_SCCB_CLK_FREQ=100000`
- `CONFIG_CAMERA_TASK_STACK_SIZE=2048`
- `CONFIG_CAMERA_CORE0=y`
- `CONFIG_CAMERA_DMA_BUFFER_SIZE_MAX=32768`
- `CONFIG_CAMERA_JPEG_MODE_FRAME_SIZE_AUTO=y`
- `CONFIG_CAMERA_CONVERTER_ENABLED=y`
- `CONFIG_GC_SENSOR_SUBSAMPLE_MODE=y`

## High-impact differences to consider unifying

1) **Flash size + partitioning**
- UserDemo: `CONFIG_ESPTOOLPY_FLASHSIZE=8MB`, custom partition table `partitions.csv`.
- Stream firmware: `CONFIG_ESPTOOLPY_FLASHSIZE=2MB`, single-app partition `partitions_singleapp.csv`.
- Why it matters: the streaming firmware currently has ~3% free space in the app partition; the UserDemo assumes more flash. If your AtomS3R-CAM has 8MB flash, updating the streaming firmware to 8MB + a larger partition may avoid memory pressure and reduce camera init failures.

2) **CPU frequency**
- UserDemo: `CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ=240`.
- Stream firmware: `CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ=160`.
- Why it matters: camera timing (SCCB, XCLK, DMA) and JPEG conversion throughput are sensitive to CPU frequency. Matching 240 MHz could reduce timeouts or race conditions.

3) **Console / USB usage**
- UserDemo: UART console + USB UVC enabled.
- Stream firmware: USB Serial/JTAG console, no UVC.
- Why it matters: UVC uses the USB OTG peripheral, which conflicts with USB Serial/JTAG. The new `CONFIG_USB_WEBCAM_ENABLE_UVC` toggle lets you disable UVC to compare logs directly via JTAG.

4) **Arduino framework**
- UserDemo: Arduino enabled (`CONFIG_ENABLE_ARDUINO_DEPENDS=y`), Async TCP settings, Arduino task pinning/stack sizes.
- Stream firmware: pure ESP-IDF, no Arduino.
- Why it matters: Arduino pulls in async TCP and changes LWIP/stack defaults. If the demo’s camera init depends on Arduino-era driver state or startup order, you may see subtle timing differences.

## Lower-impact differences (likely secondary)

- **Wi-Fi TX buffers**: UserDemo uses static TX buffers; stream firmware uses dynamic TX buffers. Could affect memory fragmentation but unlikely to block SCCB probe.
- **USB OTG options**: the UserDemo enables UVC/TinyUSB tasks; stream firmware does not.
- **PSRAM stack allow**: the UserDemo sets `CONFIG_SPIRAM_ALLOW_STACK_EXTERNAL_MEMORY`; in the stream firmware this is replaced by `CONFIG_FREERTOS_TASK_CREATE_ALLOW_EXT_MEM` by IDF.

## Recommended alignment steps

1) Confirm actual flash size on the board. If 8MB, update streaming firmware to match flash size + partition table.
2) Try matching CPU frequency to 240 MHz and retest SCCB probe.
3) Disable UVC in the demo (via menuconfig) to keep USB Serial/JTAG consistent across both builds.
4) If SCCB still fails, compare the new SCCB scan output from the demo vs the streaming firmware using the same pins and clock.
