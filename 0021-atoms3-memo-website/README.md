# 0021 — AtomS3 Memo Website (CLINTS-MEMO-WEBSITE)

ESP32-S3 firmware project (ESP-IDF) for a device-hosted “memo recorder” web UI.

## Build

From this directory:

```bash
./build.sh
```

## Flash + monitor

```bash
./build.sh -p /dev/ttyACM0 flash monitor
```

## tmux helper (optional)

This opens 2 panes: left = flash, right = monitor.

```bash
./build.sh tmux-flash-monitor -p /dev/ttyACM0
```


