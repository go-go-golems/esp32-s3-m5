# Tasks

## Done

- [x] Create ticket ESP32-P4-QUICKJS-WASM + add `esp32p4` vocab topic
- [x] Harvest 14 primary research sources into `sources/`
- [x] Analyze sources + local prior art (0079/0082 WAMR, 0099 P4)
- [x] Write intern design/implementation guide `design/01-...md`
- [x] Scaffold firmware `0100-esp32-p4-quickjs-wasm`
- [x] Upload bundle to reMarkable
- [x] **Phase 0: install wasi-sdk-33** (~/tools, clang 22, wasm32-wasip1)
- [x] **Phase 0: vendor bellard/quickjs** (v2026-06-04) into wasm-src/
- [x] **Phase 0: build quickjs.wasm** (1.2 MB reactor, exports qjs_init/qjs_eval/malloc/free)
- [x] **Phase 0: verify imports/exports** (env.host_* + wasi_snapshot_preview1) with wasm_inspect.py
- [x] **Phase 0: host smoke test passes** — print(1+2)->3, loops, throw->Error: boom (WAMR host_test built from vendored runtime)
- [x] **Phase 0: root-caused the eval blocker** — QuickJS C-stack-overflow check false-trips under WAMR interp; fixed with JS_SetMaxStackSize(rt, 0)
- [x] Copy quickjs.wasm into main/ for embedding
- [x] Commit Phase 0 build infra + diary
- [x] Write Obsidian deep-dive report (ARTICLE note, textbook style) + push go-go-parc vault

## Phase 1 — Minimal firmware 0100 (device smoke passes)

- [x] Fix sdkconfig.defaults: CONFIG_WAMR_ENABLE_REF_TYPES=y (clang 22 emits ref types)
- [x] Port wasm_runtime_service + wasm_host_api from 0079/host_test (PSRAM pool, env natives)
- [x] Write wasm_runner (load embedded quickjs.wasm, instantiate, qjs_init, qjs_eval via module_dup_data)
- [x] Write js_command (js eval / js status) on esp_console (UART0)
- [x] main/CMakeLists.txt: EMBED_FILES quickjs.wasm + REQUIRES espressif__wasm-micro-runtime
- [x] idf.py set-target esp32p4 && idf.py build (app 1.8 MB, custom 4 MB partition)
- [x] Flash device + `js eval "print(1+2)"` passes on ESP32-P4 (prints `3`)

## Phase 2 — REPL + peripherals

- [ ] `js repl` (line-buffered persistent context) + `js reset`
- [ ] Add host_gpio_write + host_millis + JS globals gpio_write/millis
- [ ] `js bench` command (eval latency measurement)

## Phase 3 — Polish / optimization

- [ ] AOT compile with `wamrc --target=riscv32` (decision DR-2)
- [ ] JS program manifest (`js run -f name`) over EMBED_FILES
- [ ] Enable CONFIG_WAMR_ENABLE_MEMORY_PROFILING=y; surface in js status
