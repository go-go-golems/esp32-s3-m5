# 0101 — ESP32-P4 Native QuickJS

Ticket: `ESP32-P4-NATIVE-QUICKJS`.

This firmware compiles upstream QuickJS directly into ESP-IDF for the ESP32-P4. It is the native/raw follow-up to `0100-esp32-p4-quickjs-wasm`, which ran QuickJS through WAMR.

## Current status

Phase 1 smoke scaffold: create a native `JSRuntime`/`JSContext`, install `print`, evaluate `print(1+2)`, and free the runtime.

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
idf.py -p /dev/ttyACM0 flash monitor
```

Expected smoke output includes:

```text
0101_qjs: native eval: print(1+2)
3
0101_qjs: native eval ok
```
