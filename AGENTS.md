# Agent Instructions (esp32-s3-m5)

> Full build/dev-environment detail lives in **`docs/01-playbook-esp-idf-build-and-dev-environment.md`**. The rules below are the concise version every coding agent must follow before running `idf.py`.

## Build and dev environment (concise rules)

- **Source the IDF version the project pins.** Multiple IDFs live under `~/esp/esp-idf-*` (5.3.4, 5.4.2, 5.5.4, …). ESP32-S3 WAMR projects (`0079`/`0082`) use **5.3.4**; ESP32-P4 projects (`0097`–`0100`) use **5.4.2**. Read the project `README.md`; then `source ~/esp/esp-idf-<ver>/export.sh`. A wrong version yields Kconfig/link errors that look like code bugs.
- **Set the target once:** `idf.py set-target esp32p4|esp32s3|esp32c6|…`. Use `idf.py build` for normal rebuilds.
- **Component dependencies go in `main/idf_component.yml`**, not a project-root `idf_component.yml` (a root manifest is ignored → `Failed to resolve component ... unknown name`).
- **`sdkconfig.defaults` only seeds absent options.** To force a defaults change (partition table, console, PSRAM): `rm -f sdkconfig && idf.py build`. `idf.py fullclean` removes `build/` and `managed_components/` but **not** `sdkconfig` — this trap costs rebuild cycles.
- **Custom partition table for large apps.** Apps embedding a big asset (Wasm, fonts) overflow the default 1 MB `factory`. Add `partitions.csv` and set `CONFIG_PARTITION_TABLE_CUSTOM=y` + `CONFIG_PARTITION_TABLE_CUSTOM_FILENAME` + `CONFIG_PARTITION_TABLE_FILENAME`; then `rm sdkconfig`. Reference: `0100-esp32-p4-quickjs-wasm`.
- **Console is target-specific** (see the S3 note below; the P4 is the exception): S3 → USB Serial/JTAG; **ESP32-P4 has no USB Serial/JTAG** — it uses UART0 via the CH343 bridge (`CONFIG_ESP_CONSOLE_UART_DEFAULT=y`, GPIO37/38).
- **Flash port:** prefer `/dev/serial/by-id/...` (P4 PicoCalc: `usb-1a86_USB_Single_Serial_*-if00`). Do not run two monitors/flashers on the same port (see Serial ownership below).
- **Commit source, not artifacts.** `build/`, `managed_components/`, `sdkconfig`, `**/wasm-build/`, `**/wasm-src/quickjs/` are gitignored. Deliberate embedded assets (e.g. `main/quickjs.wasm`) are committed; `dependencies.lock` is committed for reproducibility. Verify with `git check-ignore -v <path>`.

## Console default: USB Serial/JTAG (ESP32-S3 only)

Prefer the USB Serial/JTAG console for all interactive REPL work (`esp_console`) on **Cardputer/ESP32-S3** projects, because UART pins are frequently repurposed for peripherals (keyboard/Grove/UART-to-NCP links) and UART console output can corrupt protocol traffic.

> **ESP32-P4 exception:** the P4 has no USB Serial/JTAG. P4 boards (PicoCalc/Waveshare ESP32-P4-WIFI6) use an external CH343 USB-UART bridge on UART0 (GPIO37/GPIO38); set `CONFIG_ESP_CONSOLE_UART_DEFAULT=y`. The S3 USB-Serial/JTAG rule above does not apply to the P4.

Recommended baseline in `sdkconfig.defaults` for firmwares that use a console:

```
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y
# CONFIG_ESP_CONSOLE_UART is not set
```

If you must use UART console, explicitly document which UART/pins are reserved and ensure it does not overlap with any protocol UART (e.g. host↔NCP SLIP link).

## Serial ownership during flash/probe work

Treat each `/dev/tty*` device as single-owner during ESP-IDF flashing, monitoring, and scripted probing.

- Do not run multiple `idf.py flash`, `idf.py monitor`, `idf_monitor.py`, or custom serial probe scripts against the same serial device in parallel.
- Before starting a new flash/probe session, clear stale monitor holders for that exact port.
- If a serial experiment needs multiple commands, prefer a single boot session with one probe script instead of several competing processes.

This matters on the ESP32-S3 USB Serial/JTAG path because parallel access creates misleading failures:

- serial write timeouts
- missing or interleaved console output
- false prompt-detection failures
- confusing “crash” evidence that is actually port contention

## Manual reset coordination

If a board is known to require a human reset or boot-button step during attach, ask the user explicitly before the live probe step instead of repeatedly retrying serial opens.

- Do not keep probing silently if the current evidence suggests the device needs a manual reset to leave ROM download mode or to boot the flashed app.
- Keep the serial/monitor session open first when that is part of the recovery procedure, then ask the user to press reset.
- Record the board-specific attach/reset behavior in the active ticket diary so future sessions do not rediscover it the hard way.
