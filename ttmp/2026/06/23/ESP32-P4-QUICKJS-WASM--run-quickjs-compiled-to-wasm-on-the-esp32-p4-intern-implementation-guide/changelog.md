# Changelog

## 2026-06-23

- Created ticket ESP32-P4-QUICKJS-WASM + added `esp32p4` vocabulary topic.
- Harvested 14 primary research sources into `sources/` via defuddle (HTML) + curl (raw GitHub md/header): WAMR `wasm_export.h`, `embed_wamr.md`, `export_native_api.md`, `build_wamr.md`; QuickJS home; wasi-sdk README; vercel-labs/quickjs-wasi; ESP32-P4 memory-types; WAMR tutorial; wamr-app-framework.
- Analyzed sources and local prior art (0079/0082 WAMR embedding, 0099 ESP32-P4 target).
- Wrote intern design/implementation guide `design/01-quickjs-wasm-esp32p4-...-guide.md` (architecture, two-host-boundaries concept, build pipeline, API references, pseudocode, diagrams, phased plan, decision records).
- Scaffolded firmware `0100-esp32-p4-quickjs-wasm` (CMakeLists, sdkconfig.defaults, partitions.csv, idf_component.yml, README, main/ stub, wasm-src/ with wasm_main.c + build script).
- Uploaded bundle (design + diary) to reMarkable at `/ai/2026/06/23/ESP32-P4-QUICKJS-WASM`.

## Key decisions

- DR-1: Build QuickJS as a WASI **reactor** (library) module exporting `qjs_init`/`qjs_eval`, not a command.
- DR-2: Interpreter baseline first; AOT (`wamrc --target=riscv32`) deferred to Phase 3.
- DR-3: WAMR runtime pool (2 MB) in PSRAM, falling back to internal SRAM (as 0079).
- DR-4: Console = UART0 (CH343 bridge); the P4 has no USB Serial/JTAG, so the S3 AGENTS.md guidance does not apply.

## 2026-06-23

Wrote intern design/implementation guide (55 KB), harvested 14 sources, scaffolded firmware 0100, passed docmgr doctor, uploaded bundle to reMarkable /ai/2026/06/23/ESP32-P4-QUICKJS-WASM

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0100-esp32-p4-quickjs-wasm/main/app_main.cpp — Firmware scaffold entrypoint (buildable console stub)
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0100-esp32-p4-quickjs-wasm/wasm-src/wasm_main.c — QuickJS reactor wrapper (qjs_init/qjs_eval) for the host wasm build


## 2026-06-23

Resolved Crash B by routing QuickJS/WAMR calls through a long-lived pthread owner; ESP32-P4 device smoke now passes (qjs_init ok, print(1+2)->3, loop->10, exception path reports Error: boom).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0100-esp32-p4-quickjs-wasm/main/wasm_runner.cpp — Owner pthread and eval queue fix for WAMR pthread_self assertion
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/23/ESP32-P4-QUICKJS-WASM--run-quickjs-compiled-to-wasm-on-the-esp32-p4-intern-implementation-guide/design/02-phase1-device-bringup-post-mortem.md — Updated post-mortem status after Crash B fix and device smoke pass
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/23/ESP32-P4-QUICKJS-WASM--run-quickjs-compiled-to-wasm-on-the-esp32-p4-intern-implementation-guide/reference/01-investigation-diary.md — Steps 12-14 record investigation

