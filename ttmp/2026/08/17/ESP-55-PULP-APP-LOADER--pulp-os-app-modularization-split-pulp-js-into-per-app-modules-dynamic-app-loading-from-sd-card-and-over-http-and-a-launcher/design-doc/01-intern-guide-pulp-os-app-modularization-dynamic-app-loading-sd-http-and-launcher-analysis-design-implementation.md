---
Title: 'Intern guide: PULP OS app modularization, dynamic app loading (SD + HTTP) and launcher — analysis, design, implementation'
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
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: repo://0114-papers3-pulp-os/components/mquickjs/mquickjs.c
      Note: JS_LoadBytecode zero-RAM-atom rule and N_ROM_ATOM_TABLES_MAX=2; js_global_eval
    - Path: repo://0114-papers3-pulp-os/components/mquickjs/mquickjs.h
      Note: Bytecode/eval API and JS_EVAL flags
    - Path: repo://0114-papers3-pulp-os/main/CMakeLists.txt
      Note: Where EMBED_TXTFILES for app assets and js_assets.cpp are added
    - Path: repo://0114-papers3-pulp-os/main/app_console.cpp
      Note: Console js command arg encoding; where js load goes
    - Path: repo://0114-papers3-pulp-os/main/app_events.h
      Note: ModuleId enum and ModuleDone kinds; where ModuleId::Apps / kDoneAppsUpload go
    - Path: repo://0114-papers3-pulp-os/main/app_files.cpp
      Note: Path sanitizer, EnsureBody PSRAM buffer pattern, synchronous owner-side I/O reused by load()
    - Path: repo://0114-papers3-pulp-os/main/app_files.h
      Note: files limits (16 KiB body, 512 lines, 96 path)
    - Path: repo://0114-papers3-pulp-os/main/app_images.cpp
      Note: Directory-scan catalog precedent
    - Path: repo://0114-papers3-pulp-os/main/app_js.cpp
      Note: 'JS host core: kernel, __cbs registry, LoadBytecodeApps (boot-only), EvalBounded, js_load stub to become real load(), resetTree'
    - Path: repo://0114-papers3-pulp-os/main/app_js_internal.h
      Note: Limits (kMaxPages/kMaxJsHits/kMaxDynValues), PageEntry, ModuleId callback slots
    - Path: repo://0114-papers3-pulp-os/main/app_owner.cpp
      Note: Owner loop, console op dispatch, boot order
    - Path: repo://0114-papers3-pulp-os/main/net_http.h
      Note: http.get limits (32 KiB body) bounding pull-install
    - Path: repo://0114-papers3-pulp-os/main/net_serve.cpp
      Note: POST dispatch, ServeUpload streaming precedent for /apps/upload, index.html marker seeding, max_uri_handlers
    - Path: repo://0114-papers3-pulp-os/tools/js/os/00-kernel.js
      Note: The OS kernel (former pulp.js prelude); the monolith was split in P1 (commit 17557c9a), see git history
    - Path: repo://0114-papers3-pulp-os/tools/js/build_bytecode_apps.sh
      Note: Bytecode pipeline to be changed to concatenate os/*.js and embed apps/*.js
    - Path: repo://0114-papers3-pulp-os/tools/js/gen_pulp_stdlib.sh
      Note: Atom/stdlib regeneration protocol
    - Path: repo://0114-papers3-pulp-os/tools/js/mqjs_stdlib_pulp.c
      Note: Global table incl. load/eval/print; CONFIG_PULP block
    - Path: repo://0114-papers3-pulp-os/tools/js/pulp_stdlib.c
      Note: Stdlib definition (native API surface); where new natives are declared
    - Path: repo://0114-papers3-pulp-os/tools/js/pulpjsc.c
      Note: Host compiler + STUB list; harness reuses it
    - Path: repo://ttmp/2026/08/17/ESP-55-PULP-APP-LOADER--pulp-os-app-modularization-split-pulp-js-into-per-app-modules-dynamic-app-loading-from-sd-card-and-over-http-and-a-launcher/scripts/01-trial-split-bytecode-sizes.py
      Note: Experiment 1 (per-app bytecode sizes)
    - Path: repo://ttmp/2026/08/17/ESP-55-PULP-APP-LOADER--pulp-os-app-modularization-split-pulp-js-into-per-app-modules-dynamic-app-loading-from-sd-card-and-over-http-and-a-launcher/scripts/02-host-eval-harness.c
      Note: Experiment 2 (source-eval vs bytecode arena/time)
    - Path: repo://ttmp/2026/08/17/ESP-55-PULP-APP-LOADER--pulp-os-app-modularization-split-pulp-js-into-per-app-modules-dynamic-app-loading-from-sd-card-and-over-http-and-a-launcher/sources/01-native-side-map.md
      Note: 'Evidence collection: native side'
    - Path: repo://ttmp/2026/08/17/ESP-55-PULP-APP-LOADER--pulp-os-app-modularization-split-pulp-js-into-per-app-modules-dynamic-app-loading-from-sd-card-and-over-http-and-a-launcher/sources/02-prior-ticket-fact-sheet.md
      Note: 'Evidence collection: prior tickets'
ExternalSources: []
Summary: Intern-level analysis, design and phased implementation guide for splitting pulp.js into an embedded OS core plus per-app source modules, a native deadline-bounded load(path), an SD-card app catalog with ROM seeding, a data-driven launcher, and HTTP install (push via POST /apps/upload, pull via http.get). Grounded in file:line evidence and two host experiments. Extended with a multi-context runtime design (one engine context per app or page) and a page-script browser: web pages served as sandboxed JS builder scripts (UI-only stdlib), i.e. the builder DSL as a cheap markup language.
LastUpdated: 2026-08-17T11:02:44.294456085-04:00
WhatFor: Read to understand what pulp.js and the PULP JS runtime are today, why apps must be loaded as source (one bytecode image per context), what the loader/launcher/catalog/install design is, and how to implement it phase by phase.
WhenToUse: Before implementing ESP-55, when adding an app to PULP OS, or when touching main/app_js.cpp, tools/js/build_bytecode_apps.sh, or the serve/files modules for app loading.
---


# Intern Guide — PULP OS App Modularisation, Dynamic App Loading (SD + HTTP), and the Launcher

## 0. How to read this guide

This document is written for an engineer who has never opened the PULP OS
tree. It explains, in order: what exists today and why it is shaped the way
it is (§3), what we measured to size the design (§4), what is missing (§5),
the proposed design with its API contracts and pseudocode (§6), the
decisions and their alternatives (§7), a phased implementation plan with
file-level guidance and acceptance gates (§8), how to test it (§9), the
risks (§10), and the gotchas you will otherwise rediscover (§11).

Every claim about the current code carries a `file:line` reference. Paths
are relative to the repository root
`/home/manuel/code/wesen/go-go-golems/esp32-s3-m5` unless they start with
`main/`, `tools/`, or `components/mquickjs/`, which are relative to the
firmware directory `0114-papers3-pulp-os/`. Line numbers are as of commit
`5effe4c9` (ESP-54 close-out); if the tree has moved on, the identifiers
still find the place.

Prerequisite reading, in this order, if you want the full background:

1. `ttmp/2026/07/16/ESP-53-PULP-CONNECTIVITY--*/design-doc/02-*.md` — the
   whole-system onboarding guide (hardware, owner task, widget tree,
   engine, binding layer, toolchain, console).
2. `ttmp/2026/07/16/ESP-51-PULP-OS-V2--*/design-doc/01-*.md` — why v2 is a
   native builder API over a POD widget tree, and the kernel/callback design.
3. `ttmp/2026/07/27/ESP-54-PULP-GALLERY--*/design-doc/01-*.md` — the most
   recent module (images), the POST upload handoff, and the decision-record
   format we reuse here.

The two evidence files collected for this ticket are in the ticket's
`sources/` folder: `01-native-side-map.md` (task model, event flows, every
numeric limit, every JS-visible native function, console commands, partition
table) and `02-prior-ticket-fact-sheet.md` (engine constraints, history,
build pipeline, validation practice, storage). This guide quotes the parts
it needs; go there for the exhaustive lists.

## 1. Executive summary

PULP OS v2 is a JavaScript-first operating system for the M5Stack PaperS3
(ESP32-S3, 540×960 16-gray e-ink). Its entire user-facing surface — the
launcher and ten apps (Reader/Library, Dice Tray, Blitz Ink, 2048 INK, Tea
Timer, Postcard, Daily Pulp, Ink, Gallery, Radio, Settings) — is one
1,125-line ES5 file, `tools/js/apps/pulp.js` (36,901 bytes). A host compiler
(`tools/js/pulpjsc.c`) turns it into a single 45,332-byte MicroQuickJS
bytecode image that is embedded in the firmware, copied into internal SRAM at
boot, and run once (`main/app_js.cpp:109-129, :412-427`). Adding an app
means editing pulp.js, adding a hard-coded row to `home()`, regenerating the
image, and reflashing. In ESP-54 the image outgrew the JS arena and the
arena had to be raised from 160 to 192 KiB (`main/app_js.cpp:51-53`).

This ticket proposes to (a) split pulp.js into an **OS core** (kernel
helpers, launcher, settings, installer) that stays in the embedded bytecode
image, and **app modules** — one file per app with a small descriptor
contract; (b) load app modules **on demand from source text** with a
native, deadline-bounded `load(path)` that reads a file (SD card or an
embedded flash asset) and evaluates it into a single JS value; (c) keep an
**app catalog** on the SD card (`/sdcard/apps/<id>.js` + `<id>.json`) that
the launcher merges with the built-in list; and (d) install apps **over
HTTP** in two directions — the browser/`curl` pushes a file to
`POST /apps/upload?name=<id>` (which also gives developers a
"save → curl → tap" inner loop with no reflash), and the device pulls
`http.get(url)` from Settings → *Install from URL*.

The single most important engineering fact behind the design is an engine
constraint: MicroQuickJS can load **one** precompiled bytecode image per
context, and only before any source has been evaluated
(`components/mquickjs/mquickjs.c:182, :12947-12951`). Per-app bytecode
modules are therefore not an option without forking the engine; source
evaluation is available today (`eval` is in the stdlib table,
`main/js_stdlib.h:4323`), and our host measurements (§4) show that a
typical app (2–5 KB of source) compiles into a few KB of retained arena and
needs on the order of 16–24 KiB of transient arena on the 64-bit host — well
inside the 192 KiB arena, and comparable to what the app's *state* already
costs. Loading an app from source at launch is a fraction of an e-ink full
refresh; the exact on-device number is the first thing Phase 0 measures.

What you get at the end: a smaller ROM image (only the OS core is
bytecode), ~45 KB of internal SRAM back, an `apps/` directory of
independently editable files, a launcher that discovers apps instead of
listing them, and a way to put a new app on the device from a laptop in one
command.

Two further sections extend the design beyond the single-context loader.
§6.11 shows how the binding layer can host **several MicroQuickJS
contexts** (one for the OS, one per running app or page): the engine has no
global state, every context carries its own arena, atoms and one bytecode
image, and the existing int-only callback boundary is already
context-agnostic — only the singleton state in `main/app_js*.cpp` has to
move behind a per-context struct. §6.12 uses that to sketch the **PULP
browser**: a web server returns a page as a small JS script, the device
runs it in a context whose stdlib exposes *only* the UI builders and a
`nav` object — files, network, radio, store and the OS callbacks are
denied at the function-table level — so the builder DSL becomes a cheap,
fully sandboxed markup language for an e-ink client. These are designed
now so the loader and bindings are not built in a way that forecloses them,
and scheduled as Phases 8–10.

## 2. Problem statement and scope

### 2.1 What the operator wants

- Edit or add one app without touching the others, and without regenerating
  and reflashing the whole firmware for a JS-only change.
- Put an app on the device from a laptop or phone (over WiFi) and have it
  appear in the launcher.
- Keep the device usable if an app is broken: a bad app must fail *when
  launched*, with a readable error, never at boot.
- Keep the "paperback of computers" feel: the launcher is a list, tap opens,
  swipe-down goes home; nothing here adds chrome.

### 2.2 In scope

1. Splitting pulp.js into `tools/js/os/*.js` (core, still concatenated into
   the one bytecode image) and `tools/js/apps/*.js` (app modules).
2. An **app module contract** (descriptor object with `id`, `title`,
   `subtitle`, `abi`, `main(os)`), and an **`os` facade** that replaces the
   free helper functions apps use today (`enter`, `chrome`, `hintFooter`,
   `M`, `netUp`, `announce`, `home`, `library`, `reader`).
3. A native **`load(path)`** (repurposing the throwing stub at
   `main/app_js.cpp:572-574`) that reads a file from `/sdcard/...` or from an
   embedded flash asset (`rom:<name>`) and evaluates it under a deadline,
   returning the file's value.
4. An **app catalog** on SD (`/sdcard/apps/`), a **ROM asset registry** for
   the built-in apps, first-boot **seeding** of ROM apps onto the card, and a
   **launcher** that merges the two.
5. **HTTP install**: push (`POST /apps/upload`) and pull (`http.get` from
   Settings), plus `GET /apps/list` and a developer hot-reload route.
6. Per-app **state retention across app switches** (`os.state`) replacing
   the module-level globals (`DZ`, `BZ`, `GG`, `TT`, `PC`, `DP`, `INK`,
   `GAL`, `RA`) that survive today only because everything is global.
7. Probes, a dev script, docs, and a diary.
8. **Forward design (scheduled after Phase 7):** a multi-context runtime
   (one MicroQuickJS context per app or page, §6.11) and, on top of it, a
   **page-script browser** — a PULP app that fetches pages from any web
   server as small JS scripts and runs them in a UI-only sandbox, using the
   builder DSL as a cheap markup language (§6.12). Designed here because
   it constrains the loader and the binding layer now.

### 2.3 Out of scope (explicitly deferred)

- **Per-app bytecode modules** and any engine patch to stack ROM atom
  tables (`/* XXX: could stack atom_tables */`, `mquickjs.c:12949`). See
  R-SOURCEEVAL in §7 for why, and §10 for the "context restart per launch"
  alternative that would make it possible later.
- **A JS sandbox.** Apps run with the same authority as the OS core. The
  design bounds *time* and *memory* and keeps the OS core in ROM so a bad
  app cannot brick boot, but it does not restrict what a loaded app may call
  (see R-TRUST).
- **OTA firmware updates.** The partition table has a single 4 MiB `factory`
  slot and no OTA slot (`partitions.csv`); this ticket updates JS, not
  firmware.
- **`require()`/ES modules.** MicroQuickJS has no module system
  (`ESP-53 design-doc/01:33`); apps are single files.
- **A host simulator** for JS behaviour (the host has only the compiler and
  the harness from §4). Worth its own ticket.
- Icons, grids, folders in the launcher.

### 2.4 Non-goals that look like goals

- *"Make eval safe against hostile code."* Not attempted; see R-TRUST.
- *"Keep the old `entryRow` list."* The launcher becomes data-driven; the
  hard-coded list goes away on purpose.
- *"Zero behaviour change for users."* Two visible changes are accepted:
  app state no longer lives in globals (it lives in `os.state`, same
  observable effect), and the launcher gains an *Apps* row in Settings.

## 3. Current-state architecture (evidence-based)

### 3.1 The system in one picture

```
 ┌───────────────────────────────────────────────────────────────────────┐
 │  tools/js/apps/pulp.js  (1,125 lines, ES5)                            │
 │   prelude ─ home() ─ library/reader ─ dice ─ blitz ─ 2048 ─ tea ─      │
 │   postcard ─ daily ─ ink ─ gallery ─ settings ─ radio ─ boot           │
 └──────────────┬────────────────────────────────────────────────────────┘
                │ tools/js/build_bytecode_apps.sh  (host: pulpjsc)
                ▼
 ┌───────────────────────────────────────────────────────────────────────┐
 │  main/js_pulp.h  kJsBytecode_pulp[45,332]  ──► copied to internal SRAM │
 │  at boot, JS_RelocateBytecode + JS_LoadBytecode (before the kernel     │
 │  eval), JS_Run once with a 3 s deadline      (main/app_js.cpp:109-129) │
 └──────────────┬────────────────────────────────────────────────────────┘
                │ native builder API (stdlib table: tools/js/pulp_stdlib.c)
                ▼
 ┌───────────────────────────────────────────────────────────────────────┐
 │  main/js_*.cpp bindings   ─  main/app_js.cpp (kernel, __cbs, dispatch) │
 │  files/http/serve/wifi/mdns/images/buzzer/battery/book/store singletons│
 └──────────────┬────────────────────────────────────────────────────────┘
                ▼
 ┌───────────────────────────────────────────────────────────────────────┐
 │  s3paper_runtime (arena, present)  s3paper_core (POD widget tree,     │
 │  layout, diff, refresh planner)  s3paper_m5 (EPD backend)             │
 │  s3paper_storage (SD, state files)   — one owner task on core 1       │
 └───────────────────────────────────────────────────────────────────────┘
```

Everything above the bindings runs inside a 192 KiB MicroQuickJS arena in
PSRAM (`main/app_js.cpp:53, :377-385`), on the single `ui_owner` task
(core 1, prio 5, 8 KiB stack; `main/app_owner.cpp:610-611`). Other tasks —
touch polling, the transient HTTP worker, the httpd server, mDNS, the
console REPL — never touch JS or the widget arena; they post POD events
into a 32-entry queue that the owner drains (`main/app_owner.cpp:556-571`).
That is the "one owner" rule and it does not change in this ticket.

### 3.2 pulp.js anatomy

pulp.js is a flat script: top-level `var`s and `function`s, all global.
It is organised in fifteen banner-delimited sections (`// ---- name --`).
The table below is the mechanical trial split we ran (§4.1): every section
was cut at its banner and compiled on its own with the host compiler.

| Section | Lines | Source B | Bytecode B | Role | State globals | Native services used |
|---|---:|---:|---:|---|---|---|
| prelude | 1–77 | 2,694 | 3,612 | `P`, `ROUTES_READY`, `M`, `pad2/fmtClock`, `osRoutes()`, `enter()`, `announce()`, `chrome()`, `hintFooter()` | `P.app`, `M` | serve, battery, wifi, mdns, images, paper |
| home | 78–140 | 2,433 | 3,888 | launcher: header glyphs, 11 hard-coded `entryRow`s, tick registers OS routes | — | wifi, battery, storeGet, serve |
| library | 141–173 | 1,148 | 2,124 | book list | — | libraryCount/Line/Rescan |
| reader | 174–219 | 1,366 | 2,796 | 24-line page reader | `RD` | book* |
| dice | 220–298 | 2,388 | 4,028 | 2d6/d20/coin/d% | `DZ` | Math.random |
| blitz | 299–356 | 1,920 | 3,572 | chess clock, dyn text + tick | `BZ` | millis |
| 2048 | 357–483 | 4,152 | 5,312 | game, traps G.DOWN | `GG` | storeGet/Set, buzzer |
| tea | 484–557 | 2,489 | 4,268 | steep timer, tick, melody | `TT` | storeGet/Set, buzzer |
| postcard | 558–623 | 2,166 | 3,756 | one-line keyboard → file | `PC` | appendPostcard, buzzer |
| daily | 624–680 | 1,769 | 3,280 | random page | `DP` | book*, library* |
| ink | 681–795 | 3,607 | 4,236 | three canvas scenes | `INK` | canvas, millis |
| gallery | 796–859 | 2,359 | 3,028 | image browser | `GAL` | images, mdns, serve, paper.refreshTurns |
| settings | 860–1034 | 5,447 | 8,720 | wifi/serve/margins/radio-off, scan, password keyboard | `SET`, `WIFI_STATES`, `KB_ROWS`, `netUp()`, `setRow()` | wifi, serve, storeSet |
| radio | 1035–1119 | 2,799 | 4,052 | https quote poster | `RA` | http, files, libraryRescan |
| boot | 1120–1125 | 164 | 444 | `M = storeGet('margin', 40); home();` | — | storeGet |
| **whole file** | 1,125 | **36,901** | **45,332** (sum of parts: 57,116) | | | |

Three things to notice:

- The **prelude and settings are the OS**, not apps: `enter()`, `osRoutes()`,
  the margin `M`, the chrome helpers, `netUp()` and `setRow()` are used by
  several sections. Any split has to name this layer.
- Every app is a **function that rebuilds its page from scratch on entry**
  and keeps its state in a module-level global that survives `resetTree()`
  (`ESP-51 diary:331`: "apps rebuild their page on entry, keeping app STATE
  in JS globals"). Dynamic loading has to preserve that property
  explicitly, because a freshly evaluated file starts with fresh globals.
- Apps reach each other by **calling global functions**: `daily()` calls
  `reader(idx)` and `library()`; `2048`'s home button calls `home()`;
  `settingsScan()` calls `settingsPass()`. The facade in §6.3 turns those
  into `os.launch('reader', idx)` / `os.home()`.

### 3.3 The prelude: the OS that lives inside the monolith

```js
// pulp.js:52-64
function enter(name) {
  P.app = name;
  resetTree();                                  // native: main/app_js.cpp:603-635
  paper.home(function () { home(); });          // swipe-down fallback
  paper.sleepImage(function () { ... });        // sleep screen builder
  osRoutes();                                   // /status and /images/list
}
```

`enter()` is the **app-switch boundary**. `resetTree()` resets the widget
arena (128 nodes), invalidates every retained `Widget`/`Page` wrapper by
bumping generations, clears the dyn-text table, resets the callback
registry (`__cbs = [null]`, ids restart at 1), drops the swipe-home and
sleep-image callbacks, cancels every pending module completion, and clears
the web route table (`main/app_js.cpp:603-635`). Everything the previous
app registered is gone; only JS globals survive. That is exactly the
"unload" a dynamic app needs, and it already exists.

`osRoutes()` (`pulp.js:26-50`) re-registers the OS-owned web routes
`/status` and `/images/list` after every `resetTree`, and only when the
server is running — a lesson from the ESP-53 soak, where an app switch
silently wiped the routes (`ESP-53 diary:429`). The home page tick
re-checks `serve.url()` so routes appear once the server starts after boot
(`pulp.js:131-136`).

`chrome(title)` and `hintFooter(hint)` (`pulp.js:68-76`) are the two
layout idioms every app uses; `M` (40 or 0) is the global content margin
toggled in Settings and read at build time by every app (`pulp.js:10-12`).
`netUp(fn)` (`pulp.js:865-869`) brings the radio up with saved credentials
before Radio tunes or Serve starts.

### 3.4 The launcher today

`home()` (`pulp.js:97-139`) builds a header (title, battery + wifi glyphs as
a dyn text, tagline, rule), then a `list()` with eleven `entryRow(label,
sub, fn)` calls, each row a `col().onTap(fn)` whose `fn` is the app's entry
function. Discovery is a compile-time fact: **to add an app you edit this
list**. The footer hint and the 5 s tick (`p.every(5000)`) that refreshes
the glyphs and registers OS routes complete the page. The row list is also
the launcher's *fingerprint* in the validation harness (a known number of
tap regions, currently 11 rows + header) — a data-driven launcher changes
that fingerprint, which the probes in §9 account for.

### 3.5 The kernel and the callback registry

The RAM-evaluated kernel is two lines (`main/app_js.cpp:93-97`):

```js
var __cbs = [null];
var G = {TAP:0, LONG:1, LEFT:2, RIGHT:3, UP:4, DOWN:5, TICK:100};
```

Every JS closure the native side needs to call later — `onTap` handlers,
`page.on` gesture handlers, tick handlers, dyn text functions, module
completion callbacks, route handlers, `paper.home`, `paper.sleepImage` —
is stored in `__cbs[id]` by `RegisterCb` (`main/app_js.cpp:295-307`) and
called by `CallCb(id, a, b, c, argc)` with **int32 arguments only** and a
1 s deadline (`main/app_js.cpp:309-337`). Strings never cross the boundary
in a call; they are read back through accessor functions ("mailboxes"),
e.g. `files.line(i)` after `files.read` completes.

```
   JS closure ──RegisterCb──► __cbs[id]  (id = g_next_cb++)
                                 ▲
   native event (tap, tick,      │ CallCb(id, ints...) under 1 s deadline
   completion, route) ───────────┘
   resetTree(): __cbs = [null], g_next_cb = 1   ← whole registry dropped
```

There is no per-app ownership of ids: the registry is reclaimed wholesale
at `resetTree`. For app modules this is convenient (nothing to unregister)
and also the reason the OS must re-register its own callbacks after every
switch (`enter()` does).

Module completions use one slot per module (`g_module_cb[ModuleId]`,
`main/app_js_internal.h:50-52`; `ModuleId{Files, Wifi, Http, Serve,
Images}`, `main/app_events.h:40-47`): starting a second `files.*` op while
one is pending throws `TypeError: module busy`. The slot is cleared *before*
the callback runs so a callback may chain the next op
(`main/app_js.cpp:528-543`).

### 3.6 Two ways code enters the engine

**Path A — precompiled bytecode (boot only, one image).**
`LoadBytecodeApps()` (`main/app_js.cpp:109-129`) copies the embedded image
into internal SRAM, relocates it in place, and registers it. Two engine
rules make this a boot-time, single-image mechanism:

```c
// components/mquickjs/mquickjs.c:12943-12961 (JS_LoadBytecode)
if (ctx->unique_strings_len != 0)
    return JS_ThrowInternalError(ctx, "no atom must be defined in RAM");
/* XXX: could stack atom_tables */
if (ctx->n_rom_atom_tables >= N_ROM_ATOM_TABLES_MAX)      // == 2 (line 182)
    return JS_ThrowInternalError(ctx, "too many rom atom tables");
```

Slot 0 of the atom-table array is the stdlib; slot 1 is the app image. Any
`JS_Eval` (the kernel, a probe, `eval()`) creates RAM atoms for identifiers
not in the stdlib table, after which loading is refused for the life of the
context. Both facts were confirmed empirically on the host harness (§4.2:
the second image fails with `InternalError: too many rom atom tables`).
Bytecode is also atom-coupled: it embeds indices into the stdlib atom
table, so it must be regenerated after every stdlib change
(`tools/js/build_bytecode_apps.sh:5-7`).

**Path B — source evaluation (available at any time).** The parser is
compiled into the firmware and used by the kernel eval and the 22 probes
through `jsi::EvalBounded(code, timeout_ms, name)` (`main/app_js.cpp:163-175`),
which sets the interrupt deadline, calls `JS_Eval`, counts, and records
exceptions. The JS-visible `eval(str)` maps to the engine's `js_global_eval`
(`components/mquickjs/mquickjs.c:15330-15341`), and `new Function(...)`
compiles too. Nothing in the firmware disables them. Evaluated code
**creates RAM atoms and lives in the arena** (function bytecode, constant
pools, strings), which is what §4 measures. The `load(path)` global exists
in the stdlib table (`tools/js/mqjs_stdlib_pulp.c:380`) but throws
`"load() not supported"` (`main/app_js.cpp:572-574`) — its atom already
exists, so giving it a real implementation needs no atom regeneration for
the name itself.

Consequence: **modules can be source files, not bytecode files.** The
built-in image stays exactly one image (the OS core); apps are text.

### 3.7 The build pipeline

```
 tools/js/pulp_stdlib.c ─┐  gen_pulp_stdlib.sh (host gcc + mquickjs_build.c)
 tools/js/mqjs_stdlib_pulp.c ┘      │
                                    ├─► main/js_stdlib.h            (32-bit stdlib table)
                                    └─► components/mquickjs/mquickjs_atom.h  (atom ids)
                                              │  MUST be followed by:
 tools/js/apps/*.js ──► build_bytecode_apps.sh: builds host stdlib table + copies the
                       engine into tools/js/host/, builds pulpjsc, compiles EVERY
                       apps/*.js into main/js_<name>.h  (only js_pulp.h is included)
                                              │
                       idf.py build ──────────┘   (IDF 5.3.4 pinned; see README)
```

Two properties matter for this ticket. First, `build_bytecode_apps.sh`
already loops over every `apps/*.js` (`:35-40`) — multi-file compile costs
nothing; the constraint is on the *load* side (§3.6). Second, the atom
header is regenerated from the stdlib definition, so **adding a native
function** (a new `JS_CFUNC_DEF`) means: edit `pulp_stdlib.c` (or the
global table in `mqjs_stdlib_pulp.c`), add the `PULP_JS_FN` declaration in
`main/app_js_bindings.h`, add a `STUB()` in `tools/js/pulpjsc.c`, run
`gen_pulp_stdlib.sh`, run `build_bytecode_apps.sh`, build. Skipping the
first script gives a device that rejects the bytecode ("no atom must be
defined in RAM"); skipping the second gives stale atom indices
(`ESP-53 design-doc/02:385`).

### 3.8 Native modules the loader will lean on

| Module | JS surface (relevant subset) | Limits (file) | Notes for the loader |
|---|---|---|---|
| files (`main/js_files.cpp`, `main/app_files.cpp`) | `exists(p)`, `list(p,fn)`, `read(p,fn)`, `write(p,body,fn)`, `append`, `remove`, `name/size/isDir(i)`, `line(i)/lineCount()` | path 96, list 32 entries (names ≤39 chars), body 16 KiB, 512 lines (`app_files.h:20-23`); paths rooted at `/sdcard`, charset `[A-Za-z0-9._-/]`, no dot-segments, no `//` | I/O is synchronous on the owner; only the completion is deferred. `list` gives the launcher its SD catalog; `write` caps a pulled app at 16 KiB per call. |
| http (`main/js_http.cpp`, `main/net_http.cpp`) | `get(url).header(k,v).limit(n).done(fn).send()`, `status()`, `length()`, `body()`, `bodyLine(i)` | url 256, body cap 32 KiB (PSRAM), default 16 KiB, 10 s timeout, 3 redirects, TLS bundle, GET only (`net_http.h:18-22`) | Pull-install: `body()` returns the whole file as one JS string; 32 KiB is the ceiling for a pulled app. |
| serve (`main/js_serve.cpp`, `main/net_serve.cpp`) | `get(path).handle(fn)`, `text/json/status`, `query()`, `files(prefix,dir)`, `start(port)`, `stop()`, `url()` | 8 routes, path 64, query 256, response body 4 KiB, single request slot, 5 s handoff, 1 s JS deadline (`net_serve.h:24-27`) | Routes die at `resetTree`; the OS re-registers its own. **POST is native-only**: `Handler()` accepts POST solely for `/images/upload` (`net_serve.cpp:299-304`) and streams the body to SD on the httpd task (`:118-195`) — the precedent for `/apps/upload`. |
| images (`main/app_images.cpp`) | `count/name/display/remove/received(fn)` | 64 images, name 32, 280 KiB, `/sdcard/images` | Precedent for a directory-scan catalog and an upload completion (`kDoneImagesUpload = 20`). |
| store (`main/js_services.cpp:223-258`) | `storeGet(key, def)`, `storeSet(key, v)` | 16 records × {key[16], int32} (`s3paper_storage/src/storage.cpp:133-141`) | Ints only: the app catalog cannot live here; a "last app" id could, an app name cannot. |
| console (`main/app_console.cpp`) | `js status|probe N|pulp|tap X Y|swipe K|hits`, `serve`, `http`, `net`, `images`, `sd`, `heap` | reply queue 4, `js` timeout 15 s | No `eval` command exists; probes are fixed strings. Phase 0 adds a `js load <path>` op. |

### 3.9 The owner task and the async model (short form)

```
 [any task] ──PostEvent(POD)──► queue(32) ──► ui_owner: HandleEvent → TickHooks
                                                   │
        JS call files.read(p, fn) ─ sync fread ─ Post(kDoneFilesRead) ─┘ later pass:
                                                   JsModuleDone → CallCb(fn, 11, n, 0)
 [httpd task] route hit ─ claim slot ─ PostModuleDone(Serve, route, gen) ─ wait ≤5 s
                                                   owner: CallCb(route cb) → serve.text() → give
```

Every JS-visible async verb follows the ESP-53 five-rule contract
(`ESP-53 design-doc/01:41-65`): one in-flight op per module, `module busy`
on overlap, completion as `fn(kind, value, err)` with ints, results read
back through accessors, and `resetTree` cancels delivery. A pull-install
(`http.get` → `files.write`) chains two modules through their callbacks;
nothing new is needed.

### 3.10 Memory budget

| Resource | Size | Consumers today | Effect of this ticket |
|---|---|---|---|
| JS arena (PSRAM) | 192 KiB (`app_js.cpp:53`) | kernel, all globals/closures of the running image, transient parse | Only the OS core's globals stay resident; one app's compiled functions + state at a time; parse transient at launch (§4). |
| Internal SRAM | 512 KiB; ~76 KB free with WiFi+httpd up (`ESP-53 diary:429`) | 45 KB bytecode copy (`app_js.cpp:111`), task stacks, WiFi/lwIP | The image shrinks to the OS core (est. 12–15 KB) → ~30 KB back. Apps loaded from source use the arena (PSRAM), not internal RAM. |
| PSRAM | 8 MB | frame arena 320 K, http body 32 K, files body 16 K, image buffer 253 K | + a `load()` read buffer (proposed 64 KiB) |
| Flash | 16 MB; app image 1.84 MB of 4 MiB `factory` | fonts, TLS bundle, bytecode | + embedded app sources (~40 KB) — negligible. |
| SD card | FAT | books, .s3paper state, www, images | + `/sdcard/apps/` |

## 4. Measurements

Two host experiments were run to size the design; both scripts live in the
ticket's `scripts/` folder and are re-runnable. The device was not
reachable during this investigation (the host's `cdc_acm` module was not
loaded; see the diary), so **on-device numbers are Phase 0 work** (§8).
Read the host numbers as ratios: the host engine is a 64-bit build
(`JSValue` is 8 bytes, device 4), so absolute arena bytes are inflated by
roughly 1.5–2×, and x86 parse times are 50–100× faster than a 240 MHz
Xtensa core.

### 4.1 Per-app bytecode sizes (`scripts/01-trial-split-bytecode-sizes.py`)

The table in §3.2 is this experiment's output. Findings:

- Bytecode is **larger** than source (1.17–2.05×; whole file 1.23×): the
  image carries every function's constant pool, unique strings and
  `pc2line` debug table (`pulpjsc` compiles with `JS_EVAL_STRIP_COL`, not
  without debug info).
- Splitting into fifteen images would cost +26 % (57,116 vs 45,332 B)
  because each image repeats shared strings — irrelevant once apps are
  source, but it rules out "many small images" even if the engine allowed
  them.
- The OS core (prelude + home + settings + boot ≈ 10.7 KB source) would be
  a ~16 KB image; the eleven app sections total ~26 KB of source.

### 4.2 Source-eval vs bytecode in one arena (`scripts/02-host-eval-harness.sh`)

The harness links the firmware's vendored engine (`tools/js/host/mquickjs.c`)
with the host stdlib table and the compiler's stub natives, creates a
192 KiB context, evaluates the two-line kernel, then either `JS_Eval`s each
section as source or (mode `bc`) compiles each to a host bytecode image,
loads all images before the kernel, and `JS_Run`s each. Sections only
*define* functions and state objects (nothing calls the stubbed natives), so
what is measured is exactly the "load an app module" cost.

**Source eval, per section (host, 64-bit; arena 192 KiB):**

| section | source B | parse+run ms (x86) | arena delta (transient incl.) | retained after GC |
|---|---:|---:|---:|---:|
| prelude | 2,694 | 0.20 | 22,424 | 4,912 |
| home | 2,433 | 0.19 | 25,928 | 6,376 |
| library | 1,148 | 0.11 | 13,456 | 2,208 |
| reader | 1,366 | 0.16 | 17,840 | 3,240 |
| dice | 2,388 | 0.27 | 30,312 | 5,360 |
| blitz | 1,920 | 0.20 | 24,480 | 4,752 |
| 2048 | 4,152 | 0.49 | 47,896 | 6,496 |
| tea | 2,489 | 0.28 | 31,704 | 5,296 |
| postcard | 2,166 | 0.22 | 26,392 | 4,640 |
| daily | 1,769 | 0.19 | 23,720 | 3,584 |
| ink | 3,607 | 0.40 | 42,968 | 5,072 |
| gallery | 2,359 | 0.18 | 20,648 | 3,560 |
| settings | 5,447 | 0.62 | 64,128 | 11,960 |
| radio | 2,799 | 0.30 | 34,912 | 4,952 |
| all fourteen, one eval | 36,737 | 4.86 | 113,504 | 70,616 |

**Smallest arena in which a section evaluates without `out of memory`**
(the engine collects garbage on demand, so this is the true transient
requirement, not the "arena delta" column): dice 16 KiB, 2048 16 KiB, ink
14 KiB, settings 24 KiB, all fourteen at once 96 KiB.

**Bytecode, one image with all fourteen sections:** `JS_Run` 0.01 ms,
arena retained after GC **8,080 B** (the function code stays in the image
buffer outside the arena; only globals and closures land in the arena).
Loading a *second* image fails: `InternalError: too many rom atom tables`.

Reading these numbers on the device scale (÷1.5–2 for arena, ×50–100 for
time):

- A typical app costs **~2–4 KB of retained arena** and needs
  **~8–16 KiB of free arena during parse**; the biggest (settings)
  ~6 KB / ~16 KiB. Against a 192 KiB arena that is comfortable — *provided
  one app is loaded at a time and the previous app's functions become
  garbage at the switch* (§6.5). One caveat: the ESP-54 diary records that
  the current *bytecode* image "OOM'd at 160 KiB after the gallery app was
  added" (`ESP-54 reference/01:191, :222`) although the host run of the
  same image retains only ~8 KB — the boot-time arena high-water mark of
  the image has never been profiled and is the first number Phase 0 takes.
- Parse+run of a 2–5 KB app is estimated at **15–60 ms** on the device;
  the full clean-full e-ink present it precedes is measured at ~243 ms of
  panel work (`ESP-51 scripts/output/p56-final-a.log:8`) plus refresh time.
  Launch latency will be dominated by the panel, not the parser. Phase 0
  confirms this with `js load` timings.
- Evaluating the *whole* OS from source instead of bytecode would retain
  ~35–45 KB and need ~50–60 KiB transient — feasible, but the ROM image is
  free (0.01 ms, 8 KB), so the OS core stays bytecode (R-ONEIMAGE).

### 4.3 Device measurements (Phase 0, measured 2026-08-19)

`js measure` (commit 4d59929a) on the PaperS3, 192 KiB PSRAM arena, home
screen showing, whole ESP-54 bytecode image loaded:

| metric | value |
|---|---|
| arena at boot / after GC | 14,956 B / **7,496 B** (the image retains almost nothing — matches the host's 8,080 B) |
| dice (2,388 B source): eval / transient / retained after GC | **35.4 ms** / +24,264 B / **+3,024 B** |
| settings (5,447 B source): eval / transient / retained after GC | **80.6 ms** / +50,840 B / **+7,456 B** |
| dice ×10 with GC between | flat at 17,980 B — no creep |
| internal free before = after | 124,931 B (evals live entirely in the PSRAM arena) |

Rules of thumb: **≈15 ms and ≈1.4 KB retained per KB of app source**;
transient ≈10× retained. The Phase 0 gate passes: the largest app parses in
81 ms (< 150 ms budget, and well under one panel refresh), and the arena
returns to an exactly flat baseline after GC. Host→device conversion came
out at ×130 for time and ×0.62 for retained arena — close to the §4
estimates. Loading every app of the current OS from source at launch is
therefore comfortably affordable; R-SOURCEEVAL stands on measured ground.

Still open (moves to Phase 3/4 validation): SD read time for a 5 KB file
(expected < 5 ms) and the internal-RAM win from shrinking the image.

## 5. Gap analysis

| Requirement | Current state | Gap |
|---|---|---|
| Apps as separate files | one flat script, sections by comment banner (`pulp.js`) | split + build-time concatenation for the OS core; app files as assets |
| Add an app without reflashing | image is embedded, listing is hard-coded (`pulp.js:114-124`) | `load()`, SD catalog, data-driven launcher |
| Load code at run time | `eval()` exists but is unbounded and takes a string; `load()` throws (`app_js.cpp:572-574`) | native `load(path)` through `EvalBounded`, reading a file/asset directly |
| Discover apps | none | `/sdcard/apps/*.json` scan + ROM registry |
| Install over HTTP | POST only for `/images/upload`; `http.get` GET-only, 32 KiB body; `files.write` 16 KiB | `/apps/upload` POST route (native), pull via `http.get` + `files.write` |
| Per-app state across switches | globals survive `resetTree` | `os.state(id)` map in the OS core |
| App → app navigation | global function calls (`reader(idx)`, `home()`) | `os.launch(id, arg)`, `os.home()` |
| Error containment | exceptions recorded, deadline 1 s/callback, 3 s image run | error page on load failure, `abi` check, size caps, no auto-launch |
| Developer loop | edit → regen → flash (~minutes) | edit → `curl -T` → tap (seconds); `GET /apps/run?id=` hot reload |
| Validation | probes 1–22 fixed strings | probes 23–26 + `js load` console op + dev script |

## 6. Proposed architecture

### 6.1 System map

```
  ┌────────────── ROM (embedded, one bytecode image) ───────────────┐
  │ tools/js/os/00-kernel.js  P, M, fmtClock, enter(), osRoutes()   │
  │ tools/js/os/10-facade.js  os = {...}  (the API apps see)         │
  │ tools/js/os/20-catalog.js registry: ROM apps + SD scan + merge  │
  │ tools/js/os/30-loader.js  launch(id, arg): load → run → errpage │
  │ tools/js/os/40-launcher.js home(): rows from the catalog        │
  │ tools/js/os/50-settings.js + apps row (rescan/install/remove)   │
  │ tools/js/os/90-boot.js                                          │
  └───────────────┬─────────────────────────────────────────────────┘
                  │ launch('dice')
                  ▼
  ┌──── loader ────────────────────────────────────────────────────┐
  │ catalog[id].src = "/apps/dice.js" | "rom:dice"                  │
  │ desc = load(src)      // native: read file/asset, EvalBounded   │
  │ check desc.abi === abiVersion(), desc.id === id                 │
  │ enter(id); desc.main(os, arg)                                   │
  └───────┬───────────────────────────────┬─────────────────────────┘
          ▼                               ▼
   /sdcard/apps/dice.js             flash asset  tools/js/apps/dice.js
   /sdcard/apps/dice.json           (EMBED_TXTFILES, registry table
   (seeded from ROM on first boot,   in main/js_assets.cpp)
    or installed over HTTP)
          ▲                 ▲
          │ POST /apps/upload?name=dice   (httpd task streams to SD,
          │                                then ModuleDone → JS toast)
          │ http.get(url) → files.write   (Settings → Install from URL)
```

The ROM image is the operating system; the apps are data. If the SD card
is missing or empty, the launcher still lists every ROM app (loaded from
the flash asset), so the product does not regress.

### 6.2 The app module contract

An app is **one ES5 file whose value is a descriptor object**. The file is
an *expression*, wrapped in parentheses so that `load()` (which evaluates
with `JS_EVAL_RETVAL`) returns the object and nothing leaks into the global
scope:

```js
// tools/js/apps/dice.js  (also /sdcard/apps/dice.js)
({
  id: 'dice',
  title: 'Dice Tray',
  subtitle: '2d6 coin d20 d%',
  version: 1,
  abi: 2,                                   // must equal abiVersion()
  main: function (os, arg) {                // called after enter(id)
    var DZ = os.state('dice', function () {  // survives app switches
      return { mode: '2d6', a: 3, b: 4, big: '', hist: [] };
    });
    var ui = { dieA: [], dieB: [] };
    ...                                     // body of today's dice()
    var p = page('dice').header(os.chrome('DICE TRAY')).content(body)
      .footer(os.hintFooter('tap a roll - swipe down = home'));
    ...
    os.announce('dice');
    p.show(true);
    refresh(p);
  }
})
```

Rules the loader enforces or relies on:

1. The file must evaluate to an object with string `id`, string `title`,
   integer `abi`, and function `main`. Anything else is a load error.
2. `main(os, arg)` is called **after** the loader has called `enter(id)`
   (`resetTree` + OS callbacks + routes). The app never calls `enter`.
3. The app must not define globals. The file is a single parenthesised
   expression, so there is no top level to put a `var` in; inside `main`
   an assignment to an undeclared name throws `ReferenceError` in this
   dialect (`ESP-51 design-doc/01:251`), so accidental globals fail loudly.
   A file that adds statements before the object (`var x = 1; ({...})`)
   still evaluates — `JS_EVAL_RETVAL` returns the last value — but leaks
   `x`; the host lint in §9.3 rejects such files.
4. Everything the app needs from the OS comes through `os` (§6.3) or the
   native globals (`page`, `text`, `col`, `files`, `http`, ...). Apps do
   not call other apps by name; they call `os.launch(id, arg)`.
5. State that must survive an app switch goes through `os.state(id, init)`;
   state that must survive a reboot goes through `storeGet/storeSet`
   (ints) or a file. Nothing else survives.
6. `subtitle`, `version` are optional; `subtitle` may be a function of no
   arguments (evaluated by the launcher at build time, e.g. `'best ' +
   storeGet('2048best', 0)`).

Why an object and not a bare function: the launcher needs the metadata
before running anything (from the `.json` twin, §6.4), and the object
gives us a place to add `onSleep`, `routes`, `icon` later without changing
the loader.

### 6.3 The `os` facade

The facade is a plain object created once by the ROM core (`os/10-facade.js`)
and passed to `main`. It is not a native class (ROM prototypes cannot be
extended at run time, `ESP-51 diary:290`), so it costs a handful of arena
objects and nothing else. Every member maps to something that exists today:

| Member | Today | Semantics |
|---|---|---|
| `os.M` | global `M` | current content margin (40/0); read at build time |
| `os.chrome(title)` | `chrome()` (`pulp.js:68-71`) | header column with title + rule |
| `os.hintFooter(hint)` | `hintFooter()` (`:73-76`) | footer with rule + gray hint |
| `os.announce(name)` | `announce()` (`:66`) | prints `pulp screen: <name>` (validation evidence) |
| `os.home()` | `home()` | back to the launcher |
| `os.launch(id, arg)` | direct calls (`reader(idx)`, `library()`, `settingsPass(ssid)`) | load (if needed) and run another app; `arg` is passed to `main` |
| `os.state(id, init)` | module-level globals `DZ`, `BZ`, ... | returns the per-app state object, creating it with `init()` on first use; cleared by `os.clearState(id)` and on OOM (§6.8) |
| `os.netUp(fn)` | `netUp()` (`:865-869`) | bring WiFi up with saved credentials, then `fn(ok)` |
| `os.fmtClock(ms)`, `os.pad2(n)` | prelude helpers | formatting |
| `os.setRow(menu, label, sub, fn)` | `setRow()` (`:873-880`) | settings-style row (used by settings sub-screens, which become one app with internal screens) |
| `os.app` | `P.app` | id of the running app (read-only by convention) |
| `os.abi` | `abiVersion()` | 2 today |

`enter(name)` and `osRoutes()` are deliberately **not** on the facade: only
the loader calls them, so an app cannot half-reset the tree.

### 6.4 The catalog: SD layout, manifest, ROM registry

```
/sdcard/apps/
  dice.js          ← module (source, ≤ 32 KiB)
  dice.json        ← manifest, written by whoever installs the module
  2048.js
  2048.json
  ...
```

Manifest (`<id>.json`), parsed with `JSON.parse` on the device:

```json
{ "id": "dice", "title": "Dice Tray", "subtitle": "2d6 coin d20 d%",
  "version": 1, "abi": 2, "size": 2388, "src": "/apps/dice.js" }
```

Why a manifest twin instead of reading the module's header: the launcher
must list ~15 apps without evaluating 15 files (each eval is a parse and a
GC-visible allocation), and `files.read` returns a *line* mailbox, so a
one-line JSON is one `files.line(0)`. Why not one `index.json` for the
whole directory: two writers (upload route on the httpd task, installer on
the owner) would race on a shared file; per-app files are atomic per app,
and a missing/corrupt manifest only hides that app. Why not `storeSet`:
16 int32 records (§3.8).

The ROM registry is a JS array in `os/20-catalog.js` generated by the build
from `tools/js/apps/*.js` (id/title/subtitle/abi are read from each file's
descriptor by a small host script, `scripts/03-gen-app-registry.py` in
Phase 3):

```js
var ROM_APPS = [
  {id:'library', title:'Reader', subtitle:'books on the card', src:'rom:library'},
  {id:'dice',    title:'Dice Tray', subtitle:'2d6 coin d20 d%', src:'rom:dice'},
  ...
];
```

Merge rule (`catalog()`): start with `ROM_APPS`; for every
`/sdcard/apps/*.json` that parses and has `abi === os.abi`, **override** the
entry with the same `id` (an SD copy of a built-in app wins — that is how
you hot-patch Dice without reflashing) or append a new one. Order: ROM
order first, then SD-only apps alphabetically. Cache the merged list in the
OS core; invalidate on `apps.rescan()`, after an upload completion, and
after install/remove. A `size` field over 32 KiB or a missing `src` marks
the entry `broken` (shown grayed, tap shows the reason).

**Seeding**: on boot, if `/sdcard/apps/` does not exist, the OS core writes
every ROM app's source and manifest to the card (via `load`'s sibling
`assets.copy(name, path)` — a native, because `files.write` is capped at
16 KiB and takes a JS string). A `<id>.json` `"seed": <romVersion>` field
lets a newer firmware overwrite an older seeded copy but never a copy whose
manifest lacks `seed` (the operator's own edit) — the same marker idea as
the `<!--v5-->` index.html migration (`net_serve.cpp:460-485`).

### 6.5 The loader

Native side (`main/app_js.cpp`, replacing the stub):

```c
// load(path) -> value of the evaluated file (JS_EVAL_RETVAL)
// path: "/apps/dice.js" (rooted at /sdcard, files sanitizer rules) or "rom:<name>"
JSValue js_load(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    char path[kFilesMaxPath];
    if (argc < 1 || !ArgString(ctx, argv[0], path, sizeof(path), &err)) return err;
    const uint8_t *src; uint32_t len; bool owned = false;
    if (strncmp(path, "rom:", 4) == 0) {
        if (!AssetsFind(path + 4, &src, &len))            // main/js_assets.cpp table
            return JS_ThrowTypeError(ctx, "load: no such asset");
    } else {
        // owner-only, synchronous read into the (new) 64 KiB PSRAM load buffer
        StatusCode rc = LoadReadFile(path, &src, &len);   // uses FilesResolvePath
        if (rc != Ok) return JS_ThrowTypeError(ctx, "load: %s", StatusCodeName(rc));
    }
    s_deadline_us = esp_timer_get_time() + kLoadDeadlineUs;   // 3 s, like JsRunPulp
    JSValue v = JS_Eval(ctx, (const char *)src, len, path, JS_EVAL_RETVAL);
    s_deadline_us = 0;
    g_evals++; g_loads++;
    if (JS_IsException(v)) { RecordException(path); }         // last_error keeps the message
    return v;                                                  // exception propagates to JS
}
```

Notes: `JS_Eval` parses straight from the C buffer, so the source is never
copied into the arena as a JS string (unlike `eval(files.line(...).join())`);
the buffer is a single lazily allocated 64 KiB PSRAM block (`LoadReadFile`),
mirroring `EnsureBody()` in `app_files.cpp:46-56`; the deadline uses the
existing interrupt handler; the returned value is a live JS value the
caller must use immediately (the OS core stores it in a JS variable, never
in C). `g_loads` and the last load time (`esp_timer` delta) go into
`JsSnapshot` for `js status`.

JS side (`os/30-loader.js`):

```js
var RUN = { id: 'home', desc: null };          // the currently loaded app

function launch(id, arg) {
  var e = catalogFind(id);
  if (!e) { errorPage(id, 'not in catalog'); return; }
  if (e.broken) { errorPage(id, e.broken); return; }
  var desc = null;
  RUN.desc = null;                             // drop the previous app's functions
  gc();                                        // let the arena reclaim them BEFORE parsing
  try {
    desc = load(e.src);                        // "/apps/dice.js" or "rom:dice"
  } catch (ex) {
    errorPage(id, 'load failed: ' + ex);       // ex carries the parse/eval message
    return;
  }
  if (!desc || typeof desc.main !== 'function' || desc.abi !== os.abi
      || desc.id !== id) {
    errorPage(id, 'bad descriptor (abi ' + (desc && desc.abi) + ')');
    return;
  }
  RUN.id = id; RUN.desc = desc;
  enter(id);                                   // resetTree + OS callbacks + routes
  try { desc.main(os, arg); }
  catch (ex) { errorPage(id, 'crashed: ' + ex); }
}

function errorPage(id, why) {
  enter('error');
  var p = page('error').header(os.chrome('APP ERROR')).content(
    col().pad(20, M, 0, M).gap(10).add(
      text(id).size('lg'), text(why).size('xs').gray(96),
      text('[ back to launcher ]').size('xs').center().width(220).height(56)
        .onTap(function () { home(); })))
    .footer(os.hintFooter('swipe down = home'));
  print('pulp screen: error/' + id + ' ' + why);   // evidence line
  p.show(true);
}
```

Ordering matters: `RUN.desc = null; gc();` before `load()` so the old app's
functions are garbage when the parser needs headroom, and `enter(id)` only
after the load succeeded so a broken file never leaves the previous page
half-torn (the previous page stays presented until `errorPage` replaces
it). Exceptions thrown by `load` are real JS exceptions; the OS core's
`try/catch` turns them into a page. If `main` throws after `p.show`, the
page it built stays on screen (nothing else to show) and the exception is
counted; if it throws before any present, the previous app's page is
already gone (`enter` reset it), so `errorPage` is presented.

Unload is implicit: `resetTree` drops all callbacks and widgets, `RUN.desc
= null` drops the last strong reference to the descriptor and thus to every
function the file defined; the next `gc()` reclaims them. `os.state`
objects are the only intentional survivors.

### 6.6 The launcher

`home()` keeps its look (`pulp.js:97-139`): same header, glyphs, tagline,
rule, hint footer, 5 s tick. Only the row source changes:

```js
function home() {
  enter('home'); RUN.id = 'home'; RUN.desc = null;
  var apps = catalog();                       // merged, cached
  var menu = list().pad(4, 0, 0, 0);
  for (var i = 0; i < apps.length; i++) {
    (function (e) {
      var sub = typeof e.subtitle === 'function' ? e.subtitle() : (e.subtitle || '');
      if (e.broken) { sub = '! ' + e.broken; }
      if (e.source === 'sd') { sub = sub + ' *'; }   // installed/patched copy marker
      entryRow(e.title, sub, function () { launch(e.id); });
    })(apps[i]);
  }
  entryRow('Settings', 'wifi - serve - margins - apps', function () { launch('settings'); });
  ...
}
```

Long-press on the launcher keeps its current meaning (nothing) — the
margin toggle already moved to Settings. App management (rescan, remove,
install from URL, show `pulp.local/apps`) is an *Apps* screen inside
Settings, which is itself an app module (`settings` with internal screens
`main/scan/pass/apps` — the three `settings*` functions become one file).

### 6.7 HTTP install: push, pull, list, hot reload

**Push (`POST /apps/upload?name=<id>`)** — native, modelled on
`ServeUpload` (`net_serve.cpp:118-195`): the httpd task validates the name
(`[a-z0-9_-]{1,24}`), rejects bodies over 32 KiB with 413, streams the body
in 1 KiB chunks to `/sdcard/apps/<id>.js.part`, then renames over
`<id>.js`; if the request carries `?title=&subtitle=&version=`, it writes
`<id>.json` (else a minimal manifest with the id as title, to be replaced
by the loader on first successful launch, which knows the descriptor). It
then posts `ModuleDone{Apps, kDoneAppsUpload = 30, bytes, err}`; the OS
core's `apps.received(fn)` callback (registered in `enter()` next to
`osRoutes()`, so it survives switches) invalidates the catalog and, if the
launcher is showing, rebuilds it. This is the third sanctioned off-owner SD
write, with the same rule as the second: one directory, plain files, never
state files (`R-POSTHANDOFF`, `ESP-54 design-doc/01:370-380`).
`max_uri_handlers` stays 2: the existing POST wildcard handler dispatches
on the URI (`/images/upload` → images, `/apps/upload` → apps).

**Pull (`Settings → Apps → Install from URL`)** — pure JS in the OS core:

```js
function installFromUrl(url, fn) {           // fn(ok, msg)
  var id = url.slice(url.lastIndexOf('/') + 1).replace(/\.js$/, '');
  os.netUp(function (ok) {
    if (ok !== 1) { fn(0, 'no network'); return; }
    var rc = http.get(url).limit(32768).done(function (k, status, len) {
      if (status !== 200 || len <= 0) { fn(0, 'http ' + status); return; }
      if (len > 16384) { fn(0, 'too big for files.write (' + len + ')'); return; }
      files.write('/apps/' + id + '.js', http.body(), function (k2, wrote, err) {
        if (err !== 0) { fn(0, 'no card?'); return; }
        files.write('/apps/' + id + '.json',
          '{"id":"' + id + '","title":"' + id + '","abi":' + os.abi +
          ',"size":' + wrote + ',"src":"/apps/' + id + '.js"}',
          function () { catalogInvalidate(); fn(1, 'installed ' + id); });
      });
    }).send();
    if (rc !== 0) { fn(0, 'radio busy (' + rc + ')'); }
  });
}
```

The 16 KiB `files.write` cap is the binding limit for pull; raising
`kFilesMaxBody` to 32 KiB (it is a lazily allocated PSRAM buffer,
`app_files.cpp:46-56`) is a one-line change recorded in Phase 6. The URL is
typed on the existing password keyboard (`settingsPass`, `pulp.js:971-1033`),
which is why *Install from URL* lives in Settings.

**List and hot reload** — two OS routes added to `osRoutes()`:
`GET /apps/list` returns the merged catalog as JSON (`{"apps":[{"id":..,
"title":..,"source":"rom|sd"}]}`), and `GET /apps/run?id=<id>` calls
`launch(id)` from the route handler *after* responding (the route handler
must return within the 1 s deadline and cannot present from inside a
route; it sets `PENDING_LAUNCH = id` and the home tick / a 250 ms `every`
picks it up). Together with push this is the developer loop:

```bash
curl -T tools/js/apps/dice.js "http://pulp.local/apps/upload?name=dice"
curl "http://pulp.local/apps/run?id=dice"        # device shows the new Dice
```

Both routes are OS routes (re-registered by `enter()`), so they work
whatever app is showing.

### 6.8 App state across switches and reboots

`os.state(id, init)` is a map in the OS core:

```js
var STATE = {};                                        // survives resetTree (a JS global)
os.state = function (id, init) {
  if (!STATE.hasOwnProperty(id)) { STATE[id] = init ? init() : {}; }
  return STATE[id];
};
os.clearState = function (id) { delete STATE[id]; };
```

It reproduces today's behaviour (`DZ`, `BZ`, `GG`, `TT`, `PC`, `DP`, `INK`,
`GAL`, `RA`, `SET` survive switches; nothing survives a reboot — wake from
deep sleep is a reboot, `ESP-51 diary:377`) without globals. `Settings →
Apps → Clear app state` empties the map, and the loader empties it when a
`load()` throws `out of memory` before retrying once (the only automatic
eviction). Anything larger than a few hundred bytes per app should be
persisted to a file by the app instead.

### 6.9 Trust and safety

Downloading code crosses the "pulp.js is trusted firmware code" line
(`ESP-53 diary:355`) for the first time. The design's position (R-TRUST):
**an installed app is operator code with the same authority as the OS**,
exactly like a book on the card is the operator's book. What the design
does guarantee is that a bad app cannot take the device down:

- Boot never evaluates an SD app: the ROM launcher comes up first, apps run
  on tap. There is no "autostart last app".
- Time: `load()` runs under a 3 s deadline; every callback under 1 s
  (`main/app_js.cpp:329`); an infinite loop is killed and the context
  survives (measured, `ESP-51 diary:397`).
- Memory: `out of memory` is a catchable `InternalError`; the loader
  catches it, clears app state, retries once, then shows the error page.
- Files: `load()` paths obey the files sanitizer (rooted at `/sdcard`, no
  dot-segments — `.s3paper` state files are unreachable, `app_files.cpp:91-122`).
- Size: 32 KiB per module (upload 413, catalog `broken`), 24-char ids.
- Compatibility: `desc.abi` must equal `abiVersion()`; the launcher hides
  or grays mismatches instead of running them.
- Visibility: `js status` reports loads/exceptions/last_error; the error
  page prints an evidence line; `GET /status` reports the running app.

What it does *not* do: prevent an app from calling `wifi.forget`, deleting
images, or serving files. If that ever matters, the facade is the place to
add capability objects; the loader signature (`main(os, arg)`) is designed
so `os` can shrink per app without touching the apps.

### 6.10 Repository layout after the split

```
0114-papers3-pulp-os/tools/js/
  os/                    ← concatenated in name order into build/pulp_os.js → main/js_pulp.h
    00-kernel.js         P, M, ROUTES_READY, pad2/fmtClock, enter(), osRoutes(), announce, chrome, hintFooter
    10-facade.js         var os = {...}
    20-catalog.js        ROM_APPS (generated: 20-catalog.gen.js), catalog(), catalogFind(), seedApps()
    30-loader.js         RUN, launch(), errorPage()
    40-launcher.js       home()
    90-boot.js           M = storeGet('margin', 40); seedApps(); home();
  apps/                  ← one descriptor file per app; embedded as flash assets AND seeded to SD
    library.js reader.js dice.js blitz.js 2048.js tea.js postcard.js daily.js ink.js gallery.js radio.js settings.js
  build_bytecode_apps.sh ← concatenates os/*.js, compiles ONE image; generates the ROM registry;
                           writes main/js_assets.h (asset table) — see Phase 1/3
main/
  js_assets.cpp/.h       ← EMBED_TXTFILES table: name → (ptr, len)   (Phase 3)
  app_js.cpp             ← real js_load, load buffer, g_loads          (Phase 3)
  net_serve.cpp          ← /apps/upload POST branch                    (Phase 5)
  app_events.h           ← ModuleId::Apps, kDoneAppsUpload = 30       (Phase 5)
```

`pulp.js` itself disappears at the end of Phase 1; until then it is
regenerated from the parts so the diff stays reviewable.

### 6.11 Multi-context runtime: one engine context per app or page

Everything above runs in the single context created by `JsInit()`. The
engine does not require that. Evidence:

- `JS_NewContext(mem, size, stdlib)` takes its own arena; `mquickjs.c` has
  no mutable file-scope state (checked: no non-const statics), so two
  contexts are independent heaps with independent RAM atoms, stacks,
  interrupt handlers, log functions and — crucially — **their own
  one-bytecode-image slot** (`n_rom_atom_tables` is per context,
  `mquickjs.c:182, :231`). A context may also carry an opaque pointer
  (`JS_SetContextOpaque`, `mquickjs.h:266`) that the interrupt handler and
  log function receive.
- `JSValue`s cannot cross contexts (they point into one arena). The
  binding layer already never lets anything but int32 cross `CallCb`
  (§3.5); strings go through mailboxes; widgets and pages are packed-int
  opaques. The ABI is therefore context-agnostic by accident of design.
- What is *not* ready is the singleton state: `jsi::g_ctx` (34 uses in
  `main/*.cpp`), `g_pages[]`, `g_hits[]`, `g_dyn[]`, `g_next_cb`,
  `g_home_cb`, `g_sleep_image_cb`, `g_module_cb[]`, `s_deadline_us`,
  `s_present_seq`, and the `ServeRoutes` table are file-scope in
  `main/app_js.cpp` / `main/app_js_internal.h`.

The refactor is mechanical:

```c
// main/app_js_internal.h (after Phase 8)
struct JsCtxState {
    JSContext *ctx; uint8_t *arena; uint32_t arena_bytes;
    enum Kind { kOs, kApp, kPage } kind;
    const JSSTDLibraryDef *stdlib;          // full or UI-only (6.12)
    PageEntry pages[kMaxPages]; int32_t current_page;
    s3paper::HitRegion hits[kMaxJsHits]; uint32_t hit_count;
    DynEntry dyn[kMaxDynValues]; uint32_t dyn_count;
    int32_t next_cb, home_cb, sleep_image_cb;
    int64_t deadline_us, timer_due_us;
    uint32_t evals, exceptions; char last_error[48];
};
JsCtxState *g_os;      // the ROM-image context, always alive
JsCtxState *g_fg;      // whoever owns the panel right now (os, an app, a page)
JsCtxState *StateOf(JSContext *ctx);   // 3-entry lookup (or a JS_GetContextOpaque accessor, 3 lines in the engine)
```

Rules that replace today's single-context assumptions:

1. **Foreground owns the panel.** `JsHandleGesture`, `JsTimerTick`,
   `RefreshDynValues` and `PresentPage` operate on `g_fg`. Switching
   foreground = `Arena().Reset()` (the widget tree is native and singular)
   + `g_fg = next` + the new foreground rebuilds its page. The old
   foreground keeps its JS heap (it can be resumed) but has no widgets.
2. **Completions route to the registering context.** `g_module_cb[]`
   becomes `{JsCtxState *owner; int32_t cb}`; `JsModuleDone` calls into
   `owner`. A context that is torn down while an op is in flight drops
   the callback (same as `resetTree` today).
3. **Serve routes belong to the OS context only.** Apps/pages register
   routes through the OS (or not at all, §6.12).
4. **Teardown = `JS_FreeContext` + free the arena** — which also frees
   every RAM atom the app ever created (risk "RAM atoms accumulate" in
   §10 disappears) and every closure (no `RUN.desc = null; gc()` dance).
5. **Memory.** Each context needs its own PSRAM arena: OS 128 KiB (the ROM
   image retains ~8 KB on the host, §4.2) + app/page 96–128 KiB. Two to
   three contexts fit trivially in 8 MB PSRAM; internal RAM is untouched
   (bytecode image buffers would be per context — load them into PSRAM,
   not `MALLOC_CAP_INTERNAL`, when more than one exists).
6. **No parallelism.** All contexts run on `ui_owner`, time-sliced by the
   same event loop, under the same 1 s callback deadline and 8 KiB stack.

What it buys the loader (option (3) of R-SOURCEEVAL becomes cheap):
`launch(id)` = `newAppContext()` → `load(src)` into it (source **or** a
per-app bytecode image, since the fresh context has a free image slot and
no RAM atoms yet) → `switchForeground()` → `main(os, arg)`. The OS context
stays warm, so swipe-home is a foreground switch back, not a rebuild.
`os.state` becomes unnecessary if an app context is *kept* across
switches (it is its own state), or stays as is if contexts are torn down
at switch; both are configuration, not design.

### 6.12 The PULP browser: pages as sandboxed JS scripts

The idea: instead of inventing a markup language (an HTML/CSS subset, a
Markdown dialect, a binary widget tree), let a web server return a page as
**a small JS script in the builder DSL**, and run it on the device in a
context whose standard library can *only* build and present UI. The
builder DSL already is a declarative layout language —

```js
// GET http://host/pages/menu.js  →  text/javascript
({
  title: 'Kitchen',
  main: function (ui, nav) {
    var p = page('menu').header(ui.chrome('KITCHEN'))
      .content(list().add(
        ui.row('Soup of the day', 'tomato', function () { nav.go('/pages/soup.js'); }),
        ui.row('Stock', '3 jars', function () { nav.go('/pages/stock.js?sort=age'); }),
        text(function () { return 'clock ' + nav.clock(); }).size('xs').gray(96)))
      .footer(ui.hintFooter('tap = open - swipe left = back'));
    p.on(G.LEFT, function () { nav.back(); });
    p.every(60000);
    p.show(true);
  }
})
```

— so the "parser" is the engine we already ship, the "layout engine" is
`s3paper_core`, and the "browser" is a ~200-line PULP app plus one new
stdlib table. A 5 KB page costs ~10–20 KiB of arena (§4.2) and tens of
milliseconds to parse; the server can be a static directory, a Go or Python
handler, or the device's own `serve.files`. Pages can even be served as
**bytecode** (compiled on the server with `pulpjsc`) because every page
context is fresh and has an empty image slot — no parse at all.

**Sandbox mechanism — a second `JSSTDLibraryDef` with a filtered function
table.** The generated stdlib (`main/js_stdlib.h`) is a ROM object table
plus `js_c_function_table[]`, an array of C function pointers that the
engine indexes at call time (`ctx->c_function_table[idx]`,
`mquickjs.c:3786, :5361`). Atoms, prototypes and classes live in the ROM
table and are identical for every context. So:

```c
// main/js_stdlib_table_ui.c  (Phase 9)
#include "app_js_bindings.h"
#define js_files_read   js_ui_denied     /* every non-UI native is renamed */
#define js_http_get     js_ui_denied     /* to one stub before including   */
/* ... files/http/serve/wifi/mdns/images/buzzer/battery/store/book/load/   */
/* ... resetTree/paper.home/paper.sleepImage/paper.refreshTurns ...        */
#include "js_stdlib.h"                   /* same ROM table, different fn table */
/* emits: const JSSTDLibraryDef js_stdlib_ui = { js_stdlib_table, js_c_function_table, ... } */

JSValue js_ui_denied(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_ThrowTypeError(ctx, "not available to pages");
}
```

(The `#define` trick works because the generated header references the
bindings by bare name, `main/js_stdlib_table.c:1-6`; if the generator
grows a `--deny` list instead, same result.) Because the atoms are the same,
**page scripts compile with the same `pulpjsc` and the same bytecode format
as apps**, and the UI context costs no extra ROM table.

What the page context gets, and what it is denied:

| Allowed (UI stdlib) | Denied (throws `not available to pages`) |
|---|---|
| `page`, all widget factories and `Widget`/`Page` prototypes, `G`, `print`, `millis`, `Math`/`JSON`/`String`/… engine built-ins, `eval` (harmless inside the sandbox) | `files.*`, `http.*`, `serve.*`, `wifi.*`, `mdns.*`, `images.*`, `battery.*`, `buzzer.*` (or `beep` only), `storeGet/Set`, `book*`/`library*`, `load`, `resetTree`, `paper.home/sleepImage/refreshTurns`, `abiVersion` is fine |
| `nav` singleton (new, Phase 9): `go(url)`, `back()`, `reload()`, `url()`, `clock()` — `go/back/reload` only *record* the request in a native mailbox and post an event; the **OS-context browser app** performs the fetch | any way to start I/O or keep a callback registered in the OS |
| `ui` helper object built by the browser app *inside the page context* (chrome/hintFooter/row — plain JS, evaluated from a ROM asset into the page context before the page) | |

Isolation properties (this is the first real sandbox in PULP, unlike
R-TRUST for apps):

- Separate heap and arena: a page can OOM only itself (`InternalError`
  caught by the browser app → error page).
- Deadline per callback (1 s) and per page eval (3 s): a spinning page is
  killed, context survives or is torn down.
- No I/O, no persistence, no OS callbacks: the page can draw, react to
  gestures on its own page, and ask `nav` to go somewhere. The browser app
  decides whether to follow (same-origin policy, `http://` + `https://`
  only, 32 KiB cap, max redirects).
- Shared native resources are bounded and reset on navigation: widget
  arena 128 nodes (throws `widget arena full`), 12 pages, 48 hits, 48 dyn
  values — a page that exhausts them fails loudly, not silently.
- Residual: a page can keep the panel busy (full refreshes on every tick)
  — the browser app can cap `every()` to ≥ 1 s for pages and count
  presents per minute.

Browser app flow (OS context, `tools/js/apps/browser.js`, Phase 10):

```
 user types/opens URL ─► netUp ─► http.get(url).limit(32768).done(cb).send()
   cb: status 200 ─► pageCtx = newPageContext(96 KiB)      // JS_NewContext(..., &js_stdlib_ui)
                     evalInto(pageCtx, 'rom:ui-helpers')   // ui object
                     desc = loadInto(pageCtx, http.body())  // or bytecode image if content-type says so
                     switchForeground(pageCtx); desc.main(ui, nav)
   nav.go(u) from the page ─► native mailbox + event ─► browser app (OS ctx):
                     teardown(pageCtx) ─► push history ─► fetch u ─► repeat
   swipe-down ─► OS home (paper.home stays in the OS context; the page cannot trap it
                 because `paper` is denied — the navigation grammar is enforced, not a convention)
```

The natives that make this possible beyond §6.11 are small: `loadInto(ctx,
buf, len)` (the `js_load` core parameterised by context), `newPageContext`
/ `freeContext`, and the `nav` mailbox (`NavRequest{kind, url[256]}` +
`kDoneNavRequest` posted to the owner). Everything else is the existing
present pipeline.

Why this is worth doing: it turns the PaperS3 into a thin client for
anything that can print JavaScript text — home dashboards, a recipe book,
a Hacker News front page rendered server-side, the device's own settings
served by a laptop — without a second rendering stack, and with a trust
model that finally matches "fetched from the network". The same mechanism
(UI stdlib + per-context isolation) can later be offered to *apps* that
want to run untrusted plug-ins.

## 7. Decision records

Format follows ESP-54 §6 (`R-<TOKEN>`: context, options, decision,
rationale, consequences, status).

### R-SOURCEEVAL — apps load as source text, not bytecode

- **Context.** MicroQuickJS loads bytecode only before any RAM atom exists
  and holds at most one image per context (`mquickjs.c:182, :12947-12951`);
  the parser is on the device and `eval` is in the stdlib.
- **Options.** (1) Source eval via a native `load(path)`. (2) Fork the
  engine: raise `N_ROM_ATOM_TABLES_MAX`, and either forbid RAM atoms
  (impossible: the kernel eval creates them) or teach `JS_RelocateBytecode`
  to merge against RAM atoms. (3) "Process model": tear the context down
  and recreate it at every launch, loading OS image + app image before the
  kernel eval. (4) Keep one image and just split the *source* (no dynamic
  loading).
- **Decision.** (1), with (3) recorded as the future path if launch latency
  or arena pressure ever demands bytecode apps.
- **Rationale.** (1) needs no engine change and no context restart; measured
  costs (§4.2) are small relative to the arena and to the panel; (2) is a
  fork of a vendored engine with subtle atom-merge semantics; (3) loses all
  JS state at every launch (acceptable — `os.state` would have to become
  native) and still needs `load()` for the *download* case, so it is a
  refinement of (1), not a replacement; (4) does not deliver "load
  dynamically".
- **Consequences.** Every launch parses (~15–60 ms est.); app source is
  visible on the card (a feature for a hackable device); the ROM image
  shrinks to the OS core; the trust boundary moves (R-TRUST).
- **Status.** proposed.

### R-ONEIMAGE — the OS core stays a single embedded bytecode image

- **Context.** The core (kernel helpers, facade, catalog, loader, launcher,
  boot) must exist before the card is readable and must never fail to load.
- **Options.** (1) Core as bytecode (as today), apps as source. (2)
  Everything as source evaluated at boot from flash strings. (3) Core as
  source on SD (bootstrapped by a tiny ROM stub).
- **Decision.** (1).
- **Rationale.** Bytecode run costs 0.01 ms and 8 KB of arena on the host
  (§4.2) versus ~50 KiB transient for a whole-OS source eval; the boot path
  keeps its measured behaviour; (3) makes the card a boot dependency.
- **Consequences.** Adding a *native* function still requires the
  regeneration protocol; changing the *core JS* still requires a reflash;
  changing an *app* does not.
- **Status.** proposed.

### R-DESCRIPTOR — a module is one expression evaluating to a descriptor object

- **Context.** No module system; `load()` returns the value of the last
  expression; the launcher wants metadata; apps must not leak globals.
- **Options.** (1) `({id, title, abi, main})` object. (2) A bare
  `function (os) {...}` with metadata only in the manifest. (3) CommonJS
  emulation (`module.exports`) with a wrapper prepended by the loader.
- **Decision.** (1).
- **Rationale.** Self-describing files survive being copied around; the
  `abi` and `id` checks live in the file itself; (3) prepends source at
  load time (a copy) for no gain; (2) loses metadata when the manifest is
  missing.
- **Consequences.** A file that is not a single expression still works if
  its last statement is the object, but leaks globals — the host lint
  (§9.3) rejects it.
- **Status.** proposed.

### R-CATALOGFILE — per-app manifest files, merged with a generated ROM registry

- **Context.** The launcher must list apps without evaluating them; the
  settings store is 16 int32 records; two tasks write the apps directory.
- **Options.** (1) `<id>.json` twin per module. (2) One `index.json`.
  (3) A new CRC state file in `s3paper_storage`. (4) Directory scan +
  evaluate each file for its title.
- **Decision.** (1) now; (3) recorded as the hardening path (same tradeoff
  ESP-54 recorded for the image catalog).
- **Rationale.** Atomic per app, no cross-task write races, one line per
  manifest fits the `files.read` line mailbox; (4) costs a parse per app
  per launcher build.
- **Consequences.** A stale manifest can disagree with its module; the
  loader re-validates `id`/`abi` from the descriptor and rewrites the
  manifest after a successful launch.
- **Status.** proposed.

### R-NATIVELOAD — `load(path)` is native and reads the file itself

- **Context.** `files.read` yields ≤512 lines through a mailbox and
  `files.write` caps at 16 KiB; `eval(string)` would need the source as an
  arena string; the stub `load` already has an atom.
- **Options.** (1) Native `load(path)`: owner-side synchronous read into a
  PSRAM buffer, `JS_Eval` from that buffer with `JS_EVAL_RETVAL`, deadline
  via the existing interrupt handler. (2) JS `eval(lines.join('\n'))` over
  `files.read`. (3) A worker task that reads and posts a completion, then
  `eval`.
- **Decision.** (1).
- **Rationale.** No arena copy of the source, one call, deadline-bounded,
  and it reuses the files sanitizer and the `EnsureBody` allocation
  pattern; SD reads of a few KB are milliseconds, so a worker (3) buys
  nothing and adds a mailbox; (2) allocates O(n) strings and is unbounded.
- **Consequences.** New native code (~80 lines), a `STUB` in `pulpjsc.c`
  (already present for `js_load`), `rom:` scheme for flash assets, a
  `g_loads` counter in `js status`.
- **Status.** proposed.

### R-ROMSEED — built-in apps ship as flash assets and are seeded to the card

- **Context.** After the split the ROM image no longer contains the apps;
  the device must still work with no card, and the card must get the apps
  somehow.
- **Options.** (1) `EMBED_TXTFILES` each `apps/*.js`, `load('rom:<id>')`
  when no SD copy exists, and copy to `/sdcard/apps` on first boot with a
  `seed` version marker. (2) Only SD (device is empty without a prepared
  card). (3) Only ROM (SD holds extra apps, built-ins are never editable).
- **Decision.** (1).
- **Rationale.** No product regression; the operator can edit a seeded copy
  (the SD copy overrides the ROM one); ~40 KB of flash is free.
- **Consequences.** A build script generates the asset table and the ROM
  registry; seeding runs once (checked by directory existence + marker),
  costing ~15 file writes on a fresh card.
- **Status.** proposed.

### R-HTTPPUSHPULL — push over a native POST route, pull from Settings over `http.get`

- **Context.** POST exists only natively for images; `http.get` is GET-only
  with a 32 KiB body; JS routes are GET-only and cannot present.
- **Options.** (1) Both push (`POST /apps/upload`) and pull (`http.get`
  → `files.write`). (2) Push only. (3) Pull only. (4) A generic JS-visible
  POST route API.
- **Decision.** (1); (4) deferred (it is the ESP-53 note "consider POST
  support only when an app needs it").
- **Rationale.** Push is the developer loop and works from any browser or
  `curl`; pull is the operator flow with no laptop; both reuse existing
  handoff patterns.
- **Consequences.** A third sanctioned off-owner SD write path
  (`/sdcard/apps` only); a new `ModuleId::Apps` completion; `kFilesMaxBody`
  → 32 KiB in Phase 6.
- **Status.** proposed.

### R-TRUST — installed apps are operator code; the OS bounds time, memory and paths, not authority

- **Context.** First time code arrives over the network.
- **Options.** (1) Trusted-operator model with containment (deadline, OOM
  catch, size caps, no autostart, path sanitizer, ABI check). (2) A
  capability facade per app. (3) No network install at all.
- **Decision.** (1), with the facade shaped so (2) can be added.
- **Rationale.** The device has no user accounts and no remote attack
  surface beyond the LAN web server it already runs; the failure to
  prevent is "device unusable", which containment covers.
- **Consequences.** Documented plainly in the launcher's Apps screen and
  the README.
- **Status.** proposed.

### R-STATE — per-app state lives in an OS-owned map, keyed by app id

- **Context.** Today's apps keep state in globals that survive
  `resetTree`; evaluated modules start fresh each launch.
- **Options.** (1) `os.state(id, init)` map in the OS core. (2) Apps
  persist everything to files. (3) Keep the loaded descriptor alive per app
  (a cache of modules) so closures survive.
- **Decision.** (1).
- **Rationale.** Same observable behaviour as today, tiny cost, explicit;
  (3) keeps every launched app's functions in the arena forever, defeating
  the memory win.
- **Consequences.** State is a plain object; the loader may evict it on OOM.
- **Status.** proposed.

### R-MULTICTX — the binding layer moves to per-context state; the single-context loader ships first

- **Context.** §6.11: the engine supports many contexts; the bindings are
  singletons; per-context isolation would simplify unload, free RAM atoms,
  re-enable per-app bytecode, and is required for the page sandbox.
- **Options.** (1) Ship Phases 1–7 single-context, then refactor to
  `JsCtxState` in Phase 8 and switch the loader to per-app contexts behind
  the same `launch()` API. (2) Do the per-context refactor first. (3) Never;
  keep one context.
- **Decision.** (1).
- **Rationale.** The descriptor, catalog, `load()`, launcher and HTTP
  install are independent of how many contexts exist; shipping them first
  delivers the operator value early and keeps the refactor reviewable. The
  refactor is mechanical (state struct + foreground pointer + owner-tagged
  completions) and touches every binding, so it deserves its own phase and
  probes.
- **Consequences.** Phase 3's `js_load` must be written as
  `LoadInto(JsCtxState*, ...)` from the start (trivial now, painful later);
  bytecode image buffers go to PSRAM when more than one context exists;
  `os.state` may become redundant under (1)-with-kept-contexts.
- **Status.** proposed.

### R-UISANDBOX — pages run in a context whose stdlib has a filtered function table

- **Context.** §6.12: untrusted scripts must be able to draw and navigate
  and nothing else.
- **Options.** (1) Second `JSSTDLibraryDef` sharing the ROM table with a
  function table whose non-UI entries are one `js_ui_denied` stub. (2) A
  separately generated, smaller stdlib (own atoms, own ROM table). (3) JS-
  level capability wrapping in one context (`os` without I/O members).
- **Decision.** (1).
- **Rationale.** (1) costs no flash, keeps atoms identical (same
  `pulpjsc`, same bytecode), and is enforced by the engine's call path, not
  by JS discipline; (2) would mean a second atom space and a second
  compiler build; (3) is not a sandbox — globals like `files` remain
  reachable.
- **Consequences.** One new TU (`js_stdlib_table_ui.c`), one new singleton
  (`nav`, atom regen once), a deny-list that must be reviewed whenever a
  native is added (checklist item in §3.7's protocol).
- **Status.** proposed.

### R-PAGESCRIPT — a page is a JS descriptor `({title, main(ui, nav)})`, same shape as an app

- **Context.** The server needs a contract; the browser app needs
  metadata before running; apps already use a descriptor.
- **Options.** (1) Same descriptor shape as apps with `main(ui, nav)`
  instead of `main(os, arg)`. (2) Bare function. (3) JSON widget tree
  interpreted by the device.
- **Decision.** (1).
- **Rationale.** One mental model for authors and one loader core; (3)
  would be a second, weaker language (no dyn text, no handlers) and a
  second interpreter.
- **Consequences.** Page authors can use everything the builder API
  offers; the browser app supplies `ui` helpers so pages look like PULP
  screens; `Content-Type: text/javascript` (source) or
  `application/x-pulp-bytecode` (image) selects the load path.
- **Status.** proposed.

## 8. Implementation plan

Each phase ends with a commit, a diary step, and the acceptance gate
listed. Estimates assume one engineer familiar with the ESP-53 onboarding
guide.

### Phase 0 — Measure on the device (½ day)

- Add a console op `js load <path>` (`main/app_console.cpp`, arg encoding
  next to `js probe`; `main/app_owner.cpp:234-257`) that calls the new
  `js_load` and prints `load: <path> <bytes> in <ms> arena_used=<n>`.
  Since `js_load` is Phase 3 work, Phase 0 may temporarily wire the op to
  `EvalBounded` over a fixed embedded copy of `dice.js` — the point is the
  number, not the plumbing.
- Add `arena_used` (heap_free − heap_base) to `JsSnapshot`: expose a tiny
  engine accessor (`JS_GetHeapUsed(ctx)` — 4 lines in `mquickjs.c` next to
  `JS_DumpMemory`, the struct is private) and print it in `js status`.
- Measure: `arena_used` right after boot and after `gc()` (explains the
  ESP-54 160 KiB OOM); eval time and arena delta for `dice.js` and
  `settings.js`; internal free before/after; ten repeated evals with
  `gc()` between them (arena must return to baseline).
- **Gate:** numbers recorded in the diary; parse time < 150 ms for the
  largest app; arena returns to baseline after GC.

### Phase 1 — Mechanical split, no behaviour change (1 day)

- Create `tools/js/os/{00-kernel,10-facade,20-catalog,30-loader,40-launcher,90-boot}.js`
  and `tools/js/apps/<id>.js` by cutting pulp.js at its banners
  (`scripts/01-trial-split-bytecode-sizes.py` shows the cut points).
- Change `build_bytecode_apps.sh` to concatenate `os/*.js` **and, for this
  phase only, `apps/*.js`** into `build/pulp_all.js` and compile that one
  file to `main/js_pulp.h` — byte-identical behaviour, reviewable diff.
- Delete `tools/js/apps/pulp.js`.
- **Gate:** `idf.py build`, flash, `js status`, launcher screenshot
  fingerprint unchanged (11 rows), probes 1–22 pass.

### Phase 2 — Descriptor contract and `os` facade, still one image (1–2 days)

- Wrap each `apps/<id>.js` in the descriptor form; replace `enter(...)`
  calls with the loader's `enter(id)`; replace `chrome/hintFooter/M/netUp/
  announce/home/reader/library/settingsPass` uses with `os.*` and
  `os.launch(id, arg)`; move `DZ`/`BZ`/... into `os.state`.
- Implement `os/10-facade.js`, `os/30-loader.js` with a **static**
  registry for now (`launch(id)` looks up a JS object map built at
  concatenation time: `APPS['dice'] = ({...})`), and `os/40-launcher.js`
  from the registry.
- Fold `settings/settingsScan/settingsPass` into one `settings.js` with
  internal screens.
- **Gate:** every app launches from the launcher; `js hits` fingerprint of
  the launcher documented; app state survives switches (dice history,
  2048 board); probes pass; diary records arena numbers.

### Phase 3 — Native `load()`, flash assets, ROM registry (2 days)

- `main/js_assets.{h,cpp}`: `EMBED_TXTFILES` every `tools/js/apps/*.js`
  (`main/CMakeLists.txt`), an `AssetsFind(name, &ptr, &len)` table
  generated by the build script (`scripts/03-gen-app-registry.py` emits
  both `main/js_assets_table.inc` and `tools/js/os/20-catalog.gen.js`).
- `main/app_js.cpp`: real `js_load` (§6.5), `LoadReadFile` with a lazily
  allocated 64 KiB PSRAM buffer, `kLoadDeadlineUs = 3 s`, `g_loads`,
  `last_load_ms` in `JsSnapshot`; `js status` prints them.
- The loader calls `load(e.src)`; the concatenation step stops including
  `apps/*.js` in the image. `js_pulp.h` shrinks to the OS core.
- **Gate:** image size (bytes printed by `pulpjsc`) ≈ 16 KB; `heap` shows
  internal free up by ≥ 25 KB; ten launches of each app with `js status`
  arena flat; probe 23 (load happy path + `abi` mismatch + syntax error +
  missing asset all produce the error page and evidence lines).

### Phase 4 — SD catalog, seeding, launcher merge, error page (1–2 days)

- `os/20-catalog.js`: `files.list('/apps', ...)` → for each `*.json`,
  `files.read` → `JSON.parse(files.line(0))`; merge per §6.4; cache;
  `broken` marking; `seedApps()` on boot (native `assets.copy(name, path)`
  next to `load`, because `files.write` takes a JS string ≤ 16 KiB and
  the ROM source is a C buffer).
- Because `files.*` is one-op-at-a-time and asynchronous, the catalog scan
  is a small state machine (`scanNext()` chained through completions);
  the launcher shows ROM apps immediately and re-presents when the scan
  completes (`p.update()`), exactly like `settingsScan` re-presents after
  `wifi.scan`.
- Settings → Apps: list with source (`rom`/`sd`), remove, rescan, clear
  state.
- **Gate:** with a card: seeded files appear, an edited `/sdcard/apps/
  dice.js` overrides ROM (change a string, see it); without a card: launcher
  identical to Phase 3; corrupt manifest → grayed row; probe 24 (catalog:
  seed marker, override, broken entry).

### Phase 5 — HTTP push, list, hot reload (1–2 days)

- `main/net_serve.cpp`: `/apps/upload` branch in the POST handler (`:299-304`),
  `ServeAppsUpload` modelled on `ServeUpload` (`:118-195`): name check, 32 KiB
  cap (413), `.part` + rename, optional manifest from query, `PostModuleDone(
  Apps, kDoneAppsUpload=30, bytes, err)`; `main/app_events.h`: `ModuleId::Apps`.
- `main/js_services.cpp` (or a new `js_apps.cpp`): `apps.received(fn)`,
  `apps.uploadName()` mailbox (the ESP-54 upload never exposed the file
  name to JS — do it here).
- OS core: `osRoutes()` adds `/apps/list` and `/apps/run?id=`; `enter()`
  registers `apps.received`; `PENDING_LAUNCH` picked up by the home tick
  (and a 250 ms `every` on non-home pages is *not* added — hot reload while
  inside an app goes through the launcher's tick, i.e. swipe home first, or
  the route answers `409 not on launcher`).
- `scripts/04-pulp-app-push.sh <id> [--run]`: `curl -T` + `/apps/run`.
- **Gate:** `curl -T dice.js "http://pulp.local/apps/upload?name=dice"` →
  200, file on card, launcher row shows `*`; `/apps/run?id=dice` launches
  it; 40 KiB body → 413; bad name → 400; probe 25 (upload completion
  registration + `module busy`).

### Phase 6 — HTTP pull from Settings (1 day)

- `kFilesMaxBody` 16 → 32 KiB (`main/app_files.h:22`), `EnsureBody` unchanged.
- Settings → Apps → *Install from URL*: keyboard screen (reuse the password
  keyboard with a URL row), `installFromUrl()` (§6.7), status footer.
- **Gate:** installing `http://<laptop>:8000/dice.js` from a `python3 -m
  http.server` works; https to a public host works (TLS bundle); 40 KiB
  file → "too big"; no network → message; probe 26.

### Phase 7 — Soak, docs, close-out (½ day)

- 30-minute soak: alternate `js tap` on launcher rows across all apps +
  `/apps/list` polls; heap and arena flat; zero exceptions.
- README: build pipeline changes, `apps/` contract, curl loop; ESP-53
  onboarding guide errata (`load()` now real; `setTimeout` still not).
- Diary, changelog, tasks, reMarkable.

### Phase 8 — Multi-context binding layer (3 days)

- `main/app_js_internal.h`: `JsCtxState` (§6.11); `main/app_js.cpp`:
  `CreateContext(kind, arena_bytes, stdlib)`, `FreeContext`,
  `SwitchForeground`, `StateOf(ctx)`; every `g_*` use in `js_*.cpp` goes
  through `StateOf(ctx)` (callee) or `g_fg` (dispatch); `g_module_cb[]`
  gains an owner; `LoadInto(state, buf, len, name)` replaces the Phase 3
  `js_load` core; bytecode image buffer → PSRAM.
- Loader option: `launch(id)` creates an app context (96–128 KiB), loads
  source or `rom:` bytecode, switches foreground, runs `main`; swipe-home
  switches back to the OS context and tears the app context down (or keeps
  it, behind a setting).
- **Gate:** all existing probes pass under the new layer; probe 27 (two
  contexts alive, completions routed to the right one, teardown frees the
  arena, ten launches with `heap`/arena flat); fingerprints unchanged.

### Phase 9 — UI sandbox stdlib + `nav` (1–2 days)

- `tools/js/pulp_stdlib.c`: `nav` singleton (`go back reload url clock`);
  regenerate; `main/js_stdlib_table_ui.c` with the deny mapping;
  `js_ui_denied`; `NavRequest` mailbox + `kDoneNavRequest`; `rom:ui-helpers`
  asset (chrome/hintFooter/row in plain JS).
- **Gate:** probe 28 — a page context evaluating `files.read` /
  `http.get` / `paper.home` / `resetTree` gets `not available to pages`;
  `nav.go` posts the mailbox; widget building and `show` work; OOM inside
  the page context leaves the OS context healthy.

### Phase 10 — The browser app + a reference page server (2 days)

- `tools/js/apps/browser.js`: URL entry (keyboard), history (8 deep),
  fetch (`limit(32768)`, `http://`/`https://`, same-origin follow for
  relative `nav.go`), content-type switch (source vs bytecode), error page,
  `every()` floor for pages, evidence lines `pulp screen: browser/<url>`.
- `scripts/05-pulp-page-server.py`: static + dynamic example pages
  (menu, list with query, live clock, a form using the keyboard widget),
  optional `--bytecode` mode shelling out to `pulpjsc`.
- **Gate:** browse the example server from the device; a deliberately
  hostile page (infinite loop, `files.read`, 200 widgets, `eval` bomb)
  produces an error page and the launcher remains reachable by swipe-down;
  probe 29; soak: 200 navigations, heap flat.

## 9. Testing and validation strategy

### 9.1 Host

- `scripts/02-host-eval-harness.sh eval 192 tools/js/apps/*.js` after every
  app change: every file must evaluate to an object with `main`, and the
  retained-arena column must stay under ~12 KB (host units).
- `scripts/03-gen-app-registry.py --check`: every `apps/*.js` is a single
  expression (parse with the host compiler; refuse files whose top level has
  more than one statement), `id` equals the file name, `abi` equals the
  current ABI.
- `s3paper_core` host suite unchanged (38,186 checks) — nothing in the
  core changes.

### 9.2 Device probes (new)

| Probe | What it proves | Evidence line |
|---|---|---|
| 23 load | `load('rom:dice')` returns a descriptor; `load('rom:nope')` throws; a `rom:` asset with a syntax error throws `SyntaxError`; `abi` mismatch → error page | `probe23: load ok=1 miss=throw syntax=throw abi=errpage` |
| 24 catalog | seed marker honoured, SD override wins, broken manifest grayed, `catalog().length` | `probe24: rom=12 sd=13 broken=1` |
| 25 upload | `apps.received(fn)` registration; second registration throws `module busy`; upload name mailbox | `probe25: received=1 busy=throw` |
| 26 install | `installFromUrl` against the loopback of the device's own server (`serve.files` serving `/sdcard/www/dice.js`) | `probe26: installed=1` |

Plus the console op `js load <path>` (Phase 0) for ad-hoc timing, and
`js status` gaining `loads=<n> last_load_ms=<t> arena_used=<b>`.

### 9.3 Fingerprints and evidence

- Launcher: N rows where N = catalog length + 1 (Settings); `js hits`
  prints them. Record the number in the diary after every phase.
- Every launch prints `pulp screen: <id>`; every failure prints
  `pulp screen: error/<id> <why>`; every load prints `js load: <path> <bytes>
  <ms>` at INFO.
- Soak script pattern from ESP-53 (`until` loops, detached, one port
  owner).

## 10. Risks, alternatives, open questions

- **Parse time on the device is unmeasured.** Host ratios say tens of ms;
  if Phase 0 finds > 200 ms for a 5 KB app, the process-model alternative
  (context restart + two images) becomes attractive; the descriptor and
  catalog design survive that change unchanged.
- **Arena fragmentation.** The compacting GC should reclaim the previous
  app's functions; if `js status` shows creep across launches, the cause is
  a lingering reference (a `__cbs` entry — impossible after `resetTree` — or
  `os.state` holding closures, which the contract forbids). Probe: ten
  launches, arena flat.
- **RAM atoms accumulate.** Every new identifier evaluated from source
  becomes a RAM atom for the life of the context; atoms are never freed.
  Fifteen apps × ~50 unique identifiers is a few KB. Mitigation if it ever
  matters: the process model.
- **A misbehaving app inside `main` before its first present** leaves the
  screen on the previous page while the widget tree is already reset;
  `errorPage` handles the throw case, but a `main` that simply returns
  without presenting leaves a stale screen. Loader rule: if
  `PresentCount()` did not advance during `main`, present the error page
  ("app showed nothing").
- **Two writers on `/sdcard/apps`.** Upload (httpd task) and
  install/remove (owner) touch different files; the only shared file is a
  manifest rewritten by the loader after launch — write to `.part` and
  rename, both sides.
- **The `settings` app becomes a module too** — should it stay in ROM? It
  is needed to fix WiFi when nothing else works. Recommendation: ship it as
  an asset like the others but *never* let an SD copy override it (the merge
  rule exempts `settings`), so a broken patch cannot lock the operator out.
- **Open:** should the ROM apps be *only* assets (no seeding) to keep the
  card clean, with seeding an explicit Settings action? Default in this
  design is seed-on-first-boot; flip it if the operator prefers.
- **Open:** a host runtime for JS behaviour (stubbed natives with a fake
  widget arena) would make app development possible without a device;
  worth its own ticket after this one.
- **Alternative not chosen (single-context loader):** downloading
  *bytecode* — pointless while the one image slot is taken by the OS core.
  It becomes viable again under R-MULTICTX (fresh context per app/page).
- **Multi-context risks (Phases 8–10):** every binding touched once (regressions
  caught by the existing probes only if they run under the new layer);
  shared native singletons (widget arena, page table, present pipeline)
  must be reset on every foreground switch or a page sees another
  context's widgets; the deny-list is a maintenance duty — a new native
  added to `pulp_stdlib.c` is *allowed* in pages until someone adds it to
  the UI table's deny mapping (make the generator default to deny).
- **Browser open questions:** cookies/auth (none in v1; the browser app
  could add a header per origin via `http.header`); caching (none; pages
  are small); forms (the keyboard widget exists in Settings — expose it to
  pages through `ui`); images (a page cannot call `images.display`; a
  `rom:`/URL bitmap widget would need the ESP-54 blit path opened to the
  UI stdlib with a size cap).

## 11. Gotcha catalog (inherited + anticipated)

Inherited (do not relearn): the regeneration protocol (§3.7); `esp_psram`
must be named in `COMPONENTS`; never build inside a component dir;
`sdkconfig.defaults` seeds only absent values; array holes are `TypeError`
(seed `[null]`); text values cap at 63 chars; owner stack 8 KiB;
`max_uri_handlers` must count every registered handler; `//` comments are
fatal in single-line C-string JS; USB output is dropped when nobody reads;
one port owner; ~7 s boot before commands are accepted.

Anticipated by this design:

1. **`load()` after `JS_LoadBytecode` is fine; `JS_LoadBytecode` after any
   eval is not.** The boot order in `JsInit()` must not move.
2. **`JS_EVAL_RETVAL` returns the last statement's value.** A file that
   ends with `;` after the object still returns the object; a file that
   ends with a `function` *declaration* returns `undefined` — the lint
   catches it, the loader reports `bad descriptor`.
3. **Never hold the value returned by `load()` in C.** It is a
   pointer-tagged `JSValue` that moves under GC; return it to JS
   immediately (the implementation does exactly one thing with it: return).
4. **`RUN.desc = null; gc();` before `load()`**, or the parser competes with
   the previous app for arena.
5. **Route handlers cannot present or launch.** `/apps/run` sets a flag;
   the tick launches. Presenting from a route runs on the owner but inside
   the 1 s route deadline and the request slot's lifetime — don't.
6. **`files.list` skips names longer than 39 chars silently.** Keep ids ≤
   24 chars so `<id>.json` (≤ 29) always lists.
7. **`JSON.parse` on a manifest line with a trailing CR**: `files.line`
   strips CR already (`app_files.cpp:318-321`); a manifest written by
   Windows tools is fine, one written with a BOM is not — reject and mark
   broken.
8. **A `subtitle` function in a manifest cannot exist** (JSON) — the
   launcher only calls `subtitle()` for ROM registry entries; SD entries
   have string subtitles.
9. **`delete STATE[id]`** works on plain objects in this dialect; `delete`
   of a `var` does not — never keep app state in `var`s.
10. **The upload route's `?name=` is a file name, not the descriptor `id`.**
    The loader trusts the descriptor and rewrites the manifest; a mismatch
    (`name=foo` but `id:'dice'`) is reported as `bad descriptor` on first
    launch. Keep them equal.

## 12. References (key files)

| Topic | File(s) |
|---|---|
| The monolith | `0114-papers3-pulp-os/tools/js/apps/pulp.js` (prelude 1–77, home 97–139, enter 52–64, osRoutes 26–50, settings 860–1034, radio 1035–1119, boot 1120–1125) |
| JS host core, kernel, callbacks, bytecode load, `js_load` stub | `main/app_js.cpp` (kernel 93–97, LoadBytecodeApps 109–129, EvalBounded 163–175, RegisterCb/CallCb 295–337, JsInit 372–410, JsRunPulp 412–427, js_load 572–574, resetTree 603–635) |
| Limits, ModuleId, PageEntry | `main/app_js_internal.h:16-36`, `main/app_events.h:40-62` |
| Engine bytecode API and rules | `components/mquickjs/mquickjs.h:317-364`, `mquickjs.c:182, :12800-12962`, `:15330-15341` (eval) |
| Stdlib definition (add natives here) | `tools/js/pulp_stdlib.c`, `tools/js/mqjs_stdlib_pulp.c:384-433`, `main/app_js_bindings.h`, `tools/js/pulpjsc.c` (STUB list) |
| Build pipeline | `tools/js/gen_pulp_stdlib.sh`, `tools/js/build_bytecode_apps.sh`, `main/CMakeLists.txt` |
| files module (path rules, buffers) | `main/js_files.cpp`, `main/app_files.cpp:46-56, :91-122, :183-221`, `main/app_files.h:20-23` |
| http module | `main/js_http.cpp`, `main/net_http.cpp`, `main/net_http.h:18-22` |
| serve module, POST upload precedent | `main/js_serve.cpp`, `main/net_serve.cpp:118-195, :241-286, :289-304, :460-485, :501-524`, `main/net_serve.h:24-27` |
| images catalog precedent | `main/app_images.cpp:54-86`, `main/app_images.h:42-45` |
| owner loop, console arg encoding | `main/app_owner.cpp:234-257, :495-502, :556-571, :604-642`, `main/app_console.cpp:311, :609-654` |
| settings store limits | `components/s3paper_storage/src/storage.cpp:133-141` |
| console client, serial rules | `ttmp/2026/07/14/ESP-50-*/scripts/52-papers3-console-client.py`, `0114-papers3-pulp-os/README.md` |
| Experiments for this ticket | ticket `scripts/01-trial-split-bytecode-sizes.py`, `scripts/02-host-eval-harness.{c,sh}` |
| Evidence collections | ticket `sources/01-native-side-map.md`, `sources/02-prior-ticket-fact-sheet.md` |
| Multi-context / sandbox evidence | `components/mquickjs/mquickjs.h:260-266` (`JS_NewContext`, `JS_SetContextOpaque`), `mquickjs.c:182, :231, :3628-3656, :3786, :5361` (per-context atom tables, opaque, `c_function_table` indexing), `main/js_stdlib_table.c:1-6`, `main/js_stdlib.h:3584, :4439` (function table, `JSSTDLibraryDef`) |

## 13. Glossary

- **App module / descriptor** — one ES5 file evaluating to `({id, title,
  subtitle, version, abi, main})`.
- **Arena (JS)** — the 192 KiB PSRAM block that holds the MicroQuickJS heap
  and stack; distinct from the 320 KiB *frame arena* of the widget runtime.
- **Atom** — an interned identifier/string; ROM atoms come from the stdlib
  table (and the one bytecode image), RAM atoms are created by evaluating
  source and are never freed.
- **Catalog** — the merged list of ROM registry entries and `/sdcard/apps/
  *.json` manifests the launcher shows.
- **`enter(id)`** — the app-switch boundary: `resetTree()` + re-register OS
  callbacks and routes.
- **Facade (`os`)** — the plain object of OS helpers handed to `main`.
- **Image** — a relocatable bytecode blob produced by `pulpjsc`; one per
  context, loaded before any eval.
- **`load(path)`** — native: read a file (`/sdcard`-rooted or `rom:` asset),
  evaluate it under a deadline, return its value.
- **Loader** — `launch(id, arg)`: catalog lookup → gc → `load` → validate →
  `enter` → `main`, with the error page as the failure path.
- **Manifest** — `<id>.json` next to the module: `id, title, subtitle,
  version, abi, size, src, seed`.
- **Owner** — the single `ui_owner` task that runs all JS and owns the
  widget arena.
- **Push / pull install** — `POST /apps/upload` from a browser or `curl` /
  `http.get` from the device.
- **Seed** — first-boot copy of the ROM apps onto the card, marked in the
  manifest so a firmware update can refresh unedited copies.
- **Context (JsCtxState)** — one MicroQuickJS heap with its own arena,
  atoms, image slot, pages, hits, dyn values and callbacks; the OS has one,
  each app or page may get one (§6.11).
- **Foreground** — the context that currently owns the widget arena and
  receives gestures/ticks.
- **Page script** — a descriptor `({title, main(ui, nav)})` fetched from a
  web server and run in a UI-sandbox context (§6.12).
- **UI stdlib** — the second `JSSTDLibraryDef` sharing the ROM table whose
  non-UI function-table entries are the `js_ui_denied` stub.
- **`nav`** — the page-side navigation singleton; records `go/back/reload`
  requests in a native mailbox for the browser app to act on.
