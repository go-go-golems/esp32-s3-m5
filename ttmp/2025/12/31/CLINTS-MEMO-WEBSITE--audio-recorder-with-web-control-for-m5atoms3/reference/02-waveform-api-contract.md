---
Title: Waveform API Contract (MVP)
Ticket: CLINTS-MEMO-WEBSITE
Status: active
Topics:
    - audio-recording
    - web-server
    - esp32-s3
    - m5atoms3
    - i2s
    - waveform
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: Concrete contract for a lightweight waveform snapshot endpoint used by the web UI for live signal visualization without audio streaming.
LastUpdated: 2025-12-31T13:58:13-05:00
WhatFor: Align firmware and web UI on a small, stable payload for plotting a waveform preview (polling every 500ms).
WhenToUse: Use when implementing `/api/v1/waveform` and the corresponding canvas waveform in the UI.
---

# Waveform API Contract (MVP)

## Endpoint

- Method: `GET`
- Path: `/api/v1/waveform`
- Response: JSON

## Goals

- **Fast**: should respond in a few milliseconds; never blocks on I2S.
- **Small**: fixed-size payload (≤ 2–4 KB).
- **Simple**: browser can plot with no binary parsing.

## Response (success)

```json
{
  "ok": true,
  "sample_rate_hz": 16000,
  "window_ms": 1000,
  "n": 256,
  "points_i16": [0, 12, -24, 80, -120]
}
```

Field meanings:

- `sample_rate_hz` (number): microphone sampling rate used to interpret the window.
- `window_ms` (number): approximate time span represented by the snapshot.
- `n` (number): number of points returned; must equal `points_i16.length`.
- `points_i16` (array<number>): downsampled amplitude points, each in `[-32768, 32767]`.

Downsampling rules (recommended):

- Use the most recent `window_ms` of samples available.
- Convert to `n` points using **bucket peak** (peak absolute value, or peak signed sample) to preserve transients.
- If there is insufficient data yet, return `n` points but allow zeros/partial fill.

## Response (not ready / error)

Prefer to keep a stable shape even if recording is not active:

```json
{
  "ok": true,
  "sample_rate_hz": 16000,
  "window_ms": 1000,
  "n": 256,
  "points_i16": [0, 0, 0, 0, 0]
}
```

If returning an error, use:

```json
{ "ok": false, "error": "message" }
```

## Notes

- This endpoint is intentionally independent of “recording state”; it’s a **monitoring** view, not a “stream audio” feature.
- If we later add a binary format for performance, keep this JSON contract as the default for compatibility unless explicitly deprecated.
