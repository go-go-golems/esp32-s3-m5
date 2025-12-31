# 0021 — AtomS3 Memo Website (CLINTS-MEMO-WEBSITE)

ESP32-S3 firmware project (ESP-IDF) for a device-hosted “memo recorder” web UI.

## Build

From this directory:

```bash
./build.sh
```

## Configure WiFi STA (recommended)

This firmware will connect to WiFi STA if `CONFIG_CLINTS_MEMO_WIFI_STA_SSID` is set (otherwise it starts SoftAP).

To set credentials locally (without committing secrets), edit `sdkconfig` via script:

```bash
WIFI_SSID='YourNetwork' WIFI_PASSWORD='YourPassword' ./tools/set_wifi_sta_creds.sh
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

