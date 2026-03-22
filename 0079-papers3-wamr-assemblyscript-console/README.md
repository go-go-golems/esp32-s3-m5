# Tutorial 0079 - PaperS3 WAMR AssemblyScript Console

This project is a standalone `PaperS3` runtime demo for `esp32-s3-m5`.

It is intended to host a small curated set of precompiled AssemblyScript programs
that are compiled into WebAssembly on the host and then executed on-device
through `esp_console`.

Current milestone:

- PaperS3 scaffold
- USB Serial/JTAG console baseline
- WAMR runtime service
- `wasm` runtime status output

Planned next milestones:

- host-side AssemblyScript build pipeline
- embedded wasm demo registry
- first end-to-end runnable demo

## Build

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console
source /home/manuel/esp/esp-idf-5.3.4/export.sh
idf.py set-target esp32s3
idf.py build
```

## Flash + Monitor

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console
source /home/manuel/esp/esp-idf-5.3.4/export.sh
idf.py -p /dev/ttyACM0 flash monitor
```

## Notes

- The project intentionally uses USB Serial/JTAG for the interactive console.
- WAMR is planned as an ESP-IDF component-manager dependency from upstream.
- AssemblyScript source code will live under `wasm-src/`.

## Build AssemblyScript Demos

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/wasm-src
npm install
npm run build
```

Release artifacts are written to `../wasm-build/release/`.
