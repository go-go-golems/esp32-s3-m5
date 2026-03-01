# Tutorial 0071 - Cardputer-ADV Photo Development Timer

Firmware for Cardputer-ADV that runs film-development timers with encoder navigation,
boot-loaded preset JSON config, and a small device-hosted web UI for preset upload.

## Features

- Chain Encoder (U207 over Grove UART) navigation
- LVGL on Cardputer-ADV display
- Step-based timer engine (start/pause/resume/next/reset)
- Settings view with live Wi-Fi IP display
- SPIFFS-backed preset config (`/spiffs/presets.json`)
- Wi-Fi + HTTP API + embedded minimal web page for preset upload/edit
- `esp_console` over USB Serial/JTAG (`wifi` commands via shared `wifi_console`)
- `screenshot` console command for host-captured QOI UI snapshots

## Build

```bash
./build.sh build
```

## Flash + Monitor

```bash
./build.sh -p /dev/ttyACM0 flash monitor
```

## First-boot behavior

If `/spiffs/presets.json` does not exist, firmware seeds default presets (including a C41 CineStill-style preset) and sets it active.

## HTTP endpoints

- `GET /api/status`
- `GET /api/presets`
- `POST /api/presets`
- `POST /api/control`

Open the device root page (`/`) after Wi-Fi connect to edit/upload preset JSON.

## Screenshot Capture (Host)

```bash
python3 ../components/screenshot_qoi/tools/capture_screenshot_qoi_from_console.py \
  /dev/serial/by-id/<device> \
  /tmp/0071-ui.qoi \
  --bmp-out /tmp/0071-ui.bmp
```

The script sends `screenshot` to the device console and writes framed QOI bytes (`QOI_BEGIN/QOI_END`), optionally decoding to a BMP for quick visual checks.
