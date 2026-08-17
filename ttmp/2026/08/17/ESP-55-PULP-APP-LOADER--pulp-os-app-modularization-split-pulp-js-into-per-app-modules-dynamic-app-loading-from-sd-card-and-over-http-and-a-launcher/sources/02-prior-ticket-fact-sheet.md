# Evidence: facts from ESP-50/51/53/54 docs relevant to app modularisation (collected 2026-08-17)

Path legend: `REPO=/home/manuel/code/wesen/go-go-golems/esp32-s3-m5`, `FW=REPO/0114-papers3-pulp-os`,
`ESP50=REPO/ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--*`, `ESP51=REPO/ttmp/2026/07/16/ESP-51-PULP-OS-V2--*`,
`ESP53=REPO/ttmp/2026/07/16/ESP-53-PULP-CONNECTIVITY--*`, `ESP54=REPO/ttmp/2026/07/27/ESP-54-PULP-GALLERY--*`.

## 1. Runtime history: source-eval → bytecode
- v1 (0112, ESP-50 Phase 12): flat ABI + ES5 facade evaluated at boot; gesture dispatch evaluated a snprintf'd string per event (`ESP50/reference/03:1049-1074`). 160 KB PSRAM arena (internal not available in the full firmware; PSRAM eval speed measured identical, `:1063`).
- v2 motive (`ESP51/design-doc/01:302`): prototypes in ROM not arena, no facade eval, direct `JS_Call` dispatch, single validation point.
- Bytecode pipeline first landed ESP-50 P12 (`s3jsc.c`, `ESP50/reference/03:1145`).
- Engine numbers on device (`ESP50/design-doc/05:85-90`): `JS_NewContext` ≈0.6 ms, first eval ≈0.9 ms; 24 KB arena exhausts in 20 ms with `InternalError: out of memory` and the context survives; `for(;;);` under a 100 ms deadline stops at 100 ms. **No measurement exists of JS_Eval throughput on source (ms/KB) or arena bytes per KB.**
- Bytecode image size history: 20 KB (ESP-50 S20, launcher+6 apps) → 25 KB (ESP-51 P7) → 30,216 B (ESP-53 P1) → 45,332 B today. pulp.js: ~600 lines (ESP-51) → ~750 (ESP-53) → 1,125 today.
- JS arena: 160 KB → 192 KiB in ESP-54 because pulp.js OOM'd at 160 KiB after the gallery app (`FW/main/app_js.cpp:51-53`, `ESP54/reference/01:191`). Frame arena 320 KiB (`FW/main/app_owner.cpp:515-519`).
- Heap: internal free ~220-230 KB steady (JS only, `ESP53/design-doc/02:75`); ~76.8 KB internal free with WiFi+httpd up (`ESP53/reference/01:429`).

## 2. Engine constraints ("five facts", `ESP53/design-doc/02:275-286`; `ESP51/design-doc/01:246-272`)
1. Zero-RAM-atom rule for `JS_LoadBytecode` (`mquickjs.c:12948`); load before any eval.
2. `N_ROM_ATOM_TABLES_MAX = 2` (`mquickjs.c:182`); stdlib = slot 0; one app image per context ("all apps concatenate into one image", `ESP54/design-doc/01:251`).
3. Atom coupling: engine copy local to the firmware; bytecode atom-coupled (`FW/components/mquickjs/README.md:9-13`).
4. JSValue: 32-bit tagged word on device; pointer values move under the compacting GC; never store a pointer JSValue in C memory; only the opaque slot is safe.
5. `JS_CLASS_COUNT` must match device and host compiler (`FW/main/app_js_bindings.h:18-20`, `FW/tools/js/pulpjsc.c:178-180`).
6. Calling into JS: `JS_StackCheck`, push args in reverse, then fn, then this, `JS_Call`; `JS_MAX_CALL_RECURSE = 8` (`mquickjs.c:68`).
7. JS_EVAL flags (`mquickjs.h:291-296`): RETVAL, REPL, STRIP_COL, JSON, REGEXP; device `EvalBounded` passes 0, host compiler passes STRIP_COL.
8. Dialect: var/closures/prototypes/getters/for-of/JSON/regexp/Math.random/array+string methods yes; let/const/arrow/class/template/spread/destructuring/modules NO; undeclared-global assignment → ReferenceError; array holes → TypeError; **no `require`, no promises, no event loop** (`ESP53/design-doc/01:33`).
9. Deadline discipline: every eval/call goes through a deadline wrapper; callbacks 1 s; `JsRunPulp` 3 s; measured `for(;;){}` killed at 804 ms of an 800 ms budget.
10. Exceptions: OOM survivable; `RecordException` keeps a 48-byte last_error; `CallCb` returns undefined for both "no cb" and "cb threw" (`ESP51/reference/01:285`).
11. Owner stack 8 KiB, no multi-KB locals.

## 3. Kernel and app switching
- Kernel is 2 lines (`FW/main/app_js.cpp:93-97`): `__cbs=[null]`, `G={...}`.
- `enter()` = resetTree + re-register `paper.home`/`sleepImage` (+ `osRoutes()` since ESP-53 P7). App STATE lives in JS globals (DZ/BZ/GG/TT/PC/DP) and survives app switches; wake = reboot, so globals are not preserved across sleep (`ESP51/reference/01:331, :377`).
- ROM prototypes cannot be extended at runtime (`ESP51/reference/01:290`) — kernel sugar deferred, `[P6.1]` still open.
- Trust model: only build-produced images are loaded; pulp.js is trusted firmware code (`ESP53/reference/01:355`); downloaded app code crosses this boundary for the first time.
- Host dev loop: none for JS behaviour; `pulpjsc` validates syntax + atom coverage only.
- Adding an app today: write `function myapp()`, add `entryRow(...)` in `home()` (`pulp.js:114-124`), regenerate bytecode, flash, validate with `js hits`/`js tap`. Launcher fingerprint was home=7 tap regions, now 11 rows.
- No prior doc proposes splitting pulp.js, manifests, a loader, sandboxing, hot reload or OTA JS.

## 4. Build pipeline
- Order: `gen_pulp_stdlib.sh` → `build_bytecode_apps.sh` → `idf.py build` (`ESP53/design-doc/02:376-388`). Skip 1 → device rejects bytecode (RAM atom); skip 2 → stale atom indices.
- `build_bytecode_apps.sh` compiles every `tools/js/apps/*.js` to `main/js_<name>.h` — multi-file compile already works, but only `js_pulp.h` is included/loaded (`app_js.cpp:99, :109-129`).
- Engine copied into `tools/js/host/` so the host build does not pick up the device atom header (symptom: every keyword a parse error).
- `pulpjsc`: 4 MiB host arena, `JS_NewContext2(..., prepare=1)`, `JS_Parse(JS_EVAL_STRIP_COL)`, `JS_PrepareBytecode64to32`; 64-bit host only.
- CMake pitfalls: name `esp_psram` in COMPONENTS; never `idf.py build` inside a component dir; `sdkconfig.defaults` seeds only absent values; managed components in `main/idf_component.yml`; IDF 5.3.4 pinned.

## 5. Validation practice
- Probes 1-22 (`js probe N` → arg 20+N); console client `ESP50/scripts/52-papers3-console-client.py --cmd ... --settle S --output F` (no DTR/RTS; one reader per port; ~7 s boot; USB output dropped when unread).
- Evidence lines: `pulp screen: <name>`, `js present: page=.. hits=N mode=M`, `tap x,y hit=N`.
- Validation ladder: host tests → build → flash + `js status` → `js probe N` → `js hits` + `js tap` → soak.
- Host suite: 38,186 checks (`components/s3paper_core/tests/host && make run`).

## 6. Storage
- SD: `/sdcard/books/*.txt` (32 max), `/sdcard/.s3paper/*.bin` state files (denied to JS), `/sdcard/www` (static + versioned index.html), `/sdcard/images/*.g4`.
- Settings store: 16 × {key[16], int32} — no strings; passwords needed S3WF. An app registry cannot live in `storeSet`.
- files.*: path 96, list 32 (names ≤39), body 16 KiB, lines 512; sync I/O on owner, async completion.
- http.get: body cap 32 KiB, default 16 KiB, timeout 10 s, 3 redirects, TLS bundle, GET only (no POST).
- serve: 8 routes, path 64, query 256, body 4 KiB, single request slot, 5 s handoff, 1 s JS deadline; static mount streams on the httpd task ("the one sanctioned off-owner read"); POST upload = second sanctioned off-owner SD write (`R-POSTHANDOFF`, `ESP54/design-doc/01:370-380`); `max_uri_handlers` had to be raised to 2.

## 7. Doc conventions
- Numbered `## N.` sections, ASCII diagrams, API tables, file:line citations, "where to look" table, gotcha catalog, phases with acceptance gates, glossary; decision records `R-<TOKEN>` with Context/Options/Decision/Rationale/Consequences/Status (`ESP54/design-doc/01:344-392`).

## 8. Documentation defects noticed
- `ESP53/design-doc/02:400` claims `load()`/`setTimeout` exist — they throw.
- Kernel "~30 lines" vs 2 lines; pulp.js line counts and arena numbers stale; home fingerprint 7 → 11.
