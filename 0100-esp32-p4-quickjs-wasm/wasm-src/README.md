# wasm-src — HOST build of quickjs.wasm (NOT compiled by ESP-IDF)

This directory builds `quickjs.wasm` on your development PC using the **wasi-sdk** toolchain. The
resulting `.wasm` is then copied into `main/quickjs.wasm` and embedded into firmware `0100` via
`EMBED_FILES`.

See the design doc §5.2 (build pipeline) and §7.1 (`wasm_main.c` pseudocode) for the full
explanation.

## One-time setup

```bash
# Install wasi-sdk (e.g. release 24 / LLVM 19) and set:
export WASI_SDK_PATH=/opt/wasi-sdk
$WASI_SDK_PATH/bin/clang --version    # Target: wasm32-wasi

# Vendor QuickJS:
cd wasm-src
git clone https://github.com/bellard/quickjs.git quickjs
cd quickjs && git checkout <release-tag> && cd ..
```

## Build

```bash
./build-quickjs-wasm.sh
# produces ../wasm-build/quickjs.wasm
wasm-objdump -x ../wasm-build/quickjs.wasm | head     # verify exports/imports
cp ../wasm-build/quickjs.wasm ../main/quickjs.wasm     # embed into firmware
```

## Verify imports/exports

`wasm-objdump -x quickjs.wasm` must show:
- **imports** `env.host_print`, `env.host_millis`, `env.host_gpio_write`, and `wasi_snapshot_preview1.*`
- **exports** `qjs_init`, `qjs_eval`

## Host test with iwasm (before flashing!)

Build a tiny host C program that registers `host_print` (module `env`) and calls
`qjs_init` + `qjs_eval("print(1+2)")`. Expect console output `3`. Do **not** flash until this
passes (design §10, Phase 0).
