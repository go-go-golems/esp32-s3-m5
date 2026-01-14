---
Title: Stock ATOMS3R-CAM UserDemo Firmware Analysis
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
      Note: Camera capture and MJPEG streaming endpoints
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/main/service/service_uvc.cpp
      Note: USB UVC camera streaming pipeline
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/main/service/service_web_server.cpp
      Note: WiFi SoftAP and HTTP server setup
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/main/usb_webcam_main.cpp
      Note: Boot sequence and service startup
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/main/utils/camera/camera_init.c
      Note: Camera init logic and sensor configuration
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/main/utils/camera/camera_pin.h
      Note: Camera pin definitions for AtomS3R-CAM
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/partitions.csv
      Note: Asset pool partition layout
ExternalSources: []
Summary: Deep analysis of the stock ATOMS3R-CAM UserDemo firmware, focusing on WiFi AP mode and camera data paths.
LastUpdated: 2026-01-14T08:47:02.385420982-05:00
WhatFor: Reference for implementing websocket streaming and understanding the existing camera/WiFi pipeline.
WhenToUse: When debugging or extending the stock firmware's networking and camera behavior.
---


# Stock ATOMS3R-CAM UserDemo Firmware Analysis

This document analyzes the **stock** ATOMS3R-CAM UserDemo firmware in `../ATOMS3R-CAM-UserDemo` (not the M12 variant). It is written as a textbook-style walkthrough with concrete file references, API details, and diagrams, emphasizing **WiFi setup** and **camera data acquisition**.

The firmware is built on ESP-IDF (README calls out v5.1.4) with Arduino components embedded in the IDF app (see `main/service/service_web_server.cpp` and `initArduino()` usage). The camera stack uses `esp_camera` and a USB UVC device component for wired streaming, while the web stack uses `AsyncTCP` + `ESPAsyncWebServer`.

## 1. Scope and sources

Primary source files and documents used for this analysis:

- `ATOMS3R-CAM-UserDemo/main/usb_webcam_main.cpp` (entry point, boot sequence)
- `ATOMS3R-CAM-UserDemo/main/service/service_web_server.cpp` (WiFi + HTTP server)
- `ATOMS3R-CAM-UserDemo/main/service/apis/api_camera.cpp` (camera capture + MJPEG)
- `ATOMS3R-CAM-UserDemo/main/service/service_uvc.cpp` (USB UVC streaming)
- `ATOMS3R-CAM-UserDemo/main/utils/camera/camera_init.c` (camera init + sensor setup)
- `ATOMS3R-CAM-UserDemo/main/utils/camera/camera_pin.h` (pin maps)
- `ATOMS3R-CAM-UserDemo/partitions.csv` (asset pool partition)
- `ATOMS3R-CAM-UserDemo/PINOUT.md` (board-level pin mapping)
- `ATOMS3R-CAM-UserDemo/sdkconfig.old` (camera module + console settings)

## 2. High-level architecture

At runtime, the firmware boots, initializes hardware, then starts two main services in parallel:

- **USB UVC service**: streams camera frames over USB as MJPEG.
- **Web server**: runs a WiFi SoftAP and serves HTTP endpoints for camera capture and streaming, plus IMU and IR APIs.

High-level block diagram:

```
             +----------------------+
             |      app_main        |
             +----------+-----------+
                        |
        +---------------+------------------+
        |                                  |
        v                                  v
  start_service_uvc()                start_service_web_server()
  (USB UVC)                           (WiFi SoftAP + HTTP)
        |                                  |
        v                                  v
  esp_camera_fb_get()                esp_camera_fb_get()
        |                                  |
        +--------------+-------------------+
                       |
                       v
                  Camera sensor
```

## 3. Boot sequence (entry point)

The firmware entry point is `app_main` in `main/usb_webcam_main.cpp`. It injects shared state, maps the asset pool, initializes hardware, and starts services.

Pseudocode:

```text
app_main():
  spdlog::set_pattern(...)
  SharedData::Inject(new SharedData_StdMutex)
  AssetPool::InjectStaticAsset(partition_mmap("assetpool"))

  enable_camera_power()   // GPIO18 pulldown
  i2c_init()              // scan internal I2C
  imu_init()              // BMI270 + BMM150
  ir_init()               // IR NEC TX
  camera_init()           // my_camera_init(QVGA, RGB565)

  start_service_uvc()
  start_service_web_server()

  loop:
    sleep(1s)
    cleanup_imu_ws_client()
```

Key boot operations:

- **Asset pool**: `asset_pool_injection()` maps a custom partition (type `233`, subtype `0x23`) and injects static assets into `AssetPool`. This is how `index.html.gz` is served.
- **Camera power**: `enable_camera_power()` configures GPIO18 as output with pulldown, then delays 200 ms.
- **Camera init**: `camera_init()` uses `my_camera_init()` with `PIXFORMAT_RGB565` and `FRAMESIZE_QVGA`.

## 4. WiFi setup (SoftAP only)

WiFi is configured exclusively as a SoftAP in `main/service/service_web_server.cpp`:

```cpp
String ap_ssid = "AtomS3R-CAM-WiFi";
WiFi.softAP(ap_ssid, emptyString, 1, 0, 1, false);
```

Behavior summary:

- **Mode**: SoftAP only (no STA mode, no NVS-backed credentials).
- **SSID**: `AtomS3R-CAM-WiFi`.
- **Security**: open network (`emptyString` password).
- **Channel**: `1`.
- **Hidden**: `0` (broadcast SSID).
- **Max connections**: `1`.
- **FTM responder**: `false`.

The AP IP address is logged via `WiFi.softAPIP()`. There is no additional DHCP or captive portal logic; clients are expected to connect directly to this AP and then use HTTP endpoints.

## 5. Web server and static UI

The HTTP server is `ESPAsyncWebServer` on port 80:

- `initArduino()` is called first to bring up Arduino runtime and WiFi.
- `AsyncWebServer` handles all routes asynchronously.
- `/` serves a gzip-compressed HTML page from the asset pool.

Route setup (see `service_web_server.cpp`):

```cpp
web_server->on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
  AsyncWebServerResponse* response =
    request->beginResponse_P(200, "text/html",
      AssetPool::GetImage().index_html_gz, 234419);
  response->addHeader("Content-Encoding", "gzip");
  request->send(response);
});
```

Notes:

- `AssetPool::GetImage().index_html_gz` is loaded from the `assetpool` partition at boot.
- The asset pool is created by `asset_pool_gen` and flashed separately (see README and scripts).

## 6. Camera subsystem

### 6.1 Camera pin mapping

The AtomS3R-CAM pin map is selected via `CONFIG_CAMERA_MODULE_M5STACK_ATOMS3R_CAM` and defined in `main/utils/camera/camera_pin.h`. It aligns with the board pinout in `PINOUT.md`.

Important pins (AtomS3R-CAM):

- XCLK: GPIO21
- SIOD/SIOC: GPIO12 / GPIO9
- VSYNC/HREF/PCLK: GPIO10 / GPIO14 / GPIO40
- D0-D7: GPIO3, 42, 46, 48, 4, 17, 11, 13

### 6.2 Camera initialization (`my_camera_init`)

`main/utils/camera/camera_init.c` wraps `esp_camera_init` and caches configuration:

- Re-init is skipped if all parameters match the previous call.
- If parameters change, the camera is deinitialized and reinitialized.
- The default config:
  - `PIXFORMAT_RGB565`
  - `FRAMESIZE_QVGA` (initial boot)
  - `fb_location = CAMERA_FB_IN_PSRAM`
  - `grab_mode = CAMERA_GRAB_WHEN_EMPTY`

Sensor adjustments are applied after init:

- `set_vflip` is applied by default.
- For `GC0308`, `set_hmirror(s, 0)` (no horizontal mirror).

### 6.3 Camera power control

`enable_camera_power()` configures GPIO18 as an output with pulldown. The function logs "enable camera power" and waits 200 ms, but does not explicitly drive the pin high or low. This implies the board likely uses an external pull or default state to enable the camera rail.

## 7. Camera data acquisition over WiFi

There are two HTTP camera endpoints in `main/service/apis/api_camera.cpp`:

1. **Single JPEG**: `GET /api/v1/capture`
2. **MJPEG stream**: `GET /api/v1/stream`

### 7.1 Single capture: `/api/v1/capture`

`sendJpg()` calls `esp_camera_fb_get()` and then:

- If the framebuffer is already JPEG, it returns the buffer directly.
- Otherwise, it converts RGB565 to JPEG using `frame2jpg()` with quality 80.

Response behavior:

- Uses `AsyncFrameResponse` (zero-copy) when JPEG is already available.
- Uses `AsyncBufferResponse` when conversion is required.
- Adds `Access-Control-Allow-Origin: *`.

### 7.2 MJPEG stream: `/api/v1/stream`

`streamJpg()` returns an `AsyncJpegStreamResponse`, which writes a multipart stream:

- Content type: `multipart/x-mixed-replace;boundary=...`
- Uses chunked responses (no fixed length).
- Each frame has a boundary + header + JPEG payload.

The constructor calls `SharedData::SetServiceMode(ServiceMode::mode_web_server)` so the USB UVC capture path can block itself while streaming.

Pseudocode for the stream loop:

```text
while response open:
  if need new frame:
    fb = esp_camera_fb_get()
    if fb.format != JPEG:
      jpg_buf = frame2jpg(fb, quality=80)
    write("--boundary\r\nContent-Type: image/jpeg\r\nContent-Length: N\r\n\r\n")
    write(jpg_buf)
  else:
    write(next chunk of jpg_buf)
```

## 8. USB UVC streaming pipeline

The USB webcam service is implemented in `main/service/service_uvc.cpp` using `usb_device_uvc`.

Key callbacks:

- `camera_start_cb()`: re-inits the camera for the requested UVC frame size and quality.
- `camera_fb_get_cb()`: captures a frame, converts it to JPEG with `frame2jpg(quality=60)`, and hands it to the UVC stack.
- `camera_fb_return_cb()`: returns the framebuffer to the camera driver.

The UVC stream is described by `uvc_frame_config.h` and constrained by `UVC_MAX_FRAMESIZE_SIZE` (90 KB on ESP32-S3).

UVC flow diagram:

```
Host USB UVC request
  -> uvc_device_uvc (callbacks)
     -> camera_start_cb() [reinit camera]
     -> camera_fb_get_cb()
        -> esp_camera_fb_get()
        -> frame2jpg()
        -> return uvc_fb_t
  -> Host receives MJPEG frames
```

Concurrency guard:

- `camera_fb_get_cb()` checks `SharedData::GetServiceMode()`.
- If the web server is streaming MJPEG (`mode_web_server`), UVC returns `NULL`, preventing double-use of the camera.

## 9. Shared data and concurrency

`SharedData` is a global singleton guarded by a `std::mutex` (`SharedData_StdMutex`). It carries:

- `service_mode` (used to gate UVC vs web streaming).
- IMU state and raw IMU readings.

Important interaction points:

- `AsyncJpegStreamResponse` sets `service_mode` on construct/destruct.
- `camera_fb_get_cb()` checks `service_mode` and aborts UVC capture if streaming.
- `sendJpg()` calls `SharedData::BorrowData()` but does not always call `ReturnData()` on all early returns, which can deadlock the mutex.

## 10. IMU and IR APIs (adjacent services)

While not the primary focus, the web server also exposes:

- `GET /api/v1/ws/imu_data` (WebSocket): pushes IMU data every 100 ms in JSON.
- `POST /api/v1/ir_send` (JSON): sends NEC IR commands.

These are implemented in `main/service/apis/api_imu.cpp` and `main/service/apis/api_ir.cpp`. The IMU WebSocket task runs indefinitely and is cleaned up via `cleanup_imu_ws_client()` in the main loop.

## 11. Asset pool and partitions

`partitions.csv` defines a custom partition:

```
assetpool, 233, 0x23, , 2M
```

At boot, the firmware:

- Uses `esp_partition_find_first()` with type `233` / subtype `0x23`.
- Maps 2 MB of flash into memory via `esp_partition_mmap()`.
- Injects the static assets into `AssetPool`.

The web UI (`index.html.gz`) is stored inside this asset pool and served on `/`.

## 12. Key API references

Core APIs used by the firmware:

- **WiFi (Arduino)**: `WiFi.softAP(ssid, pass, channel, hidden, max_conn, ftm)`
- **Async web server**: `AsyncWebServer::on`, `AsyncWebSocket::onEvent`, `AsyncCallbackJsonWebHandler`
- **Camera**: `esp_camera_init`, `esp_camera_fb_get`, `esp_camera_fb_return`, `esp_camera_sensor_get`
- **JPEG conversion**: `frame2jpg`, `frame2bmp`
- **UVC**: `uvc_device_config`, `uvc_device_init`

## 13. Observations and potential risks

- **Open WiFi AP**: the AP uses no password, which is simple for demos but insecure in real deployments.
- **Mutex pairing risk**: `sendJpg()` calls `SharedData::BorrowData()` without guaranteed `ReturnData()` on early exits. With `SharedData_StdMutex`, this can deadlock.
- **Service mode gating**: only MJPEG streaming flips `service_mode`; `/api/v1/capture` does not, so UVC and capture can run concurrently.
- **Camera power pin**: `enable_camera_power()` does not drive GPIO18 high/low; it relies on external board behavior.
- **Console config**: `sdkconfig.old` shows UART console enabled; for ESP32-S3 projects the recommended console is USB Serial/JTAG.

## 14. Extension points for websocket streaming

For websocket-based camera streaming, the most relevant insertion points are:

- Replace or extend `/api/v1/stream` in `api_camera.cpp`.
- Reuse `AsyncWebSocket` (similar to IMU) to push JPEG buffers.
- Align with `SharedData::service_mode` to prevent UVC conflicts.

