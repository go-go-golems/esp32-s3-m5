---
Title: Camera Stream FPS Bottleneck Analysis
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
  - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/stream_client.c
    Note: Capture/convert/send loop, stream stats, websocket send, frame2jpg path.
  - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0040-atoms3r-cam-streaming/esp32-camera-stream/firmware/main/Kconfig.projbuild
    Note: Stream interval and JPEG quality defaults.
  - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0040-atoms3r-cam-streaming/esp32-camera-stream/server.go
    Note: Server-side WebSocket + FFmpeg pipeline, viewer/reader behavior.
  - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0040-atoms3r-cam-streaming/esp32-camera-stream/client.html
    Note: Viewer WebSocket client.
ExternalSources: []
Summary: Explains why the camera stream runs ~4 fps and stalls, based on stream stats, and outlines fixes.
LastUpdated: 2026-01-15T23:23:30-05:00
---

# Camera Stream FPS Bottleneck Analysis

## Context

Recent runs show the camera stream stabilizing around ~3–5 fps and occasionally “bursting” then stalling. This report analyzes the per-stage timings reported by `cam_stream` and correlates them with server-side FFmpeg logs. It concludes that camera capture is not the bottleneck; JPEG conversion plus WebSocket send backpressure dominate.

## Observed telemetry (excerpt)

From the device stream stats:

```
I (...) cam_stream: stream stats: frames=22 fps=4.2 bytes=288840 avg=13129 cap_ms=0.03 conv_ms=144.67 send_ms=34.13 cap_fail=0 conv_fail=0 ws_fail=0
I (...) cam_stream: stream stats: frames=19 fps=3.7 bytes=194342 avg=10229 cap_ms=0.03 conv_ms=141.08 send_ms=71.30 cap_fail=0 conv_fail=0 ws_fail=0
I (...) cam_stream: stream stats: frames=17 fps=3.2 bytes=256546 avg=15091 cap_ms=0.03 conv_ms=145.89 send_ms=108.01 cap_fail=0 conv_fail=0 ws_fail=0
cam_hal: EV-VSYNC-OVF
```

From server-side FFmpeg stats:

```
FFmpeg: ... fps=5.1 ... speed=0.17x ... dup=281 drop=0
```

Additional notes:
- `WebSocket data op=10 len=0` entries indicate PONG frames (normal keepalive).
- `EV-VSYNC-OVF` indicates a camera DMA overflow (downstream consumer is too slow).

## Pipeline anatomy and where time is spent

The streaming loop is effectively:

```
while streaming:
  fb = esp_camera_fb_get()              # camera capture
  if fb->format != JPEG:
     jpeg = frame2jpg(fb, quality)      # software JPEG conversion
  ws_send(jpeg)                         # websocket send
  return fb
  delay(stream_interval_ms)
```

Key timing signals from the logs:
- `cap_ms ~ 0.03 ms` → capture is fast and not the bottleneck.
- `conv_ms ~ 140–146 ms` → JPEG conversion dominates CPU time.
- `send_ms ~ 35–110 ms` → transport backpressure adds significant latency.

### Why this yields ~3–5 fps
Even in the best case:
- 140 ms (convert) + 35 ms (send) ≈ 175 ms/frame → ~5.7 fps.
In worse cases:
- 145 ms (convert) + 100 ms (send) ≈ 245 ms/frame → ~4.1 fps.

Thus the reported 3–5 fps is consistent with the measured stage timings.

## Why it “bursts” then stalls

The stream task is single-threaded and synchronous:
- Each frame must complete conversion and send before the next frame can be processed.
- When the network or server blocks, `send_ms` spikes → the loop falls behind.
- Backlog triggers `EV-VSYNC-OVF` (camera DMA overflow) because frames can’t be drained.

This creates the observed “burst then stall” pattern: a few frames flow quickly, then conversion/send latency spikes, the camera overruns, and the loop recovers slowly.

## Server-side notes (FFmpeg)

FFmpeg is targeting 30 fps while the camera provides ~4–5 fps. The logs show:
- `speed=0.17x`
- large `dup` counts

This means FFmpeg is duplicating frames to maintain its target FPS. The server is not the root cause of the slow camera output, but it is amplifying jitter and buffering effects. A lower target FPS in FFmpeg will better match the real feed and reduce duplicate frames.

## Root causes (ranked)

1. **Software JPEG conversion cost**
   - `frame2jpg()` dominates CPU time on ESP32-S3 when the sensor outputs RGB565.
2. **WebSocket backpressure**
   - Send time varies with network/CPU load; long sends reduce the effective FPS and cause DMA overflow.
3. **Frame size and quality**
   - Larger frames increase conversion + send time.
4. **Server FPS mismatch**
   - FFmpeg expects higher fps than available; duplicates increase load and latency.

## Recommended fixes (prioritized)

### A) Reduce conversion cost (highest impact)
- Lower resolution (e.g., QVGA → QQVGA or QCIF).
- Reduce JPEG quality (`CONFIG_ATOMS3R_CONVERT_JPEG_QUALITY`).
- If the sensor supports native JPEG: switch to `PIXFORMAT_JPEG` to avoid `frame2jpg` entirely.

### B) Drop frames when behind
- If supported by esp32-camera: set `grab_mode = CAMERA_GRAB_LATEST` to drop stale frames instead of blocking the pipeline.
- This reduces latency and prevents `EV-VSYNC-OVF`.

### C) Match server FPS to camera FPS
- Update the FFmpeg pipeline to target ~5 fps:
  - e.g. `-r 5` (or insert `-vf fps=5`) to align the transcode with the real camera rate.

### D) Adjust stream loop pacing
- Increase `CONFIG_ATOMS3R_STREAM_INTERVAL_MS` to reflect actual conversion+send time (e.g., 150–250 ms).
- This makes the loop stable and avoids overshooting the camera/transport.

## Where to implement

- **Firmware loop and stats**: `firmware/main/stream_client.c`
  - `frame2jpg()` conversion
  - `esp_websocket_client_send_bin()` send path
  - stream interval and logging
- **Defaults**: `firmware/main/Kconfig.projbuild`
  - `CONFIG_ATOMS3R_STREAM_INTERVAL_MS`
  - `CONFIG_ATOMS3R_CONVERT_JPEG_QUALITY`
- **Server pipeline**: `server.go`
  - FFmpeg command line construction

## Suggested next experiments

1. **Lower framesize to QQVGA**, keep same quality, measure `conv_ms` and `send_ms`.
2. **Lower quality** (e.g., 12 → 6), re-measure conversion time.
3. **Match FFmpeg target FPS** to camera output to reduce duplication.
4. **Enable drop-latest grab mode** if available.

## Success criteria

- `conv_ms` significantly below 100 ms (ideally < 60 ms for 10–15 fps).
- `send_ms` stays < 30–50 ms with fewer spikes.
- No `EV-VSYNC-OVF` in steady streaming.
- Sustained FPS stable for > 60s without stalls.
