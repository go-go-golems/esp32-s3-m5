# 0101 — ESP32-P4 Native QuickJS

Ticket: `ESP32-P4-NATIVE-QUICKJS`.

This firmware compiles upstream QuickJS directly into ESP-IDF for the ESP32-P4. It is the native/raw follow-up to `0100-esp32-p4-quickjs-wasm`, which ran QuickJS through WAMR.

## Current status

Interactive console firmware: start `components/qjs_service`, which owns one native `JSRuntime`/`JSContext` on a FreeRTOS owner task, install `print`, `millis`, and `gc`, and expose UART0 console commands.

Available commands:

```text
js status
js eval <source>
js reset
js gc
js bench
```

Verified on the ESP32-P4 hardware:

- native service runtime init: about 6 ms
- `js eval "print(1+2)"`: captured output `3`
- `js eval "throw new Error('boom')"`: reports `Error: boom`
- `js reset`: `ESP_OK`
- `js gc`: completes successfully
- `js bench`:
  - 10k integer loop: about 13 ms roundtrip / 11 ms JS-side
  - 100k integer loop: about 133 ms roundtrip / 133 ms JS-side
  - `fib(20)`: about 32 ms roundtrip / 31 ms JS-side
- `js eval "while(true){}"`: interrupted after about 1000 ms with `InternalError: interrupted` and `timed_out=1`

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

Expected startup output includes:

```text
I (...) qjs_service: runtime init status=ESP_OK elapsed=6 ms
I (...) 0101: QuickJS ready. Try: js eval "print(1+2)" or js bench
0101>
```

Example command output:

```text
0101> js eval "print(1+2)"
[console-eval] ok=1 timed_out=0 elapsed=2ms
3

0101> js bench
[bench-10k] ok=1 timed_out=0 elapsed=13ms
sum10k=11,s=49995000
[bench-100k] ok=1 timed_out=0 elapsed=133ms
sum100k=133,s=4999950000
[bench-fib20] ok=1 timed_out=0 elapsed=32ms
fib20=6765,ms=31
```

## Notes

- The service owner task uses a 32 KiB stack. A smaller 12 KiB stack crashed during `fib(20)` with a FreeRTOS stack protection fault in recursive QuickJS execution.
- Console eval timeout is 1000 ms so `while(true){}` interrupts before the task watchdog fires.
