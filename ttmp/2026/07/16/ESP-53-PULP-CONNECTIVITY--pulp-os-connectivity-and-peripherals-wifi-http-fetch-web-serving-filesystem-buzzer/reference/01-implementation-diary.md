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

## Step 4: P3 WiFi — station module, S3WF credentials, joinSaved sequencer

The radio went in. Three moving parts: the S3WF credentials file as a fifth member of the storage component's persistence family (8 fixed `{ssid[33], pass[65], last_ok}` records, magic+version+count+CRC, tmp/fsync/bak/rename, coalesced flush, fault-injection kind 5); `net_wifi.{h,cpp}` as the station module (lazy `esp_wifi` init, non-blocking scan into a 16-entry mailbox, join with 2 event-handler retries plus a 15 s owner-tick timeout, `off` = `esp_wifi_stop`); and the `wifi` JS singleton (16 functions) plus a `net` console command whose join/save/forget subcommands travel in new bounded `str_a[33]/str_b[65]` fields on `ConsolePayload`.

The interesting design piece is the joinSaved sequencer. Credentials are storage (owner-only), but join failures surface on the system event task. Rather than let the event task touch storage, the owner intercepts `ModuleDone{Wifi}` events (`WifiOwnerOnModuleDone`) before JS delivery: a failed join in saved mode quietly starts the next ranked credential and suppresses the event; success marks the credential's `last_ok` ordinal (a monotonic per-file counter, not a clock — it must order across reboots) and lets the event through. JS sees exactly one completion for the whole sequence.

### Prompt Context

**User prompt (verbatim):** (see Step 2; plus mid-turn: "the network locally is yolobolo / bring3248camera, btw. (for later)")

**Assistant interpretation:** Continue with P3; use the provided network for the hardware join gate.

**Inferred user intent:** Same as Step 2; enable real end-to-end validation on their LAN.

**Commit (code):** e65d70b — "ESP-53 P3: wifi module (scan/join/joinSaved/off), S3WF credentials in s3paper_storage, net console command"

### What I did
- `s3paper_storage`: `WifiCredential`/`kMaxWifiCreds` + `WifiCredsLoad/Save/Set/Forget/Count/GetRanked/MarkOk`; wired into mount-load, `DebugReloadState`, `StorageFlushNow/IfDue` (new dirty flag), and `DebugCorruptStateFile` kind 5.
- `net_wifi.{h,cpp}`: state machine (Off/Idle/Scanning/Joining/Up in an atomic), event handlers that touch only POD + `PostModuleDone`, `EnsureUp()` lazy init, `StartJoin`/`SavedTryNext`, `WifiTick` timeout, `WifiOwnerOnModuleDone` interceptor, status + scan-mailbox accessors, `FillNetSnapshot`.
- `app_events.h`: `ConsoleOp::Net`, `NetSnapshot`, string fields on `ConsolePayload`.
- `app_owner.cpp`: Net console case (9 subops incl. `saved`/`results` printers), Wifi interception in the ModuleDone case, `WifiTick` in TickHooks.
- `app_main.cpp`: `nvs_flash_init` with erase-on-version-mismatch (esp_wifi needs it; was absent, as the design's gotcha 6 predicted).
- `js_wifi.cpp` + stdlib/stub/prototype/CMake plumbing (esp_wifi, esp_netif, esp_event, nvs_flash components).

### Why
- Owner-side interception is the only place the joinSaved sequencer can live without violating either the owner-only storage rule or the event-task POD rule.
- `last_ok` as an ordinal instead of a timestamp: `esp_timer` restarts at zero every boot, so a clock would rank a network joined 10 s after this boot above one joined yesterday.

### What worked
- Everything passed on the first flash, including esp_wifi's first-ever inclusion in this firmware: 16-AP scan (yolobolo at −44 dBm), `joinsaved` → `state=up ip=192.168.0.149 rssi=-45`, `last_ok` bumped 0→1, deep-sleep reboot → creds reloaded → `joinsaved` re-acquired the same IP, wrong password → joining→idle after retries, `forget` 1→0, re-save 0→1.
- sdkconfig needed no hand-editing: adding the esp_wifi component let the reconfigure append its defaults.

### What didn't work
- Nothing failed outright this step. (First draft of `WifiCredsGetRanked` was a recursive selection monstrosity — rewritten as a plain index sort before it ever compiled.)

### What I learned
- `net saved` sent immediately after a deep-sleep wake was swallowed once (the boot race from ESP-50 applies after wake too); the follow-up command in the same session worked. Known discipline: settle + retry after wake.

### What was tricky to build
- The join failure path has three writers: the event handler (disconnect+retry), the owner tick (timeout), and the owner interceptor (saved-mode advance). The invariants that keep them from double-firing: the handler only posts fail after retries exhaust while state==Joining; the tick only fires when state==Joining and flips state before disconnecting (so the resulting disconnect event sees Idle and stays silent); the interceptor runs only on posted events. State transitions all go through the atomic.

### What warrants a second pair of eyes
- A join started while state==Up is allowed (OpInFlight only blocks Scanning/Joining) and implicitly drops the current link — intended (how you switch networks) but worth confirming as product behavior.
- The unexpected-disconnect path (state Up → link lost) only flips state to Idle; no JS notification. Apps must poll `wifi.status()`. Fine for v1; the settings app should surface it.

### What should be done in the future
- P7: wire `wifi.off()` into the sleep quiesce sequence (design §9 step 0).
- Consider a `wifi.onDrop(fn)` persistent callback if any app needs push-style link-loss handling.

### Code review instructions
- Start at `net_wifi.cpp`: `HandlerWifi` (event-task constraints), `WifiTick` + `WifiOwnerOnModuleDone` (the three-writer failure path), then `storage.cpp` WifiCreds family.
- Validate: `net save <ssid> <pass> && net scan` → `net results` → `net joinsaved` → `net status` shows `state=up ip=...`; `sleep deep 2` → `net joinsaved` still works.

### Technical details
- Gate transcript: scan=16 APs; `net state=up ip=192.168.0.149 ssid="yolobolo" rssi=-45 saved=1`; post-reboot identical; wrong-pass → `state=idle`; forget → `saved=0`.
- ConsolePayload grew by 98 bytes; event queue (32 deep) static cost +~3 KB. Accepted for POD-rule compliance.

## Step 5: P4 HTTP — bounded fetch builder over a worker task

The first true worker-task module. `net_http.{h,cpp}` holds one request slot (`url[256]`, up to 4 headers, limit ≤ 32 KiB, default 16 KiB) that the owner mutates through builder calls; `HttpSend()` launches a short-lived 6 KiB worker (`xTaskCreatePinnedToCore`, prio 4, core 0) because `esp_http_client` blocks. The worker opens, fetches headers, reads into the PSRAM body mailbox up to the limit (checking an atomic abort flag between reads), indexes lines, clears `in_flight`, posts `ModuleDone{Http, 3, status, len_or_negative_err}`, and deletes itself. HTTPS rides `esp_crt_bundle_attach`. Redirects cap at 3, timeout at 10 s.

The JS surface is the express-flavored builder from the design: `http.get(url).header(k,v).limit(n).done(fn).send()` — the chainable methods return the singleton itself and THROW on misuse (a broken chain should fail at the call site, not surface as a mystery status later); only `send()` returns a status int. Accessors (`body/bodyLine/status/length`) read the mailbox after completion.

### Prompt Context

**User prompt (verbatim):** (see Step 2)

**Assistant interpretation:** Continue with P4 per the phase plan.

**Inferred user intent:** Same as Step 2.

**Commit (code):** eaa9626 — "ESP-53 P4: http fetch builder (worker task, PSRAM mailbox, TLS bundle), http console command, probe 16"

### What I did
- `net_http.{h,cpp}` (module + worker), `js_http.cpp` (builder bindings), `HttpSnapshot` + `ConsoleOp::Http` (`http [status|get URL [LIMIT]|body|abort]`), probe 16, stdlib/stubs/CMake (esp_http_client, esp-tls, mbedtls); `ConsolePayload.str_a` widened to 128 for console URLs.
- Gate on hardware (network joined via `net joinsaved` first): probe 16 full transcript.

### Why
- Worker-task pattern here (unlike files) because esp_http_client genuinely blocks for seconds; the owner must keep servicing touch and ticks during a fetch.
- `in_flight` atomic as the slot-ownership token: owner writes the slot only while false, worker reads it only while true, and the store-release before PostModuleDone pairs with the owner's read after the queue receive.

### What worked
- Probe 16, one flash: `http st=200 len=559`, `https st=200 len=559` (cert bundle first try), `trunc st=200 len=256 (limit 256)`, `timeout st=0 err=-28674` (ESP_ERR_HTTP_CONNECT) after the clean 10 s bound, `busy=yes` for an overlapped get.
- sdkconfig again needed nothing: the mbedtls cert bundle is on by default.

### What didn't work
- `-Werror=format-truncation` on the snapshot's url copy (256 → 64 field) — precision-capped (`%.63s`); intentional truncation, display-only.
- The probe's timeout-leg print vanished from the first transcript: it fired between console sessions, and USB-Serial-JTAG drops output with no reader attached (§12 gotcha, met again). Re-ran with `--settle 30` so the reader stayed attached; full transcript captured.

### What I learned
- `err=-28674` = −ESP_ERR_HTTP_CONNECT (0x7002): esp_http_client's connect failure surfaces on open, not as a status; mapping transport errors to negative `len` keeps the JS signature `(k, status, len)` unambiguous (status 0 ⇒ len is a negative esp_err).

### What was tricky to build
- Callback/slot lifecycle asymmetry: `.done(fn)` registers the module callback but `.send()` may fail synchronously (Busy/OOM) — send cancels the registration on failure. Conversely `.done()` without `.send()` parks a callback until resetTree; documented as single-slot behavior rather than adding timers to reap it.

### What warrants a second pair of eyes
- Worker stack size (6 KiB) with TLS: mbedtls handshakes run mostly on their own internal allocations, but a deep cert chain could stress the worker stack — the 30-min P7 soak should include an https leg.
- `HttpAbort` only stops the read loop; a worker blocked inside connect/handshake still holds the slot until the 10 s timeout. Acceptable v1 semantics, worth documenting in the guide.

### What should be done in the future
- P6 Radio demo will exercise bodyLine() against a real text feed.
- Consider POST support (body upload) only when an app needs it — the slot design extends naturally.

### Code review instructions
- Start at `net_http.cpp` `HttpWorker` (the never-touch-JS rule) and `HttpSend` (slot ownership handoff), then `js_http.cpp` throw-vs-status split.
- Validate: join network, `js probe 16` with a 30 s reader; expect the five `probe16:` lines quoted above.

### Technical details
- Completion: `fn(3, status, len)`; status=0 ⇒ transport failure and len = −esp_err.
- Mailbox: 32 KiB PSRAM body + 1024 line refs; CR stripped at `bodyLine`.

## Step 6: P5 serve — JS routes over the semaphore handoff, static mount

The web server landed: `net_serve.{h,cpp}` (route table, request slot, handoff, static streaming) and `js_serve.cpp` (the express-flavored `serve.get(path).handle(fn)` builder plus `serve.text/json/status` response tokens). The httpd task claims the single request slot, fills uri/query, bumps the slot generation, drains any stale semaphore give, posts `ModuleDone{Serve, route, gen}`, and blocks 5 s on the response semaphore. The owner routes Serve completions to `ServeOwnerDispatch` (they bypass the single-slot module-callback path — routes own their callbacks), runs the handler under the JS deadline, and gives the semaphore only if the generation still matches. `serve.text()` writes the response slot through the same generation guard, so a response computed after an httpd-side timeout is dropped rather than corrupting the next request. Static files stream from `/sdcard/www` on the httpd task itself — the one sanctioned off-owner storage read — with a default `index.html` auto-created at mount. `resetTree()` clears the route table (the callback ids just died with `__cbs`), so an app switch degrades routes to 404/static rather than dangling.

The curl gate from the workstation: `/status` returned live JSON (`{"battery":100,"ssid":"yolobolo","rssi":-45,...}`), `/` served the auto-created index.html, `/note?hello-from-curl` appended to the postcard journal, unknown paths 404. The planned "concurrent second request → 503" test produced 200/200 instead — and investigation showed why: **the default esp_http_server processes all requests on a single worker task**, so a second request is serialized behind the first, never concurrent. The busy-503 path is an unreachable defensive net under this config; the real protection is the 5 s owner-wedge timeout (whose logic is generation-guarded and code-reviewed, but not provokable on demand — nothing in the firmware wedges the owner past the 1 s JS deadline).

### Prompt Context

**User prompt (verbatim):** (see Step 2)

**Assistant interpretation:** Continue with P5 per the phase plan.

**Inferred user intent:** Same as Step 2.

**Commit (code):** 0e404e9 — "ESP-53 P5: serve module (JS routes via semaphore handoff, static mount, default site), serve console command, probe 17"

### What I did
- `net_serve.{h,cpp}`: 8-route exact-match table, `RequestSlot` with atomic busy + generation, `Send503` helper (IDF 5.3 httpd has no 503 enum), `ServeStatic` with `..` rejection + MIME table + 1 KiB chunk streaming, `ServeJsRoute` handoff, `ServeOwnerDispatch`/`ServeRespond` with the generation guard, wildcard GET handler, default-index writer.
- `js_serve.cpp` (10 bindings incl. `JsServeInvokeRoute` owner entry), `ServeSnapshot` + `ConsoleOp::Serve` + `serve` console command, resetTree → `ServeRoutesClear()`, probe 17 (three routes incl. a 600 ms `/slow`, files mount, start, URL print), stdlib/stubs/CMake (esp_http_server).

### Why
- Serve events bypass `g_module_cb`: a server has N persistent route callbacks, not one pending completion — forcing it through the single-slot path would serialize route registration with every other module.
- Generation numbers instead of locks: the only cross-task disagreement is "did httpd give up on this request?", which a monotonic counter answers without blocking either side.

### What worked
- Full curl gate on the first flash after two compile fixes. Auto-created default site worked (index.html appeared on the card and served with text/html).
- `serve status` counters: `requests=6 busy503=0 timeout503=0`, `js exceptions=0` after the whole gate.

### What didn't work
- `HTTPD_503_SERVICE_UNAVAILABLE` does not exist in IDF 5.3's httpd error enum — hand-rolled `Send503` with `httpd_resp_set_status`.
- Two more `-Werror=format-truncation` hits on uri copies (`%.127s` precision caps).
- The concurrent-503 gate as designed cannot fire: single-worker httpd serializes. Documented as an architecture finding rather than forcing an artificial pass.

### What I learned
- Default esp_http_server = one task, sequential request processing. This makes the request slot uncontendable in practice (simplifying the correctness argument to just the timeout path) and means heavy static transfers block JS routes behind them — acceptable for a device status page, worth remembering if anyone mounts big assets.

### What was tricky to build
- The three-way timeout dance: owner wedged pre-dispatch → event processed late → slot generation already bumped by httpd's timeout → dispatch's give is skipped AND ServeRespond drops the write; owner wedged mid-dispatch is bounded by the 1 s JS deadline (< 5 s semaphore), so the give always races ahead of a new claim; a stale give that loses anyway is drained by the next request's `xSemaphoreTake(sem, 0)`. Each leg is one line of code; the correctness story took longer than the implementation.

### What warrants a second pair of eyes
- The mid-dispatch wedge window: if a route handler runs right up against the JS deadline while the semaphore window is nearly exhausted, `ServeRespond`'s gen check and the httpd timeout's gen bump can interleave (check-then-memcpy is not atomic). Requires an owner stall > 4 s before dispatch even starts; documented, accepted for v1.
- `serve.files` accepts a JS-supplied directory (unlike the files module's rooted paths) — deliberate, since pulp.js is trusted firmware code, but the intern guide should flag it.

### What should be done in the future
- P6 settings app: serve on/off toggle + URL display; P7: 30-min curl soak.
- If multi-connection serving ever matters, `httpd_config.max_open_sockets` + worker tasks would resurrect the busy-503 path — the slot logic is already correct for it.

### Code review instructions
- Start at `net_serve.cpp` `ServeJsRoute` (handoff, drain, timeout) and `ServeOwnerDispatch`/`ServeRespond` (generation guard), then `js_serve.cpp` `js_serve_handle`.
- Validate: `net joinsaved`, `js probe 17`, then from the workstation: `curl http://<ip>/status` (JSON), `curl http://<ip>/` (HTML), `curl "http://<ip>/note?x"`, `curl http://<ip>/nope` (404).

### Technical details
- Handoff: slot claim (CAS) → fill → gen++ → drain sem → PostModuleDone → take(5 s); owner: dispatch under 1 s JS deadline → give iff gen matches.
- Response slot: status/type(text|json)/4 KiB body; unfilled slot after dispatch → 500 "route returned nothing".
