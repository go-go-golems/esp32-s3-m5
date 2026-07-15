# 0113 — PaperS3 MicroQuickJS Feasibility Spike (ESP-50 Phase 11)

Bounded spike answering the design doc's Phase 11 questions (memory arenas,
GC rooting discipline, opaque handles, cancellation, syntax subset, trusted
bytecode) on the actual PaperS3 hardware. **Deliberately separate from
`0112-papers3-reader-primitives`** — the engine is not linked into the
production reader path (task atze).

## Engine provenance (task zyxp)

- Engine: MicroQuickJS (Fabrice Bellard / Charlie Gordon), MIT license
  (`components/mquickjs/` — see file headers).
- Source: byte-identical copy of the repo's proven vendored engine at
  `imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/` (already
  hardware-validated on ESP32-S3 by that REPL project and used by the shared
  `components/mqjs_service`). Only `mquickjs_atom.h` differs: it is
  regenerated from this spike's stdlib (atoms depend on stdlib contents).
- The vendored revision predates upstream `bellard/mquickjs@84d793e0`
  ("more compatible Object.defineProperty (#46)", the newest commit in the
  local reference clone at `~/code/others/mquickjs`); `mquickjs.h` is
  byte-identical to that commit, `mquickjs.c` differs in internal
  StringBuffer bookkeeping. Record kept so an upgrade has a known baseline.
- Build flags: component compiles with the ESP-IDF defaults plus
  `-Wno-unused-function -Wno-unused-variable -Wno-format -Wno-type-limits`
  (upstream style), see `components/mquickjs/CMakeLists.txt`.

## Stdlib generation

`tools/spike_stdlib.c` + `tools/mqjs_stdlib_spike.c` (upstream stdlib with a
`CONFIG_SPIKE` block adding `millis`, `widgetDestroyAll`, `widgetLiveCount`,
and the `S3Widget` class) are compiled on the host by
`tools/gen_spike_stdlib.sh` into `spike_stdlib_gen`, which emits:

- `main/spike_stdlib.h` — 32-bit stdlib table (function references resolve
  against `main/spike_stdlib_runtime.c`);
- `components/mquickjs/mquickjs_atom.h` — atom table for this stdlib.

Regenerate both after changing the stdlib definition.

## Running

```bash
unset IDF_PYTHON_ENV_PATH && source ~/esp/esp-idf-5.3.4/export.sh
idf.py build flash
# capture: the suite autoruns at boot and prints SPIKE|<probe>|PASS/FAIL|...
python3 ../ttmp/2026/07/14/ESP-50-*/scripts/52-papers3-console-client.py \
    --settle 20 --cmd "" 
```

Probes: context startup at 8–4096 KB arenas (internal + PSRAM), syntax
subset classification, OOM + recovery, compacting-GC rooting, opaque
widget handles with generation-safe staleness + finalizers, deadline
cancellation of runaway scripts, and an on-device trusted-bytecode
round trip (compile → relocate → load → run in a fresh context).
