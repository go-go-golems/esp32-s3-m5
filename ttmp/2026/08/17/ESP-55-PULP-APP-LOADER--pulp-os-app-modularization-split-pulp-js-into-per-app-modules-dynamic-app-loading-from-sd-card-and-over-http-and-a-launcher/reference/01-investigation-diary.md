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
    - Path: repo://0114-papers3-pulp-os/main/app_js.cpp
      Note: Step 1 first-hand read of the JS host core
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
