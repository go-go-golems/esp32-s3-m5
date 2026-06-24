# 0101 — ESP32-P4 Native QuickJS

Ticket: `ESP32-P4-NATIVE-QUICKJS`.

This firmware compiles upstream QuickJS directly into ESP-IDF for the ESP32-P4. It is the native/raw follow-up to `0100-esp32-p4-quickjs-wasm`, which ran QuickJS through WAMR.

## Current status

Service smoke scaffold: start `components/qjs_service`, which owns one native `JSRuntime`/`JSContext` on a FreeRTOS owner task, install `print`, `millis`, and `gc`, evaluate small scripts, report status, and reset the runtime.

Verified on the ESP32-P4 hardware:

- native service runtime init: about 6 ms
- `print(1+2)` captured output: `3`
- 10k integer loop: about 14 ms JavaScript-side
- exception formatting: `Error: boom`
- service reset: `ESP_OK`

## Build

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0101-esp32-p4-native-quickjs
source /home/manuel/esp/esp-idf-5.4.2/export.sh
idf.py set-target esp32p4
idf.py build
```

## Flash / monitor

Use a single owner for `/dev/ttyACM0`.

```bash
idf.py -p /dev/ttyACM0 flash
idf.py -p /dev/ttyACM0 monitor
```

In non-interactive agent sessions, run `idf.py monitor` inside tmux, capture the pane, and kill the tmux session afterward so the serial port is released.

Expected service smoke output includes:

```text
I (...) qjs_service: runtime init status=ESP_OK elapsed=6 ms
I (...) 0101_qjs: eval boot-smoke result: ok=1 timed_out=0 elapsed=2ms
3
I (...) 0101_qjs: eval sum10k result: ok=1 timed_out=0 elapsed=14ms
sum10k=14,s=49995000
E (...) 0101_qjs: eval exception exception: Error: boom
I (...) 0101_qjs: reset: ESP_OK
```
