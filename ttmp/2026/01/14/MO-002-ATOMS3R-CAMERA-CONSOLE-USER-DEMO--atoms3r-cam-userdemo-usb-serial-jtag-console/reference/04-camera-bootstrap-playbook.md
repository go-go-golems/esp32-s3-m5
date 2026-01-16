---
Title: Camera Bootstrap Playbook
Ticket: MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO
Status: active
Topics:
    - camera
    - esp32
    - firmware
    - usb
    - console
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0041-atoms3r-cam-jtag-serial-test/components/esp32-camera/driver/esp_camera.c
      Note: Driver probe order and XCLK behavior
    - Path: 0041-atoms3r-cam-jtag-serial-test/components/esp32-camera/driver/sccb.c
      Note: SCCB probe implementation
    - Path: 0041-atoms3r-cam-jtag-serial-test/main/main.c
      Note: Reference implementation of the boot sequence and steps
    - Path: 0041-atoms3r-cam-jtag-serial-test/sdkconfig.defaults
      Note: Baseline sdkconfig values referenced in the playbook
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-15T22:12:34-05:00
WhatFor: ""
WhenToUse: ""
---


# Camera Bootstrap Playbook

## Goal

Provide a durable, copy/paste-ready playbook for bootstrapping an ESP32-S3 camera (AtomS3R-CAM) using the esp32-camera driver, including an optional SCCB probe, frame capture, required sdkconfig values, and a troubleshooting approach.

## Context

This playbook assumes:
- ESP32-S3 with a parallel camera sensor and esp32-camera component.
- SCCB (I2C-like) control on GPIO 12/9 with I2C port 1.
- XCLK driven by LEDC at 20 MHz.
- External PSRAM enabled and used for frame buffers.
- USB Serial/JTAG console enabled (UART pins are often repurposed).

Key facts:
- Many sensors will not ACK SCCB unless XCLK is running.
- esp32-camera enables XCLK, initializes SCCB, probes known addresses, then configures DMA and frame buffers.
- Frame buffers in PSRAM require `CONFIG_SPIRAM=y` and matching PSRAM settings.

## Quick Reference

### Initialization sequence (ASCII diagram)

```
Power Up
  |
  v
[Step 0] Boot log
  |
  v
[Step 1A] Power polarity sweep (debug-only)
  |
  v
[Step 1B] Power polarity sweep (debug-only)
  |
  v
[Step 1C] Power enable (GPIO18 active-low)
  |
  v
[Step 1D] Warmup delay (explicit)
  |
  v
[Step 2] esp_camera_init()
  |    - XCLK on
  |    - SCCB init
  |    - Probe known SCCB addrs
  |    - DMA + frame buffers
  v
[Step 3] Optional SCCB probe (post-XCLK)
  |
  v
[Step 4] Capture loop (esp_camera_fb_get/return)
```

### Bootstrapping checklist (minimal path)

1) Power enable
   - Configure GPIO18 output with pulldown.
   - Active-low: set level 0 to enable.
   - Delay 200 ms.
2) Warmup delay
   - Add explicit 1000 ms delay before `esp_camera_init()`.
3) Initialize camera
   - `esp_camera_init(&camera_config)` with PSRAM buffers.
4) Optional SCCB probe (post-XCLK)
   - Probe known addresses after `esp_camera_init()`.
   - Use existing I2C driver; do not reinstall.
5) Capture frames
   - `esp_camera_fb_get()` -> process -> `esp_camera_fb_return()`.

### Camera init pseudocode

```c
camera_config_t cfg = {
  .pin_pwdn = -1,
  .pin_reset = -1,
  .pin_xclk = 21,
  .pin_sccb_sda = 12,
  .pin_sccb_scl = 9,
  .pin_d7 = 13, .pin_d6 = 11, .pin_d5 = 17, .pin_d4 = 4,
  .pin_d3 = 48, .pin_d2 = 46, .pin_d1 = 42, .pin_d0 = 3,
  .pin_vsync = 10, .pin_href = 14, .pin_pclk = 40,
  .xclk_freq_hz = 20000000,
  .ledc_timer = LEDC_TIMER_0,
  .ledc_channel = LEDC_CHANNEL_0,
  .pixel_format = PIXFORMAT_RGB565,
  .frame_size = FRAMESIZE_QVGA,
  .jpeg_quality = 14,
  .fb_count = 2,
  .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
  .fb_location = CAMERA_FB_IN_PSRAM,
  .sccb_i2c_port = 1,
};

power_enable_gpio18_active_low();
delay_ms(200);
delay_ms(1000); // warmup

esp_err_t err = esp_camera_init(&cfg);
if (err != ESP_OK) {
  // handle error
}
```

### Optional SCCB probe (post-XCLK)

Use known addresses and the existing driver (no install):

```c
static const uint8_t known_addrs[] = { 0x21, 0x30, 0x3C, 0x2A, 0x6E, 0x68 };
for each addr in known_addrs:
  try i2c write (addr << 1) | WRITE with ACK check
  if ESP_OK -> log ack and stop
```

Notes:
- This probe is only valid after XCLK is running (post-`esp_camera_init()`).
- Do not call `i2c_driver_install()` again; the camera driver already owns the port.

### Frame acquisition lifecycle

```c
camera_fb_t *fb = esp_camera_fb_get();
if (!fb) {
  // capture failed
} else {
  // use fb->buf, fb->len, fb->width, fb->height, fb->format
  esp_camera_fb_return(fb);
}
```

Rules:
- Always return the buffer with `esp_camera_fb_return()`.
- Use PSRAM buffers for QVGA+ frames.
- If frames stall, confirm all buffers are returned.

### Required sdkconfig values (baseline)

```
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y
# CONFIG_ESP_CONSOLE_UART is not set
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG_ENABLED=y
CONFIG_ESP_CONSOLE_UART_NUM=-1

CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCT=y
CONFIG_SPIRAM_TYPE_AUTO=y
CONFIG_SPIRAM_CLK_IO=30
CONFIG_SPIRAM_CS_IO=26
CONFIG_SPIRAM_SPEED_80M=y
CONFIG_SPIRAM_SPEED=80
CONFIG_SPIRAM_BOOT_INIT=y
CONFIG_SPIRAM_USE_MALLOC=y
CONFIG_SPIRAM_MEMTEST=y
CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=6384
CONFIG_SPIRAM_MALLOC_RESERVE_INTERNAL=32768

CONFIG_CAMERA_MODULE_M5STACK_ATOMS3R_CAM=y
CONFIG_CAMERA_XCLK_FREQ=20000000
CONFIG_SCCB_HARDWARE_I2C_PORT1=y
CONFIG_SCCB_CLK_FREQ=100000
CONFIG_CAMERA_TASK_STACK_SIZE=2048
CONFIG_CAMERA_CORE0=y
CONFIG_CAMERA_DMA_BUFFER_SIZE_MAX=32768
CONFIG_CAMERA_JPEG_MODE_FRAME_SIZE_AUTO=y

CONFIG_ATOMS3R_CAMERA_POWER_GPIO=18
CONFIG_ATOMS3R_CAMERA_POWER_ACTIVE_LOW=y
```

## Usage Examples

### Boot + capture loop (minimal)

```c
power_enable_gpio18_active_low();
vTaskDelay(pdMS_TO_TICKS(200));
vTaskDelay(pdMS_TO_TICKS(1000));

ESP_ERROR_CHECK(esp_camera_init(&cfg));

while (true) {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    ESP_LOGE(TAG, "esp_camera_fb_get failed");
    vTaskDelay(pdMS_TO_TICKS(1000));
    continue;
  }
  ESP_LOGI(TAG, "frame len=%u %ux%u fmt=%d", fb->len, fb->width, fb->height, fb->format);
  esp_camera_fb_return(fb);
}
```

### Optional post-XCLK SCCB probe

```c
if (esp_camera_init(&cfg) == ESP_OK) {
  camera_sccb_probe_known_addrs();
}
```

## Troubleshooting and Investigation Approach

### Symptom: `ESP_ERR_NOT_FOUND` in `esp_camera_init()`
- Likely causes:
  - Sensor not ready yet (insufficient warmup delay).
  - Power gate polarity incorrect.
  - XCLK not running.
  - SCCB port mismatch.
- Actions:
  1) Add explicit warmup delay (1000 ms) after power enable.
  2) Confirm GPIO18 active-low with a polarity sweep (debug-only).
  3) Verify XCLK pin and frequency (20 MHz).
  4) Ensure `CONFIG_SCCB_HARDWARE_I2C_PORT1=y`.

### Symptom: SCCB scan shows "no devices"
- If scan runs before XCLK, false negatives are expected.
- Use a post-XCLK, known-address probe instead.

### Symptom: `i2c_driver_install failed`
- Cause: the driver is already installed by esp32-camera.
- Fix: do not reinstall; reuse the existing I2C driver.

### Symptom: frame buffer allocation failure
- Cause: PSRAM disabled or misconfigured.
- Fix: ensure PSRAM config matches hardware; confirm `CONFIG_SPIRAM=y`.

### Symptom: port busy during flashing
- Cause: monitor holds `/dev/ttyACM0`.
- Fix: flash first, then start monitor.

### General debugging posture
- Favor explicit delays over incidental timing.
- Keep step logs and use `STEP:` markers for correlation.
- Change one variable per experiment and log the result.

## Related

- `ttmp/2026/01/14/MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO--atoms3r-cam-userdemo-usb-serial-jtag-console/analysis/01-camera-init-analysis-userdemo-vs-0041.md`
- `ttmp/2026/01/14/MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO--atoms3r-cam-userdemo-usb-serial-jtag-console/reference/03-diary-0041-debugging.md`
