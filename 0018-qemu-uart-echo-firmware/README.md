# Track C — ESP32-S3 QEMU UART Echo Firmware (0018)

This is a minimal ESP-IDF firmware used to test whether ESP32-S3 QEMU can deliver **UART RX** bytes into ESP-IDF at all. It intentionally avoids MicroQuickJS, SPIFFS, and any REPL parsing. It should:

- log a heartbeat (“still alive”) when no bytes are received
- echo any received bytes back to UART0 using `uart_write_bytes()`

## Build (ESP-IDF 5.4.x)

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0018-qemu-uart-echo-firmware && \
./build.sh set-target esp32s3 && \
./build.sh build
```

## Run in QEMU

### Option A: QEMU with stdio serial (`mon:stdio`)

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0018-qemu-uart-echo-firmware && \
./build.sh qemu
```

If QEMU starts with `-serial mon:stdio`, you should be able to type into the terminal and see bytes echoed back.

### Option B: QEMU + `idf_monitor` (`tcp::5555,server`)

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0018-qemu-uart-echo-firmware && \
./build.sh qemu monitor
```

This mode typically runs QEMU with `-serial tcp::5555,server` and attaches `idf_monitor` to `socket://localhost:5555`.

## What to look for

- If you type `abc<enter>` and see `abc` echoed (plus logs like `RX N bytes`), **UART RX works** in QEMU.
- If you only see heartbeat logs and never see `RX ...`, **UART RX is not reaching the firmware** (likely a QEMU limitation or serial backend mismatch).


