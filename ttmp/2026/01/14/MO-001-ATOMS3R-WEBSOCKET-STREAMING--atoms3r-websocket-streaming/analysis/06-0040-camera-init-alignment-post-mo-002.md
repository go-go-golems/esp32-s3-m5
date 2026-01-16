---
Title: 0040 Camera Init Alignment (Post MO-002)
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
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/app_main.c
      Note: |-
        Entry point and init order for the streaming firmware
        Entry point ordering analyzed
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c
      Note: |-
        Camera init and capture loop used by the stream task
        Camera init and capture flow analyzed
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/sdkconfig
      Note: |-
        Active config used for build
        Active streaming firmware config
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/sdkconfig.defaults
      Note: |-
        Baseline config values
        Baseline streaming firmware defaults
    - Path: ttmp/2026/01/14/MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO--atoms3r-cam-userdemo-usb-serial-jtag-console/reference/04-camera-bootstrap-playbook.md
      Note: Camera bootstrap playbook with timing and SCCB probe guidance
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-15T22:17:35-05:00
WhatFor: ""
WhenToUse: ""
---


# 0040 Camera Init Alignment (Post MO-002)

## Goal

Analyze the current camera initialization path in `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/app_main.c` and its sdkconfig values, compare them to the camera bootstrap playbook, and list the divergences and fixes needed to align 0040 with the now-verified bring-up sequence.

## Scope and Inputs

This analysis covers:
- Entry point: `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/app_main.c`
- Camera init path: `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c`
- Config: `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/sdkconfig` and `.../sdkconfig.defaults`
- Reference sequence: `ttmp/.../reference/04-camera-bootstrap-playbook.md`

## Current 0040 Initialization Sequence (as implemented)

### app_main (entry point)

`app_main()` is minimal and does not touch the camera directly. It sets log level, starts WiFi STA, initializes the stream client, and starts the console. Camera initialization is deferred until the stream task requests frames.

File: `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/app_main.c`

Sequence:
- `esp_log_level_set("*", ESP_LOG_INFO)`
- `cam_wifi_start()`
- `stream_client_init()`
- `cam_console_start()`
- Idle loop with `vTaskDelay(1000 ms)`

### stream_client camera init

Camera initialization happens lazily inside `camera_init_once()` when the stream task sees the stream enabled. The function powers the camera, checks PSRAM, builds `camera_config_t`, and calls `esp_camera_init()`.

File: `0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c`

Key steps:
- `camera_power_on()` (uses GPIO18 + active-low config)
- `psram_ready()` with DRAM fallback (fb_count=1, fb_location=DRAM)
- `esp_camera_init(&cfg)`
- Sensor tweaks (vflip, GC0308 hmirror, etc.)

## Current 0040 Camera Init Pseudocode (simplified)

```c
// File: 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c
if (!s_camera_ready) {
  camera_power_on();                 // GPIO18 active-low
  bool use_psram = psram_ready();    // fallback to DRAM if missing

  camera_config_t cfg = {
    .pin_xclk = 21,
    .pin_sccb_sda = 12,
    .pin_sccb_scl = 9,
    .pixel_format = PIXFORMAT_RGB565,
    .frame_size = FRAMESIZE_QVGA,
    .jpeg_quality = CONFIG_ATOMS3R_SENSOR_JPEG_QUALITY,
    .fb_count = use_psram ? 2 : 1,
    .fb_location = use_psram ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM,
  };

  esp_camera_init(&cfg);
  // apply sensor tweaks
  s_camera_ready = true;
}
```

## sdkconfig Summary (0040)

### Console and PSRAM (defaults)

From `.../sdkconfig.defaults`:
- `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`
- `# CONFIG_ESP_CONSOLE_UART is not set`
- `CONFIG_SPIRAM=y`
- `CONFIG_SPIRAM_MODE_OCT=y`
- `CONFIG_SPIRAM_SPEED_80M=y`
- `CONFIG_SPIRAM_BOOT_INIT=y`
- `CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=6384`
- `CONFIG_SPIRAM_MALLOC_RESERVE_INTERNAL=32768`

### Camera and SCCB

From `.../sdkconfig.defaults`:
- `CONFIG_CAMERA_MODULE_M5STACK_ATOMS3R_CAM=y`
- `CONFIG_CAMERA_XCLK_FREQ=20000000`
- `CONFIG_SCCB_HARDWARE_I2C_PORT1=y`
- `CONFIG_SCCB_CLK_FREQ=100000`
- `CONFIG_CAMERA_TASK_STACK_SIZE=2048`
- `CONFIG_CAMERA_CORE0=y`
- `CONFIG_CAMERA_DMA_BUFFER_SIZE_MAX=32768`
- `CONFIG_CAMERA_JPEG_MODE_FRAME_SIZE_AUTO=y`
- `CONFIG_ATOMS3R_CAMERA_POWER_ACTIVE_LOW=y`
- `CONFIG_ATOMS3R_SENSOR_JPEG_QUALITY=14`

The generated `sdkconfig` mirrors these values, so the configuration is already aligned with the working UserDemo/0041 settings.

## Divergences from the Camera Bootstrap Playbook

1) **No explicit warmup delay after power enable**
   - Playbook uses a 200 ms power delay plus a 1000 ms warmup delay before `esp_camera_init()`.
   - 0040 calls `camera_power_on()` and proceeds immediately to `esp_camera_init()`.
   - Risk: probe can return `ESP_ERR_NOT_FOUND` if the sensor is not yet responsive.

2) **No optional post-XCLK SCCB probe**
   - Playbook recommends an optional post-init probe using known addresses.
   - 0040 does not include any SCCB probe step for diagnostics.

3) **DRAM fallback when PSRAM missing**
   - 0040 will fall back to DRAM buffers if PSRAM is not initialized.
   - The playbook prefers strict PSRAM usage for parity with UserDemo and to avoid silent quality reductions.

4) **Camera init timing depends on stream task**
   - Camera init occurs inside the stream task after runtime config, not during boot.
   - If the stream is enabled quickly after power-on, the init may be too early without a warmup delay.

## Fix Recommendations for 0040

### 1) Add explicit warmup delay

Add a delay after `camera_power_on()` and before `esp_camera_init()`:

```c
// stream_client.c
camera_power_on();
vTaskDelay(pdMS_TO_TICKS(200));  // power settle
vTaskDelay(pdMS_TO_TICKS(1000)); // warmup
```

Consider a Kconfig option (e.g., `CONFIG_ATOMS3R_CAMERA_WARMUP_MS`) if you want to tune this without code changes.

### 2) Optional post-XCLK SCCB probe

After a successful `esp_camera_init()`, run a known-address probe using the existing I2C driver:

```c
if (esp_camera_init(&cfg) == ESP_OK) {
  camera_sccb_probe_known_addrs(); // reuse driver, no install
}
```

This mirrors the approach validated in MO-002 and avoids pre-XCLK false negatives.

### 3) Decide on PSRAM fallback policy

If strict parity with UserDemo is required:
- Remove the DRAM fallback and fail fast when PSRAM is unavailable.

If robustness is preferred:
- Keep the fallback, but log it loudly and reduce frame size (e.g., QQVGA) to avoid memory pressure.

### 4) Ensure logging captures timing

Add explicit log lines around power enable, warmup, and camera init so failures can be correlated to timing.

## Validation Checklist

- `esp_camera_init()` returns `ESP_OK` and logs sensor ID.
- SCCB probe (post-XCLK) reports `ack at 0x21` for GC0308.
- Frame capture loop logs stable `len=153600` frames for QVGA RGB565.
- No `i2c_driver_install failed` messages (probe should reuse existing driver).

## Open Questions

- Should 0040 enforce strict PSRAM usage to match UserDemo, or keep DRAM fallback for robustness?
- Should camera init move earlier (boot-time) or remain lazy in the stream task?

## Recommended Next Steps

1) Add the warmup delay in `stream_client.c` and re-test camera init.
2) Add optional post-XCLK SCCB probe for diagnostics.
3) Decide on PSRAM fallback policy and document it in the README.
