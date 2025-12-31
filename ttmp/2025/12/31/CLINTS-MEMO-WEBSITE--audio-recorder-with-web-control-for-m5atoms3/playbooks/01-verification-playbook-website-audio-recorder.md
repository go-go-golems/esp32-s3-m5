---
Title: "Verification playbook: Website Audio Recorder (MVP)"
Ticket: CLINTS-MEMO-WEBSITE
Status: active
Topics:
    - audio-recording
    - web-server
    - esp32-s3
    - m5atoms3
    - i2s
    - wav
    - waveform
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/ttmp/2025/12/31/CLINTS-MEMO-WEBSITE--audio-recorder-with-web-control-for-m5atoms3/design-doc/01-website-audio-recorder-design-document.md
      Note: MVP contract and endpoints
    - Path: esp32-s3-m5/ttmp/2025/12/31/CLINTS-MEMO-WEBSITE--audio-recorder-with-web-control-for-m5atoms3/reference/02-waveform-api-contract.md
      Note: `/api/v1/waveform` response shape and semantics
    - Path: esp32-s3-m5/ttmp/2025/12/31/CLINTS-MEMO-WEBSITE--audio-recorder-with-web-control-for-m5atoms3/tasks.md
      Note: Implementation checklist for the MVP
ExternalSources: []
Summary: "End-to-end manual verification steps for the MVP: connect to device, start/stop recording, list/download/delete WAV files, and confirm waveform preview updates."
LastUpdated: 2025-12-31
WhatFor: "Validate the firmware + web UI integration on real hardware before calling the MVP done."
WhenToUse: "After implementing the REST endpoints/UI, or after any changes to I2S, filesystem, or HTTP handlers."
---

# Verification playbook: Website Audio Recorder (MVP)

## Success criteria (MVP)

- Browser loads the device-hosted UI from `/`.
- `POST /api/v1/recordings/start` begins recording and status reflects it.
- `POST /api/v1/recordings/stop` finalizes a valid **WAV** file.
- `GET /api/v1/recordings` lists the new recording.
- `GET /api/v1/recordings/:name` downloads with `Content-Type: audio/wav` and plays in the browser.
- `DELETE /api/v1/recordings/:name` removes the file (and list updates).
- `GET /api/v1/waveform` returns fixed-size points and the UI waveform updates (every ~500ms).

## Prereqs

- Target board flashed with the firmware that implements the MVP endpoints.
- A client device (phone/laptop) and a modern browser.
- Optional but helpful: `curl` on the client for direct API checks.

## Config assumptions / defaults

- Device is running **SoftAP** (MVP default).
- Base URL for SoftAP mode is typically `http://192.168.4.1`.
- Audio contract: **WAV PCM16 mono @ 16kHz**.
- Waveform contract: JSON response with `n=256` and `points_i16` (see reference doc).

## Step 0: Determine the base URL

Pick one base URL and reuse it below:

- **SoftAP mode**:
  - Connect your client to the device SSID.
  - Set: `DEVICE_URL=http://192.168.4.1`
- **STA (DHCP) mode** (if supported):
  - Find the printed IP in serial logs.
  - Set: `DEVICE_URL=http://<sta-ip>`

## Step 1: Confirm base HTTP + status endpoint

```bash
curl -sS ${DEVICE_URL}/api/v1/status | jq
```

Expect:

- `"ok": true`
- `"recording": false` on a fresh boot
- `sample_rate_hz=16000`, `channels=1`, `bits_per_sample=16` (or explicit equivalents)

## Step 2: Confirm UI loads from embedded assets

1. Open `${DEVICE_URL}/` in a browser.
2. Confirm the page renders and controls are visible (Start/Stop/List).
3. In browser devtools Network tab, confirm assets load with appropriate Content-Types.

## Step 3: Start recording (API)

```bash
curl -sS -X POST ${DEVICE_URL}/api/v1/recordings/start | jq
```

Expect:

- `"ok": true`
- a `"filename"` is returned

Then confirm status flips:

```bash
curl -sS ${DEVICE_URL}/api/v1/status | jq
```

Expect `"recording": true` and `bytes_written` increasing over time.

## Step 4: Confirm waveform is updating during recording

Run this a few times ~500ms apart:

```bash
curl -sS ${DEVICE_URL}/api/v1/waveform | jq
```

Expect:

- `"ok": true`
- `"n": 256`
- `points_i16` is length 256
- Values are not all zeros when speaking near the mic

## Step 5: Stop recording

```bash
curl -sS -X POST ${DEVICE_URL}/api/v1/recordings/stop | jq
```

Expect `"ok": true` and status returns to `"recording": false`.

## Step 6: List recordings

```bash
curl -sS ${DEVICE_URL}/api/v1/recordings | jq
```

Expect the filename from Step 3 appears with `size_bytes > 44` (WAV header + samples).

## Step 7: Download WAV and verify headers

Download:

```bash
NAME="<recording-name-from-list>"
curl -fLo ${NAME} ${DEVICE_URL}/api/v1/recordings/${NAME}
```

Check headers:

```bash
curl -sSI ${DEVICE_URL}/api/v1/recordings/${NAME}
```

Expect:

- `Content-Type: audio/wav`
- (Optional) `Content-Disposition: attachment; filename="..."`

Playback:

- In browser: open `${DEVICE_URL}/api/v1/recordings/${NAME}` and confirm the audio plays, or use the UI’s `<audio>` playback.

## Step 8: Delete recording

```bash
curl -sS -X DELETE ${DEVICE_URL}/api/v1/recordings/${NAME} | jq
curl -sS ${DEVICE_URL}/api/v1/recordings | jq
```

Expect:

- delete returns `"ok": true`
- the file no longer appears in the list

## Negative tests (recommended)

- Start while already recording:
  - `POST /api/v1/recordings/start` should return `{ ok:false, error }` (or be idempotent and return the active filename).
- Stop while not recording:
  - `POST /api/v1/recordings/stop` should return `{ ok:false, error }` (or be idempotent with no-op semantics).
- Download missing file:
  - `GET /api/v1/recordings/nope.wav` should return 404.

## Exit criteria

In one run, all success criteria pass, and the downloaded WAV plays without artifacts.
