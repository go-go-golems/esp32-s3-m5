# Tasks

## Done (this research/design pass)

- [x] Create ticket ESP32-P4-QUICKJS-WASM + add `esp32p4` vocab topic
- [x] Harvest 14 primary research sources into `sources/` (WAMR API header/docs, QuickJS, wasi-sdk, ESP32-P4 memory)
- [x] Analyze sources + local prior art (0079/0082 WAMR, 0099 P4)
- [x] Write intern design/implementation guide `design/01-...md`
- [x] Scaffold firmware `0100-esp32-p4-quickjs-wasm` (buildable stub + wasm-src build)
- [x] Upload bundle to reMarkable

## Phase 0 — Host-side wasm build (intern, on PC)

- [ ] Install wasi-sdk; verify `$WASI_SDK_PATH/bin/clang` targets wasm32-wasi
- [ ] Vendor bellard/quickjs into `0100/.../wasm-src/quickjs`
- [ ] Build `quickjs.wasm` with `build-quickjs-wasm.sh`
- [ ] Verify with `wasm-objdump -x` (imports env.* + wasi_*, exports qjs_init/qjs_eval)
- [ ] Host smoke test with `iwasm`: register host_print, call qjs_eval("print(1+2)") → expect 3

## Phase 1 — Minimal firmware 0100

- [ ] Copy `quickjs.wasm` into `main/`, uncomment WAMR REQUIRES + EMBED_FILES in `main/CMakeLists.txt`
- [ ] Port `wasm_runtime_service.cpp` + `wasm_host_api.cpp` from 0079 (design §7.2–7.3)
- [ ] Write `wasm_runner.cpp` (instantiate once, qjs_init, qjs_eval via module_dup_data)
- [ ] Write `js_command.cpp` (`js eval`, `js status`)
- [ ] `idf.py build` + flash + `js eval "print('hello from wasm quickjs')"`

## Phase 2 — REPL + peripherals

- [ ] `js repl` (line-buffered persistent context) + `js reset`
- [ ] Add `host_gpio_write` + `host_millis` + JS globals `gpio_write`/`millis`
- [ ] `js bench` command (eval latency measurement)

## Phase 3 — Polish / optimization

- [ ] AOT compile with `wamrc --target=riscv32` (decision DR-2)
- [ ] JS program manifest (`js run -f name`) over EMBED_FILES
- [ ] Enable `CONFIG_WAMR_ENABLE_MEMORY_PROFILING=y`; surface in `js status`
