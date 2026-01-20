---
Title: 'UserDemo vs 0040: Camera Stream Pipeline Differences and frame2jpg Deep Dive'
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
      Note: |-
        MJPEG HTTP streaming and JPEG conversion pipeline in UserDemo.
        UserDemo MJPEG path and frame2jpg usage
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/main/usb_webcam_main.cpp
      Note: |-
        UserDemo camera init parameters and service order.
        UserDemo camera init defaults
    - Path: ../../../../../../../ATOMS3R-CAM-UserDemo/main/utils/camera/camera_init.c
      Note: |-
        UserDemo camera init settings (RGB565, QVGA, PSRAM).
        UserDemo sensor format and PSRAM buffers
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/client.html
      Note: |-
        Viewer WebSocket client.
        Viewer behavior
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/components/esp32-camera/conversions/include/img_converters.h
      Note: frame2jpg API definition.
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/components/esp32-camera/conversions/to_jpg.cpp
      Note: |-
        frame2jpg implementation (jpge encoder, line conversion, buffer allocation).
        frame2jpg implementation
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/Kconfig.projbuild
      Note: |-
        0040 stream interval and JPEG quality defaults.
        0040 stream interval and quality
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c
      Note: |-
        0040 streaming loop, frame2jpg usage, WebSocket send path.
        0040 stream loop and frame2jpg
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/server.go
      Note: |-
        0040 server ingest (MJPEG → FFmpeg) and viewer WebSocket.
        Server pipeline and FFmpeg usage
ExternalSources: []
Summary: Compares the stock UserDemo camera pipeline to 0040 streaming, explains why frame2jpg dominates FPS, and lists firmware/server/HTML changes that reduce drops.
LastUpdated: 2026-01-16T09:40:35-05:00
WhatFor: ""
WhenToUse: ""
---


# UserDemo vs 0040: Camera Stream Pipeline Differences and `frame2jpg` Deep Dive

## Executive summary

- **UserDemo** streams camera frames over **HTTP MJPEG** (`multipart/x-mixed-replace`) and uses **software JPEG conversion** (`frame2jpg`) when the sensor outputs RGB565.
- **0040** streams camera frames over **WebSocket binary** to a Go server which pipes the JPEG stream into FFmpeg; it still uses the same **software JPEG conversion** when the sensor is RGB565.
- The **performance bottleneck** is the `frame2jpg` conversion: it converts each scanline and compresses with a software JPEG encoder (jpge), which is CPU-heavy on ESP32-S3.
- To reduce frame drops, you must **lower conversion cost** (smaller framesize, lower quality, or native JPEG), **reduce backpressure** (drop-latest grab mode, lower fps target, or server fps), and **avoid mismatched expectations** (FFmpeg target fps too high).

---

## 1) Pipeline comparison (UserDemo vs 0040)

### 1.1 Camera acquisition and format

**UserDemo** (`main/usb_webcam_main.cpp` + `main/utils/camera/camera_init.c`):
- Pixel format: `PIXFORMAT_RGB565`
- Framesize: `FRAMESIZE_QVGA`
- Frame buffers: PSRAM, `fb_count=2`

**0040** (`firmware/main/stream_client.c`):
- Pixel format: `PIXFORMAT_RGB565`
- Framesize: `FRAMESIZE_QVGA`
- Frame buffers: PSRAM, `fb_count=2`

**Conclusion:** Both are identical at the sensor and `camera_fb_t` level.

### 1.4 Camera configuration + JPEG settings (full comparison)

Despite the different transport layers, the *camera configuration* is effectively the same in both firmware variants. The main differences are in **how often frames are pulled** and **how they are delivered**.

**Camera config values (UserDemo):**
- File: `main/utils/camera/camera_init.c`
- `pixel_format = PIXFORMAT_RGB565`
- `frame_size   = FRAMESIZE_QVGA`
- `jpeg_quality = 14` (passed from `my_camera_init(..., 14, ...)`)
- `fb_count     = 2`
- `grab_mode    = CAMERA_GRAB_WHEN_EMPTY`
- `fb_location  = CAMERA_FB_IN_PSRAM`

**Camera config values (0040):**
- File: `firmware/main/stream_client.c`
- `pixel_format = PIXFORMAT_RGB565`
- `frame_size   = FRAMESIZE_QVGA`
- `jpeg_quality = CONFIG_ATOMS3R_SENSOR_JPEG_QUALITY` (default `14`)
- `fb_count     = 2`
- `grab_mode    = CAMERA_GRAB_WHEN_EMPTY`
- `fb_location  = CAMERA_FB_IN_PSRAM`

**JPEG conversion quality:**
- UserDemo: `frame2jpg(fb, 80, ...)` (hard-coded in `api_camera.cpp`)
- 0040: `frame2jpg(fb, CONFIG_ATOMS3R_CONVERT_JPEG_QUALITY, ...)` (default `80`)

**Stream pacing:**
- UserDemo: no explicit interval; MJPEG chunks are emitted when the HTTP response buffer is ready (implicit backpressure).
- 0040: explicit `CONFIG_ATOMS3R_STREAM_INTERVAL_MS` (currently `66 ms`), plus WebSocket send time.

**Takeaway:** The *image format and quality* are the same by default. The observed drop behavior is dominated by **conversion + transport pacing**, not by mismatched JPEG quality settings.

### 1.2 Transport layer and protocol

| Aspect | UserDemo | 0040 |
|---|---|---|
| Transport | HTTP | WebSocket (binary) |
| Endpoint | `/api/v1/stream` (port 80) | `/ws/camera` (server-side, 0040 Go server) |
| Payload | MJPEG multipart | JPEG binary frames |
| Browser consumption | `<img src=...>` MJPEG | JS WebSocket + viewer (H.264) |

**Key difference:** UserDemo emits MJPEG directly to the browser; 0040 emits JPEGs to a server, which re-encodes to H.264 and forwards to viewers.

### 1.3 Software JPEG conversion

Both pipelines call the same conversion function when the sensor is RGB565:

```
frame2jpg(fb, quality, &jpeg_buf, &jpeg_len)
```

In UserDemo:
- `frame2jpg(fb, 80, ...)` inside `AsyncJpegStreamResponse` and `sendJpg()`.

In 0040:
- `frame2jpg(fb, CONFIG_ATOMS3R_CONVERT_JPEG_QUALITY, ...)` inside the stream task.

This means **both pipelines are CPU-bound by JPEG conversion**, but 0040 also adds **network WebSocket send + server re-encode**.

---

## 2) Why 0040 drops frames more than UserDemo

### 2.1 Timing and backpressure

In 0040, the stream task is synchronous:

```
fb = esp_camera_fb_get()
if (not JPEG) -> frame2jpg()
ws_send(jpeg)
return fb
```

If any stage stalls (JPEG conversion or WebSocket send), the camera can’t drain fast enough and will report VSYNC overflows.

UserDemo MJPEG streaming also converts and sends, but:
- The MJPEG flow is built on `AsyncWebServer` and chunked responses, which naturally pace delivery.
- The browser is directly consuming the MJPEG stream without server-side transcoding.

**Result:** 0040 has more opportunities for backpressure: conversion + socket send + server input + FFmpeg encode.

### 2.2 Server-side re-encoding cost

The 0040 server uses FFmpeg to re-encode MJPEG into H.264 for viewers. This can introduce:
- Higher latency if FFmpeg is configured for 30 fps while the device outputs 3–5 fps.
- Frame duplication and buffering (observed `dup` counts and `speed<1.0x`).

### 2.3 Default stream interval vs actual conversion time

`CONFIG_ATOMS3R_STREAM_INTERVAL_MS` may be shorter than the time required to convert+send a frame.
When conversion takes ~140 ms and send takes ~35–100 ms, a 100 ms interval will oversubscribe the pipeline.

---

## 3) Deep dive: How `frame2jpg` works

File: `components/esp32-camera/conversions/to_jpg.cpp`

### 3.1 API surface

```c
bool frame2jpg(camera_fb_t *fb, uint8_t quality,
               uint8_t **out, size_t *out_len);
```

- **Input:** `camera_fb_t` (raw frame data, often RGB565)
- **Output:** Newly allocated JPEG buffer and its length

### 3.2 Internal pipeline (high-level)

```
frame2jpg(fb):
  return fmt2jpg(fb->buf, fb->len, fb->width, fb->height, fb->format)

fmt2jpg(...):
  allocate 128 KB JPEG buffer
  create memory_stream to write into buffer
  convert_image(src, width, height, format, quality, stream)
  return out_buf + size
```

### 3.3 Conversion stages

#### Stage A: Line conversion

The function `convert_line_format()` converts each scanline into a 24‑bit RGB buffer (or grayscale) suitable for the JPEG encoder:

- **RGB565 → RGB888** conversion (per pixel):
  - Extract R5, G6, B5 bits and expand to 8-bit channels.
- **YUV422 → RGB888** conversion uses `yuv2rgb()` for each pixel.

This step is done **line-by-line** for the entire image.

#### Stage B: JPEG encoding (jpge)

- The encoder is initialized with quality and subsampling:
  - `quality` in range 1–100
  - `subsampling` typically `H2V2` for RGB
- Each scanline is passed to `jpge::jpeg_encoder::process_scanline()`.

This is CPU-heavy and is usually the largest time cost per frame.

### 3.4 Memory behavior

`fmt2jpg()` allocates a **fixed 128 KB buffer**:

```c
int jpg_buf_len = 128*1024;
uint8_t *jpg_buf = (uint8_t*)_malloc(jpg_buf_len);
```

This size is a compromise (sufficient for CIF/QVGA in many cases). If images or quality are larger than expected, the buffer can truncate output (partial frames).

**Implication:**
- The larger the frame size and quality, the more likely you hit buffer limits and CPU time spikes.
- Use smaller frames or lower quality to stabilize.

### 3.5 Pseudocode (line-level)

```
convert_image(src, w, h, format, quality, stream):
  choose channels/subsampling
  init jpge encoder
  allocate line buffer: w * channels
  for each line in [0..h-1]:
    convert_line_format(src, format, line)
    encoder.process_scanline(line)
  encoder.process_scanline(NULL)  // finalize
```

**Callout:** This is a *full-frame* conversion; there is no hardware JPEG encoder on ESP32-S3 for the camera pipeline in this configuration.

---

## 4) Configuration differences that affect dropped frames

### 4.1 Firmware changes (0040)

**High impact (reduce conversion time):**
- **Framesize**: drop from QVGA (320×240) → QQVGA (160×120)
  - Cuts pixels by 4×, reduces conversion time dramatically.
- **JPEG quality**: lower `CONFIG_ATOMS3R_CONVERT_JPEG_QUALITY` (e.g., 80 → 50)
  - Smaller JPEG, faster encode and send.

**Medium impact (reduce latency and backpressure):**
- **Grab mode**: use `CAMERA_GRAB_LATEST` if available
  - Drops stale frames instead of blocking on empty buffers.
- **Stream interval**: increase `CONFIG_ATOMS3R_STREAM_INTERVAL_MS` to match actual conversion time.
  - E.g., 150–250 ms for 4–7 fps stability.

**Structural change (avoid conversion entirely):**
- If the sensor supports native JPEG: set `PIXFORMAT_JPEG` in `camera_config_t`.
  - For GC0308, native JPEG is typically **not** supported.

### 4.4 “Pull frames on demand” — how the stock firmware behaves

The stock UserDemo **appears to “pull on demand”** because it only calls `esp_camera_fb_get()` when the HTTP MJPEG response needs to emit a new part. The AsyncWebServer response buffer acts as a natural backpressure mechanism.

In 0040, the stream loop is time‑based (`CONFIG_ATOMS3R_STREAM_INTERVAL_MS`) and independent of downstream backpressure. Even if the server or viewer is slow, the loop tries to fetch and convert new frames on a fixed cadence, which can lead to overruns and `EV-VSYNC-OVF`.

**To make 0040 behave more like the stock firmware:**

1. **Remove or relax the fixed interval**  
   - Set `CONFIG_ATOMS3R_STREAM_INTERVAL_MS` to a larger value (e.g., 150–250 ms).  
   - Or set it to `0` and rely on the send loop time (capture → convert → send) as the pacing mechanism.

2. **Gate capture on WebSocket readiness**  
   - Keep the current structure where a new frame is only captured *after* the previous send returns.  
   - This mimics the MJPEG “send next part only when ready” model.

3. **Adopt drop-latest mode if supported**  
   - Change `grab_mode` to `CAMERA_GRAB_LATEST` to avoid blocking on full buffers.  
   - This trades frame continuity for lower latency and fewer DMA overflows.

4. **Match server expectations to device output**  
   - Reduce FFmpeg target fps so the server doesn’t pressure the device to send faster than it can convert.

### 4.5 Concrete changes to align 0040 with UserDemo behavior

**Firmware (stream_client.c):**
- Increase `CONFIG_ATOMS3R_STREAM_INTERVAL_MS` (or set to 0 and rely on conversion + send as pacing).
- Consider `CAMERA_GRAB_LATEST` to reduce backlog stalls.
- Keep `PIXFORMAT_RGB565` unless the sensor supports `PIXFORMAT_JPEG` (GC0308 likely does not).

**Firmware (Kconfig defaults):**
- Set `CONFIG_ATOMS3R_STREAM_INTERVAL_MS` to a safer default (e.g., 150–200 ms).

**Server (server.go / FFmpeg):**
- Reduce target FPS (e.g., 5) to match device output and avoid duplication.
- Consider a debug MJPEG endpoint for 1:1 comparison with UserDemo.

### 4.2 Server-side changes (0040)

**Match server FPS to device FPS:**
- Update FFmpeg to target ~5 fps rather than 30.
- This reduces duplication and buffering (`dup` counts).

**Alternative**:
- Skip FFmpeg and serve MJPEG directly to viewers (like UserDemo).

### 4.3 Browser/HTML changes

- If using MJPEG, `<img src="/api/v1/stream">` is simplest and avoids WebSocket overhead.
- If using WebSocket + H.264 viewer, ensure viewer expects the correct fps and buffering behavior.

---

## 5) Concrete knobs to change in 0040

### Firmware (Kconfig + stream_client.c)

- `CONFIG_ATOMS3R_CONVERT_JPEG_QUALITY`
  - Lower to reduce CPU time and payload size.
- `CONFIG_ATOMS3R_STREAM_INTERVAL_MS`
  - Increase to match conversion time and avoid overruns.
- `camera_config_t.frame_size`
  - Set to `FRAMESIZE_QQVGA` or `FRAMESIZE_QCIF`.
- `camera_config_t.grab_mode`
  - Use `CAMERA_GRAB_LATEST` if supported by component.

### Server (server.go)

- Reduce FFmpeg target fps:
  - Add `-r 5` or `-vf fps=5`.
- Consider direct MJPEG viewer for debugging.

### Client (client.html)

- Ensure viewer WebSocket uses correct host/port (fixed earlier).
- If MJPEG fallback is desired, add a simple `<img>` viewer path.

---

## 6) Summary table of differences

| Feature | UserDemo | 0040 | Consequence |
|---|---|---|---|
| Camera output | RGB565 | RGB565 | Same conversion cost |
| JPEG conversion | `frame2jpg` in HTTP stream | `frame2jpg` in stream loop | Same CPU cost |
| Transport | HTTP MJPEG | WebSocket binary | 0040 adds WS backpressure |
| Server pipeline | None | FFmpeg → H.264 | Adds latency/duplication |
| Browser view | MJPEG `<img>` | H.264 viewer | More moving parts |

---

## 7) Recommended next steps

1. **Lower framesize** to QQVGA for a quick fps gain.
2. **Lower JPEG quality** to reduce encode time and payload size.
3. **Increase stream interval** to match real encode time.
4. **Tune FFmpeg** target fps to match device output.
5. **Optionally add MJPEG viewer** for debugging without the server pipeline.

These changes are the most direct levers to reduce frame drops driven by `frame2jpg` CPU cost and WebSocket backpressure.
