# AtomS3R Camera Stream Firmware

ESP-IDF firmware that connects an AtomS3R-CAM to the Go streaming server at `ws://<host>:<port>/ws/camera` and sends JPEG frames over WebSocket.

## Build / Flash

```bash
cd 0040-atoms3r-cam-streaming/esp32-camera-stream/firmware
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

## Console setup

This firmware uses `esp_console` over USB Serial/JTAG. Use the REPL to configure WiFi and stream target:

```text
cam> wifi scan
cam> wifi set <ssid> <password> save
cam> wifi connect
cam> wifi status

cam> stream set host <server-ip> 8080 save
cam> stream start
cam> stream status
```

To clear saved values:

```text
cam> wifi clear
cam> stream clear
```

## Notes

- Stream endpoint must be reachable at `ws://<host>:<port>/ws/camera`.
- Each WebSocket binary message is a full JPEG frame (no extra headers).
- Uses PSRAM for camera frame buffers; `sdkconfig.defaults` enables it.
