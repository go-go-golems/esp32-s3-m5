# Tasks

## TODO

- [ ] Implement **waveform display** in the web UI (canvas) that refreshes every **500ms**
- [ ] Add waveform API endpoint (MVP): `GET /api/v1/waveform` returning a fixed-size downsampled snapshot suitable for plotting (e.g., 128 or 256 points, int16 or normalized floats)
- [ ] Wire waveform UI polling: fetch `/api/v1/waveform` every 500ms, render waveform, and show basic signal stats (min/max/RMS optional)
- [ ] Confirm target board + mic hardware (M5AtomS3 vs AtomS3R+EchoBase); document I2S pins + sample rate constraints
- [ ] Choose firmware stack for MVP: Arduino + ESPAsyncWebServer; verify deps and build flow
- [ ] Serve frontend assets from flash (index.html + app.js + app.css) and ensure correct Content-Type headers
- [ ] Implement Recorder module + state machine (Idle/Recording/Stopping/Error) and dedicated audio capture task (I2S -> file)
- [ ] Decide and implement filesystem backend (prefer LittleFS; fallback SPIFFS); define recordings directory and quota policy
- [ ] Implement WAV writer (PCM16 mono @16kHz): write placeholder header, append frames, finalize header sizes on stop
- [ ] Implement REST API (MVP): /api/v1/status, /api/v1/recordings/start, /stop, /recordings (list), /recordings/:name (download), DELETE /recordings/:name
- [ ] Build web UI MVP: start/stop controls, status polling, list recordings, download + <audio> playback
- [ ] Add file naming strategy (monotonic counter) and expose metadata (bytes, duration estimate) in /api/v1/status and /api/v1/recordings
- [ ] (Optional) Add live monitor: WS /api/v1/ws/audio streaming PCM frames; browser waveform or AudioWorklet playback
- [x] Create a playbook doc: manual test steps for recording, downloading, and playback; include expected JSON + headers
- [x] Write waveform API contract doc: `reference/02-waveform-api-contract.md`
