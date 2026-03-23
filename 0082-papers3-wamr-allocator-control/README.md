# Tutorial 0082 - PaperS3 WAMR Allocator Control

This project is a stripped `PaperS3` control harness for isolating the WAMR
instantiate-vs-PSRAM contamination bug.

It intentionally removes the demo display stack and keeps only:

- USB Serial/JTAG console
- WAMR runtime initialization and status
- minimal embedded Wasm modules (`return-42`, `log-only`)
- instantiate lifecycle probes
- PSRAM and internal-RAM touch controls

## Build

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0082-papers3-wamr-allocator-control
source /home/manuel/esp/esp-idf-5.3.4/export.sh
idf.py set-target esp32s3
idf.py build
```

## Flash + Monitor

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0082-papers3-wamr-allocator-control
source /home/manuel/esp/esp-idf-5.3.4/export.sh
idf.py -p /dev/ttyACM0 flash monitor
```

## Notes

- The project intentionally uses USB Serial/JTAG for the interactive console.
- This firmware is for debugging, not for end-user demos.
- Display initialization and display host imports are deliberately removed.
- The main control matrix is `psram-persistent-init` / `psram-persistent-touch-sync`
  before and after `instantiate-bare-keepalive return-42`.
- Allocator A/B is supported through `sdkconfig.defaults`, `sdkconfig.system_allocator`, and `sdkconfig.internal_pool`.
