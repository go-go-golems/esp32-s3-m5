# Tutorial 0030: Cardputer console + esp_event bus

Small demo firmware for ESP-IDF on the M5Stack Cardputer (ESP32-S3):

- Keyboard key edges are posted as `esp_event` events.
- An `esp_console` REPL can post custom events into the same event bus.
- A UI task dispatches the event loop and renders received events on screen.

## Build / flash

```bash
cd 0030-cardputer-console-eventbus
idf.py set-target esp32s3
idf.py build flash monitor
```

## Console usage

When you see the `bus>` prompt:

```text
bus> help
bus> evt post hello
bus> evt spam 10
bus> evt status
bus> evt monitor on
bus> evt monitor off
bus> evt pb on
bus> evt pb last
bus> evt pb off
bus> evt clear
```

If `idf.py monitor` can't send input to the device, use a real serial terminal (e.g. `picocom`/`minicom`) on the same port.
