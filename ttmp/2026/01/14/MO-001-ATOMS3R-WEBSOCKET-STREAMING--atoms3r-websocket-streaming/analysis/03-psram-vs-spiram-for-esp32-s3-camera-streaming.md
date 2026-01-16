---
Title: PSRAM vs SPIRAM for ESP32-S3 Camera Streaming
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
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/components/esp32-camera/README.md
      Note: EDMA note for 16MHz XCLK on S2/S3.
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/components/esp32-camera/driver/cam_hal.c
      Note: Frame buffer allocation and psram_mode handling.
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/components/esp32-camera/target/esp32s3/ll_cam.c
      Note: ESP32-S3 GDMA path differences with psram_mode.
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/main/utils/camera/camera_init.c
      Note: Sets CAMERA_FB_IN_PSRAM and camera init parameters.
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/sdkconfig
      Note: Demo SPIRAM/PSRAM configuration and IO pins.
ExternalSources: []
Summary: Textbook-style explanation of PSRAM vs SPIRAM, how ESP-IDF configures and initializes external RAM on ESP32-S3, and how the esp32-camera pipeline uses PSRAM for video streaming buffers.
LastUpdated: 2026-01-14T10:57:06.09362416-05:00
WhatFor: Understand why the ATOMS3R-CAM demo requires PSRAM and how PSRAM affects camera capture and streaming reliability.
WhenToUse: When deciding sdkconfig PSRAM settings or debugging camera frame buffer allocation and boot failures.
---


# PSRAM vs SPIRAM for ESP32-S3 Camera Streaming

## Executive summary

On ESP32-S3, **PSRAM** is the external pseudo-static RAM chip on the module, while **SPIRAM** is Espressif's name for the ESP-IDF driver/configuration that makes that PSRAM usable over the SPI/OPI memory interface. Enabling `CONFIG_SPIRAM=y` in `sdkconfig` tells the bootloader and ESP-IDF to initialize the PSRAM chip at boot, add it to the heap, and expose it through `heap_caps_malloc()` and regular `malloc()` (when `CONFIG_SPIRAM_USE_MALLOC=y`).

For camera streaming, PSRAM is the difference between "only tiny frame buffers" and "real video frames": the camera driver (esp32-camera) uses PSRAM to store JPEG frames and, on ESP32-S3, can set up DMA descriptors that write camera data directly into PSRAM-backed frame buffers. The ATOMS3R-CAM demo explicitly sets `camera_config.fb_location = CAMERA_FB_IN_PSRAM` and expects PSRAM to exist; if PSRAM is missing or misconfigured, the bootloader may abort early or the camera buffer allocation will fail.

## Terminology (PSRAM vs SPIRAM)

**PSRAM**
- **Hardware**: Pseudo-static RAM, a DRAM cell array with internal refresh but a SRAM-like interface.
- **Physical chip**: Typically a separate die/package on the module (often 8 MB on ESP32-S3 boards).
- **Access characteristics**: Higher latency and lower bandwidth than internal SRAM, but much larger capacity.

**SPIRAM**
- **Software term (ESP-IDF)**: The driver and Kconfig options that enable **SPI/OPI-attached external RAM** (aka PSRAM).
- **Kconfig naming**: Many ESP-IDF options use the name "SPIRAM" even though the hardware is PSRAM.
- **Practical translation**: "Enable SPIRAM" means "initialize and use PSRAM".

In short: **PSRAM is the chip**; **SPIRAM is the ESP-IDF support layer** that makes the chip usable.

## ESP32-S3 memory layout and why PSRAM matters

### Memory tiers

```
+------------------------------+
| Internal SRAM                |
| - Fast, low latency          |
| - DMA capable                |
| - Limited size (~512 KB)     |
+------------------------------+
             |
             | Cache + Memory Controller
             v
+------------------------------+
| External PSRAM (SPIRAM)      |
| - Larger (often 8 MB)        |
| - Higher latency             |
| - Not always DMA-capable     |
+------------------------------+
```

Key implications for camera streaming:
- A single **QVGA RGB565 frame** is ~150 KB (320 * 240 * 2 bytes).
- A single **VGA RGB565 frame** is ~600 KB (640 * 480 * 2 bytes).
- JPEG frames are smaller but still can be tens of KB, and you often want 2+ buffers.
- **Internal SRAM is not enough** for multiple full-sized frames + networking stacks.
- **PSRAM provides the memory headroom** for buffering frames while Wi-Fi transmits them.

## What the ATOMS3R-CAM demo enables in sdkconfig

The demo's `sdkconfig` enables PSRAM via SPIRAM and configures it for Octal (OPI) mode and 80 MHz:

- `CONFIG_SPIRAM=y`
- `CONFIG_SPIRAM_MODE_OCT=y`
- `CONFIG_SPIRAM_SPEED_80M=y`
- `CONFIG_SPIRAM_CLK_IO=30`
- `CONFIG_SPIRAM_CS_IO=26`
- `CONFIG_SPIRAM_BOOT_INIT=y`
- `CONFIG_SPIRAM_USE_MALLOC=y`
- `CONFIG_SPIRAM_ALLOW_STACK_EXTERNAL_MEMORY=y`

These options are visible in `ATOMS3R-CAM-UserDemo/sdkconfig`.

### Why these flags matter

- **`CONFIG_SPIRAM_BOOT_INIT=y`**
  - PSRAM is initialized during early boot. If initialization fails, boot aborts (unless `CONFIG_SPIRAM_IGNORE_NOTFOUND=y`).
- **`CONFIG_SPIRAM_MODE_OCT=y`**
  - Uses Octal (OPI) PSRAM on ESP32-S3 modules that wire PSRAM in octal mode.
- **`CONFIG_SPIRAM_USE_MALLOC=y`**
  - Adds PSRAM to the default heap so `malloc()` can allocate from PSRAM automatically.
- **`CONFIG_SPIRAM_MALLOC_RESERVE_INTERNAL=32768`**
  - Reserves internal SRAM for DMA and time-critical code, preventing exhaustion.
- **`CONFIG_SPIRAM_ALLOW_STACK_EXTERNAL_MEMORY=y`**
  - Allows task stacks to live in PSRAM if needed (useful for large stacks).

If any of these are wrong for the module (wrong mode, wrong IO pins, speed too high), the bootloader will fail to read the PSRAM ID and abort, which is exactly the boot log error:

```
E (225) quad_psram: PSRAM ID read error: 0x00ffffff
E cpu_start: Failed to init external RAM!
```

## How ESP-IDF uses PSRAM (SPIRAM) at runtime

ESP-IDF uses PSRAM as an extra heap region when SPIRAM is enabled. Conceptually, the boot flow is:

```text
bootloader
  -> init_flash()
  -> init_psram()  (if CONFIG_SPIRAM_BOOT_INIT)
  -> add_psram_to_heap()
  -> start_app()

app_main
  -> malloc() / heap_caps_malloc() can return PSRAM-backed buffers
```

Common APIs and flags:
- `esp_psram_init()` / `esp_psram_is_initialized()`
- `heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)`
- `heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM)`
- `MALLOC_CAP_DMA` is for internal DMA-capable SRAM (PSRAM is not always DMA-capable)

In the demo config, `CONFIG_SPIRAM_USE_MALLOC=y` means **regular `malloc()` will use PSRAM** for large allocations unless constrained by caps.

## How esp32-camera uses PSRAM on ESP32-S3

### The demo explicitly requests PSRAM

The ATOMS3R-CAM demo sets:

```c
.fb_location = CAMERA_FB_IN_PSRAM
```

in `ATOMS3R-CAM-UserDemo/main/utils/camera/camera_init.c`. That choice tells the camera driver to allocate the frame buffers in PSRAM.

### Driver behavior in cam_hal.c

In `components/esp32-camera/driver/cam_hal.c`:
- Frame buffers are allocated with `MALLOC_CAP_SPIRAM` when `fb_location` is PSRAM.
- If `psram_mode` is enabled, DMA descriptors are built for the PSRAM buffers.
- If `psram_mode` is disabled, DMA writes into internal SRAM and the camera task copies into the framebuffer.

Simplified pseudocode:

```text
if (fb_location == PSRAM) {
  caps |= MALLOC_CAP_SPIRAM;
}
fb = heap_caps_alloc(caps);

if (psram_mode) {
  // align PSRAM buffer and build DMA descriptors targeting PSRAM
  fb.dma = allocate_dma_descriptors(fb.buf);
} else {
  // allocate internal DMA buffer and copy into fb
  dma_buf = heap_caps_malloc(MALLOC_CAP_DMA);
}
```

### psram_mode is tied to XCLK on ESP32-S3

In `cam_hal.c` the driver sets:

```c
cam_obj->psram_mode = (config->xclk_freq_hz == 16000000);
```

This matches the README note in `components/esp32-camera/README.md`:

```
Set to 16MHz on ESP32-S2 or ESP32-S3 to enable EDMA mode
```

Meaning:
- On ESP32-S3, **16 MHz XCLK enables the EDMA mode** that writes directly into PSRAM.
- Other XCLK frequencies may force the internal DMA + copy path, even if the frame buffer itself lives in PSRAM.

### Data flow with and without PSRAM

```
Camera sensor -> LCD_CAM -> GDMA -> [internal DMA buffer] -> cam_task copy -> PSRAM frame buffer
                                  (psram_mode == false)

Camera sensor -> LCD_CAM -> GDMA -> PSRAM frame buffer (aligned)
                                  (psram_mode == true)
```

The PSRAM path reduces CPU copying and improves throughput, which is critical for continuous JPEG streaming.

## Why video streaming needs PSRAM (memory math)

Approximate buffer sizes (uncompressed RGB565):

- QVGA (320x240): ~150 KB
- VGA (640x480): ~600 KB
- SVGA (800x600): ~960 KB

Even with JPEG compression, you still want **2+ frame buffers** to keep capture and network transmission decoupled. Internal SRAM cannot comfortably hold multiple full frames plus Wi-Fi/LwIP buffers, USB stacks, and FreeRTOS tasks.

Rule of thumb:
- **PSRAM is required** for steady streaming at VGA/SVGA or higher.
- **PSRAM is strongly recommended** even for QVGA when using JPEG and Wi-Fi simultaneously.

## Failure modes and diagnostics

Common failure patterns:

- **Boot abort due to missing/misconfigured PSRAM**
  - Symptom: `quad_psram: PSRAM ID read error` + `Failed to init external RAM`.
  - Cause: PSRAM not present, wrong mode (quad vs octal), wrong IO pins, or too high speed.

- **Camera init fails**
  - Symptom: `frame buffer malloc failed` in `cam_hal.c`.
  - Cause: PSRAM not initialized or heap too fragmented.

- **Low frame rate / dropped frames**
  - Symptom: `FB-OVF` or `FBQ-SND` errors.
  - Cause: DMA buffer overruns or insufficient buffering, often from PSRAM being disabled or psram_mode not enabled.

Key places to check:
- Boot logs for PSRAM init status.
- `heap_caps_get_free_size(MALLOC_CAP_SPIRAM)` after boot.
- XCLK frequency and whether EDMA (psram_mode) is actually in use.

## Practical guidance for matching the demo

To match the ATOMS3R-CAM demo behavior, the following are core expectations:

- Enable PSRAM (SPIRAM) with **Octal mode + 80 MHz** in `sdkconfig`.
- Keep `CONFIG_SPIRAM_BOOT_INIT=y` so PSRAM is ready before the camera driver allocates buffers.
- Use `camera_config.fb_location = CAMERA_FB_IN_PSRAM`.
- Use **16 MHz XCLK** if you want the EDMA path on ESP32-S3 (per esp32-camera README).

If PSRAM is not actually available on the module, you must either:
- Disable SPIRAM and reduce camera settings (frame size, fb_count), or
- Use `CONFIG_SPIRAM_IGNORE_NOTFOUND=y` and accept degraded/no camera capability.

## References (code and configs)

- PSRAM config: `ATOMS3R-CAM-UserDemo/sdkconfig`
- Camera init with PSRAM buffers: `ATOMS3R-CAM-UserDemo/main/utils/camera/camera_init.c`
- Camera DMA/PSRAM behavior: `ATOMS3R-CAM-UserDemo/components/esp32-camera/driver/cam_hal.c`
- ESP32-S3 camera low-level DMA: `ATOMS3R-CAM-UserDemo/components/esp32-camera/target/esp32s3/ll_cam.c`
- esp32-camera note on EDMA/XCLK: `ATOMS3R-CAM-UserDemo/components/esp32-camera/README.md`

## API references (ESP-IDF)

- `esp_psram_init()`, `esp_psram_is_initialized()`
- `heap_caps_malloc()`, `heap_caps_get_free_size()`
- `camera_config_t`, `esp_camera_init()`, `esp_camera_fb_get()`
- `MALLOC_CAP_SPIRAM`, `MALLOC_CAP_DMA`
