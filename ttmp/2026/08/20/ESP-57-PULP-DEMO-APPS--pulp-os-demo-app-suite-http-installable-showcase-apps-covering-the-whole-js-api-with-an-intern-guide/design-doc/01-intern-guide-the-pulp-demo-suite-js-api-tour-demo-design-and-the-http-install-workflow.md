---
Title: 'Intern guide: the PULP demo suite — JS API tour, demo design, and the HTTP install workflow'
Ticket: ESP-57-PULP-DEMO-APPS
Status: active
Topics:
    - papers3
    - esp32s3
    - microquickjs
    - eink
    - javascript
    - demo
    - design-system
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: Intern guide for the PULP demo suite - twelve HTTP-installable apps that together exercise every surface of the PULP OS JS API (widgets, canvas, gestures, ticks, files, store, http, wifi, serve, buzzer, battery, mdns, books, catalog), plus the one OS change they need (hidden SD manifests + a Demos index app) and the push/verify workflow with on-device screenshots.
LastUpdated: 2026-08-20T10:57:30.990390581-04:00
WhatFor: Read to learn the whole JS API by example, to add a new demo, or to use the demo suite as the acceptance harness for API changes.
WhenToUse: When touching the JS stdlib, onboarding onto the PULP JS API, or preparing a device demo.
---

# Intern Guide — The PULP Demo Suite

## 0. How to read this guide

This guide assumes you know what PULP OS is (ESP-53 onboarding guide) and
how dynamic apps work (ESP-55 intern guide: apps are single-file
descriptors loaded as source by the native `load()`, installable over
HTTP). What it adds: a complete inventory of the JS API a program on this
device can call (§3), the design of twelve small applications that
together touch every row of that inventory (§4), the single OS change the
suite needs (§5), and the workflow that puts the suite on a device with
nothing but `curl` (§6). File references are relative to
`0114-papers3-pulp-os/`.

The demo suite has three jobs, in priority order:

1. **Teach.** Each demo is a readable, single-purpose example of one API
   area — the document you hand someone who asks "how do I play a sound /
   read a file / register a route from JS?".
2. **Prove.** Together the demos are a living acceptance harness: after
   any stdlib change, pushing the suite and walking it answers "does the
   whole API still work?" with screens rather than probe strings.
3. **Show off.** Every demo is a real screen built on the ESP-56 design
   system; the suite doubles as the sales demo for the device.

## 1. Executive summary

Twelve demo apps live in `tools/js/demos/` — repository-versioned next to
the API they exercise, but **not** embedded in the firmware: they reach a
device exclusively through `PUT /apps/upload` (the ESP-55 push loop) and
land on the SD card like any operator-installed app. Eleven of them
install hidden (a one-line merge() change makes SD manifests' `hidden`
flag effective) and are reached through the twelfth, a visible **Demos**
index that `os.launch`es them — so the launcher gains exactly one row, not
twelve. Each demo covers one API area and states on-screen what it
demonstrates; the ticket's `scripts/02-push-demos.sh` installs the whole
suite in one command, and the ESP-56 screenshot pipeline captures every
demo for the ticket record.

## 2. What a demo app is

A demo is an ordinary app module (ESP-55 §6.2): one file evaluating to
`({id, title, subtitle, version, abi: 2, main: function (os, arg) {…}})`.
It is trusted operator code (R-TRUST) with the full API — unlike browser
pages, demos may use files, network and sound; that is the point. Each
demo follows four suite rules:

- **One area per demo.** A demo demonstrates its own area completely and
  uses the rest of the API only incidentally (every demo necessarily uses
  widgets and pages).
- **Design-system only.** Layout goes through `os.body`, `os.chrome`,
  `os.hintFooter`, `os.menuRow`, `os.button`, `os.buttonRow`, `os.label`,
  `os.keyboard`/`os.key` — a demo that needs a raw `pad()` is showing a
  gap in the system, which is itself a finding.
- **Evidence on screen and on serial.** Anything asynchronous prints a
  `demo:` line and mirrors its state into a dyn text, so both a human and
  the console harness can follow.
- **Back rows.** Every demo's action row ends with a `demos` button
  (`os.launch('demos')`) in addition to the universal swipe-down-home.

## 3. The JS API inventory

This is the coverage checklist. "Demo" names the app responsible for
showing the row; areas marked *(browser)* are showcased by the ESP-55
browser + page server rather than duplicated here.

### 3.1 Globals and engine

| API | Semantics | Demo |
|---|---|---|
| `print(...)` | serial evidence line | all |
| `millis()`, `Date.now()`, `performance.now()` | ms since boot (same clock) | ticker |
| `gc()` | force a collection | sysinfo |
| `abiVersion()`, `paper.version()` | ABI (2) | sysinfo |
| `load(path)` | evaluate a module (`rom:` / `page:` / SD) | sysinfo (shows loads counter via catalog) |
| `resetTree()` | app-switch boundary — loader-only, demos must NOT call it | — (documented) |
| `eval`, `JSON`, `Math`, `String`/`Array` methods, regexp | engine built-ins | storage (JSON), canvas (Math) |

### 3.2 Widgets and pages (the builder)

| API | Demo |
|---|---|
| `text(str|fn)` sizes `xs sm md lg xl title`, `gray`, `center`, `align`, `invert`, `width/height`, `flex` | widgets (type specimen) |
| `row col list spacer divider progressBar` + `pad gap mainAlign crossAlign` | widgets |
| `onTap` / `hit` fat targets | touch |
| `canvas()` + `line disc ring box paint wipe` | canvas |
| `page(name)` `header content footer overlay` `show(full)` `update()` | all; ticker shows update-vs-show |
| `page.on(G.*)` gestures, `page.every(ms)` + `G.TICK` | touch, ticker |
| dyn text (`text(fn)`) | ticker, power |
| `paper.refreshTurns(n)` | canvas (clean-full policy for art) |
| `paper.home/sleepImage` | OS-owned — documented, not demoed |

### 3.3 Services and peripherals

| Singleton | Surface | Demo |
|---|---|---|
| `files` | `exists list read write append remove name size isDir line lineCount` (async, `module busy`, 32 KiB body, path sanitizer) | storage |
| `storeGet/storeSet` | 16 × int32 named settings | storage |
| `buzzer` | `tone(freq,ms) beep stop melody(spec)` | sound |
| `battery` | `level mv charging statusText` (+`batteryLevel()` alias) | power |
| `wifi` | `status ip ssidCurrent rssiCurrent scan join joinSaved save forget savedCount savedSsid off` | net (status + saved; join stays in Settings) |
| `http` | `get header limit done send abort status length body bodyLine(s)` (GET, 32 KiB) | net |
| `serve` | `get(path).handle(fn) text json status query files start stop url` (8 routes, 4 KiB responses) | webserver |
| `mdns` | `status host url` | power |
| `images` | `count name display remove received` | *(gallery app — referenced)* |
| `apps` | `count name copy writeText received uploadName` | sysinfo (registry listing) |
| `bookOpen/Title/Line/LineCount/Next/Prev/Progress`, `libraryCount/Line/Rescan`, `appendPostcard` | books |
| `nav`, `browser` | page sandbox + runtime | *(browser + scripts/08 pages — referenced)* |
| `catalog()/catalogFind()`, `os.*` facade | loader + design system | demos index, sysinfo |

## 4. The demo suite

| Id | Title | Row in launcher | Covers |
|---|---|---|---|
| `demos` | Demos | **visible** | the index; os.launch, catalog filtering |
| `d-widgets` | Type & Widgets | hidden | every widget kind + text style |
| `d-canvas` | Canvas | hidden | all five draw verbs + wipe + refreshTurns |
| `d-touch` | Touch Lab | hidden | gestures, hit regions, tap coordinates |
| `d-ticker` | Ticker | hidden | every(ms), dyn text, update vs show |
| `d-storage` | Files & Store | hidden | files chain + JSON + storeGet/Set |
| `d-net` | Network | hidden | wifi status/saved, netUp, http fetch |
| `d-serve` | Web Server | hidden | JS routes, query, hit counter |
| `d-sound` | Sound | hidden | tone scale, beep, melody |
| `d-power` | Power & mDNS | hidden | battery singleton, mdns, dyn refresh |
| `d-books` | Books | hidden | book/library API paging |
| `d-sysinfo` | System | hidden | millis/abi/gc, ROM registry, catalog |

Sketches of the non-obvious ones (full sources in `tools/js/demos/`):

```text
d-widgets   a type specimen: one column showing xs/sm/md/lg/xl/title with
            face names, gray steps 0/64/96/128/176, invert chip, progress
            bars at 250/500/750, row/col/spacer alignment diagram built
            from dividers — the design system as a screen.

d-touch     full-screen page; header shows a dyn text "last: TAP 132,408"
            updated from p.on(G.*) handlers for all six gestures; a 3x3
            grid of numbered fat targets proves hit routing; footer counts
            dispatches. Traps G.DOWN deliberately and offers a home
            button — demonstrating (and documenting) the trap rule.

d-storage   a scripted chain with an on-screen log list:
            write /demo/log.txt -> append -> read (show lines) ->
            list /demo -> remove; then JSON.parse/stringify round trip;
            then storeSet('demoruns', +1) shown across relaunches.
            Every completion appends a log row (files is one-op-at-a-time:
            the chain IS the demo of the completion contract).

d-serve     starts serve if down (via os.netUp), registers
            /demo -> serve.json({hits}) and /demo/echo?msg= -> echo;
            the screen shows serve.url() + a dyn hit counter. Registering
            routes from an app also demonstrates that resetTree clears
            them (leave and return: counter resets) — stated on screen.

d-books     bookOpen(0) if the library has entries; shows title, progress
            permille, first 12 lines; next/prev buttons page; library
            row count shown. Falls back to a "no books" screen.
```

## 5. The one OS change: hidden SD manifests

The launcher lists every non-hidden catalog entry. Installing eleven
demos would add eleven rows to a list that already clips at ~13. The
merge rule (`os/20-catalog.js`) already honors `hidden` on ROM entries
(that is how `reader` stays unlisted); the change makes an SD manifest's
`"hidden": true` effective too — for overrides and for new entries — so
the suite's manifests can declare themselves hidden while remaining fully
launchable by id via `os.launch` and `/apps/run`. The Demos index is then
the suite's single visible row: it filters `catalog()` for ids starting
`d-` and builds menu rows.

```text
merge(): SD entry m ->
  hidden = m.hidden === true (new entries)   // the one-line change
  hit.hidden stays ROM's when overriding     // an SD patch cannot
                                             // unhide reader by accident
```

## 6. The install workflow

```bash
ttmp/.../ESP-57-*/scripts/02-push-demos.sh --host pulp.local
# for each tools/js/demos/*.js:
#   curl -T $f "http://$host/apps/upload?name=<id>"      (module)
#   curl -T <generated manifest> is NOT possible -- manifests are written
#   by the device only if absent; the push script instead PUTs the module
#   and then uploads a manifest via the same route? NO: the upload route
#   writes modules only. The script uses the pull path instead? NO.
#   -> the suite ships manifests through the module itself: see below.
```

Manifests carry `title/subtitle/hidden`; the upload route only writes a
minimal manifest when none exists. The suite therefore installs in two
steps per app, both plain HTTP: the module via `/apps/upload?name=<id>`,
then the manifest by overwriting `<id>.json` — for which the route gained
nothing: the script drives the DEVICE's own `apps.writeText` through a
tiny one-shot page? That would be circular. The honest resolution, and
what is implemented: **the upload route accepts an optional
`&hidden=1&title=...&subtitle=...` query** and writes the full manifest
when creating one (net_serve.cpp change, ~10 lines, query values are
`[A-Za-z0-9 _.-]` sanitized). curl remains the only tool needed.

Verification: `curl /apps/list` shows the suite (hidden entries included,
marked), the launcher shows exactly one new row (Demos), and the ESP-56
`shot` pipeline captures every demo screen into `sources/` for review.

## 7. Implementation notes (as built)

The suite shipped as designed (twelve modules in `tools/js/demos/`, HTTP-only
distribution, hidden manifests fronted by the Demos index), but installing it
was the real test campaign — five platform defects surfaced, each fixed at the
root rather than routed around. The diary carries the narrative; the facts:

1. **httpd stack overflow (crash).** The upload route's new manifest-writing
   path (224 B query buffer + title/subtitle buffers + FATFS fprintf) blew the
   default 4096 B httpd task stack on the first suite push and rebooted the
   device. `cfg.stack_size = 6144` in `main/net_serve.cpp`. Lesson: the first
   failure read as "network went away" — only re-running with the console
   attached showed the `***ERROR*** A stack overflow in task httpd` banner.
2. **Query encoding contract.** The upload route deliberately does not
   urldecode; a naive push script let `&` in a title split the query. The
   push script (`scripts/02-push-demos.sh`) now uses `quote_plus`, titles are
   plain ASCII by rule (§5), and a title-bearing upload rewrites a stale
   manifest instead of being blocked by write-once behavior.
3. **`files.list` cap, again.** The suite pushed `/sdcard/apps` past 64
   entries; `kFilesMaxList` raised to 128 (`main/app_files.h`) with a sizing
   comment. Second occurrence of the class — pagination is now a standing
   open question, not a someday note.
4. **Silent catalog-scan death (the ESP-57 regression).** Boots (and every
   `js pulp` re-eval) intermittently came up with a ROM-only catalog and no
   scan diagnostics at all. Instrumenting every silent exit in the dispatch
   path caught it live: `FilesList` runs synchronously on the owner task and
   posts its completion to the 32-slot owner event queue; during a long owner
   event (full OS re-eval + CleanFull present ≈ 700 ms) the 20 ms touch-tick
   producer fills the queue, the completion post fails `CapacityExceeded`,
   and the scan callback waits forever. Two-layer fix (commit `147507c7`):
   `PostEvent` denies droppable TimerDue ticks the last 8 queue slots (ticks
   self-heal 20 ms later; completions are load-bearing), and every files-op
   completion post that fails now propagates as a non-Ok status so the JS
   binding cancels the callback and the caller sees a nonzero rc. The
   formerly silent paths (resetTree cancelling a live cb, dropped ModuleDone,
   lost `__cbs` slot) now log permanently.
5. **The reproducer that earned its keep:** `js pulp` forces a full OS
   re-eval and was a 100% reproducer of (4) — worth remembering as the
   cheapest way to stress the queue-under-long-event path.

## 8. Validation

Evidence captured on hardware 2026-08-20 (shots in `sources/shots/`):

- **Install:** 12/12 uploads returned 200 in one `02-push-demos.sh` run
  (after the stack fix); `/apps/list` shows 16 visible entries with all
  `d-*` hidden, `demos` visible; launcher fingerprint is +1 row exactly
  (hits 12 → 13).
- **Scan robustness:** cold boot + three consecutive `js pulp` re-evals =
  4/4 `pulp apps: scanned 27 manifest(s)`, zero `completion post failed`,
  zero dropped completions — the previously 100%-reproducible silent death
  is gone.
- **Walk:** all 12 demos launched over HTTP (`/apps/run?id=…` → 200, forced
  home first per the 409 rule) and screenshot-verified: `demos.png` (index
  listing 11), `d-widgets` (every face + INVERT CHIP label rendering — the
  BlitCoverage light-ink path at work), `d-canvas` (line/disc/ring/box/16
  grays), `d-touch`, `d-ticker` (caught mid diff-blit, demonstrating its own
  point), `d-storage` (full async chain: write/append/read/list/remove +
  JSON round trip, err=0 throughout), `d-net` (live station status),
  `d-serve` (routes registered, counter at 0), `d-sound`, `d-power` (battery
  100% + live mdns status), `d-books` (Alice page + progress 100%),
  `d-sysinfo` (catalog introspection: 28 entries, 2 rom-sourced, 26 card,
  12 hidden — matching the design arithmetic).
- **Stability:** `js status` across the walk: exceptions=0, last_error="".

Known cosmetic notes (deliberate or accepted): the widgets specimen's `xl`
line clips at the right edge (a scale specimen, not a layout bug); index-row
subtitles truncate at the margin; the sound demo's 7-key scale row clips its
last key at 540 px — a future pass could drop it to 6 keys.

## 9. References

| Topic | File |
|---|---|
| Demo sources | `0114-papers3-pulp-os/tools/js/demos/*.js` |
| App contract + loader | ESP-55 guide §6.2–6.5; `tools/js/os/30-loader.js` |
| Design-system idioms | ESP-56 guide; `tools/js/os/10-facade.js` |
| Catalog merge + hidden | `tools/js/os/20-catalog.js` |
| Upload route (+ manifest query) | `main/net_serve.cpp` (ServeAppsUpload) |
| Push script | ticket `scripts/02-push-demos.sh` |
| Screenshot pipeline | ESP-56 `scripts/01-pulp-shot.py`, `main/app_shot.cpp` |
| JS API ground truth | `tools/js/pulp_stdlib.c`, `tools/js/mqjs_stdlib_pulp.c`, `main/js_*.cpp` |
