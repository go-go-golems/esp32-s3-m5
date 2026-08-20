---
Title: Investigation diary
Ticket: ESP-55-PULP-APP-LOADER
Status: active
Topics:
    - papers3
    - esp32s3
    - microquickjs
    - architecture
    - eink
    - javascript
    - storage
    - webserver
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: repo://0114-papers3-pulp-os/components/mquickjs/mquickjs.c
      Note: Step 1 bytecode load rules verified
    - Path: repo://0114-papers3-pulp-os/main/CMakeLists.txt
      Note: EMBED_TXTFILES wiring
    - Path: repo://0114-papers3-pulp-os/main/app_js.cpp
      Note: Step 1 first-hand read of the JS host core
    - Path: repo://0114-papers3-pulp-os/main/js_assets.cpp
      Note: flash asset registry
    - Path: repo://0114-papers3-pulp-os/tools/js/apps/settings.js
      Note: largest descriptor conversion (multi-screen relaunch)
    - Path: repo://0114-papers3-pulp-os/tools/js/os/10-facade.js
      Note: the os facade (P2)
    - Path: repo://0114-papers3-pulp-os/tools/js/os/20-catalog.js
      Note: ROM_APPS catalog (P3)
    - Path: repo://0114-papers3-pulp-os/tools/js/os/30-loader.js
      Note: loader (P2/P3)
    - Path: repo://0114-papers3-pulp-os/tools/js/os/40-launcher.js
      Note: catalog-driven launcher
    - Path: repo://ttmp/2026/08/17/ESP-55-PULP-APP-LOADER--pulp-os-app-modularization-split-pulp-js-into-per-app-modules-dynamic-app-loading-from-sd-card-and-over-http-and-a-launcher/scripts/01-trial-split-bytecode-sizes.py
      Note: Step 2 experiment script
    - Path: repo://ttmp/2026/08/17/ESP-55-PULP-APP-LOADER--pulp-os-app-modularization-split-pulp-js-into-per-app-modules-dynamic-app-loading-from-sd-card-and-over-http-and-a-launcher/scripts/02-host-eval-harness.c
      Note: Step 2 harness
    - Path: repo://ttmp/2026/08/17/ESP-55-PULP-APP-LOADER--pulp-os-app-modularization-split-pulp-js-into-per-app-modules-dynamic-app-loading-from-sd-card-and-over-http-and-a-launcher/scripts/02-host-eval-harness.sh
      Note: Step 2 harness build/run wrapper
ExternalSources: []
Summary: ""
LastUpdated: 2026-08-17T11:02:44.989239565-04:00
WhatFor: ""
WhenToUse: ""
---



# Investigation diary — ESP-55 PULP app loader

## Goal

Chronological record of the ESP-55 investigation: how the PULP OS v2 JS
runtime, build pipeline and native modules were surveyed, which host
experiments were run to size a dynamic-app design, what was blocked, and
how the intern guide was written. Each step is continuation-friendly:
commands and errors are verbatim.

## Step 1: Ticket creation, skills, and the first evidence sweep

The user asked for a new docmgr ticket that plans the modularisation of
`tools/js/apps/pulp.js` (one 1,125-line file holding the launcher and ten
apps) into per-app modules, plus dynamic loading of apps from the SD card
and over HTTP behind a launcher, delivered as an intern-level design and
implementation guide. I created `ESP-55-PULP-APP-LOADER`, seeded the task
list, and started two parallel read-only surveys (prior tickets' design
docs and diaries; the native `main/*` sources) while reading the JS host
core (`app_js.cpp`), the stdlib definition and the engine's bytecode API
myself.

### Prompt Context

**User prompt (verbatim):** "Let's create a new docmgr ticket to cleanup / modularize the @0114-papers3-pulp-os/tools/js/apps/pulp.js itself (by splitting out the apps) and allow apps to be loaded dynamically (over HTTP and stored on SD card), with a launcher.

Create  a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and the nupload to remarkable.

The device is attached over serial / USB.

commit at appropriate intervals and keep a detailed diary as you work (using the diary format from the skill). Store all scripts you write in the scripts/ folder of the ticket."

Mid-turn addition: "here's some docs /home/manuel/code/wesen/go-go-golems/go-go-parc/Projects/2026/07/28/PROJ - PULP OS Image Gallery - mDNS Browser Upload and the Bitmap Blit.md"

**Assistant interpretation:** Produce a research/design ticket (not the implementation) whose main deliverable is an exhaustive, evidence-anchored intern guide covering: what pulp.js is today, the JS runtime + build pipeline it depends on, a design for splitting it into app modules, a loader that pulls apps from SD or HTTP, a launcher, and a phased implementation plan; keep a diary; store scripts in the ticket; upload to reMarkable.

**Inferred user intent:** Make PULP OS extensible without reflashing (apps as files), shrink the monolith, and have a document a newcomer can implement from.

### What I did
- Loaded skills `ticket-research-docmgr-remarkable` and `diary`.
- Read `0114-papers3-pulp-os/tools/js/apps/pulp.js` (1,125 lines, 36,901 bytes; 15 banner-delimited sections), `main/app_js.cpp` (JS host core), `tools/js/pulp_stdlib.c` + `mqjs_stdlib_pulp.c` (stdlib table), `tools/js/pulpjsc.c`, `tools/js/build_bytecode_apps.sh`, `gen_pulp_stdlib.sh`, `components/mquickjs/mquickjs.h` (bytecode API) and `mquickjs.c:12800-12990` (`JS_RelocateBytecode`, `JS_LoadBytecode`).
- Read the reMarkable/Obsidian project note the user pointed at (ESP-54 gallery write-up) for the current-state narrative and the "device never decodes images" working rule.
- `docmgr ticket create-ticket --ticket ESP-55-PULP-APP-LOADER ...`, `docmgr doc add` (design-doc + diary), `docmgr task add` ×10.
- Launched two Explore agents (prior-ticket doc facts; native-side map). The native map is saved at `sources/01-native-side-map.md`.

### Why
- The design hinges on engine constraints (how bytecode is loaded, whether source can be evaluated at run time), so the engine and host were read first-hand rather than summarised from memory.

### What worked
- The docmgr scaffolding and the direct reads; the engine constraint was found quickly: `JS_LoadBytecode` refuses to run once any RAM atom exists (`mquickjs.c:12947-12948`) and `N_ROM_ATOM_TABLES_MAX = 2` (`mquickjs.c:182`), i.e. bytecode images are boot-time only and there is exactly one slot for the app image.
- `eval(str)` is present in the generated stdlib (`main/js_stdlib.h:4323`) — the parser is on the device, so source-eval loading needs no engine change.

### What didn't work
- The device is not reachable: `lsusb` shows `303a:1001 Espressif USB JTAG/serial debug unit` on bus 3 dev 33, but no `/dev/ttyACM*` / `/dev/serial/by-id/` exists because the `cdc_acm` kernel module is not loaded (`ls /sys/bus/usb/drivers/` has no `cdc_acm`; interface `3-3:1.0` has no driver). `sudo -n modprobe cdc_acm` → `sudo: a password is required`. On-device measurements are therefore deferred (task "Device experiment" left open) and the user needs to run `sudo modprobe cdc_acm` (or re-plug after loading it).
- zsh gotchas while scripting: `echo =====` fails (`no matches found: =====`, zsh `=cmd` expansion) and an unquoted `$F` holding a space-separated list is not word-split — use arrays.

### What I learned
- pulp.js source is 36,901 B but its bytecode image is 45,332 B (host `pulpjsc` count) and is copied into *internal* SRAM at boot (`app_js.cpp:111`, `MALLOC_CAP_INTERNAL`) — modularising the apps out of the image is also an internal-RAM win.
- The current app-switch boundary (`enter()` → `resetTree()`) is already the natural unload point for a dynamically loaded app.

### What was tricky to build
- Nothing built yet; the tricky part was establishing which loading path is *possible*: bytecode (boot-only, 1 slot) vs. source eval (available, but arena cost unknown → next step measures it).

### What warrants a second pair of eyes
- The claim that a second bytecode image cannot be loaded at run time — verified in the engine source, and re-verified empirically on the host in Step 2.

### What should be done in the future
- Re-run the on-device measurements once the console port is back.

### Code review instructions
- Start with `sources/01-native-side-map.md`, then `0114-papers3-pulp-os/main/app_js.cpp:100-130` and `components/mquickjs/mquickjs.c:12940-12962`.

### Technical details
- Bytecode facts: `JS_LoadBytecode` checks `unique_strings_len == 0`, `n_rom_atom_tables < 2`, magic/version/word size, and that the buffer was relocated in place; the image buffer must outlive the context.

## Step 2: Host experiments — trial split sizes and source-eval vs bytecode cost

With the engine rules established (one bytecode image per context, boot
only; source eval live), the open question was *cost*: how much arena does
an app take when evaluated from source, how long does it take, and how big
are the per-app pieces. The device was unreachable, so I built two host
experiments on the firmware's own vendored engine and host stdlib table.
The results size the whole design (§4 of the guide) and empirically confirm
the "too many rom atom tables" limit.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Ground the design in numbers before writing it.

**Inferred user intent:** A design that will not OOM the arena or make launches slow.

### What I did
- `scripts/01-trial-split-bytecode-sizes.py`: cuts pulp.js at its
  `// ---- name --` banners, compiles each section with
  `tools/js/host/pulpjsc`, tabulates source vs bytecode bytes.
- `scripts/02-host-eval-harness.c` + `.sh`: `#include`s `pulpjsc.c` (stubs
  + host stdlib) with `main` renamed, then either `JS_Eval`s files into a
  192 KiB context (mode `eval`, kernel first, `JS_DumpMemory` parsed for
  `heap size=`) or compiles each file to a host image and loads all images
  before the kernel, then `JS_Run`s (mode `bc`).
- Ran: `02-host-eval-harness.sh eval 192 <14 sections>`, `bc 192 noboot.js`,
  `bc 192 <two files>` (fails as predicted), and an arena bisection loop
  (8..160 KiB) per app to find the smallest arena that evaluates.

### Why
- The choice between source and bytecode modules, and the arena/deadline
  budgets in the loader, need numbers; host ratios are the best available
  without the port.

### What worked
- Per-app bytecode 2.1–8.7 KB; whole image 45,332 B vs sum-of-parts 57,116 B.
- Source eval (host64): retained 2.2–12.0 KB per section after GC; smallest
  arena that evaluates: dice 16 KiB, 2048 16 KiB, ink 14 KiB, settings
  24 KiB, whole file 96 KiB; parse+run 0.1–0.6 ms per section on x86.
- Bytecode: whole image `JS_Run` 0.01 ms, retained 8,080 B; second image →
  `InternalError: too many rom atom tables` (verifies `N_ROM_ATOM_TABLES_MAX=2`).

### What didn't work
- First harness run reported `4294965472` for the prelude's after-GC delta:
  the baseline was taken before the kernel's garbage was collected. Fixed by
  `JS_GC` before both baselines.
- Zsh: `F="a b c"; cmd $F` passed one argument (`No such file or directory`
  with the whole list as one path) — used an array `F=(...)`.
- `gcc -m32` is unavailable (`bits/libc-header-start.h: No such file`), so
  device-word-size numbers could not be produced on the host; the guide
  states the 64-bit caveat and the ×0.5–0.67 / ×50–100 conversion.

### What I learned
- Bytecode is *larger* than source (debug tables, unique strings) and lives
  in internal SRAM; source-eval'd code lives in the PSRAM arena. Moving
  apps out of the image is an internal-RAM win.
- The engine GCs on demand during parse, so the "arena delta" column
  overstates the requirement; the bisection numbers are the true transient.
- The ESP-54 "OOM at 160 KiB" for the *bytecode* image is unexplained by
  the host numbers (8 KB retained) — flagged as the first Phase 0 measurement.

### What was tricky to build
- Measuring heap use without access to the private `JSContext` struct: I
  captured `JS_DumpMemory`'s summary line through `JS_SetLogFunc` and parsed
  `heap size=`. It works because the harness sets its own log function; on
  the device the guide proposes a 4-line `JS_GetHeapUsed()` accessor instead.

### What warrants a second pair of eyes
- The ×0.5–0.67 arena and ×50–100 time conversion factors are estimates,
  not measurements; Phase 0 replaces them.

### What should be done in the future
- Run the same harness against a 32-bit build if a multilib toolchain is
  installed; port the `bc` mode to exercise the "process model" alternative.

### Code review instructions
- `scripts/02-host-eval-harness.c` (`mode_eval`, `mode_bc`, `heap_used`);
  re-run with `scripts/02-host-eval-harness.sh eval 192 <files>`.

### Technical details
- Build line: `gcc -O2 -w -I tools/js/host -I components/mquickjs -I tools/js
  02-host-eval-harness.c tools/js/host/mquickjs.c cutils.c dtoa.c libm.c -lm`.
- The harness must include the *host copy* of `mquickjs.c` (device atom
  header otherwise; every keyword becomes a parse error — the same trap
  `build_bytecode_apps.sh` documents).

## Step 3: Writing the intern guide

With the evidence in `sources/` and the measurements in hand I wrote the
design doc: current-state analysis with pulp.js anatomy, the two code paths
into the engine, the build pipeline, the native modules the loader leans
on, the measurements, a gap table, the proposed architecture (descriptor
contract, `os` facade, catalog + manifests + ROM registry, native `load`,
launcher, HTTP push/pull/hot-reload, `os.state`, trust), nine decision
records, seven phases with gates, tests, risks, gotchas, references and a
glossary.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** The main deliverable; long-form, evidence-anchored, implementable.

**Inferred user intent:** Hand the ticket to someone new and have them build it.

### What I did
- Wrote `design-doc/01-*.md` (~1,420 lines) in three parts and assembled it
  under the docmgr frontmatter (Summary/WhatFor/WhenToUse filled).
- Verified the line-number citations for `app_js.cpp` (`RegisterCb` 295,
  `CallCb` 309, `JsInit` 372, `js_load` 572, `resetTree` 603), `net_serve.cpp`
  (`ServeUpload` 118, POST dispatch 301, `max_uri_handlers` 503, marker 473),
  `app_files.cpp` (CR strip 318-321) and fixed three off-by-two references.
- Verified dialect features used in the pseudocode exist in the stdlib:
  `hasOwnProperty`, `String.prototype.replace/lastIndexOf`, the `delete`
  opcode, `JSON.parse`.

### Why
- The user asked for an intern-level guide with prose, bullets, pseudocode,
  diagrams, API and file references; the ESP-54 guide's structure
  (numbered sections, `R-<TOKEN>` decision records, gotcha catalog, phases
  with gates) is the house style and was followed.

### What worked
- The design falls out of two facts (one image per context; eval is live)
  plus the measurements; every component reuses an existing pattern
  (`enter()`, module mailboxes, POST handoff, index.html marker seeding).

### What didn't work
- N/A (documentation step; no failures beyond the port).

### What I learned
- The `load` atom already exists (stub throws), so a real `load(path)` needs
  no atom-name regeneration; adding `assets.copy`/`apps.received` does.

### What was tricky to build
- Deciding where `settings` lives: it is both an app and the recovery tool
  for WiFi. Resolved as "an asset like the others, but exempt from SD
  override" (§10).

### What warrants a second pair of eyes
- The loader ordering (`RUN.desc=null; gc(); load(); validate; enter();
  main()`) and the "app showed nothing" rule; the trust position (R-TRUST).

### What should be done in the future
- Phase 0 numbers; a host JS runtime for app development (own ticket).

### Code review instructions
- Read the guide top to bottom; cross-check §3 citations against
  `main/app_js.cpp` and `main/net_serve.cpp`; run the two scripts.

### Technical details
- Doc outline: 0 how to read · 1 summary · 2 scope · 3 current state
  (3.1–3.10) · 4 measurements · 5 gaps · 6 architecture (6.1–6.10) · 7
  decision records (R-SOURCEEVAL, R-ONEIMAGE, R-DESCRIPTOR, R-CATALOGFILE,
  R-NATIVELOAD, R-ROMSEED, R-HTTPPUSHPULL, R-TRUST, R-STATE) · 8 phases 0–7 ·
  9 tests · 10 risks · 11 gotchas · 12 references · 13 glossary.

## Step 4: Bookkeeping, doctor, and the reMarkable upload

Related every file that shaped the guide, wrote the changelog entries,
ran `docmgr doctor` (clean), and uploaded the bundle. The upload hit a
reMarkable cloud write failure that turned out to be a stale local rmapi
tree cache; the fix and the evidence are below so the next person does not
lose an hour on it.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Close the loop: bookkeeping, validation, delivery to the tablet.

**Inferred user intent:** Read the guide on the reMarkable.

### What I did
- `docmgr doc relate` ×23 (design doc) and ×5 (diary), three
  `docmgr changelog update` entries, `docmgr doctor --ticket
  ESP-55-PULP-APP-LOADER --stale-after 30` → "All checks passed".
- `remarquee upload bundle <guide> <diary> <sources/01> <sources/02> --name
  "ESP-55 PULP App Loader Intern Guide" --remote-dir /ai/2026/08/17/ESP-55-PULP-APP-LOADER --toc-depth 2 --non-interactive`.

### What didn't work
- Every cloud *write* failed with HTTP 400 while reads worked:
  `ERR rmcloud mkdir-all: create remote directory failed error="request failed with status 400" entry=17 parent_id=... parent_name=08`,
  the same for `remarquee cloud mkdir /ai/2026/08/16/ESP-55-test`, and a
  probe `remarquee upload md` into the existing `/ai/2026/08/16` folder
  (`ERROR-UPLOAD ... request failed with status 400`). `--reauth` did not
  help (auth was fine: `cloud ls` listed folders).
- First hypothesis (stale local tree cache → generation mismatch) was
  wrong: after `mv ~/.cache/rmapi/tree.cache ~/.cache/rmapi/tree.cache.bak-20260817-esp55`
  and a ~6-minute rebuild (26 MB), `remarquee cloud mkdir /ai/2026/08/17`
  still returned 400.
- Root cause from `--log-level debug`: the blob PUTs succeed (200/202) and
  the final `PUT .../sync/v3/files/<root hash>` (`rm-filename: root.docSchema`)
  is rejected with `{"message":"invalid root schema"}` — the reMarkable
  cloud no longer accepts the v4 root index that the rmapi fork pinned by
  remarquee writes (`github.com/marcobarcelos/rmapi@v0.0.0-20260518…`,
  `api/sync15/tree.go:155-190`: "always emit v4 root indexes"). This is a
  tooling/server-format regression outside this ticket; reads work, all
  writes fail. Not fixed here.
- Consequence: **the bundle is not on the tablet yet.** Re-run once
  remarquee/rmapi is updated:
  `remarquee upload bundle <design-doc/01> <reference/01> <sources/01> <sources/02> --name "ESP-55 PULP App Loader Intern Guide" --remote-dir /ai/2026/08/17/ESP-55-PULP-APP-LOADER --toc-depth 2 --non-interactive`.

### What worked
- Doctor clean; relations and changelog in place.

### What I learned
- When reMarkable reads work and every write returns 400, run the command
  with `--log-level debug` first: the response body names the cause
  (`invalid root schema` here) and saves a cache-rebuild detour.

### What was tricky to build
- Distinguishing an auth failure from a cache/generation failure: `ls`
  succeeding rules out auth.

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- Re-run the on-device Phase 0 measurements once `cdc_acm` is loaded
  (`sudo modprobe cdc_acm`, then re-plug the PaperS3 or re-enumerate).

### Code review instructions
- `docmgr doctor --ticket ESP-55-PULP-APP-LOADER --stale-after 30`;
  `remarquee cloud ls /ai/2026/08/17/ESP-55-PULP-APP-LOADER --long --non-interactive`.

### Technical details
- Upload result: FAILED (server rejects root schema); ticket task
  "Upload bundle to reMarkable" left open. `remarquee` binary:
  `~/.local/bin/remarquee` built 2026-08-10 from
  `~/code/wesen/go-go-golems/remarquee` @183a9d3.

## Step 5: Root cause of the reMarkable "invalid root schema" failure (investigation only)

The user asked what changed on the reMarkable side and whether the local
remarquee checkout still uses the unfixed rmapi. It does; the cause is a
server-side validation change that landed the same day.

### Prompt Context

**User prompt (verbatim):** "can you find information online about the root schema change?" / "the local code for remarquee is in ~/code/wesen/go-go-golems/remarquee, is it still using the old without fix?" / "(don't fix it, just investigate)"

**Assistant interpretation:** Diagnose, do not patch.

**Inferred user intent:** Decide how to unblock uploads (bump the rmapi dependency).

### What I did
- Serialized the cached tree with the fork's own `IndexReader()` in a scratch
  Go program (`scratchpad/rootcheck`): schema 4 header `0:.:10478:11516896433`,
  10,478 entries, all `type 0`, no duplicates, **already sorted** (because it
  was mirrored from the server) — so the format itself matches rmapi-js's
  schema-4 writer.
- Web search: `ddvk/rmapi` issues #75 and #76 (both opened 2026-08-17): "All
  write operations fail with 400 'invalid root schema'"; maintainer
  diagnosis: **the server now requires root index entries strictly sorted by
  document ID**; `HashTree.Add()` appends and `Remove()` swaps, so any write
  that adds a document (our `mkdir` created a new folder id) breaks the
  order → 400. Fix: PR #77 "sort root index by document ID before
  serializing (fixes #75)" — sorts in `IndexReader()` /
  `IndexReaderWithSchema()`, still open at the time of writing.
- `~/code/wesen/go-go-golems/remarquee/go.mod:22` pins
  `replace github.com/juruen/rmapi => github.com/marcobarcelos/rmapi v0.0.0-20260518211546-a0d079936d46`
  (May 18 fork; writes v4 but does not sort) → **still the unfixed version**.

### What worked / didn't work
- The earlier hypotheses (stale cache, wrong schema) were ruled out by
  reproducing the writer's output offline.

### What should be done in the future
- In remarquee: bump the replace to a fork containing ddvk/rmapi PR #77 (or
  the equivalent one-line sort in `IndexReader()`), rebuild, re-run the
  Step 4 upload command. Not done here by request.

### Technical details
- Sources: https://github.com/ddvk/rmapi/issues/76 , /issues/75 , /pull/77 ;
  rmapi-js CHANGELOG 10.1.0 (2026-06-12: schema-3 roots rejected with
  "Software must be updated", always write schema 4).

## Step 6: reMarkable upload succeeded after the remarquee fix

Once remarquee was rebuilt with the sorted-root-index fix (go-go-golems/remarquee#23), the Step 4 command ran clean.

### Prompt Context

**User prompt (verbatim):** "remarquee is fixed, upload original guide"

**Assistant interpretation:** Re-run the bundle upload.

**Inferred user intent:** Read the guide on the tablet.

### What I did
- `remarquee upload bundle <design-doc/01> <reference/01> <sources/01> <sources/02> --name "ESP-55 PULP App Loader Intern Guide" --remote-dir /ai/2026/08/17/ESP-55-PULP-APP-LOADER --toc-depth 2 --non-interactive`
  → `OK: uploaded ESP-55 PULP App Loader Intern Guide.pdf -> /ai/2026/08/17/ESP-55-PULP-APP-LOADER`.
- Checked the upload task; changelog updated.

### What worked / didn't work
- Upload OK first try (mkdir of `17/ESP-55-…` included).

### Code review instructions
- `remarquee cloud ls /ai/2026/08/17/ESP-55-PULP-APP-LOADER --long --non-interactive`.

## Step 7: Design extension — multi-context runtime and the page-script browser

The user asked whether the app work approaches running several MicroQuickJS
contexts at once, then asked to fold a concrete idea into the design: a
little browser where the backend returns pages as JS scripts that are fully
sandboxed to UI display only — the builder DSL as a cheap markup language.
I verified the engine-side facts and extended the guide.

### Prompt Context

**User prompt (verbatim):** "Does the work on the apps approach the possibility of running multiple microquickjs at once?" then "Update the deisgn. The idea is that we could actually build a little browser where the backend returns pages as actual JS scripts that can be fully sandboxed to only do UI display, as a "cheap" markup languages"

**Assistant interpretation:** Add a designed-but-later multi-context runtime and a page-script browser (with a real sandbox) to the intern guide, consistent with the single-context loader that ships first.

**Inferred user intent:** Keep the loader work from foreclosing a thin-client/browser direction; get a concrete sandbox mechanism.

### What I did
- Verified: `mquickjs.c` has no mutable file-scope state; `JS_NewContext`
  takes its own arena; `n_rom_atom_tables` is per context (`:182, :231`);
  `JS_SetContextOpaque` exists (`mquickjs.h:266`; opaque reaches the
  interrupt handler/log func; no getter — bindings map `ctx`→state by
  lookup or a 3-line accessor); native calls index
  `ctx->c_function_table[idx]` (`mquickjs.c:3786, :5361`) and the generated
  stdlib references bindings by bare name (`main/js_stdlib_table.c`),
  so a second `JSSTDLibraryDef` sharing the ROM table with a filtered
  function table is a zero-atom sandbox.
- Counted single-context assumptions in the bindings (`jsi::g_ctx` ×34,
  page/hit/dyn tables, module-cb slots, deadline, serve routes).
- Guide: scope item 8; executive-summary addendum; §6.11 (per-context
  `JsCtxState`, foreground rule, owner-tagged completions, teardown frees
  atoms, memory, no parallelism; loader becomes new-context + load +
  switch, per-app bytecode viable again); §6.12 (page descriptor
  `({title, main(ui, nav)})`, UI stdlib via `#define`-renamed natives to
  `js_ui_denied`, allow/deny table, `nav` mailbox, isolation properties,
  browser app flow, bytecode pages); decision records R-MULTICTX,
  R-UISANDBOX, R-PAGESCRIPT; Phases 8–10 with gates; risks, references,
  glossary; frontmatter summary.

### What worked
- The existing int-only `CallCb` boundary and packed-int handles make the
  ABI context-agnostic already — the refactor is state plumbing, not API.

### What didn't work
- N/A

### What I learned
- `ctx->opaque` is the same pointer the log/interrupt callbacks get; there
  is no public getter, so per-context binding state needs either a tiny
  engine accessor or a lookup table.

### What was tricky to build
- Keeping two designs coherent: the single-context loader ships first
  (R-SOURCEEVAL/R-ONEIMAGE unchanged), and R-MULTICTX only requires Phase 3
  to write `js_load` as `LoadInto(state, …)` so the later switch is cheap.

### What warrants a second pair of eyes
- The deny-list default (generator should default to *deny* for new natives
  in the UI table); the foreground-switch rule that resets the widget arena
  (a kept-alive background context must rebuild its page on return).

### What should be done in the future
- Phases 8–10 after the loader ships; a `scripts/05-pulp-page-server.py`
  reference server.

### Code review instructions
- Guide §6.11, §6.12, R-MULTICTX/R-UISANDBOX/R-PAGESCRIPT, Phases 8–10;
  cross-check the `c_function_table` claim at `mquickjs.c:3786`.

## Step 8: Phase 0 instrumentation built and flashed; console blocked by ROM download-mode latch

Phase 0 began: the `js measure` console op, the `JS_GetHeapUsed` engine
accessor and `arena_used` in `js status` are implemented, built and flashed.
Measurement is blocked at the last step: the device is wedged in ROM
download mode and no USB-side reset sequence brings the app up — a
physical power-cycle is needed. The debugging that established this is
recorded verbatim because it invalidates the ESP-50 client's "no modem
control = safe" assumption on this host.

### Prompt Context

**User prompt (verbatim):** "alright, cdc_acm loaded. \n\ncommit at appropriate intervals and keep a detailed diary as you work (using the diary format from the skill)"

**Assistant interpretation:** Start implementation (Phase 0 first, as recommended), with commits and diary.

**Inferred user intent:** Get the on-device numbers that gate the ESP-55 design.

**Commit (code):** 4d59929a — "ESP-55 P0: js measure console op, JS_GetHeapUsed accessor, arena_used in js status"

### What I did
- `scripts/03-gen-measure-src.py` → `main/js_measure_src.h` (dice 2,388 B +
  settings 5,447 B sections embedded as byte arrays).
- Engine: `JS_GetHeapUsed(ctx)` (heap_free − heap_base) in
  `components/mquickjs/mquickjs.c` + header declaration.
- `JsSnapshot.arena_used` (`app_events.h`), filled in `FillJsSnapshot`,
  printed by `js status` (`app_console.cpp`).
- `js measure` (console arg 14 → `JsRunMeasure()` in `js_probes.cpp`):
  baseline arena/GC/internal/PSRAM, timed eval of dice and settings with
  arena before/after/GC, dice ×10 with GC (flatness), evidence lines
  prefixed `measure:`.
- `idf.py build` clean; flash OK ("Hash of data verified", hard reset).
- Console debugging (below); wrote `scripts/04-papers3-console-hold.py`
  (hold-open client with in-fd reset).

### What worked
- Build + flash on the first try; the instrumentation compiles into the
  existing probe/console plumbing with no stdlib/atom regeneration (console
  ops are native-side).

### What didn't work
- The console is entirely silent — before AND after flashing:
  `52-papers3-console-client.py --settle 12 --cmd ping --cmd status` reads
  zero bytes; no other process holds the port (`lsof` empty, no
  ModemManager); kernel bound `cdc_acm 3-3:1.0: ttyACM0` at 17:50 (device
  re-plugged today; an iPhone used bus 3-3 yesterday).
- Boot-catch attempts read zero bytes for 20–28 s after esptool
  "Hard resetting via RTS pin...".
- Diagnosis: `python -m esptool --before no_reset flash_id` syncs
  immediately → **the chip is sitting in ROM download mode**; the app never
  runs. Explains everything (esptool works; app console dead).
- In-fd reset attempts through a held cdc_acm fd (script 04):
  - `(1,1)→(0,1)→(0,0)` and `(0,0)→(0,1)→(0,0)`: chip resets but
    `rst:0x15 (USB_UART_CHIP_RESET), boot:0x0 (DOWNLOAD(USB/UART0))`,
    "waiting for download".
  - `(1,1)→(1,0)`: no reset at all.
- `esptool --after soft_reset`: `A fatal error occurred: Soft resetting is
  currently only supported on ESP8266`.
- Conclusion: either the ESP32-S3 **force-download strap latch** is set
  (entering download via USB-Serial-JTAG persists across non-POR resets;
  `rst:0x15` is not a POR) or the BOOT strap is physically low. No USB-side
  sequence can clear it; a power-on reset (power the device off and on via
  the side button, or a full battery/USB power cycle) is required.

### What I learned
- On this kernel (6.8, cdc_acm), **every open of /dev/ttyACM0 asserts
  DTR+RTS**; the ESP-50 client's "no modem-control ioctls" does not cover
  the open itself. The repo's serial discipline should gain: if the console
  goes silent, first test `esptool --before no_reset flash_id` — instant
  sync means download mode, and the fix is physical (POR), not more client
  tweaks.
- `rst:0x15 USB_UART_CHIP_RESET` + `boot:0x0 DOWNLOAD` is the fingerprint
  of the latched state.

### What was tricky to build
- Distinguishing four identical-looking silences: asleep device, stolen
  port, wrong console backend, and download mode. The discriminator was
  esptool with `--before no_reset` (talks to the ROM only if the ROM is
  listening).

### What warrants a second pair of eyes
- The reset-sequence table above, if anyone wants to derive the S3's real
  DTR/RTS latch semantics; and whether `04-papers3-console-hold.py` should
  replace the ESP-50 client wholesale once verified after the power cycle.

### What should be done in the future
- After POR: run `js measure`, record the Phase 0 numbers, close task 8udr.
- Add the download-mode diagnostic to `0114-papers3-pulp-os/README.md`'s
  serial discipline section.

### Code review instructions
- `git show 4d59929a`; start at `main/js_probes.cpp` (`JsRunMeasure`,
  `MeasureOne`), then `components/mquickjs/mquickjs.c` (`JS_GetHeapUsed`).
- Validate after power cycle:
  `python3 <ticket>/scripts/04-papers3-console-hold.py --no-reset --cmd "js measure"`.

### Technical details
- `js measure` evidence grammar: `measure: baseline arena_used=<n>
  after_gc=<n> internal_free=<n> psram_free=<n>`, `measure: <name>
  bytes=<n> rc=<s> us=<n> arena before=<n> after=<n> (+d) gc=<n> (+d)`,
  `measure: dice x10 arena_after_gc: n0..n9`, `measure: done
  internal_free=<n>`.

## Step 9: Phase 0 numbers measured on the device

After the user re-seated the flaky USB connection (the device had dropped
off the bus entirely; the brief power interruption also cleared the
force-download latch from step 8), `idf.py flash` succeeded and the app
booted normally. `js measure` produced the full Phase 0 dataset; the gate
passes and the guide's §4.3 now carries measured numbers.

### Prompt Context

**User prompt (verbatim):** "esp-idf.py flash ? try again" / "could be that the serial was a bit flaky" / "ok now maybe?"

**Assistant interpretation:** Retry flash + measurement after the re-plug.

**Inferred user intent:** Unblock Phase 0.

### What I did
- `idf.py flash` (clean), then
  `04-papers3-console-hold.py --no-reset --cmd status --cmd "js status"
  --cmd heap --cmd "js measure" --cmd "js status"`.
- Updated guide §4.3 from "what remains to measure" to the measured table;
  checked ticket task 8udr.

### What worked
- Everything, first try after the re-plug:
  - boot arena 14,956 B, after GC 7,496 B (the 45 KB image retains ~7.5 KB
    — the ESP-54 "OOM at 160 KiB" was a transient, not steady retention);
  - dice 2,388 B: eval 35,362 us, transient +24,264 B, retained +3,024 B;
  - settings 5,447 B: eval 80,582 us, transient +50,840 B, retained +7,456 B;
  - dice ×10 with GC: flat 17,980 B ×10 (zero creep);
  - internal_free identical before/after (124,931 B) — arena-only cost.
- Gate: largest app 81 ms < 150 ms; flat baseline. ≈15 ms and ≈1.4 KB
  retained per KB of source; transient ≈10× retained.

### What didn't work
- First `idf.py flash` attempt failed mid-write ("Waiting for the chip to
  reconnect / Connecting...") when the device dropped off USB — root cause
  of the whole step-8 mystery was most plausibly the flaky cable/port all
  along (enumerations on two different ports, one-second connects).

### What I learned
- The step-8 download-mode latch cleared with the USB power interruption,
  confirming the POR theory without a button press.
- Device/host conversion factors: time ×130, retained arena ×0.62 — worth
  reusing when the host harness sizes future apps.

### What was tricky to build
- N/A (measurement run; the tricky part was step 8).

### What warrants a second pair of eyes
- The interpretation that ESP-54's 160 KiB OOM was transient (during
  JS_Run/page build), given steady retention is only ~18 KB total.

### What should be done in the future
- Phase 1 (mechanical split); add the download-mode diagnostic to the
  firmware README serial section (from step 8).

### Code review instructions
- Transcript: scratchpad p0-measure2.log; evidence lines all prefixed
  `measure:`. Re-run: `python3 <ticket>/scripts/04-papers3-console-hold.py
  --no-reset --cmd "js measure"`.

## Step 10: Phase 1 — mechanical split, one image, byte-equivalent behavior

pulp.js is gone: the fifteen banner sections now live as
`tools/js/os/{00-kernel,40-launcher,90-boot}.js` plus twelve
`tools/js/apps/<id>.js`, and `build_bytecode_apps.sh` concatenates them
back into one `build/pulp_all.js` compiled to `main/js_pulp.h`. Nothing
about the code changed — the image grew by exactly 20 bytes (the embedded
filename string) — and the device gate confirms identical behavior.

### Prompt Context

**User prompt (verbatim):** "phase 1 - 3. commit at appropriate intervals and keep a detailed diary as you work (using the diary format from the skill)"

**Assistant interpretation:** Implement Phases 1, 2 and 3 from the guide, commit per phase, diary per phase.

**Inferred user intent:** Get the modular loader working on the device.

**Commit (code):** (this commit) — "ESP-55 P1: mechanical split of pulp.js into os/ + apps/, single-image concat build"

### What I did
- `scripts/05-phase1-split.py` (one-off): cut pulp.js at its banners into
  os/00-kernel.js (prelude, 2,694 B), os/40-launcher.js (home, 2,433 B),
  os/90-boot.js (164 B), and apps/{library,reader,dice,blitz,2048,tea,
  postcard,daily,ink,gallery,settings,radio}.js — verbatim; deleted pulp.js.
- `build_bytecode_apps.sh` step 3: replaced the per-app compile loop with
  `cat os/[0-3]*.js apps/*.js os/[4-9]*.js > build/pulp_all.js` +
  one pulpjsc call → `main/js_pulp.h` (45,352 B vs 45,332 B).
- Ignored `tools/js/build/` in the repo .gitignore; froze
  `main/js_measure_src.h` (script 03 noted as frozen baseline).
- Build + flash + gate.

### Why
- Reviewable next phases: every later diff is against a small file, and the
  concatenation order (kernel → apps → launcher → boot) keeps the single
  executing statement (boot's `home()`) after all definitions, with function
  hoisting covering forward references.

### What worked
- Gate: `js hits` = 11 regions on home (fingerprint unchanged); probe 1
  (13-op builder page), probe 5 (services), probe 13 (runaway killed at
  802 ms, context alive after a 51-exception storm); `js pulp` → home, 11
  regions; zero unexpected exceptions; arena_used at boot 14,812 B (same as
  pre-split 14,860 ± jitter).

### What didn't work
- N/A (the ATOM_ALIGN "Too many properties" warning is the documented
  benign one).

### What was tricky to build
- Only the concat order: apps sort lexically (2048 first), so top-level
  `var` initializers now run in a different order than the original file —
  safe because every section's initializers are self-contained; the diary
  records this so nobody adds a cross-section initializer dependency.

### What warrants a second pair of eyes
- `git diff` of build_bytecode_apps.sh; confirm no other consumer expected
  per-app `js_<name>.h` headers (grep found none).

### What should be done in the future
- Phase 2 rewrites the app files as descriptors; do not merge P1 alone
  without noting pulp.js is regenerable as `build/pulp_all.js`.

### Code review instructions
- `python3 <ticket>/scripts/05-phase1-split.py` is not idempotent (pulp.js
  removed); review the split by `cat`ing build/pulp_all.js against the old
  pulp.js from git history: `git show 4d59929a:0114-papers3-pulp-os/tools/js/apps/pulp.js | diff - 0114-papers3-pulp-os/tools/js/build/pulp_all.js` — expect only section reordering.

## Step 11: Phase 2 — descriptors, the os facade, catalog-driven launcher

Every app file is now a bare descriptor expression `({id, title, subtitle,
version, abi, main: function (os, arg) {...}})` — the exact shape `load()`
will consume in Phase 3 — and the build script generates the registry glue
(`APPS['<id>'] = (<file>);`). The OS grew three files: the facade
(os/10-facade.js: os.M getter, os.state/clearState, chrome/hintFooter/
announce/pad2/fmtClock, home, launch, netUp, abi), the catalog
(os/20-catalog.js: APPS + ROM_ORDER, reader launchable but unlisted), and
the loader (os/30-loader.js: validate → enter → main, errorPage). The
launcher builds its rows from the catalog.

### Prompt Context

**User prompt (verbatim):** (see Step 10)

**Assistant interpretation:** Phase 2 of the plan.

**Inferred user intent:** Apps in their final module shape, one image still.

**Commit (code):** (this commit) — "ESP-55 P2: app descriptors + os facade + loader + catalog launcher"

### What I did
- Rewrote all 12 app files as descriptors: enter() removed (loader owns
  it); chrome/hintFooter/announce/fmtClock/pad2/netUp → os.*; margins → 
  os.M; cross-app calls → os.launch('reader', idx) / os.launch('library')
  / os.home(); persistent state → os.state('<id>', init) (dice, blitz,
  2048, tea, postcard, gallery, radio, settings msg); per-entry state left
  local (reader, daily, ink).
- settings became one app with three internal screens (main/scan/pass)
  navigated by relaunching itself: os.launch('settings', {screen, ssid}).
  The old code called enter() at every screen change; enter is deliberately
  not on the facade, so screen changes cross the loader (guide §6.3).
- build_bytecode_apps.sh wraps each app file as `APPS['<id>'] = (…);` at
  concatenation time — files stay load()-ready.
- Gate on device (transcripts p2-*.log in the session scratchpad).

### Why
- The descriptor + facade contract is the whole point of the ticket; doing
  it while everything is still one image isolates the contract change from
  the loading change.

### What worked
- pulpjsc accepts the ES5 object-literal getter (`get M() { return M; }`).
- Gate: launcher 11 rows from the catalog (same fingerprint, same y
  rects); all 11 rows tap-launch with correct `pulp screen:` lines
  (library, dice, blitz, 2048, tea, postcard, daily, ink/0, gallery —
  which displayed its stored image —, radio, settings); settings margin
  toggle relaunches through the loader twice; dice rolled twice and
  re-entered (os.state path); probes 1 and 13 pass; zero unexpected
  exceptions across the whole walk.
- Image: 49,408 B (+4 KB for facade/loader/registry glue + descriptor
  wrappers).

### What didn't work
- N/A — no failed attempts this step; the settings-relaunch design
  question was settled in the guide beforehand.

### What I learned
- arena_used climbs across app switches without eval (18 KB → 114 KB over
  the walk + probe storm): garbage from rebuilt pages accumulates until
  the engine GCs on allocation pressure. Not a leak (the measure op's
  dice ×10 with explicit gc() is flat), but the P3 loader's gc()-before-
  load is doing real work, and `js status` arena_used is only meaningful
  after a gc().

### What was tricky to build
- settings: three enter()-calling screens plus completion callbacks that
  rebuilt the page (netUp → serve.start → settings()). Every rebuild
  became os.launch('settings', arg); the "no rebuild until the callback
  lands" rule from ESP-53 is preserved because the relaunch happens inside
  the completion callback, exactly like the old code.
- The 2048 subtitle is a function (`'best ' + storeGet(...)`) evaluated by
  the launcher at build time — descriptors may carry function subtitles
  only in ROM (JSON manifests in Phase 4 cannot, guide gotcha 8).

### What warrants a second pair of eyes
- apps/settings.js (the largest rewrite; compare against git history of
  the settings section); the facade getter (`get M()`) — if any tool in
  the pipeline ever chokes on getters, os.M becomes a function.
- Launcher dispatches launch(id) through a closure per row; cb ids 4–14
  map to rows in ROM_ORDER order — the fingerprint table in p2-hits.log.

### What should be done in the future
- Phase 3: load() + assets; the walk's arena numbers justify the gc()
  before load.

### Code review instructions
- `git show <this commit>`; run `tools/js/build_bytecode_apps.sh` and read
  `tools/js/build/pulp_all.js` top to bottom — it is the whole OS in
  execution order.
- Device: `js hits` on home must show 11 rows at y=150..777; tap-walk per
  p2-walk.log.

## Step 12: Phase 3 — native load(), flash assets, the image shrinks to the OS core

Apps are no longer in the bytecode image. `load(path)` is real: it serves
`rom:<id>` from NUL-terminated flash assets (zero-copy `JS_Parse` straight
from flash) and `/sdcard`-rooted paths through the files sanitizer into a
lazy 64 KiB PSRAM buffer, evaluates under a 3 s deadline with the caller's
deadline saved/restored, and returns the file's value. The loader
gc()s, loads, validates and only then crosses the app-switch boundary.
The image dropped 49,408 → 10,656 bytes and internal RAM gained exactly
the difference.

### Prompt Context

**User prompt (verbatim):** (see Step 10)

**Assistant interpretation:** Phase 3 of the plan.

**Inferred user intent:** Apps load dynamically; reflash only for OS changes.

**Commit (code):** (this commit) — "ESP-55 P3: native load(), ROM app assets, catalog/loader on load(), image = OS core"

### What I did
- `main/js_assets.{h,cpp}`: EMBED_TXTFILES all 12 `tools/js/apps/*.js`
  (main/CMakeLists.txt), `AssetsFind(name, &src, &len)` registry (len
  excludes the appended NUL; symbol names confirmed against IDF's
  `data_file_embed_asm.cmake` — `_binary_2048_js_start` is legal).
- `main/app_js.cpp`: real `js_load` replacing the throwing stub (same
  name/arity → **no stdlib/atom regeneration**); `s_loads`,
  `s_last_load_ms`, 64 KiB `s_load_buf`; deadline save/restore because
  load() runs inside a live CallCb deadline; evidence lines
  `js load: <path> <bytes> <ms>` / `js load FAILED: <path>`.
- `JsSnapshot.loads/last_load_ms` + `js status` print; probe 23.
- `os/20-catalog.js`: ROM_APPS with metadata + `src:'rom:<id>'` (reader
  hidden); `os/30-loader.js`: `RUN.desc=null; gc(); load(e.src)` →
  validate → enter → main; launcher iterates ROM_APPS.
- `build_bytecode_apps.sh`: image = `cat os/*.js` only.

### Why
- Descriptor-shaped files from P2 made this a plumbing change: no app file
  was touched in P3.

### What worked (gate)
- Image 10,656 B (estimate was ~16 KB); internal_free 124,947 → 159,631
  (+34,684 B — the bytecode copy shrank by exactly 34,696 B).
- Probe 23: `dice typeof=function id=dice abi=2`; rom miss / bad path /
  missing SD file all throw catchable TypeErrors.
- All 11 launcher rows load-and-launch from flash: `js load: rom:library
  1323 bytes 15 ms` … `rom:settings`; every `pulp screen:` line correct;
  12 returns to home.
- Ten consecutive dice launches: 10 × `35 ms`, arena stable ≈40 KB after
  22 loads (the gc()-before-load keeps the P2 walk's 114 KB drift down),
  exceptions 0 throughout.

### What didn't work
- N/A first-try on device; one design nit found while writing: failing
  loads return before `s_loads++`, so `loads=` counts *evaluated* loads
  only — documented here, acceptable.

### What I learned
- `EMBED_TXTFILES` `_end` points past the appended NUL (len = end−start−1),
  and IDF's MAKE_C_IDENTIFIER only prefixes `_` when the *whole* string
  starts with a digit — `_binary_2048_js_start` needs no special casing.
- Measured load() cost matches the Phase 0 prediction exactly (dice 35 ms
  both as raw eval and as load()).

### What was tricky to build
- The deadline nesting: `load()` is called from inside a JS callback that
  already has a 1 s CallCb deadline; naively clearing `s_deadline_us` after
  load would have removed the outer deadline. Saved/restored instead.
- Asset symbol naming for `2048.js` (digit-leading basename) — verified in
  the IDF cmake script rather than trial-and-error.

### What warrants a second pair of eyes
- `js_load`'s error paths (fclose before every return; `more || got ==
  kLoadBufBytes` oversize check); the loader's `desc && typeof desc.main`
  validation order; whether `loads` should also count failed loads.

### What should be done in the future
- Phase 4: SD catalog + seeding (needs `assets.copy` native + manifest
  scan); Phase 5–6: HTTP push/pull; the README "adding an app" section
  should be updated when Phase 4 lands (today: EMBED_TXTFILES + js_assets
  row + ROM_APPS entry).

### Code review instructions
- `git show <this commit>`: `main/app_js.cpp` (js_load), `main/js_assets.*`,
  `os/{20-catalog,30-loader}.js`, `build_bytecode_apps.sh`.
- Device validation: `js probe 23`; tap-walk (p3-walk.log); flatness
  (p3-flat.log): ten `js tap 270 235` + `js swipe 5` cycles → ten
  `js load: rom:dice 2685 bytes 35 ms` lines, arena_used stable.

## Step 13: Phase 4 — SD catalog, seeding, launcher merge (+P5 natives)

The card is now an app store: on first boot the OS seeds all 12 ROM apps
to `/sdcard/apps/<id>.js` with one-line seed-marked manifests, an async
manifest scan builds `SD_APPS`, and the launcher shows the merged catalog
— SD overrides ROM (except `settings`, the recovery app), seeded copies
keep ROM metadata (2048's dynamic subtitle survives), operator installs
get a `*`, broken manifests a `!`. Settings grew an Apps screen
(remove/rescan/clear-state). The `apps` native singleton and the
`/apps/upload` POST route landed in the same stdlib regeneration.

### Prompt Context

**User prompt (verbatim):** "ok phases 4-7 (included). commit at appropriate intervals and keep a detailed diary as you work (using the diary format from the skill) ."

**Assistant interpretation:** Implement Phases 4, 5, 6 and 7 with per-phase commits and diary steps.

**Inferred user intent:** Finish the ticket's original scope: card + network app management.

**Commit (code):** (this commit) — "ESP-55 P4: SD catalog + seeding + merge, apps singleton, upload route, probes 23-25"

### What I did
- Stdlib (ONE regeneration for P4+P5): `apps` singleton — `count/name/
  copy/writeText/received/uploadName` (`tools/js/pulp_stdlib.c`,
  `mqjs_stdlib_pulp.c`, `app_js_bindings.h`, `pulpjsc.c` STUBs,
  `gen_pulp_stdlib.sh` + bytecode rebuild).
- `main/js_assets.cpp`: `AssetsCopy`/`AssetsWriteText` (sync owner-side
  writes: sanitizer → mkdir parent → write → fsync); `main/js_apps.cpp`
  bindings; `ModuleId::Apps` + `kDoneAppsUpload=30`.
- `main/net_serve.cpp`: `POST /apps/upload?name=<id>` — name regex
  [a-z0-9_-]{1,24}, 32 KiB cap (413), single upload slot (503), stream to
  `upload.part`, unlink+rename (FATFS rename does not overwrite), minimal
  manifest only if none exists, `PostModuleDone(Apps, …)`; upload-name
  mailbox + `ServeAppsUploadName()`.
- OS core: `scanApps` (files.list → sequential files.read of `*.json` →
  `JSON.parse(files.line(0))`), `merge()` per the guide's rule, `seedApps`
  (sync via `apps.copy`/`apps.writeText`, only when `/apps` missing),
  `appsWatch` (upload watcher re-registered by every `enter()`), OS routes
  `/apps/list` + `/apps/run?id=` with `PENDING_LAUNCH` picked up by the
  home tick, launcher markers, boot = seed → home → scan → rebuild home,
  `os.clearAllState`, settings Apps screen.
- Probes 24 (assets/copy/writeText/SD-load/catalog) and 25 (watcher holds
  the slot; name mailbox).

### What worked
- First boot: `pulp apps: seeded 12/12`, `scanned 12 manifest(s)`; the
  launcher stays 11 rows (seeded copies carry no `*` by design); dice now
  loads from the card: `js load: /apps/dice.js 2685 bytes 36 ms` (first
  SD access 114 ms, warm 36 ms — same as flash); settings correctly stays
  `rom:settings`; settings main screen fingerprint 5 → 6 rows (Apps row);
  probes 23/24/25 pass.

### What didn't work
- `js probe 25` initially returned InvalidArgument silently: the owner's
  probe dispatch was capped at `arg <= 44` (= probe 24) — widened to 49
  (`app_owner.cpp:238`). Symptom to remember: evals does not advance and
  the snapshot is unchanged.
- Probe 25 v1 then failed with `TypeError: module busy` — correctly: the
  OS's own `appsWatch()` holds the Apps slot from every `enter()`. The
  probe was wrong, not the code; it now asserts busy=yes.

### What I learned
- A module-cb slot can be OS-owned-by-convention: `appsWatch` re-registers
  after every resetTree exactly like `osRoutes`, so the slot is
  permanently taken and apps can never steal upload completions.
- FATFS `rename()` fails onto an existing name — unlink first.

### What was tricky to build
- Seeding synchronously vs. the async files module: manifests are written
  with the new sync `apps.writeText` because chaining 24 `files.write`
  completions through the boot sequence (under JsRunPulp's 3 s deadline)
  would be a state machine for no benefit. The whole seed pass is ~24
  small writes on the owner, well under the deadline.
- Merge metadata semantics: seeded entries keep ROM titles/subtitles
  (functions can't live in JSON), operator manifests win when `seed` is
  absent.

### What warrants a second pair of eyes
- `ServeAppsUpload` (error paths, `.part` cleanup, the shared
  `s_upload_busy` slot with the images upload); `merge()` precedence; the
  probe-range constant (now 49).

### What should be done in the future
- P5 device gate over real WiFi (curl push + /apps/run), P6 pull install.

### Code review instructions
- `git show <this commit>`: `main/js_apps.cpp`, `js_assets.cpp` (WriteWhole),
  `net_serve.cpp` (ServeAppsUpload), `tools/js/os/20-catalog.js`.
- Device: `js probe 24` / `25`; first-boot evidence `pulp apps: seeded
  12/12`; `js hits` on settings main = 6 rows.

## Step 14: Phase 5 — HTTP push, /apps/list, hot reload: verified over real WiFi

The developer loop is real: `06-pulp-app-push.sh hello.js --host <ip>
--run` uploaded a brand-new app over WiFi, the OS watcher rescanned, the
launcher rebuilt, `/apps/run` queued it, and the home tick launched it —
`GET /status` then reported `"app":"hello"`. No reflash anywhere.

### Prompt Context

**User prompt (verbatim):** (see Step 13)

**Assistant interpretation:** Phase 5 with a live-network gate.

**Inferred user intent:** One-command app deploys from a laptop.

**Commit (code):** (this commit) — "ESP-55 P5: PUT support for curl -T, push script, live HTTP gate"

### What I did
- Brought the device up (`net joinsaved` → 192.168.0.149, `serve start 80`).
- `scripts/06-pulp-app-push.sh` (curl -T + optional /apps/run).
- Wrote a demo `hello.js` descriptor; pushed; hot-ran.
- Fixed the one failure: `curl -T` sends **PUT**, the wildcard handler only
  accepted POST → 405. Added a PUT wildcard handler
  (`max_uri_handlers` 3), dispatching PUT/POST `/apps/upload` to
  `ServeAppsUpload` (images upload stays POST-only).

### What worked
- Push 200 → `/apps/list` gained `{"id":"hello","source":"sd"}` → run
  `{"run":"hello"}` → within one home tick `/status` `"app":"hello"`.
- Error paths from the host: bad name → 400, 40 KB body → 413, unknown id
  → 404, run while an app is showing → 409 (launcher-only by design).

### What didn't work
- `curl: (22) ... 405` on the first push (PUT vs POST, above); the
  follow-up `/apps/run` then 404'd because the app never landed.

### What I learned
- `curl -T` = PUT; the guide's own §6.7 examples used `-T`, so the design
  implied PUT support all along — now explicit.
- `/status` is the most reliable remote assertion of "what is on the
  panel" (`"app":"hello"`), better than scraping console output.

### What was tricky to build
- Nothing beyond the method mismatch; the P4 plumbing did all the work.

### What warrants a second pair of eyes
- The PUT handler registration (third wildcard slot) and the POST/PUT
  dispatch asymmetry (images stays POST-only on purpose).

### What should be done in the future
- P6 pull install; P7 soak.

### Code review instructions
- `git show <this commit>` (net_serve.cpp PUT paths, push script).
- Live: `06-pulp-app-push.sh <app.js> --host pulp.local --run`, then
  `curl http://pulp.local/status`.

## Step 15: Phase 6 — pull install, and two real bugs the gate caught

`installFromUrl(url, done)` lives in the OS core (Settings drives it from
a new Install-from-URL keyboard screen; test drivers can call it
directly). The end-to-end gate was deliberately meta: a driver app was
*pushed* over HTTP (P5) whose `main` *pulls* `pulled.js` from a laptop
HTTP server (P6) — and the device ended up running the app it had
downloaded itself (`/status` → `"app":"pulled"`). The gate caught two
genuine firmware bugs on the way.

### Prompt Context

**User prompt (verbatim):** (see Step 13)

**Assistant interpretation:** Phase 6 with a live pull gate.

**Inferred user intent:** Install apps with no laptop-side tooling beyond a URL.

**Commit (code):** (this commit) — "ESP-55 P6: installFromUrl + URL screen, load NUL fix, files list cap 64, probe 26"

### What I did
- `kFilesMaxBody` 16→32 KiB; `installFromUrl` + `idFromUrl` in
  os/20-catalog.js (http.get limit 32768 → `apps.writeText` module +
  manifest-if-missing → `scanApps` → `done(msg)`); Settings `url` screen
  (URL keyboard: letters/digits + `: / . - _` row + GET); probe 26
  (writeText+load round trip, id-derivation edge cases) with cleanup.
- Gate rig: `python3 -m http.server 8123` on the host (192.168.0.39),
  `driver.js` pushed via script 06, `pulled.js` served.

### What didn't work (both found by the gate, both real)
1. **`load()` of SD files parsed past EOF.** Symptoms: probe 26's file
   failed `SyntaxError: expecting ';' at 1:66` (one past EOF), later
   `invalid lvalue at 2:2`, and `driver.js` failed with `undefined` —
   positions *beyond the file*. Cause: the mquickjs lexer uses a NUL
   sentinel (flash assets are NUL-terminated by EMBED_TXTFILES; every
   JS_Eval caller passes C strings); my SD read buffer wasn't terminated,
   so parsing continued into stale bytes from the previous load. Fix:
   `s_load_buf[got] = '\0'` (first attempt appended `\n` on the ASI
   theory — wrong, disproved by the next run). P4's SD loads had only
   *appeared* to work because the buffer tail happened to be benign.
2. **The 33rd file in /sdcard/apps silently vanished.** After seeding
   (24 files) + hello + driver + probe leftovers + pulled (33 files),
   `files.list`'s 32-entry mailbox dropped `pulled.json`: install
   reported success, catalog never showed it. Fix: `kFilesMaxList` 32→64
   and probes now delete their scratch files. This was the P4 "fine v1"
   assumption failing within hours — recorded as a lesson.

### What worked
- probe26: `write=0`, `load=probe26 main=function`, `id1=dice id2= id3=`
  (uppercase and empty rejected).
- Driver chain: `js load: /apps/driver.js 584 bytes 13 ms` →
  `driver: installed pulled (542B)` → after the cap fix `/apps/list`
  shows hello+driver+pulled → `/apps/run?id=pulled` → `"app":"pulled"`.

### What I learned
- "Success" one layer up (installFromUrl's done) hid the list-cap loss —
  end-to-end assertions (`/apps/list`, `/status`) catch what unit-level
  status codes cannot.
- The NUL-sentinel contract for `JS_Parse`/`JS_Eval` buffers is
  undocumented in the engine header; recorded here and in the load()
  comment.

### What was tricky to build
- Diagnosing parse errors "past EOF" required correlating byte counts
  (65/584) with error positions and remembering what previously occupied
  the shared load buffer.

### What warrants a second pair of eyes
- The NUL fix (`fread` cap now kLoadBufBytes−1); kFilesMaxList memory
  cost (64 × ~48 B static); installFromUrl's manifest-if-missing rule.

### What should be done in the future
- P7 soak + docs; consider `files.list` pagination if /apps ever grows
  past 64 entries (launcher + images + books also share the mailbox).

### Code review instructions
- `git show <this commit>`; on device: `js probe 26`, then the meta-gate:
  push driver (script 06), `curl /apps/run?id=driver`, watch
  `driver: installed pulled`, `curl /apps/list`, run pulled.

## Step 16: Phase 7 — soak, README, close-out

The 22-minute soak walked the launcher through every app in a loop
(scripts/07-pulp-soak.py) while an HTTP poller hit /status and /apps/list
every 10 s. Result: 25 full cycles, 275 dynamic loads, 274 screen
presents, zero JS exceptions, arena oscillating in GC steady-state
(126,224 / ~137,000 B, final 43,456 B after the last collection —
never approaching the 192 KiB ceiling). HTTP polling: 119 ok / 11 failed
(timeouts while the single-worker httpd waited behind long panel
refreshes — the known ESP-53 "one task, sequential requests" behavior,
not a regression). README gained the apps workflow and the download-mode
serial gotcha.

### Prompt Context

**User prompt (verbatim):** (see Step 13)

**Assistant interpretation:** Final phase: soak + docs + close-out.

**Inferred user intent:** Confidence the loader holds up under sustained use.

**Commit (code):** (this commit) — "ESP-55 P7: soak script + results, README apps section"

### What I did
- scripts/07-pulp-soak.py (hold-open port, tap-walk all 11 rows per cycle,
  `js pulp` after 2048's G.DOWN trap, per-cycle js status sampling).
- Ran 22 min soak + HTTP poller; collected transcripts (scratchpad
  p7-soak.log / p7-soak-summary.txt / p7-http.txt).
- README: serial discipline (download-mode diagnostic, hold-open client),
  console list updated, new "Apps (ESP-55)" section (descriptor contract,
  override order, adding apps with and without reflash, caps).

### What worked
- Zero exceptions over 275 loads / 25 cycles; arena flat in steady state;
  every app present cycled correctly.

### What didn't work
- 11/130 HTTP polls timed out during panel refreshes (single httpd worker
  blocked behind e-ink presents; pre-existing, documented).

### What should be done in the future
- Phases 8–10 (multi-context runtime, UI sandbox, browser) per the guide.

### Code review instructions
- Re-run: `python3 <ticket>/scripts/07-pulp-soak.py --minutes 5 --output /tmp/soak.log`.

## Step 17: Phase 8 — the binding layer goes multi-context

Every piece of per-engine state — pages, hit regions, dynamic text
values, the callback registry, home/sleep callbacks, the page timer —
moved from file-scope globals into `JsCtxState`, resolved in O(1) from
the engine's per-context opaque slot (a new 4-line `JS_GetContextOpaque`
accessor). `g_os` holds the ROM image and all built-in apps exactly as
before; `g_fg` names the panel owner; contexts are created and destroyed
at run time. Probe 27 proves the model on hardware.

### Prompt Context

**User prompt (verbatim):** "Do phase 8 - 10, commit at appropriate intervals and keep a detailed diary as you work (using the diary format from the skill)" (plus mid-turn: "also do the QR code / pulp.local/apps")

**Assistant interpretation:** Implement the forward phases from the guide (multi-context runtime, UI sandbox, browser), and add a pulp.local/apps URL + QR surface to Settings along the way.

**Inferred user intent:** The browser vision, on the runtime that makes it safe.

**Commit (code):** (this commit) — "ESP-55 P8: multi-context binding layer (JsCtxState), probe 27"

### What I did
- `app_js_internal.h`: `JsCtxState` (ctx, arena, kind, pages[12], dyn[48],
  hits[48], next_cb, home/sleep cbs, timer), `ModuleCb{owner, cb}`,
  `kMaxContexts=3`; lifecycle API `CreateContext / DestroyContext /
  SwitchForeground / StateOf / EvalInto / LoadInto / CallCbIn /
  PresentPage(st, …)`.
- `app_js.cpp` rewritten around the model: kernel eval per context; the
  bytecode image is a CreateContext argument (OS only); gesture dispatch,
  tick, dyn refresh operate on `g_fg`; module completions deliver to the
  registering context; serve routes hard-target `g_os`; `resetTree`
  scopes to the calling context (arena reset only when it owns the
  panel); `p.show()` claims the foreground for a background context
  (the browser error-page path); the swipe-home grammar is now ENFORCED
  for non-OS foregrounds: control returns to the OS and a reclaim hook
  (`g_page_reclaim`) frees the page context.
- Dependent TUs patched: `js_pages` (per-state page table, home/sleep),
  `js_widgets` (dyn), `js_serve` (route → OS), `js_files/http/wifi`
  (shared `CancelModuleCb`), `js_probes` (`g_os->ctx`).
- Engine: `JS_GetContextOpaque` (vendored copy, next to the setter).
- Probe 27 (native): second 64 KiB context — globals isolated (`os sees=
  undefined`), independent cb registries (os_next_cb=20, probe=1),
  foreground untouched, OOM caught inside the small context with the OS
  healthy after, and PSRAM delta after teardown = 0.

### What worked
- Two-error first build (dyn scope), then clean; full regression on
  hardware: launcher fingerprint, dice/settings loads, probes 1/13/23
  unchanged.

### What didn't work
- Probe 27's OOM leg caught the exception but died building the report
  message: `'' + e` allocates while the arena is still full, so the
  *report* OOM'd (uncatchable second failure, `last_error="InternalError:
  out of memory"`, missing print). Fixed by `gc()` before printing —
  a lesson worth keeping: after catching OOM, collect before allocating.

### What I learned
- The int-only `CallCb` boundary and packed-int handles made the ABI
  context-agnostic already; the refactor was state plumbing, no API
  change — exactly what R-MULTICTX predicted.
- `ctx->opaque` doubles as the log-function cookie; storing the state
  pointer there is free and makes `StateOf` one load.

### What was tricky to build
- Ownership rules more than code: the widget arena belongs to the
  foreground (so `resetTree` from a background context must not reset
  it), routes belong to the OS, module completions belong to their
  registrant, and `show()` claims the panel. Each rule is one guard, but
  finding where each belonged was the design work.

### What warrants a second pair of eyes
- `SwitchForeground` clearing the outgoing context's cursor/hits/dyn;
  DestroyContext orphaning module cbs; the enforced home grammar in
  `JsHandleGesture` (teardown-before-callback ordering).

### What should be done in the future
- P9: UI stdlib + nav; P10: browser + QR/URL surface.

### Code review instructions
- `git show <this commit>` — read `app_js_internal.h` first, then
  `app_js.cpp` top to bottom; `js probe 27` on hardware.

## Step 18: Phase 9 — the UI sandbox stdlib and the nav mailbox

Pages now run in a context whose standard library can only draw. The
second `JSSTDLibraryDef` (`js_stdlib_ui`) is the generated stdlib re-
emitted with every non-UI native #defined to one `js_ui_denied` stub —
identical ROM object table, identical atoms (page scripts parse exactly
like apps), different C function table, enforced by the engine's call
dispatch rather than JS discipline. `nav.go/back/reload` record a request
in a native mailbox and post `ModuleDone{Nav}`; the browser (OS context)
watches. Probe 28 runs a probe page inside the sandbox and prints the
denial matrix from within.

### Prompt Context

**User prompt (verbatim):** (see Step 17)

**Assistant interpretation:** Phase 9 of the forward design.

**Inferred user intent:** Untrusted page scripts that cannot do anything but display.

**Commit (code):** (this commit) — "ESP-55 P9: UI sandbox stdlib, nav mailbox, page assets, probe 28"

### What I did
- Stdlib (one regeneration): `nav` {go, back, reload, url} + `browser`
  {run, close, watch, navUrl, navKind} singletons; `ModuleId::Nav` +
  `kDoneNavRequest=40`; pulpjsc stubs.
- `main/js_stdlib_table_ui.c`: 100+ denial #defines with a stated
  maintenance rule (new natives are denied by default) and
  `#define js_stdlib js_stdlib_ui` to re-emit the generated tables.
- `main/js_browser.cpp`: `RunPage(path, url)` — teardown old page ctx →
  CreateContext(96 KiB, UI stdlib) → eval `page:ui-helpers` → LoadInto
  the page script → root the descriptor as a page-global (`__page`) →
  validate via a driver eval → SwitchForeground → `__page.main(ui, nav)`
  under a 3 s deadline; every failure tears down and returns to the OS.
  nav verbs post the deferred Nav completion (the watcher runs on a later
  owner pass, safely outside the page's call frame). The reclaim hook
  frees the page context on the enforced swipe-home.
- Page assets namespace (`page:`): `tools/js/pages/{ui-helpers,
  probe-page}.js` embedded and served by `PageAssetsFind` — separate from
  app assets so seeding never copies them to the card.
- Probe 28.

### What worked
- On hardware, first try after two missing-declaration fixes:
  `probe-page: files=denied http=denied serve=denied wifi=denied
  load=denied reset=denied paper=denied store=denied apps=denied
  browser=denied`; `url=probe://x abi=2`; page presents
  (`pulp screen: page/probe`, foreground kind 1→0); `run=0 close=0`;
  zero exceptions.

### What didn't work
- First build: `g_page_reclaim` was defined in app_js.cpp but not
  declared in the internal header; js_browser.cpp also needed
  app_files.h for `kFilesMaxPath`. Two-line fixes.

### What I learned
- Re-emitting the generated stdlib under #define renames costs one extra
  ROM table in flash but keeps the atom space literally identical — the
  property that makes page scripts and app modules the same dialect with
  different capabilities.
- Rooting a loaded descriptor by storing it as a context global
  (`__page`) then driving it with fixed eval strings avoids every native
  GC-rooting hazard the direct JS_Call path would have had.

### What was tricky to build
- The teardown ordering: nav requests must reach the watcher OUTSIDE the
  page's call frame (the watcher usually destroys the posting context) —
  hence the completion queue, not a direct call; and `DestroyContext`
  switches foreground first so the arena never renders dead widgets.

### What warrants a second pair of eyes
- The deny list completeness (audit against app_js_bindings.h); RunPage's
  failure paths (each TeardownPage returns foreground to the OS).

### What should be done in the future
- P10: the browser app over this runtime; the deny-by-default rule should
  eventually be enforced by the stdlib generator rather than by review.

### Code review instructions
- `git show <this commit>`: js_stdlib_table_ui.c (rule + list),
  js_browser.cpp (RunPage), probe 28 output in scratchpad p9-gate.log.

## Step 19: Phase 10 — the PULP browser, live over WiFi, plus the QR install surface

The device now browses. A page is a JS descriptor served by any web
server; the browser app (OS context) fetches it, caches it to
`/web/page.js`, and `browser.run` executes it in a fresh sandboxed page
context. The whole navigation loop ran live against a laptop `python3 -m
http.server`: remote launch with a URL, relative `nav.go`, a ticking
clock page, the enforced swipe-home reclaim, and a hostile page that was
deadline-killed onto the browser's error page with the launcher one swipe
away. Settings gained the requested `pulp.local/apps` QR screen.

### Prompt Context

**User prompt (verbatim):** (see Step 17)

**Assistant interpretation:** Phase 10 + the QR/URL surface.

**Inferred user intent:** The browser vision demonstrated end to end.

**Commit (code):** (this commit) — "ESP-55 P10: browser app, page server, /apps/run url arg, QR web-install screen"

### What I did
- `tools/js/apps/browser.js` (ROM app, catalog row "Browser"): URL
  keyboard + history (8), relative/absolute URL resolution, fetch via
  `http.get(...).limit(32768)` → `apps.writeText('/web/page.js')` →
  `browser.run(path, url)`; `browser.watch` re-armed on every delivery;
  failures close the page context and present a `bfail` error page.
- Kernel: `/apps/run?id=<id>&url=<u>` fills `PENDING_ARG` (launcher tick
  passes it to `launch(id, arg)`) — remote `curl` can open a page in one
  request.
- `scripts/08-pulp-page-server.py`: reference server + four demo pages
  (menu, clock with dyn-text tick, about, hostile).
- Settings → Apps → **Web install**: QR of `http://pulp.local/apps`
  (version-2 matrix precomputed by `scripts/09-gen-qr.py`, embedded as 25
  row bitmasks, drawn run-length-encoded across TWO canvases because the
  canvas command list caps at 96 and the code needs 170 row-runs) + the
  URL and the curl push one-liner.
- Fixed a gate-found UX bug: appsScreen listed 15 apps before the action
  rows, pushing Web install off the 960 px panel — actions now come first.

### What worked (all on hardware, live network)
- `curl "/apps/run?id=browser&url=http://192.168.0.39:8123/menu.js"` →
  `js load: /web/page.js 571 bytes 6 ms`, foreground kind 1,
  `pulp screen: browser/...menu.js`, `/status` `"app":"browser"`.
- Tap on a menu row → `nav.go('about.js')` → relative resolve → fetch →
  new page context (`browser/...about.js`), zero exceptions.
- Clock page: **39 diff presents** from the page context's dyn text +
  1 s tick — the full present machinery works inside the sandbox.
- Swipe-home (twice): `browser: page reclaimed (home gesture)` → home.
- Hostile page (tries `files.remove` + `wifi.forget`, then `for(;;){}`):
  denials no-op, the loop dies at the 3 s deadline
  (`last_error="InternalError: interrupted"`), `page=bfail` error page,
  exactly one exception, launcher reachable, dice.js untouched.
- QR screen: home → Settings → Apps (14 regions, actions on top) →
  `settings-web`, both canvases under the command cap, no exceptions.

### What didn't work
- Two empty console transcripts during the gate: the hold-open client
  silently exits when another instance still flocks the port, and an
  `until pgrep …` wait-loop matched its own command line (the documented
  ESP-53 self-match trap, hit again). Bracket-expressions or just waiting
  are the fixes; noted for the client's future --wait flag.
- First tap test aimed at launcher-row coordinates on a *page* (pages
  have no launcher header, rows sit higher) — js hits, then tap.

### What I learned
- A page context exercises every runtime path (pages, hits, dyn, tick,
  presents) with zero page-specific code in the present pipeline — the P8
  state model carried it.
- The `/web/page.js` cache file makes the fetched-source path identical
  to the SD-app path; the browser adds no second loading mechanism.

### What was tricky to build
- URL resolution without regexes worth trusting: three cases (absolute,
  host-relative `/x`, dir-relative `x`) against the stored base.
- The QR under the canvas cap: run-length rows (170 runs) split 13/12
  across two stacked canvases (86/84 ≤ 96).

### What warrants a second pair of eyes
- browser.js's watch/re-arm ordering (re-arm BEFORE dispatching, so a
  nav during fetch is not lost); the `/apps/run` query parsing (url= takes
  the raw remainder); the appsScreen reorder.

### What should be done in the future
- nav.reload/back deserve a dedicated probe; bytecode pages
  (content-type switch) remain unimplemented by choice; a `--wait` flag
  for the console client.

### Code review instructions
- `git show <this commit>`; live demo: `scripts/08-pulp-page-server.py` +
  `curl "http://pulp.local/apps/run?id=browser&url=http://<host>:8123/menu.js"`.
- Transcripts: scratchpad p10-*.log (menu/about/clock/hostile/QR).
