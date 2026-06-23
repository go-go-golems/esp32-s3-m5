# 0100 — ESP32-P4 QuickJS-WASM (run QuickJS compiled to WASM via WAMR)

Ticket **ESP32-P4-QUICKJS-WASM**. This is the firmware target for running the QuickJS JavaScript
engine — compiled to a WebAssembly module — sandboxed inside the **WAMR** runtime on the
**ESP32-P4** (PicoCalc / Waveshare ESP32-P4-WIFI6 board).

## Read this first

The complete analysis, design, API references, pseudocode, diagrams, and phased plan live in:

```
ttmp/2026/06/23/ESP32-P4-QUICKJS-WASM--run-quickjs-compiled-to-wasm-on-the-esp32-p4-intern-implementation-guide/design/01-quickjs-wasm-esp32p4-analysis-design-and-implementation-guide.md
```

Read it end to end before editing this firmware. The architecture is **JS-in-WASM-in-WAMR** with two
host boundaries (WAMR↔wasm, and QuickJS↔user JS).

## Scaffold status

This scaffold builds a minimal console-only firmware now. The intern must:

1. **Build `quickjs.wasm` on a host PC** (see `wasm-src/`), then copy it to `main/quickjs.wasm`.
2. Port `wasm_runtime_service` + `wasm_host_api` + `wasm_runner` from `0079` (pseudocode in the
   design doc §7), and uncomment the WAMR `REQUIRES` + `EMBED_FILES` lines in `main/CMakeLists.txt`.
3. Implement `js eval` / `js repl` / `js status` (design §5.8, §7.4).

## Build / flash / monitor

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0100-esp32-p4-quickjs-wasm
source ~/esp/esp-idf-5.4.2/export.sh
idf.py set-target esp32p4
idf.py build
PORT=/dev/serial/by-id/usb-1a86_USB_Single_Serial_*-if00   # CH343 USB-UART bridge (from 0099)
idf.py -p "$PORT" flash monitor
```

## Console (UART0 — P4 has no USB Serial/JTAG)

```
0100> js status          # WAMR runtime + pool/heap status (stub until wired)
0100> js eval "print(1+2)"   # after Phase 1: prints 3
```

## Layout

```
0100-esp32-p4-quickjs-wasm/
├── CMakeLists.txt            # project() esp32p4
├── sdkconfig.defaults        # P4 console/PSRAM + WAMR toggles
├── partitions.csv            # 4 MB factory app
├── idf_component.yml         # espressif/wasm-micro-runtime 2.4.0~1
├── main/
│   ├── CMakeLists.txt        # minimal now; WAMR + EMBED quickjs.wasm commented
│   └── app_main.cpp          # console + banner + js status stub
└── wasm-src/                 # HOST build of quickjs.wasm (not built by ESP-IDF)
    ├── README.md
    ├── wasm_main.c           # reactor wrapper: qjs_init / qjs_eval (from design §7.1)
    └── build-quickjs-wasm.sh # one-shot wasi-sdk build
```
