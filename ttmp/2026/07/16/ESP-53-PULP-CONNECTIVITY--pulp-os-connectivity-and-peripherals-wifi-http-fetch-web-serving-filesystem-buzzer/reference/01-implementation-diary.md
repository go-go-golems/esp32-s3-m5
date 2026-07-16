---
Title: Implementation Diary
Ticket: ESP-53-PULP-CONNECTIVITY
Status: active
Topics:
    - papers3
    - esp32s3
    - microquickjs
    - architecture
    - eink
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Chronological diary of ESP-53 work: design docs, onboarding guide, and (later) implementation steps."
LastUpdated: 2026-07-16T18:36:11.328080972-04:00
WhatFor: "Resuming ESP-53 work: read this before touching the ticket."
WhenToUse: "Before resuming or reviewing ESP-53 work."
---

# Diary

## Goal

Capture the ESP-53 connectivity journey: design phase (connectivity guide, system onboarding guide), then implementation (buzzer → files → wifi → http → serve → settings → hardening).

## Step 1: Full-system onboarding guide (design-doc/02)

The ticket already carried the connectivity design guide (design-doc/01), but that document assumes its reader knows the whole PULP OS stack. The user asked for a guide "explaining all the parts of the system needed to understand what it is" — a genuine onboarding document. So this step produced design-doc/02: a 15-section, ~530-line system guide covering the hardware and its constraints, repo layout and toolchain rules, all four s3paper components, the present pipeline, the five MicroQuickJS facts that dictate every binding convention, the binding layer (handles, `__cbs`, dispatch, tick), the stdlib/bytecode regeneration protocol, the complete v2 JS API reference (verified against `app_js_bindings.h`), pulp.js patterns, the owner/event architecture, serial and validation discipline, a pointer into design-doc/01, a ten-item gotcha catalog of bugs we actually hit, and a glossary.

It was written from verified sources, not memory: the API tables came from grepping `PULP_JS_FN` prototypes and the console command registrations, and the file map from listing the component include dirs. The PDF was uploaded to the reMarkable next to the connectivity guide.

### Prompt Context

**User prompt (verbatim):** "Ok, Create Create  a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and the nupload to remarkable."

**Assistant interpretation:** The existing design-doc/01 is a 255-line connectivity design summary; the user (having just viewed it in md-view) wants the full intern treatment — a comprehensive document explaining every part of the system a newcomer needs, not just the ESP-53 delta. Store it in the ESP-53 ticket and upload the PDF to reMarkable.

**Inferred user intent:** Make the ticket self-sufficient for onboarding: someone (or a future session) should be able to read the ticket's docs alone and understand the whole stack well enough to implement connectivity safely.

### What I did
- `docmgr doc add` → design-doc/02, then wrote the 15-section guide (architecture diagrams, pipeline diagram, pseudocode for handle unpacking and dispatch, API tables, file references throughout).
- Verified API surface against `0114-papers3-pulp-os/main/app_js_bindings.h` (all `PULP_JS_FN` entries) and console commands against `app_console.cpp` registrations before writing sections 9 and 12.
- Related the four load-bearing source files to the doc; updated the ticket changelog.
- Uploaded via `remarkable_upload.py --force` (pandoc/xelatex → rmapi); the file landed in `ai/2026/07/16/` root, so moved it with `rmapi mv` into `ai/2026/07/16/ESP-53-PULP-CONNECTIVITY/` next to the connectivity guide.

### Why
- design-doc/01 deliberately stayed lean (255 lines) and defers system context; a new reader hitting it cold lacks the widget-tree, GC, and toolchain background it presumes.
- Writing API references from grep output rather than recall avoids documenting methods that don't exist — the guide is meant to be trusted verbatim.

### What worked
- The `PULP_JS_FN` prototype list in `app_js_bindings.h` is a complete, single-file inventory of the JS API — ideal documentation source of truth.
- `rmapi mv` cleanly relocated the misplaced upload without re-rendering.

### What didn't work
- `remarkable_upload.py <file.md>` without `--ticket-dir` infers the ticket dir as `~/.local` and uploads to the date root instead of the ticket subfolder. Worked around with `rmapi mv`; next time pass the md file *and* let the remote dir be fixed afterwards, or use `--mirror-ticket-structure` from the ticket dir.

### What I learned
- The upload script's remote-folder inference is based on the ticket dir of the *default documents*, not of the md arguments; explicit paths bypass it.

### What was tricky to build
- Scope control: the guide had to cover ten subsystems without duplicating the three vault deep-dives or design-doc/01. The resolution was a strict "what a newcomer needs before design-doc/01 makes sense" test per section, with pointers out to the deep-dives for rationale. The gotcha catalog (section 14) compresses the ESP-50/51/52 postmortems into ten actionable items instead of retelling them.

### What warrants a second pair of eyes
- Section 9's API tables: verified against binding prototypes, but signatures/argument orders (e.g. canvas verb parameter order `line(x0,y0,x1,y1,t,gray)`) should be spot-checked against `js_widgets.cpp` parsing before an intern treats them as gospel.
- Section 2's timing claims (full refresh ~1 s, partial ~hundreds of ms) are order-of-magnitude from observation, not measured on this exact panel firmware.

### What should be done in the future
- When ESP-53 implementation starts, keep design-doc/02 sections 9 and 12 updated as `wifi`/`http`/`serve`/`files`/`buzzer` singletons and any new console commands land — the guide claims to be the API reference.

### Code review instructions
- Read `design-doc/02-pulp-os-system-onboarding-guide-every-part-of-the-system-for-a-new-intern.md` end to end; cross-check section 9 against `0114-papers3-pulp-os/main/app_js_bindings.h` and section 12 against `0114-papers3-pulp-os/main/app_console.cpp` `RegisterCommand` calls.
- Validate the reMarkable copy exists: `rmapi ls ai/2026/07/16/ESP-53-PULP-CONNECTIVITY`.

### Technical details
- Upload pipeline: `python3 ~/.local/bin/remarkable_upload.py --force <md>` (pandoc/xelatex + DejaVu → PDF → `rmapi put`), then `rmapi mv "ai/2026/07/16/<name>" "ai/2026/07/16/ESP-53-PULP-CONNECTIVITY/"`.
- Guide structure: §1 what it is, §2 hardware/constraints, §3 repo+toolchain, §4 s3paper_core, §5 m5/storage/runtime, §6 MicroQuickJS facts, §7 binding layer, §8 bytecode toolchain, §9 JS API reference, §10 pulp.js, §11 owner/events, §12 console/validation, §13 ESP-53 pointer, §14 gotcha catalog, §15 glossary.

## Step 2: P0 orientation + P1 buzzer (module, console command, product chimes)

Implementation began. P0 was a baseline `idf.py build` of 0114 (green) plus re-reading the binding/owner/console/stdlib sources to refresh the exact contracts. P1 then added the first ESP-53 module end to end: `app_buzzer.{h,cpp}` (LEDC on GPIO21, lazy init, owner-tick note sequencer, 16-note melody parser), a thin `js_buzzer.cpp` bindings TU exposing the `buzzer` singleton (`tone/beep/stop/melody`), a `buzz` console command riding a new `ConsoleOp::Buzz`, and three product chimes in pulp.js (tea READY rising melody, postcard seal click, 2048 merge tone scaled by points gained, frequency clamped to 1760 Hz).

The hardware gate passed on first flash: `buzz beep` lazily initialized LEDC and sounded, `buzz tone 440 300` played, the demo melody (`880:120,0:40,1109:120,0:40,1319:200` — triad with rests) sequenced all five notes off the owner tick and self-silenced, and `js status` showed `exceptions=0` — which also proves the regenerated atom table and bytecode image load cleanly, since boot would have failed the zero-RAM-atom check otherwise.

### Prompt Context

**User prompt (verbatim):** "Ok, now implement." (followed mid-turn by: "keep a detailed diary as you work (see skill for the format), and commit at appropriate intervals")

**Assistant interpretation:** Execute the ESP-53 task plan phase by phase (buzzer → files → wifi → http → serve → settings → hardening) with hardware gates, per-phase commits, and diary steps.

**Inferred user intent:** Ship the connectivity feature set designed in design-doc/01, with the same disciplined evidence trail as ESP-51/52.

**Commit (code):** f57c61b — "ESP-53 P1: buzzer module (GPIO21 LEDC), buzz console command, product chimes"

### What I did
- `main/app_buzzer.{h,cpp}`: LEDC port of `M5PaperS3-UserDemo/main/hal/hal.cpp:385` (timer 0, low-speed, 13-bit, duty 4096); `BuzzerTone/Beep/Stop/Melody` + `BuzzerTick` sequencer; bounds: 40–12000 Hz, ≤10 s/note, ≤16 notes; freq 0 = rest.
- `main/js_buzzer.cpp`: `js_buzzer_*` bindings; `buzzer` object added to `pulp_stdlib.c`/`mqjs_stdlib_pulp.c`, STUBs to `pulpjsc.c`, prototypes to `app_js_bindings.h`.
- `app_events.h`: `ConsoleOp::Buzz` (arg 0 status / 1 beep / 2 stop / 3 tone arg2=freq<<16|ms / 4 demo melody) + `BuzzSnapshot` in the reply union. Owner handles the op and calls `BuzzerTick` from `TickHooks`.
- `app_console.cpp`: `buzz [status|beep|stop|tone F MS|melody]`.
- pulp.js chimes at the three product moments; regenerated stdlib + bytecode; built; flashed; ran the console gate.

### Why
- Buzzer first per the phase plan: smallest module, exercises the full stdlib-regeneration + console + owner-tick pipeline before any networking complexity arrives.
- Rests are notes with freq 0 rather than a separate mechanism — one code path through `StartNote` and the tick.

### What worked
- The entire chain (stdlib regen → bytecode → flash → audible) passed on the first hardware attempt; the only build fix was a wrong StatusCode name.
- Owner-tick sequencing at the touch producer's 20 ms cadence is audibly clean for 40–300 ms notes.

### What didn't work
- `BuzzStatusCode::Internal` — the s3paper StatusCode enum has no `Internal`; compile error `'Internal' is not a member`. Used `Busy` for LEDC init failure instead.
- First draft of `CmdBuzz` posted the tone op twice (a RunConsoleOp then a RunConsoleOpWithArgs); caught on self-review before build, collapsed to one call.

### What I learned
- `TickHooks` cadence is 20 ms only while touch is enabled (the touch producer task is the metronome); with touch off it degrades to the 500 ms queue timeout. Fine for chimes; anything needing precise off-screen timing must not rely on it.

### What was tricky to build
- Melody advance on skip: an unplayable note (bad freq after storage corruption, say) must not abandon the melody or leave the buzzer stuck sounding. `MelodyAdvance` loops past failed notes and `Silence()`s at the end; a direct `tone()` call preempts an active melody by clearing `melody_active` before `StartNote`.

### What warrants a second pair of eyes
- The `arg2 = freq<<16 | ms` console packing caps ms at 65535 — fine for chimes, but the JS path allows 10 s notes while the console path truncates silently above 65.5 s (both above the 10 s module cap, so no real exposure; still, asymmetry).
- `BuzzerTick` runs on every owner loop pass; confirm no audible glitch when a long present (~1 s full refresh) delays the tick mid-melody (notes stretch, never overlap — by construction, but worth an ear).

### What should be done in the future
- P6's settings app could add a mute toggle (`storeGet('mute')` checked in the JS chime helpers).

### Code review instructions
- Start at `0114-papers3-pulp-os/main/app_buzzer.cpp` (StartNote/MelodyAdvance/BuzzerTick state machine), then `app_owner.cpp` ConsoleOp::Buzz case and TickHooks.
- Validate: `buzz beep && buzz melody && buzz status` over the console client; expect `tones` to advance and `melody (n/n)` to complete; `js status` exceptions=0.

### Technical details
- Gate transcript (device): `buzz init=0→1`, `tones=0→5`, `melody=1 (1/5)` → `melody=0 (5/5)`, all `result=Ok`.
- Bytecode grew to 30216 bytes (one image, well under the ROM atom table budget).

## Step 3: P2 files module + P3.1 completion-mailbox plumbing

This step built the ticket's async backbone and its first consumer. The `ModuleDone` machinery went in first: a new `AppEventKind::ModuleDone` with `ModulePayload{module, kind, value, err}`, `PostModuleDone()` (postable from any task; the queue send/receive is the memory barrier), a per-module pending-callback registry in the JS host (`g_module_cb[ModuleId::kCount]`, one in-flight operation per module, `RegisterModuleCb` throwing "module busy" on overlap), and `JsModuleDone()` in the owner loop which consumes-then-calls so a callback can immediately start the next operation. `resetTree()` clears the registry — an app switch cancels delivery, per design §3 rule 5.

The files module then rode that backbone: `app_files.{h,cpp}` (path sanitizer, mailboxes, five verbs) plus `js_files.cpp` bindings. All ops run synchronously on the owner (whole-file ops at the 16 KiB cap are single-digit ms on this card) but complete through the mailbox path, so JS sees exactly one async model whether the native side blocked or not. Probe 15 validated everything on hardware in one pass, including the full callback-chained write→read→list→remove cycle.

### Prompt Context

**User prompt (verbatim):** (see Step 2 — same implementation directive)

**Assistant interpretation:** Continue the phase plan: P2 files + the P3.1 plumbing it depends on.

**Inferred user intent:** Same as Step 2.

**Commit (code):** 558f4e8 — "ESP-53 P2+P3.1: files module with completion mailboxes, ModuleDone plumbing, probe 15"

### What I did
- `app_events.h`: `AppEventKind::ModuleDone`, `ModuleId{Files,Wifi,Http,Serve}`, `ModuleDoneKind` vocabulary (10–14 for files), `ModuleDonePayload` in the event union.
- `app_owner.{h,cpp}`: `PostModuleDone()`, ModuleDone case → `JsModuleDone`.
- `app_js.{h,cpp}` + `app_js_internal.h`: `g_module_cb[]`, `RegisterModuleCb` (non-function → TypeError; overlap → "module busy"), `JsModuleDone` (clear-before-call), resetTree clearing.
- `app_files.{h,cpp}`: `FilesResolvePath` (leading `/`, charset `[A-Za-z0-9._-/]`, no `//`, no trailing `/`, and ALL dot-leading segments denied — one rule covers `..`, `.`, and `/.s3paper`), `FilesExists` sync, `FilesList` (32-entry mailbox, hidden + overlong names skipped and counted), `FilesRead` (16 KiB PSRAM body + 512-line index, CR stripped at the accessor), `FilesWrite`/`FilesAppend` (16 KiB cap, one-level parent mkdir), `FilesRemove` (unlink, rmdir fallback).
- `js_files.cpp`: `files` singleton (11 functions) with shared `PathOp`/`BodyOp` helpers.
- Probe 15 in `js_probes.cpp`; stdlib/pulpjsc/bindings/CMake updates; regen + build + flash + gate.

### Why
- One async model everywhere: even synchronous ops complete via ModuleDone so app code never needs to know which verbs block. The delivery lands on the next owner-loop pass — real async semantics without worker tasks for ops that don't need them.
- Deny-dot-segments beats a denylist of `/.s3paper`: hidden entries are native-owned by definition, and `..` falls out of the same check.

### What worked
- Probe 15 green on the first flash: `exists /books=1 rel=0`, `deny dotdot=1 state=1 slashes=1`, `cap=2` (CapacityExceeded), `busy=yes`, then the chain `list books n=3` → `write bytes=16` → `read lines=3 l0=alpha l2=gamma` → `list notes first=probe.txt size=16` → `remove ok=1 exists=0`.

### What didn't work
- `-Werror=format-truncation` rejected `snprintf(out.name, 40, "%s", d_name)` (d_name is 256 bytes). Fixed semantically, not just syntactically: names that don't fit the mailbox field are now skipped and counted instead of truncated — a truncated name couldn't be re-opened by JS anyway.

### What I learned
- The engine's argv values can be consumed via `JS_ToCStringLen` after other allocating calls as long as no JS call happens between fetch and use — the write/append bindings register the callback FIRST (allocates), then fetch the body pointer, then run the op (no JS calls). Order documented in js_files.cpp's header comment.

### What was tricky to build
- Callback lifecycle around synchronous failures: `RegisterModuleCb` must precede the op (the op posts the completion), but a synchronous failure (bad path, cap) means no completion will ever arrive — the binding must cancel the registration or the module stays busy forever. `PathOp`/`BodyOp` centralize the register→op→cancel-on-error dance so all five verbs share one correct implementation.
- Probe sequencing: only ONE module operation may be pending when an eval ends, so the probe nests the whole verb chain inside completion callbacks and runs its busy-check against the single outer pending op.

### What warrants a second pair of eyes
- The GC-safety argument in `BodyOp` (holding a JS heap string pointer across the native op) — sound per the no-JS-calls rule, but it is the subtlest invariant added this step.
- `EnsureParentDir` creates one level silently; confirm that matches the intern-guide contract (it documents write() convenience, but deep-tree behavior is "fails" — that is the case here since only one level is created).

### What should be done in the future
- If an app ever needs concurrent files + wifi operations, nothing blocks it (separate module slots); but two concurrent files ops require a queue — noted as out of v1 scope.

### Code review instructions
- Start at `app_files.cpp` `FilesResolvePath` (the security surface) and `js_files.cpp` `BodyOp` (the GC-safety comment), then `app_js.cpp` `JsModuleDone`/`RegisterModuleCb`.
- Validate: `js probe 15` on device; every `probe15:` line in the transcript should match the values quoted above.

### Technical details
- Mailboxes: listing 32 × `{name[40], size, is_dir}`; read body 16 KiB PSRAM + 512 × `{start,len}` uint16 line refs.
- Callback vocabulary: `fn(kind, value, err)` with kinds 10 list / 11 read / 12 write / 13 append / 14 remove; `err` is errno (or −1) when the op itself failed, 0 on success.
