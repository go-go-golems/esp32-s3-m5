---
Title: Website Audio Recorder - Design Document
Ticket: CLINTS-MEMO-WEBSITE
Status: active
Topics:
    - audio-recording
    - web-server
    - esp32-s3
    - m5atoms3
    - i2s
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: Design for a device-hosted website that controls an on-device audio recorder (I2S mic) on M5AtomS3/AtomS3R-class ESP32-S3 boards.
LastUpdated: 2025-12-31T13:59:30-05:00
WhatFor: Provide a concrete architecture (device firmware + web UI + APIs) to implement recording, listing, and downloading audio recordings from a browser.
WhenToUse: Use before implementation to align on protocol, formats, concurrency model, and MVP scope.
---

# Website Audio Recorder - Design Document

## Executive Summary

We will build a **device-hosted website** (served directly from the ESP32-S3) that can **start/stop audio recording**, show **status**, list available recordings, and **download/play** them in the browser. The firmware will capture audio from an **I2S microphone** at **16 kHz, 16-bit mono**, write it into a **WAV** file on flash (LittleFS/SPIFFS), and expose a small **REST API** for control and file management.

The design intentionally separates concerns:
- **Audio task**: real-time I2S read → ring buffer → file writer.
- **Web server**: responds quickly; handlers only flip state / read metadata.
- **Storage**: append audio frames; finalize WAV header at stop.

MVP is “record → stop → list → download/play”. Live streaming is a phase-2 extension.

## Problem Statement

We want a simple “memo recorder” for **AtomS3/AtomS3R-class ESP32-S3** devices that can be controlled from a phone/laptop without installing anything. Requirements:

- **Web-controlled**: start/stop, status, list recordings, download recording.
- **Robust recording**: no audio dropouts, predictable file format, bounded memory.
- **Simple UX**: works in AP mode (device hosts WiFi) and serves its own UI.
- **Implementation reuse**: leverage patterns already present in this repo:
  - I2S capture patterns from `echo-base--openai-realtime-embedded-sdk/src/media.cpp`
  - Async web server patterns from `ATOMS3R-CAM-M12-UserDemo/main/service/service_web_server.cpp`
  - Alternative native HTTP patterns from `esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp`

## Proposed Solution

### System overview

- **WiFi**: device runs as **SoftAP** (MVP) so clients can connect directly.
- **HTTP server**: **ESPAsyncWebServer** (Arduino stack) serving:
  - `GET /` → embedded `index.html` (+ minimal JS/CSS assets).
  - REST APIs under `/api/v1/*`.
- **Audio pipeline**:
  - Configure I2S for **16kHz, 16-bit, mono** (target voice memos).
  - A dedicated FreeRTOS task reads from I2S into a fixed buffer (e.g. 40ms frames).
  - Frames are written to a file (WAV) via a writer module that:
    - writes a placeholder WAV header at open,
    - appends PCM frames,
    - patches header sizes at close.
- **Storage**: **LittleFS** preferred (Arduino-ESP32) with SPI flash; SPIFFS acceptable if LittleFS not available in the chosen environment.
- **Waveform preview (optional but recommended)**:
  - Firmware maintains a lightweight “recent samples” window or pre-computed peaks.
  - Web UI polls `GET /api/v1/waveform` every **500ms** and renders a canvas waveform.

### API surface (MVP)

All responses JSON unless noted.

- `GET /api/v1/status`
  - returns `{ ok, recording, filename, bytes_written, sample_rate_hz, channels, bits_per_sample }`
- `POST /api/v1/recordings/start`
  - body optional: `{ sample_rate_hz?: 16000 }` (keep MVP fixed if needed)
  - returns `{ ok, filename }`
- `POST /api/v1/recordings/stop`
  - returns `{ ok, filename }`
- `GET /api/v1/waveform`
  - returns `{ ok, n, points_i16, window_ms, sample_rate_hz }`
- `GET /api/v1/recordings`
  - returns `{ ok, recordings: [{ name, size_bytes, created_ms? }] }`
- `GET /api/v1/recordings/:name`
  - returns WAV file with `Content-Type: audio/wav`
- `DELETE /api/v1/recordings/:name`
  - returns `{ ok }`

### Web UI (MVP)

Single-page minimal UI:
- Connect instructions (SSID shown on device screen, if available).
- Buttons: **Start**, **Stop**, **Refresh list**.
- Table of recordings with **Download** and in-browser **Play** (using `<audio controls>` on the WAV URL).
- Status indicator: recording/not recording; file name; bytes written.
- Live waveform preview (canvas) refreshed every **500ms** (driven by `GET /api/v1/waveform`).

### Concurrency model (important)

- Web handlers **must not** perform long/blocking I2S or heavy FS loops.
- One owner for the recording state:
  - `Recorder::Start()` transitions IDLE → RECORDING
  - audio task runs while recording
  - `Recorder::Stop()` transitions RECORDING → STOPPING → IDLE (after header finalize)

Suggested state machine:

```cpp
enum class RecState { Idle, Recording, Stopping, Error };
```

Shared state access:
- Use a mutex (or critical section) around state & metadata reads.
- Use a queue or ring buffer between capture and writer if file writes can block.

### File format

WAV (PCM) parameters:
- **PCM**, little-endian
- sample rate: **16000 Hz**
- channels: **1**
- bits per sample: **16**

WAV header must be finalized after stop (sizes depend on total bytes).

## Waveform Preview (Phase 1.5)

We add a small “am I getting signal?” UI element without implementing live audio streaming.

### Contract

- Endpoint: `GET /api/v1/waveform`
- Purpose: return a **fixed-size** downsampled snapshot of recent microphone samples suitable for plotting.
- Polling: browser polls every **500ms**.
- Payload size target: **≤ 2–4 KB** per request.

Example response:

```json
{
  "ok": true,
  "sample_rate_hz": 16000,
  "window_ms": 1000,
  "n": 256,
  "points_i16": [0, 12, -24, 80, -120]
}
```

`points_i16` represents PCM sample amplitudes in the range `[-32768, 32767]`, pre-downsampled to `n` points.

### Firmware strategy (recommended)

Maintain a small circular buffer of the most recent samples (or peaks), independent from the file writer:

- Capture task appends samples into `waveform_ring`.
- On request, compute `n` bucket peaks (or average) over the last `window_ms`.
- Never block on I2S reads inside the HTTP handler; compute from already-captured data.

### UI rendering guidance

- Draw a horizontal center line.
- Map each value `v` to `y = mid - (v / 32768.0) * (height/2 - padding)`.
- Optionally show min/max/peak for quick “mic working” debugging.

## Design Decisions

### 1) Use ESPAsyncWebServer (Arduino) for MVP

- **Rationale**: We already have working patterns and assets-serving examples in `ATOMS3R-CAM-M12-UserDemo`. The intended target (“Atoms3r like in `M5AtomS3-UserDemo`”) is Arduino/PlatformIO-friendly.
- **Implication**: Prefer `initArduino()` + `WiFi.softAP()` + `AsyncWebServer` + `beginResponse_P()` patterns.

### 2) SoftAP-first networking

- **Rationale**: eliminates dependency on external WiFi credentials; easiest demo flow.
- **Future**: support STA/AP+STA later (config UI or serial provisioning).

### 3) WAV over “raw PCM” for storage/download

- **Rationale**: Browser-native playback via `<audio>` without custom JS decoders.
- **Trade-off**: need to patch WAV header at end; slightly more code.

### 4) 16 kHz / 16-bit / mono as a fixed MVP contract

- **Rationale**: voice memos; manageable bandwidth/storage; matches existing patterns.
- **Future**: allow optional 8k/32k, but keep compatibility explicit in API.

### 5) Separate audio capture task from web server

- **Rationale**: avoid dropouts and handler timeouts; keep latency predictable.

## Alternatives Considered

### A) esp_http_server (native ESP-IDF)

- **Pros**: fewer Arduino dependencies; used in `esp32-s3-m5/0017-atoms3r-web-ui`.
- **Cons**: target project style here is Arduino/PlatformIO; would increase integration cost.

### B) Live Opus streaming to browser

- **Pros**: smaller bandwidth; closer to “real-time”.
- **Cons**: browser decoding + framing complexity; requires more JS/audio plumbing; not needed for MVP.

### C) Store raw PCM + metadata JSON

- **Pros**: simplest write loop; no header patching.
- **Cons**: browser playback not trivial; adds tooling burden.

## Implementation Plan

### Phase 0: Groundwork

- Decide FS backend: LittleFS vs SPIFFS (prefer LittleFS).
- Validate I2S pins and mic hardware on the target board revision.

### Phase 1 (MVP): Record + list + download/play

- Implement `Recorder` module:
  - I2S init/config
  - capture task (fixed-size frames)
  - WAV writer (open/append/finalize)
- Implement REST API endpoints listed above.
- Serve minimal frontend assets.
- Manual testing: record 5–10 seconds, stop, download, play in browser.

### Phase 2: UX + reliability

- Add automatic file naming (monotonic counter).
- Add delete endpoint and UI button.
- Add “max recordings / storage quota” policy.
- Add better status: estimated duration, free space.

### Phase 3 (optional): Live monitor

- Add `WS /api/v1/ws/audio` to stream PCM frames.
- Implement browser-side playback (AudioWorklet) OR waveform visualization only.

## Open Questions

- Exact target board variant:
  - Is this **M5AtomS3** (mic pins?) or **AtomS3R + EchoBase (ES8311)**?
  - If EchoBase/ES8311 is involved, we may reuse the codec init path from `echo-base--openai-realtime-embedded-sdk/src/media.cpp`.
- Storage backend availability in the chosen Arduino core:
  - prefer LittleFS; confirm it’s enabled in the build environment.
- Recording length expectations:
  - short memos (<1–2 min) vs long recordings (flash wear, space, performance).

## References

- Ticket analysis: `analysis/01-audio-recorder-architecture-analysis-files-symbols-and-concepts.md`
- I2S + Opus reference: `echo-base--openai-realtime-embedded-sdk/src/media.cpp`
- ESPAsyncWebServer patterns: `ATOMS3R-CAM-M12-UserDemo/main/service/service_web_server.cpp`
- Native HTTP server (alternative): `esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp`
