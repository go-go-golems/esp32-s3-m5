# Tutorial 0028: Cardputer `esp_event` demo

Demo firmware for ESP-IDF `esp_event` on the M5Stack Cardputer (ESP32-S3):

- Keyboard key edges are posted as `esp_event` events.
- A heartbeat task posts periodic memory statistics events.
- 0–8 random-producer tasks post random events in parallel.
- A UI task dispatches the event loop and renders the received events on screen.

## Build / flash

```bash
cd 0028-cardputer-esp-event-demo
idf.py set-target esp32s3
idf.py build flash monitor
```

## UI controls

- `Fn+W/S`: scroll the event log (via captured nav bindings)
- `Fn+Space`: pause/resume appending new log lines
- `Del`: clear the log
- `Fn+1`: jump scrollback back to tail (via `Back` action)

