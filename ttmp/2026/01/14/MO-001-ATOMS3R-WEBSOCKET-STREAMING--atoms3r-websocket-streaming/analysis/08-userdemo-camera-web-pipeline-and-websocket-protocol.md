---
Title: ATOMS3R-CAM UserDemo Camera→Browser Pipeline and WebSocket Protocol
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
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/components/ESPAsyncWebServer/src/AsyncWebSocket.h
      Note: |-
        WebSocket opcode definitions and event types.
        WebSocket opcodes and frame types
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/main/service/apis/api_camera.cpp
      Note: |-
        Camera capture/stream HTTP endpoints and MJPEG implementation.
        MJPEG HTTP pipeline and JPEG conversion
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/main/service/apis/api_imu.cpp
      Note: |-
        WebSocket pipeline (IMU JSON over /api/v1/ws/imu_data).
        IMU WebSocket JSON pipeline
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/main/service/service_web_server.cpp
      Note: Web server startup, route registration, asset serving.
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/main/usb_webcam_main.cpp
      Note: |-
        System init, camera init parameters, service startup order.
        Camera init parameters and service order
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/main/utils/camera/camera_init.c
      Note: |-
        esp_camera init, pixel format/framesize, PSRAM buffer config.
        Camera pixel format and frame buffer configuration
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/main/utils/camera/camera_pin.h
      Note: AtomS3R-CAM pin map used by esp_camera.
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/main/utils/shared/shared.h
      Note: SharedData service mode and mutex guards used by camera APIs.
ExternalSources: []
Summary: Exhaustive walk-through of the UserDemo camera frame pipeline to the browser, all image format transitions, and how WebSocket frames are constructed (IMU path). Notes the camera stream uses HTTP MJPEG, not WebSocket.
LastUpdated: 2026-01-15T23:27:30-05:00
WhatFor: ""
WhenToUse: ""
---


# ATOMS3R-CAM UserDemo Camera→Browser Pipeline and WebSocket Protocol

## Executive summary

The **stock ATOMS3R‑CAM UserDemo does not stream camera frames over WebSocket**. Camera frames are delivered to the browser using **HTTP MJPEG** (`multipart/x-mixed-replace`) on `/api/v1/stream` and single-frame HTTP snapshots on `/api/v1/capture`. The only WebSocket stream in the UserDemo is **IMU telemetry**, sent as JSON text frames on `/api/v1/ws/imu_data`.

This document therefore splits into two detailed pipelines:

1. **Camera frame pipeline (sensor → esp_camera → JPEG → HTTP MJPEG → browser)**
2. **WebSocket pipeline (IMU data → JSON → WS frames → browser)**

Along the way, it explains all major image formats, encoder steps, and WebSocket frame structure in textbook style.

---

## 1) System entry point and camera configuration

### 1.1 Entry point and service start order

File: `main/usb_webcam_main.cpp`

Key sequence (simplified):

```
shared_data_injection();   // SharedData_StdMutex
asset_pool_injection();    // static web assets

// Hardware init
enable_camera_power();
i2c_init();
imu_init();
ir_init();
camera_init();

// Services
start_service_uvc();
start_service_web_server();
```

The **web server** is started on port 80 and serves static HTML from the asset pool as well as the camera APIs.

### 1.2 Camera init parameters

File: `main/usb_webcam_main.cpp`

```
my_camera_init(CONFIG_CAMERA_XCLK_FREQ,
               PIXFORMAT_RGB565,
               FRAMESIZE_QVGA,
               14,
               2);
```

Important defaults:
- **Pixel format**: `PIXFORMAT_RGB565` (uncompressed 16‑bit RGB)
- **Frame size**: `FRAMESIZE_QVGA` (320×240)
- **JPEG quality**: `14` (used when converting to JPEG)
- **Frame buffer count**: `2` (double buffering)

These parameters mean the **sensor produces raw RGB565 frames**, not JPEG. JPEG is generated in software when streaming or capturing.

### 1.3 Camera driver setup

File: `main/utils/camera/camera_init.c`

`my_camera_init()` builds a `camera_config_t` and calls `esp_camera_init()`:

```
camera_config_t camera_config = {
  .pin_*      = CAMERA_PIN_*,
  .xclk_freq_hz = xclk_freq_hz,
  .pixel_format = pixel_format,
  .frame_size   = frame_size,
  .jpeg_quality = jpeg_quality,
  .fb_count     = fb_count,
  .grab_mode    = CAMERA_GRAB_WHEN_EMPTY,
  .fb_location  = CAMERA_FB_IN_PSRAM,
};
```

Notable consequences:
- **Frame buffers live in PSRAM** (`CAMERA_FB_IN_PSRAM`), not internal DRAM.
- The camera driver **pulls frames on demand** (`CAMERA_GRAB_WHEN_EMPTY`).

### 1.4 Pin mapping for AtomS3R‑CAM

File: `main/utils/camera/camera_pin.h` (AtomS3R‑CAM section)

```
XCLK  = GPIO21
SIOD  = GPIO12
SIOC  = GPIO9
VSYNC = GPIO10
HREF  = GPIO14
PCLK  = GPIO40
D0..D7 = GPIO3,42,46,48,4,17,11,13
```

This mapping is essential for understanding the sensor’s parallel data bus (D0–D7) and synchronization signals (VSYNC/HREF/PCLK).

---

## 2) Camera frame pipeline (sensor → JPEG → HTTP MJPEG → browser)

### 2.1 Step-by-step flow (ASCII diagram)

```
[Image sensor] --(DVP parallel RGB565)--> [esp_camera DMA]
     |                                           |
     |                                           v
     |                                   camera_fb_t (PSRAM)
     |                                           |
     |                         if not JPEG --> frame2jpg()
     |                                           |
     |                                           v
     |                                 JPEG byte buffer
     |                                           |
     |            HTTP multipart/x-mixed-replace boundary
     |                                           |
     v                                           v
[Browser HTTP client] <-- MJPEG stream <-- AsyncWebServer
```

### 2.2 Frame acquisition (esp_camera_fb_get)

File: `main/service/apis/api_camera.cpp`

For both **single snapshot** and **streaming**, the pipeline starts with:

```
camera_fb_t* fb = esp_camera_fb_get();
```

The returned `camera_fb_t` includes:
- `fb->buf` : pointer to image data (RGB565 or JPEG)
- `fb->len` : number of bytes
- `fb->format` : `PIXFORMAT_RGB565` (here) or `PIXFORMAT_JPEG`
- `fb->width`, `fb->height` : image dimensions

### 2.3 Image formats in use (callouts)

**Callout: RGB565**
- 16-bit packed RGB: 5 bits red, 6 bits green, 5 bits blue.
- Size = width × height × 2 bytes. At 320×240: **153,600 bytes**.
- Not directly usable in most browsers without decoding.

**Callout: JPEG**
- Compressed DCT-based image format.
- Much smaller payload (often 5–20 KB at QVGA with medium quality).
- Ideal for streaming over HTTP or WebSocket.

**Callout: MJPEG (HTTP multipart)**
- Not a new codec: **a sequence of JPEG images**.
- Delivered as `multipart/x-mixed-replace` with boundaries:
  - Each part includes `Content-Type: image/jpeg` and `Content-Length`.

### 2.4 Single-frame snapshot (/api/v1/capture)

Function: `sendJpg(AsyncWebServerRequest* request)`

Behavior:
1. Acquire framebuffer (`esp_camera_fb_get`).
2. If `fb->format == PIXFORMAT_JPEG`, send directly as HTTP response.
3. Otherwise convert to JPEG using `frame2jpg()`.
4. Wrap as `AsyncBufferResponse` or `AsyncFrameResponse` and send.

Key code path:

```
if (fb->format == PIXFORMAT_JPEG) {
    response = new AsyncFrameResponse(fb, "image/jpeg");
} else {
    frame2jpg(fb, 80, &jpg_buf, &jpg_buf_len);
    response = new AsyncBufferResponse(jpg_buf, jpg_buf_len, "image/jpeg");
}
```

### 2.5 MJPEG streaming (/api/v1/stream)

Function: `streamJpg()` → `AsyncJpegStreamResponse`

The MJPEG stream is implemented by `AsyncJpegStreamResponse::_content()`:

**Pseudo‑algorithm**:

```
loop:
  if no frame or frame fully sent:
     if previous frame exists:
        free jpeg buffer (if converted)
        return fb
     fb = esp_camera_fb_get()
     if fb->format != JPEG:
        jpeg_buf = frame2jpg(fb, quality=80)
     else:
        jpeg_buf = fb->buf
     send "--boundary\r\n"
     send "Content-Type: image/jpeg\r\n"
     send "Content-Length: N\r\n\r\n"
     send first chunk of JPEG
  else:
     send next chunk of JPEG
```

**Key detail:** `AsyncJpegStreamResponse` sets:
- `Content-Type: multipart/x-mixed-replace; boundary=...`
- `chunked = true` (no fixed content length)

### 2.6 The HTTP multipart boundary format

A typical MJPEG part looks like:

```
--123456789000000000000987654321
Content-Type: image/jpeg
Content-Length: 12345

<raw JPEG bytes>
```

The browser treats each part as a complete JPEG image and replaces the previous image on screen.

### 2.7 Browser consumption

The embedded HTML (served from the asset pool) loads the MJPEG stream. Typical browser patterns:

- `<img src="/api/v1/stream">` for MJPEG
- or `fetch()` + custom decoder

The important property is that **the browser never sees RGB565**. It only sees **JPEG images** within a multipart HTTP response.

---

## 3) WebSocket pipeline (IMU JSON → WS frames → browser)

### 3.1 Location of the WebSocket stream

File: `main/service/apis/api_imu.cpp`

The UserDemo exposes a WebSocket endpoint:

```
AsyncWebSocket("/api/v1/ws/imu_data")
```

This WebSocket is **not for camera images**. It is for IMU telemetry only.

### 3.2 IMU data flow

```
SharedData::UpdateImuData()
→ JSON encode (ax, ay, az, gx, gy, gz, mx, my, mz)
→ imu_data_ws->textAll(json)
```

Example JSON payload:

```
{
  "ax": 0.12, "ay": -0.03, "az": 9.81,
  "gx": 0.01, "gy": 0.02, "gz": -0.04,
  "mx": 32.1, "my": -14.8, "mz": 7.2
}
```

### 3.3 WebSocket handshake (HTTP Upgrade)

**Conceptual handshake**:

```
GET /api/v1/ws/imu_data HTTP/1.1
Host: <device>
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Key: <random>
Sec-WebSocket-Version: 13

HTTP/1.1 101 Switching Protocols
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Accept: <computed>
```

This is implemented by `ESPAsyncWebServer` and `AsyncWebSocket` internally.

### 3.4 WebSocket frame layout (callout)

A WebSocket frame has:

```
0               1               2               3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-------+-+-------------+-------------------------------+
|F|R|R|R| opcode|M| payload len |    Extended payload length    |
+-+-+-+-+-------+-+-------------+-------------------------------+
| Masking-key (client→server)   |  Payload data (masked/unmasked)|
+---------------------------------------------------------------+
```

**Opcodes used here**:
- `0x1` = text frame (JSON)
- `0x2` = binary frame (not used in IMU path)
- `0x9` = ping
- `0xA` = pong

The server sends **unmasked** frames; browsers send masked frames. This detail is handled by `AsyncWebSocket` internally.

### 3.5 WebSocket usage in UserDemo

- Connection events are handled in `imu_ws_on_event()`.
- On connect, the server sends a ping to validate liveness.
- The IMU task sends text frames at ~10 Hz (`vTaskDelay(100 ms)`).

---

## 4) Why camera frames are not on WebSocket in UserDemo

The camera stream is implemented as MJPEG over HTTP, which is simple and efficient for browsers. The stock demo does **not** implement camera images over WebSocket, likely because:

- MJPEG works with `<img>` tags and doesn’t require custom JS decoding.
- HTTP chunking is already supported by `ESPAsyncWebServer`.
- WebSocket would require custom frame framing + reassembly in JS.

If a future design wants WebSocket camera streaming, it would need:
- A binary frame protocol (e.g., “frame length + JPEG bytes”).
- Explicit backpressure handling to avoid queue overflows.

---

## 5) Quick reference: formats along the camera pipeline

| Stage | Format | Where it appears | Notes |
|------:|--------|------------------|-------|
| Sensor output | RGB565 (DVP) | `esp_camera` DMA | 16-bit per pixel; raw | 
| Frame buffer | RGB565 | `camera_fb_t` | PSRAM buffer, QVGA 320×240 | 
| Encoder output | JPEG | `frame2jpg()` | Quality=80 in streaming | 
| Network stream | MJPEG (HTTP multipart) | `/api/v1/stream` | Sequence of JPEG images | 
| Browser | JPEG | `<img src>` or JS | Browser decodes each JPEG |

---

## 6) Exercises (for deep understanding)

1) **Compute raw RGB565 size**
   - QVGA = 320×240. RGB565 uses 2 bytes/pixel.
   - **Answer:** 320×240×2 = 153,600 bytes.

2) **Explain why JPEG conversion is necessary**
   - Why can’t the browser display RGB565 directly over HTTP? What must change?

3) **Design a WebSocket camera frame**
   - Propose a binary frame format: `uint32 length + JPEG bytes`.
   - Explain how the browser should reassemble and decode it.

4) **Describe backpressure effects**
   - If the network is slower than the camera, what happens to the frame buffer? How would `CAMERA_GRAB_LATEST` change behavior?

---

## 7) Pseudocode summary (camera stream path)

```
function streamJpg(request):
  response = new AsyncJpegStreamResponse()
  response.contentType = "multipart/x-mixed-replace; boundary=..."
  response.chunked = true
  request.send(response)

class AsyncJpegStreamResponse:
  on_fill(buffer, maxLen):
    if no frame or frame sent:
       fb = esp_camera_fb_get()
       if fb.format != JPEG:
          jpeg = frame2jpg(fb, quality=80)
       else:
          jpeg = fb.buf
       write boundary + headers
       write first jpeg chunk
    else:
       write next jpeg chunk
```

---

## 8) Key APIs and where they live

- **Camera capture**: `esp_camera_fb_get()`
  - File: `esp_camera.h` (ESP32 Camera component)
- **JPEG conversion**: `frame2jpg()`
  - File: camera conversion utilities (ESP32 Camera component)
- **HTTP stream response**: `AsyncAbstractResponse`
  - File: `components/ESPAsyncWebServer`
- **WebSocket server**: `AsyncWebSocket`
  - File: `components/ESPAsyncWebServer/src/AsyncWebSocket.*`

---

## 9) Takeaways

- The **camera stream is HTTP MJPEG**, not WebSocket.
- The **sensor output is RGB565**, and software JPEG conversion is required.
- WebSocket is used only for **IMU JSON telemetry** in the stock UserDemo.
- Any future WebSocket camera stream must define a binary framing protocol and handle backpressure explicitly.
