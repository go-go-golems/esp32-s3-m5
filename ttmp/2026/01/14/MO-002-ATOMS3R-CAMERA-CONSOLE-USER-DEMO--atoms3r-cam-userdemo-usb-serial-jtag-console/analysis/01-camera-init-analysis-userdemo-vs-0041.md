---
Title: Camera Init Analysis — UserDemo vs 0041
Ticket: MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO
Status: active
Topics:
    - camera
    - esp32
    - firmware
    - usb
    - console
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/main/hal_config.h
      Note: UserDemo peripheral pin inventory and conflicts
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/main/service/service_uvc.cpp
      Note: UVC start callback reinitializes camera
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/main/usb_webcam_main.cpp
      Note: UserDemo init order and hardware bring-up
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/main/utils/camera/camera_init.c
      Note: UserDemo camera_config_t setup and sensor tweaks
    - Path: 0041-atoms3r-cam-jtag-serial-test/main/camera_pin.h
      Note: Camera pin map used by 0041 (matches UserDemo)
    - Path: 0041-atoms3r-cam-jtag-serial-test/main/main.c
      Note: 0041 init phases
    - Path: 0041-atoms3r-cam-jtag-serial-test/sdkconfig.defaults
      Note: 0041 console
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-14T18:53:21.311480709-05:00
WhatFor: ""
WhenToUse: ""
---


# Camera Init Analysis: UserDemo vs 0041

## Scope

This is a deep audit of how the working `ATOMS3R-CAM-UserDemo` firmware initializes the camera and how the `0041-atoms3r-cam-jtag-serial-test` firmware initializes the same camera. USB UVC and WiFi are not desired in the new firmware, but any camera-touching behavior inside those subsystems is still documented because it can change camera configuration or timing.

## Background: how the ESP32-S3 camera stack works (textbook-style)

The ESP32-S3 camera pipeline is a small data-acquisition system composed of four parts: the image sensor, the XCLK generator, the SCCB control bus, and the parallel pixel bus (D0-D7 + sync signals). The firmware drives XCLK to the sensor, uses SCCB (I2C-like) to configure sensor registers, and then captures pixel data with the LCD_CAM peripheral and DMA into frame buffers. The `esp32-camera` component hides most of this wiring behind `esp_camera_init()` and the `esp_camera_fb_get()` / `esp_camera_fb_return()` API pair.

At a high level, the boot sequence is: power the sensor, enable XCLK, initialize SCCB, probe the sensor ID, configure sensor registers (resolution, format, timing), configure the capture engine (DMA and frame buffers), and then start capturing frames. If any one of these steps is skipped or misconfigured (wrong GPIO, wrong SCCB port, no XCLK, no PSRAM), the sensor may respond on SCCB but frame capture will fail or return garbage.

### Signals and buses in plain language

The camera needs three kinds of signals:

- **Control bus (SCCB):** an I2C-like bus used to read and write sensor registers. On ESP32 this is implemented by the I2C peripheral. The camera driver initializes SCCB using `pin_sccb_sda` and `pin_sccb_scl`. The bus speed is `CONFIG_SCCB_CLK_FREQ` (100 kHz here).
- **Clock (XCLK):** a continuous clock provided by the ESP32 LEDC peripheral. The sensor uses XCLK to time its internal pixel pipeline. If XCLK is missing or the frequency is wrong, the sensor may not stream.
- **Pixel data + sync:** the sensor outputs an 8-bit parallel bus (D0-D7) plus `PCLK`, `VSYNC`, and `HREF`. The ESP32-S3 LCD_CAM peripheral captures this stream and DMA writes frames into memory buffers.

### Why PSRAM matters

Camera frames are large. A QVGA RGB565 frame is 320 * 240 * 2 bytes = 153,600 bytes. Larger frames can exceed the internal DRAM budget quickly. The camera driver therefore supports two frame buffer locations:

- `CAMERA_FB_IN_PSRAM`: store frame buffers in external PSRAM (requires `CONFIG_SPIRAM=y`).
- `CAMERA_FB_IN_DRAM`: store buffers in internal DRAM (fast but limited).

If `CAMERA_FB_IN_PSRAM` is selected while `CONFIG_SPIRAM` is disabled, buffer allocation may fail, or it may silently fall back to DRAM depending on the driver version. This is why the UserDemo config mismatch is a serious risk even though the firmware appears to work.

### What `esp_camera_init()` actually does

The `esp_camera_init()` call is a multi-step procedure:

1) **Allocate camera state:** the driver creates a state struct and validates inputs.
2) **Enable XCLK:** LEDC is configured with `xclk_freq_hz`, `ledc_timer`, and `ledc_channel`.
3) **Initialize SCCB:** I2C is configured on `pin_sccb_sda` / `pin_sccb_scl` and the SCCB port is selected.
4) **Probe sensor:** SCCB is used to read sensor ID registers; a supported sensor is selected.
5) **Configure capture pipeline:** DMA buffers, LCD_CAM, and frame buffers are set up.
6) **Configure sensor registers:** resolution, pixel format, JPEG quality, and other defaults are applied.
7) **Return ready state:** `esp_camera_fb_get()` can now return frames.

The driver will also toggle PWDN/RESET lines if they are provided, but in AtomS3R-CAM these pins are `-1` so the driver does not power-cycle the sensor.

### Frame capture lifecycle (`esp_camera_fb_get()` / `esp_camera_fb_return()`)

The capture API is intentionally simple, but it depends on strict ownership rules:

- `esp_camera_fb_get()` returns a pointer to a frame buffer owned by the camera driver.
- The caller must return the same pointer using `esp_camera_fb_return()` when done.
- If buffers are not returned, the driver will eventually stall because all buffers are in use.

This is the typical usage pattern:

```
camera_fb_t *fb = esp_camera_fb_get();
if (fb == NULL) {
    // handle failure
} else {
    // use fb->buf, fb->len, fb->width, fb->height, fb->format
    esp_camera_fb_return(fb);
}
```

In UserDemo, the UVC service and web server take frames, optionally convert them (RGB565 to JPEG), and then return the camera frame to keep the pipeline flowing.

### UVC and WiFi services are not just transport

The UVC and web server layers are not passive. They can reinitialize the camera or mutate sensor settings:

- The UVC `camera_start_cb()` calls `my_camera_init()` with new frame sizes and JPEG quality based on the host request.
- The web server API can call sensor setters such as `set_framesize`, `set_quality`, `set_hmirror`, and `set_vflip`.

This means "camera configuration" is not a single point-in-time event; it is a system that can be reconfigured at runtime by higher layers.

### Key APIs (reference list)

Camera init and capture:
- `esp_camera_init(const camera_config_t *config)`
- `esp_camera_deinit()`
- `esp_camera_fb_get()`
- `esp_camera_fb_return(camera_fb_t *fb)`
- `esp_camera_sensor_get()`

Sensor configuration (subset from `sensor_t`):
- `set_framesize(sensor_t *s, framesize_t size)`
- `set_pixformat(sensor_t *s, pixformat_t fmt)`
- `set_quality(sensor_t *s, int quality)`
- `set_hmirror(sensor_t *s, int enable)`
- `set_vflip(sensor_t *s, int enable)`

SCCB (I2C-like) operations (driver-internal, referenced for context):
- `SCCB_Init(int pin_sda, int pin_scl)`
- `SCCB_Use_Port(int i2c_num)`
- `SCCB_Probe()`

## Shared camera pin map (M5STACK AtomS3R-CAM)

Both projects use identical `camera_pin.h` definitions (confirmed identical). Camera PWDN and RESET are not wired (both `-1`), so only the external power GPIO controls power. The camera driver will not toggle PWDN/RESET.

| Signal | GPIO | Notes |
| --- | --- | --- |
| PWDN | -1 | Not wired; driver does not power-cycle camera |
| RESET | -1 | Not wired; driver does not reset sensor |
| XCLK | 21 | Driven by LEDC timer 0, channel 0 |
| SIOD (SDA) | 12 | SCCB data |
| SIOC (SCL) | 9 | SCCB clock |
| D0 | 3 | Camera data |
| D1 | 42 | Camera data |
| D2 | 46 | Camera data |
| D3 | 48 | Camera data |
| D4 | 4 | Camera data |
| D5 | 17 | Camera data |
| D6 | 11 | Camera data |
| D7 | 13 | Camera data |
| VSYNC | 10 | Frame sync |
| HREF | 14 | Line sync |
| PCLK | 40 | Pixel clock |

SCCB/I2C port is configured at build time (`CONFIG_SCCB_HARDWARE_I2C_PORT1=y`), so SCCB defaults to I2C port 1 when SCCB is initialized by the camera driver.

## UserDemo: peripheral and pin inventory (non-camera)

From `main/hal_config.h`:

| Peripheral | Signal | GPIO | Notes |
| --- | --- | --- | --- |
| LCD | MOSI | 21 | Shares with camera XCLK if LCD used |
| LCD | SCLK | 15 | |
| LCD | DC | 42 | Shares with camera D1 if LCD used |
| LCD | CS | 14 | Shares with camera HREF if LCD used |
| LCD | RST | 48 | Shares with camera D3 if LCD used |
| I2C internal | SDA | 45 | `m5::In_I2C` on I2C_NUM_0 |
| I2C internal | SCL | 0 | `m5::In_I2C` on I2C_NUM_0 |
| I2C external | SDA | 2 | `m5::Ex_I2C` on I2C_NUM_1 (commented out) |
| I2C external | SCL | 1 | `m5::Ex_I2C` on I2C_NUM_1 (commented out) |
| Button A | GPIO | 8 | |
| IMU INT | GPIO | 16 | Hardware pull-up |
| IR TX | GPIO | 47 | NEC IR transmitter |

Note: The LCD pins conflict with camera data and sync pins (D1, D3, HREF, XCLK). In the current UserDemo flow, LCD is not initialized for AtomS3R-CAM, so those pins are effectively camera-only, but any future LCD bring-up would collide.

## UserDemo: initialization phases (order as executed in `app_main`)

### Phase 0: logging + shared state
- `spdlog::set_pattern` sets log format.
- `shared_data_injection()` allocates `SharedData_StdMutex` and sets service mode to `mode_none`.
- `asset_pool_injection()` calls `nvs_flash_init()`, then maps a custom asset partition into address space (2 MiB) and injects assets. This affects memory use but not camera bring-up directly.

### Phase 1: camera power enable (GPIO 18)
- `enable_camera_power()`:
  - `gpio_reset_pin(18)`
  - `gpio_set_direction(18, GPIO_MODE_OUTPUT)`
  - `gpio_set_pull_mode(18, GPIO_PULLDOWN_ONLY)`
  - No explicit `gpio_set_level()` call; the pin stays low by default and has a pulldown.
  - `vTaskDelay(200 ms)` before continuing.
- Implication: camera power enable relies on the default low level. If the power gate is active-low (as in 0041), this enables power; if active-high, power would stay off.

### Phase 2: internal I2C bus init and scan (I2C0)
- `i2c_init()`:
  - `m5::In_I2C.begin(I2C_NUM_0, SDA=45, SCL=0)`
  - `i2c_scan()` scans addresses 0x08-0x77 and logs detected devices.
  - External I2C1 bus (SDA=2, SCL=1) is present but commented out.
- This I2C bus is separate from the camera SCCB bus, which uses GPIO 12/9 and I2C port 1.

### Phase 3: IMU init (BMI270 + BMM150)
- `imu_init()` allocates `BMI270_Class` on the internal I2C bus and calls `init()` and `initAuxBmm150()`.
- Errors are logged, but the firmware continues.

### Phase 4: IR transmitter init
- `ir_init()` calls `ir_nec_transceiver_init(HAL_PIN_IR_TX)` with GPIO 47.

### Phase 5: camera init (static call)
- `camera_init()` calls `my_camera_init(CONFIG_CAMERA_XCLK_FREQ, PIXFORMAT_RGB565, FRAMESIZE_QVGA, 14, 2)`.
- This is a direct initialization before UVC/WiFi services are started.

### Phase 6: services start (UVC + WiFi)
- `start_service_uvc()` configures USB UVC; its callbacks can reinitialize the camera on host requests (see below).
- `start_service_web_server()` starts WiFi SoftAP and HTTP server; camera APIs use `esp_camera_fb_get()` and can adjust sensor settings.

## Step-by-step comparison: UserDemo vs 0041 (old vs new)

This section mirrors the boot sequence of the working UserDemo firmware and places the 0041 firmware next to each step. The goal is to make it obvious where behaviors diverge, even when the end result looks similar.

### Step 0: logging and shared state

**UserDemo (working):** uses `spdlog::set_pattern`, injects shared data, and maps the asset partition via `nvs_flash_init()` before any hardware init. This does not directly touch the camera but does consume memory early.

**0041 (new, failing):** logs basic chip info via `esp_chip_info()` and does not allocate shared state or assets.

**Implication:** not camera-specific, but memory and task scheduling are slightly different. If a memory edge case exists, it could surface here.

**Pseudo-compare:**
```
UserDemo:
  set_log_pattern()
  SharedData::Inject(...)
  nvs_flash_init()
  map_asset_partition()

0041:
  esp_chip_info()
  log_chip()
```

### Step 1: camera power enable (GPIO 18)

**UserDemo:** resets GPIO 18, sets it as output with pulldown, and waits 200 ms. No explicit output level is set, so the default level (low) is relied upon.

**0041:** resets GPIO 18, sets it as output with pulldown, explicitly sets level based on `CONFIG_ATOMS3R_CAMERA_POWER_ACTIVE_LOW`, and waits 10 ms.

**Implication:** this is a prime suspect. UserDemo implicitly assumes power enable is the default low level. 0041 explicitly drives a level based on config. If the polarity is wrong, power never comes up.

**Pseudo-compare:**
```
UserDemo:
  gpio_reset_pin(18)
  gpio_set_direction(18, OUTPUT)
  gpio_set_pull_mode(18, PULLDOWN)
  delay(200ms)

0041:
  gpio_reset_pin(18)
  gpio_set_direction(18, OUTPUT)
  gpio_set_pull_mode(18, PULLDOWN)
  gpio_set_level(18, active_low ? 0 : 1)   // enable
  delay(10ms)
```

**Expected observation when power is correct:**
- SCCB scan should find the sensor address (typically 0x30 or similar).
- `esp_camera_init()` should pass SCCB probe.

### Step 2: I2C/SCCB handling

**UserDemo:** initializes internal I2C0 (SDA 45, SCL 0) and scans devices. This is not the camera SCCB bus. SCCB is initialized later by `esp_camera_init()` using camera pins (SDA 12, SCL 9) and I2C port 1 defaults.

**0041:** performs an SCCB scan directly on the camera pins (SDA 12, SCL 9), using I2C port 1, then deletes the I2C driver if it installed it.

**Implication:** 0041 does more with the camera bus before `esp_camera_init()`. That should be safe but it changes timing and ownership of the I2C driver. If the SCCB scan leaves the bus in a bad state, `esp_camera_init()` can fail.

**Pseudo-compare:**
```
UserDemo:
  In_I2C.begin(I2C0, SDA=45, SCL=0)
  scan(I2C0)
  // later: esp_camera_init() -> SCCB_Init(SDA=12, SCL=9, port=1)

0041:
  i2c_param_config(I2C1, SDA=12, SCL=9, 100kHz)
  i2c_driver_install(I2C1)
  scan(I2C1)
  i2c_driver_delete(I2C1)
  // later: esp_camera_init() -> SCCB_Init(SDA=12, SCL=9, port=1)
```

**What to log for verification:**
- SCCB scan results (expected: at least one device at sensor address).
- `esp_camera_init()` return code after scan.

### Step 3: camera init call

**UserDemo:** calls `my_camera_init()` with fixed parameters (RGB565, QVGA, fb_count=2) before starting services.

**0041:** calls `camera_init_and_log()` with the same parameters, but includes additional logging and explicit `sccb_i2c_port`.

**Implication:** the core configuration is the same. Differences are logging, SCCB scan ordering, and PSRAM setup.

**Config equivalence check (both):**
- `xclk_freq_hz = 20000000`
- `pixel_format = PIXFORMAT_RGB565`
- `frame_size = FRAMESIZE_QVGA`
- `jpeg_quality = 14`
- `fb_count = 2`
- `fb_location = CAMERA_FB_IN_PSRAM`
- `grab_mode = CAMERA_GRAB_WHEN_EMPTY`

**Potential divergence inside the driver:**
- PSRAM availability (allocation success vs failure).
- XCLK enable sequence (timing relative to power enable).

### Step 4: post-init sensor configuration

**UserDemo:** applies vflip and sensor-specific tweaks for OV3660/OV2640/GC0308/GC032A.

**0041:** applies identical sensor tweaks.

**Implication:** not a likely divergence.

**Pseudo-compare:**
```
s = esp_camera_sensor_get()
s->set_vflip(1)
if OV3660: set_brightness(1), set_saturation(-2)
if OV3660 or OV2640: set_vflip(1)
if GC0308: set_hmirror(0)
if GC032A: set_vflip(1)
```

### Step 5: frame capture path

**UserDemo:** frame capture happens later and through services (UVC callbacks or web server APIs). There is no immediate capture loop in `app_main`.

**0041:** immediately loops on `esp_camera_fb_get()` once per second, logs frame metadata and a hex preview, and returns the frame.

**Implication:** 0041 stresses the capture path immediately after init; UserDemo defers it. Timing or buffer allocation issues can surface only in the immediate loop.

**Pseudo-compare:**
```
UserDemo:
  start_service_uvc()
  start_service_web_server()
  // capture only when host requests

0041:
  while (true) {
    fb = esp_camera_fb_get()
    log_frame_info(fb)
    esp_camera_fb_return(fb)
    delay(1s)
  }
```

**Failure signature differences:**
- UserDemo: failures may appear only when UVC/WebServer accesses frames.
- 0041: failures appear immediately after boot if capture is broken.

### Step 6: runtime reconfiguration

**UserDemo:** UVC start callbacks and web server APIs can reinitialize or mutate camera settings during runtime.

**0041:** no runtime mutation; the camera stays at the initial config.

**Implication:** UserDemo’s runtime paths may hide a startup defect by reinitializing with a different config when the host connects. 0041 never does this.

**Pseudo-compare:**
```
UserDemo:
  on UVC start:
    my_camera_init(new_frame_size, new_quality)
  on HTTP params:
    s->set_framesize(...)
    s->set_quality(...)

0041:
  no runtime mutation
```

## How the two firmwares differ and what to do about it

### Differences that can explain a non-working camera

1) **Power polarity and explicit level control**
   - UserDemo never sets GPIO18’s level; 0041 does.
   - If the power gate is not active-low, 0041 will keep the camera off.
   - If the power gate is active-low (likely), UserDemo’s implicit low level is sufficient, while 0041 depends on `CONFIG_ATOMS3R_CAMERA_POWER_ACTIVE_LOW`.

2) **PSRAM enablement and buffer location**
   - UserDemo config sets `CAMERA_FB_IN_PSRAM` but `CONFIG_SPIRAM` is disabled.
   - 0041 enables PSRAM explicitly and relies on PSRAM for buffers.
   - A PSRAM hardware or config mismatch can break 0041 while UserDemo still appears to work.
   - If PSRAM fails to init on 0041, frame buffer allocation can fail and `esp_camera_init()` will return errors (often `ESP_ERR_NO_MEM`).

3) **SCCB pre-scan timing and I2C ownership**
   - 0041 installs and deletes an I2C driver on the camera pins before `esp_camera_init()`.
   - UserDemo leaves SCCB init entirely to the camera driver.
   - If the SCCB scan leaves the bus or pins in a bad state, 0041 will fail during probe.

4) **Immediate capture loop vs deferred capture**
   - 0041 starts `esp_camera_fb_get()` immediately.
   - UserDemo doesn’t pull frames until UVC/web server uses them.
   - If the camera needs a stabilization window after power-up, 0041 can fail while UserDemo succeeds.

5) **Console differences**
   - UserDemo uses UART0; 0041 uses USB Serial/JTAG.
   - Not directly camera-related, but timing and logging behavior are different.

6) **Initialization order differences**
   - UserDemo does longer power delay (200 ms) before touching the camera.
   - 0041 uses 10 ms before SCCB scan and init.
   - If the sensor needs more time after power-up, 0041 can fail SCCB probe even when everything else is correct.

### What to do about it (actionable checks)

1) **Power sanity check**
   - Force GPIO18 low and high explicitly, with logs before and after.
   - Run SCCB scan after each state.
   - Expected: one polarity yields a valid SCCB address and sensor ID.

2) **SCCB isolation check**
   - Disable `camera_sccb_scan()` entirely in 0041.
   - If `esp_camera_init()` starts working, the scan is interfering.
   - Alternative: keep scan but do not delete the driver before init.

3) **PSRAM check**
   - Log `esp_psram_is_initialized()` and `esp_psram_get_size()` before init.
   - If PSRAM is absent, force `CAMERA_FB_IN_DRAM` and reduce frame size to QVGA or lower.

4) **Timing check**
   - Increase delay after power enable to 200-300 ms, matching UserDemo.
   - Increase SCCB probe timeout if needed (driver-level change).

5) **Capture check**
   - Replace the loop with a single capture after a long delay.
   - If that works, reintroduce the loop and observe when it fails.

6) **Expected log checklist**
   - SCCB scan: "found device at 0x.."
   - `esp_camera_init`: `ESP_OK`
   - Sensor ID: `PID`, `MIDH`, `MIDL`
   - First frame: `len`, `width`, `height`, `format`

## Debugging plan / guidebook for 0041 bring-up

This plan is a staged debugging guide intended to bring the 0041 firmware to parity with the working UserDemo camera path. Each step includes references to the analysis above, the files and functions to touch, the expected result, and a strict workflow: flash in tmux, capture logs, commit after each step, and update the debugging diary.

### Debugging workflow (required)

1) **Use tmux for flashing + monitoring**
   - Create a tmux session with two panes: one for `idf.py flash` and one for `idf.py monitor`.
   - Example:
     ```
     tmux new -s atoms3r
     # Pane 1
     idf.py -p /dev/ttyACM0 flash
     # Split and attach monitor
     tmux split-window -h
     idf.py -p /dev/ttyACM0 monitor
     ```
   - Replace `/dev/ttyACM0` with the actual USB Serial/JTAG device.

2) **Commit after each step**
   - Example:
     ```
     git add 0041-atoms3r-cam-jtag-serial-test/main/main.c
     git commit -m "0041: debug step N - <short description>"
     ```
   - If docs are updated in the step, commit them separately.

3) **Keep a detailed debugging diary**
   - Update `ttmp/.../reference/03-diary-0041-debugging.md` after each step.
   - Include logs, exact commands, and observed behavior.

### Step 1: Validate power polarity and power timing

**Goal:** confirm the camera power gate is enabled in 0041 and allow sufficient stabilization time.

**References:** "Step-by-step comparison" Step 1; "Differences" item 1; "Initialization order differences" item 6.

**Files/functions:**
- File: `0041-atoms3r-cam-jtag-serial-test/main/main.c`
- Functions: `camera_power_set`, `camera_sccb_scan`, `app_main`

**Actions:**
- Temporarily add a forced GPIO18 level toggle sequence (low -> high -> low) with logs.
- Increase the post-power delay to 200-300 ms before SCCB scan and `esp_camera_init()`.
- Run SCCB scan after each polarity state to see if any device appears.

**Expected result:**
- Exactly one polarity yields a consistent SCCB device address.
- `esp_camera_init()` returns `ESP_OK` after the correct polarity and delay.

**Commit:** `0041: debug step 1 - power polarity + delay`

### Step 2: Remove SCCB pre-scan side effects

**Goal:** determine if the SCCB scan is interfering with the driver’s SCCB initialization.

**References:** "Step-by-step comparison" Step 2; "Differences" item 3.

**Files/functions:**
- File: `0041-atoms3r-cam-jtag-serial-test/main/main.c`
- Functions: `camera_sccb_scan`, `camera_init_and_log`

**Actions:**
- Disable `camera_sccb_scan()` entirely, or move it after a successful `esp_camera_init()`.
- Alternatively, keep the scan but do not delete the I2C driver before init.

**Expected result:**
- If SCCB scan was interfering, `esp_camera_init()` should start succeeding.

**Commit:** `0041: debug step 2 - SCCB scan isolation`

### Step 3: PSRAM presence and buffer fallback

**Goal:** verify PSRAM availability and prevent allocation failures.

**References:** "Differences" item 2; "Background" PSRAM subsection.

**Files/functions:**
- File: `0041-atoms3r-cam-jtag-serial-test/main/main.c`
- Functions: `psram_ready`, `camera_init_and_log`

**Actions:**
- Log PSRAM status (`esp_psram_is_initialized()`, size).
- If PSRAM is missing, switch `fb_location` to `CAMERA_FB_IN_DRAM` and reduce frame size to QVGA or QQVGA.

**Expected result:**
- `esp_camera_init()` succeeds even when PSRAM is absent.

**Commit:** `0041: debug step 3 - PSRAM fallback`

### Step 4: Capture timing and buffer health

**Goal:** rule out capture timing issues immediately after init.

**References:** "Step-by-step comparison" Step 5; "Differences" item 4.

**Files/functions:**
- File: `0041-atoms3r-cam-jtag-serial-test/main/main.c`
- Functions: `app_main` capture loop

**Actions:**
- Replace the capture loop with a single capture after a long delay (e.g., 500 ms or 1 s).
- If single capture works, reintroduce loop with a larger delay between frames.

**Expected result:**
- First frame is valid; loop stability indicates timing is not the root cause.

**Commit:** `0041: debug step 4 - capture timing`

### Step 5: Optional reinit parity test (UserDemo behavior)

**Goal:** test if a reinit with different frame size makes the camera come alive.

**References:** "Step-by-step comparison" Step 6; "UVC-driven reinit" in UserDemo sections.

**Files/functions:**
- File: `0041-atoms3r-cam-jtag-serial-test/main/main.c`
- Functions: `camera_init_and_log`

**Actions:**
- After a failed init, re-run `esp_camera_deinit()` and `esp_camera_init()` with a smaller frame size.
- Compare with UserDemo’s reinit behavior as a surrogate for UVC start.

**Expected result:**
- If reinit with smaller frames works, the failure is likely memory or timing related.

**Commit:** `0041: debug step 5 - reinit parity test`

### Step 6: Deep driver trace (only if needed)

**Goal:** validate internal SCCB probe and capture configuration in the esp32-camera driver.

**References:** "Shared driver behavior (esp32-camera component)" section.

**Files/functions:**
- File: `0041-atoms3r-cam-jtag-serial-test/components/esp32-camera/driver/esp_camera.c`
- Functions: `camera_probe`, `cam_config`, `esp_camera_init`

**Actions:**
- Add temporary logs around SCCB probe, sensor detection, and DMA buffer allocation.
- Remove logs once root cause is found.

**Expected result:**
- Explicit error codes pinpoint the failure (SCCB probe vs memory allocation vs capture config).

**Commit:** `0041: debug step 6 - driver trace logs`


## UserDemo: camera initialization details (`my_camera_init`)

### Inited cache and restart logic
- Function caches last `xclk`, `pixel_format`, `frame_size`, `jpeg_quality`, and `fb_count`.
- If params match and `inited == true`, it returns immediately.
- If params differ, it calls:
  - `esp_camera_return_all()`
  - `esp_camera_deinit()`
  - Logs "camera RESTART"

### camera_config_t fields (explicit values)
- Pins: uses `pin_sscb_sda` / `pin_sscb_scl` (deprecated alias; same storage as `pin_sccb_*`).
- Data and sync pins: from `camera_pin.h` (table above).
- `xclk_freq_hz = CONFIG_CAMERA_XCLK_FREQ` (20 MHz)
- `ledc_timer = LEDC_TIMER_0`, `ledc_channel = LEDC_CHANNEL_0`
- `pixel_format = PIXFORMAT_RGB565`
- `frame_size = FRAMESIZE_QVGA` (initial call; later UVC can change)
- `jpeg_quality = 14`
- `fb_count = 2`
- `grab_mode = CAMERA_GRAB_WHEN_EMPTY`
- `fb_location = CAMERA_FB_IN_PSRAM`
- `sccb_i2c_port` is not explicitly set, but SCCB uses the default port from config because SCCB is initialized with pins.

### Camera driver call
- `esp_camera_init(&camera_config)` triggers SCCB init, sensor probe, and image pipeline configuration (see "Shared driver behavior" below).

### Post-init sensor tweaks
- `set_vflip(1)` always applied.
- If sensor is OV3660: `set_brightness(1)` and `set_saturation(-2)`.
- If sensor is OV3660 or OV2640: `set_vflip(1)`.
- If sensor is GC0308: `set_hmirror(0)`.
- If sensor is GC032A: `set_vflip(1)`.

## UserDemo: UVC service path (camera reinit on host request)

`start_service_uvc()` sets up TinyUSB UVC and installs callbacks:

- `camera_start_cb(format, width, height, rate)`:
  - Validates MJPEG format.
  - Maps UVC frame size to a `framesize_t` and `jpeg_quality`.
  - Calls `my_camera_init(CAMERA_XCLK_FREQ, PIXFORMAT_RGB565, frame_size, jpeg_quality, CAMERA_FB_COUNT)`.
  - This can reinitialize the camera with a new frame size/quality when a host starts streaming.

- `camera_fb_get_cb()`:
  - Calls `esp_camera_fb_get()`.
  - Converts RGB565 frames to JPEG with `frame2jpg()` (quality 60).
  - Enforces `UVC_MAX_FRAMESIZE_SIZE` and returns the UVC frame.

This is a second camera initialization pathway that is absent in the 0041 firmware. If the host never starts UVC streaming, the camera remains in the initial QVGA config from `app_main`.

## UserDemo: web server camera APIs

`start_service_web_server()` starts a SoftAP and HTTP server. Camera APIs in `api_camera.cpp`:
- Fetch frames via `esp_camera_fb_get()`.
- Convert frames to JPEG when needed (`frame2jpg`).
- Expose sensor status and allow sensor parameter changes (e.g., framesize, quality, hmirror, vflip).
- These APIs can mutate sensor settings after initialization.

## UserDemo: configuration highlights (sdkconfig)

Key camera and console options:
- `CONFIG_CAMERA_MODULE_M5STACK_ATOMS3R_CAM=y`
- `CONFIG_CAMERA_XCLK_FREQ=20000000`
- `CONFIG_SCCB_HARDWARE_I2C_PORT1=y`
- `CONFIG_SCCB_CLK_FREQ=100000`
- `CONFIG_CAMERA_TASK_STACK_SIZE=2048`
- `CONFIG_CAMERA_CORE0=y`
- `CONFIG_CAMERA_DMA_BUFFER_SIZE_MAX=32768`
- `CONFIG_CAMERA_JPEG_MODE_FRAME_SIZE_AUTO=y`
- `CONFIG_CAMERA_CONVERTER_ENABLED` is not set
- `CONFIG_SPIRAM` is not set (PSRAM disabled in sdkconfig)
- Console uses UART0 (`CONFIG_ESP_CONSOLE_UART=y`), USB Serial/JTAG enabled as secondary.
- UVC uses isochronous mode and frame sizes defined in `uvc_frame_config.h` via sdkconfig.

Note: Camera frame buffers are configured as `CAMERA_FB_IN_PSRAM` even though `CONFIG_SPIRAM` is disabled. The working firmware suggests PSRAM is actually available at runtime, but this is a configuration mismatch worth flagging.

## 0041: initialization phases (order as executed in `app_main`)

### Phase 0: boot log
- Reads chip info via `esp_chip_info()` and logs model/revision/cores.

### Phase 1: camera power enable (GPIO 18, active-low)
- `camera_power_set(true)`:
  - `gpio_reset_pin(18)`
  - Output mode, pulldown
  - Sets output level based on `CONFIG_ATOMS3R_CAMERA_POWER_ACTIVE_LOW`
  - `vTaskDelay(10 ms)`
  - Logs active_low and actual level

### Phase 2: SCCB scan (I2C1, camera pins)
- `camera_sccb_scan()`:
  - Uses `I2C_NUM_1` when `CONFIG_SCCB_HARDWARE_I2C_PORT1=y`
  - Configures I2C with `CAMERA_PIN_SIOD=12` and `CAMERA_PIN_SIOC=9`, 100 kHz
  - Installs I2C driver, scans addresses 0x08-0x77, logs detected devices
  - Deletes the driver if it was installed here

### Phase 3: camera init + logging
- `camera_init_and_log()`:
  - `psram_ready()` logs PSRAM status when `CONFIG_SPIRAM=y` (does not change config).
  - Configures `camera_config_t` with same pins as UserDemo.
  - Explicitly sets `sccb_i2c_port` from config (port 1).
  - Uses `PIXFORMAT_RGB565`, `FRAMESIZE_QVGA`, `jpeg_quality=14`, `fb_count=2`, `CAMERA_FB_IN_PSRAM`.
  - Logs camera options, pins, and config before calling `esp_camera_init()`.
  - Logs sensor ID and basic info; applies the same sensor tweaks as UserDemo.

### Phase 4: frame capture loop
- In a 1-second loop:
  - Calls `esp_camera_fb_get()`
  - Logs frame len, dimensions, format, timestamp
  - Logs an 8-byte hex preview of frame data
  - Returns the frame via `esp_camera_fb_return()`

## 0041: configuration highlights (sdkconfig.defaults)

Key differences from UserDemo:
- `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y` and UART console disabled
- `CONFIG_SPIRAM=y`
- `CONFIG_CAMERA_MODULE_M5STACK_ATOMS3R_CAM=y`
- `CONFIG_CAMERA_XCLK_FREQ=20000000`
- `CONFIG_SCCB_HARDWARE_I2C_PORT1=y`
- `CONFIG_SCCB_CLK_FREQ=100000`
- `CONFIG_CAMERA_TASK_STACK_SIZE=2048`
- `CONFIG_CAMERA_CORE0=y`
- `CONFIG_CAMERA_DMA_BUFFER_SIZE_MAX=32768`
- `CONFIG_CAMERA_JPEG_MODE_FRAME_SIZE_AUTO=y`
- `CONFIG_ATOMS3R_CAMERA_POWER_GPIO=18`
- `CONFIG_ATOMS3R_CAMERA_POWER_ACTIVE_LOW=y`

## Shared driver behavior (esp32-camera component)

Both firmwares use the same `esp_camera_init()` driver behavior:
- Enables XCLK on the configured pin.
- Initializes SCCB on the configured SDA/SCL pins (port selection uses `CONFIG_SCCB_HARDWARE_I2C_PORT1`).
- Probes sensor via SCCB and reads sensor ID.
- Configures the camera pipeline and frame buffers.
- Applies sensor settings from the `sensor_t` API in each app.

## Comparison and risk flags (inspection notes)

1) **Camera power gating**
   - UserDemo sets GPIO18 as output with pulldown but never sets a level.
   - 0041 explicitly sets level with an active-low assumption.
   - If the power gate is active-high, UserDemo would never enable the camera; the fact that it works suggests active-low is correct, but this is an implicit dependency.

2) **PSRAM configuration mismatch (UserDemo)**
   - UserDemo sets `CAMERA_FB_IN_PSRAM` but `CONFIG_SPIRAM` is disabled in sdkconfig.
   - If the build truly has PSRAM disabled, camera init would likely fail or allocate in DRAM unexpectedly.
   - 0041 explicitly enables PSRAM and also logs PSRAM status.

3) **SCCB pre-scan**
   - 0041 performs an SCCB scan before `esp_camera_init()`.
   - This is a useful diagnostic step, but it adds I2C driver install/delete and a timing difference vs UserDemo.

4) **UVC-driven reinit (UserDemo)**
   - UVC start callback can reinitialize the camera with new frame sizes and JPEG quality values.
   - 0041 never reinitializes after startup; it remains fixed at QVGA unless changed manually.

5) **WiFi camera API mutations (UserDemo)**
   - Web server APIs can change sensor settings at runtime (framesize, quality, vflip, hmirror).
   - 0041 does not expose any runtime control path.

6) **Console path differences**
   - UserDemo uses UART0 for console output (USB Serial/JTAG is secondary).
   - 0041 uses USB Serial/JTAG only, matching the AGENTS requirement and avoiding UART pin conflicts.

## Camera init checkpoints (summary checklist)

For a working camera bring-up, both firmwares depend on:
1) Power to the camera module (GPIO18, active-low expected).
2) XCLK at 20 MHz on GPIO21 (LEDC timer 0 / channel 0).
3) SCCB traffic on GPIO12/9 using I2C port 1 at 100 kHz.
4) Correct camera pin map from `camera_pin.h`.
5) Sufficient memory for frame buffers (PSRAM enabled, or an alternate fb_location).
6) The sensor-specific tweaks (vflip, brightness/saturation) do not fail.
