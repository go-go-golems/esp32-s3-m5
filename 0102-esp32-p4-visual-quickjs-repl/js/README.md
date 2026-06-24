# PicoCalc Visual QuickJS Script Playbook

This directory is for JavaScript code that can be developed on a desktop and later run on the ESP32-P4 PicoCalc visual QuickJS REPL.

The goal is to let a JS-focused colleague work in parallel without touching firmware files or fighting the active device bring-up branch.

## Working tree and branch

Use this worktree:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5-js
```

This worktree is on branch:

```bash
feature/0102-js-scripts
```

Keep changes mostly under:

```text
0102-esp32-p4-visual-quickjs-repl/js/**
```

Avoid editing firmware code unless explicitly coordinated:

```text
0102-esp32-p4-visual-quickjs-repl/main/**
components/qjs_service/**
components/visual_repl/**
components/picocalc_*/*
```

## Runtime contract

Write portable QuickJS scripts that depend only on the APIs available on both desktop and the ESP32 firmware.

Available globals:

- `print(...args)` — writes text output.
- `millis()` — returns milliseconds since boot/start.
- `gc()` — requests garbage collection.

Do not use these unless a shim or firmware binding is added intentionally:

- `console.log`
- `require`
- `import`
- Node APIs such as `fs`, `path`, `process`, `Buffer`
- QuickJS `std` / `os`
- Browser APIs such as `window`, `document`, `fetch`, DOM APIs

Prefer a single-file script with an explicit `main()` entrypoint:

```js
function main() {
  print("hello from portable QuickJS");
  print(1 + 2);
}

main();
```

## Desktop test loop

From the repository root of this worktree:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5-js
qjs 0102-esp32-p4-visual-quickjs-repl/js/host-shim.js \
    0102-esp32-p4-visual-quickjs-repl/js/examples/smoke.js
```

If `qjs` is not installed globally, build/use the vendored upstream QuickJS CLI:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5-js/0100-esp32-p4-quickjs-wasm/wasm-src/quickjs
make qjs
./qjs ../../../../0102-esp32-p4-visual-quickjs-repl/js/host-shim.js \
      ../../../../0102-esp32-p4-visual-quickjs-repl/js/examples/smoke.js
```

## Device integration model

The firmware evaluates submitted text through `qjs_service_eval()` using native QuickJS. Scripts that pass the desktop contract can later be integrated by one of these routes:

1. Paste/run through the visual REPL.
2. Embed as C/C++ strings and evaluate with filename labels such as `<demo>`.
3. Add a small firmware command table mapping visual commands like `/demo` to embedded JS snippets.

When adding scripts, include a short header comment with:

- purpose,
- expected output,
- memory/runtime assumptions,
- any required firmware binding.

## Suggested task split

Good JS-side tasks:

- write small demo scripts for `print`, loops, arrays, objects, errors, and timing;
- build tiny self-tests that print `PASS` / `FAIL`;
- design examples that fit a 40-column display;
- propose future firmware bindings in comments or markdown without implementing C++.

Avoid for now:

- modules/imports,
- large libraries,
- async/network/filesystem behavior,
- scripts requiring more than about 1 second to run.

## Git workflow

Commit only focused JS-side changes:

```bash
git status --short
git add 0102-esp32-p4-visual-quickjs-repl/js
git commit -m "0102 js: add portable QuickJS examples"
```

Before handoff:

```bash
git status --short
git log --oneline --max-count=5
```

Then tell the firmware branch owner the branch name and the scripts to embed/test.
