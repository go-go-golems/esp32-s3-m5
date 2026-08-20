---
Title: Investigation diary
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
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: repo://0114-papers3-pulp-os/main/app_files.cpp
      Note: Completion-post failure now propagates as CapacityExceeded from every op (commit 147507c7)
    - Path: repo://0114-papers3-pulp-os/main/app_js.cpp
      Note: Permanent diagnostics on formerly silent completion-loss paths (commit 147507c7)
    - Path: repo://0114-papers3-pulp-os/main/app_owner.cpp
      Note: PostEvent tick reservation — the root fix for the lost-completion race (commit 147507c7)
ExternalSources: []
Summary: ""
LastUpdated: 2026-08-20T10:57:32.176562863-04:00
WhatFor: ""
WhenToUse: ""
---




# Investigation diary — ESP-57 demo suite

## Goal

Chronological record of the demo-suite build: the guide, the twelve demos,
the platform changes they forced, and the hardware gates — including the
crash and the regression the suite flushed out.

## Step 1: Guide and suite

The ticket began with the intern guide (API inventory as the coverage
checklist, twelve-demo design, the hidden-manifest change, the push
workflow), then the twelve demo sources in `tools/js/demos/` — deliberately
NOT firmware-embedded: the suite reaches a device only through the ESP-55
HTTP push loop. All twelve parse on the host harness.

### Prompt Context

**User prompt (verbatim):** "make a whole set of demo apps that can be loaded over http through the webserver, to showcase all the functionality of the JS API. Create a new docmgr ticket for this, Create  a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and the nupload to remarkable. / And then commit at appropriate intervals and keep a detailed diary as you work (using the diary format from the skill)"

**Assistant interpretation:** A complete, HTTP-installable API showcase + intern guide + diary + reMarkable.

**Inferred user intent:** Living documentation and acceptance harness for the JS API.

**Commit (code):** 244293f2 (guide) + (this commit: suite + platform changes)

### What I did
- Guide (design-doc/01): §3 API inventory tables, §4 suite design, §5
  hidden-manifest change, §6 install workflow.
- Demos: demos (index), d-widgets, d-canvas, d-touch, d-ticker,
  d-storage, d-net, d-serve, d-sound, d-power, d-books, d-sysinfo — every
  one on the ESP-56 design system.
- Platform: merge() honors `hidden` on SD manifests (new entries only —
  ROM's flag stays authoritative on overrides); the upload route accepts
  `title/subtitle/hidden` query fields and (re)writes the manifest when
  metadata is present; `scripts/02-push-demos.sh`.

## Step 2: The gates giveth — a crash, an encoding trap, and a regression

Pushing the suite was the test the platform needed.

### What didn't work (and what each cost)
1. **httpd task stack overflow — device crash.** The first suite push
   crashed the device after one upload: `***ERROR*** A stack overflow in
   task httpd has been detected`, uptime reset to ~40 s. The new
   manifest-writing path (224 B query + title/subtitle buffers + FATFS
   fprintf) exceeded the 4096 B default. Fix: `cfg.stack_size = 6144`.
   Caught only because the second push attempt ran with the console
   attached — the first failure looked like "network went away".
2. **`&` in titles split the upload query.** The push script's naive
   urlenc (spaces only) let `Type & Widgets` truncate its own title and
   eat the subtitle; and since the route deliberately does NOT urldecode,
   `%26` arrives literally. Resolution: proper quote_plus in the script
   AND plain-ASCII titles (`Type and Widgets`) — documented in the script.
3. **Manifests were write-once.** Broken metadata from (2) persisted
   because the route never rewrote an existing manifest. Rule change:
   metadata in the query rewrites; a bare push still never clobbers.
4. **`files.list` cap outgrown AGAIN** (64 this time, by the suite's 24
   files on top of the seeded card): raised to 128 with a sizing comment.
   Second occurrence of this failure class in one ticket pair — the
   pagination question in the ESP-55 diary is now a real TODO.
5. **A catalog-scan regression appeared** after the final flash: boot
   completes with `catalog=13` (ROM only), no `pulp apps: scanned` line —
   scanApps dies on one of its silent error paths. Error paths are now
   instrumented (prints added), but the instrumented build could not be
   flashed: the device physically dropped off USB mid-flash (the known
   flaky connection). Investigation paused at the hardware.

### What worked
- The Demos index ON THE PANEL listing all 11 hidden demos with correct
  metadata (sources/shots/index3.png) — the hidden-manifest mechanism,
  the index filter, and the /apps/list hidden-skip all verified.
- The suite installs in one command; 12/12 uploads 200 after the stack
  fix.

### What warrants a second pair of eyes
- The scanApps regression (open): suspicion list ordered — files.list rc
  path, module-cb interference from the upload watcher, FATFS state after
  the stack-overflow crash mid-write.
- The goto-based manifest guard in ServeAppsUpload.

### What should be done in the future
- Resume when USB returns: flash instrumented build, read the scan error,
  fix, re-walk all 11 demos with the shot pipeline, finalize guide §7/§8,
  reMarkable upload.

### Code review instructions
- `git show` this commit: `tools/js/demos/*`, `net_serve.cpp`
  (stack + manifest query), `20-catalog.js` (hidden + instrumentation),
  ticket scripts/02.

## Step 3: The scan-death hunt — a lost ModuleDone under queue pressure

The device came back on USB and the "regression" turned out to be the best
find of the ticket: not a FATFS casualty, not a scan bug, but a systemic
event-queue loss. The instrumented build initially would not reproduce at
cold boot (scan succeeded, "scanned 27"), but `js pulp` — a full OS
re-eval — reproduced it 100%: catalog wiped to ROM-only, and none of the
three instrumented scanApps error paths printed. That silence was the
clue: the failure was below JS.

Three rounds of C-side instrumentation (resetTree cancelling a live cb;
JsModuleDone dropping a cancelled completion; CallCbIn's not-a-function
and stack-check outs; a Files delivery trace) narrowed it until the log
handed over the verdict: `W (18990) files: completion post failed:
CapacityExceeded`. `FilesList` runs synchronously on the owner and posts
its completion to the 32-slot owner queue; during the long `js pulp`
event (main re-eval + CleanFull present ≈ 700 ms) the 20 ms touch-tick
producer fills the queue; the completion is rejected and the freshly
re-initialized `SD_APPS = []` never gets repopulated — the scan callback
waits forever, silently. Cold-boot losses were the same race with worse
luck.

### Prompt Context

**User prompt (verbatim):** (standing task from Step 1 — "commit at appropriate intervals and keep a detailed diary as you work"; resumed on the USB re-enumeration monitor firing, no new user prompt)

**Assistant interpretation:** Resume the gated work: flash the instrumented build, diagnose the scan regression, fix it, finish the demo walk and the ticket.

**Inferred user intent:** A demo suite that reliably survives boot and app switches, and a root-caused platform instead of a papered-over symptom.

**Commit (code):** 147507c7 — "ESP-57: fix silent catalog-scan death — lost ModuleDone under queue pressure"

### What I did
- Flashed the instrumented firmware; confirmed scan works at cold boot
  but `js pulp` (and Settings-era catalog reads) showed ROM-only.
- Found `/apps/list` all-`rom` while "scanned 27" had printed; Settings →
  Apps → Rescan healed it live (screenshots captured en route).
- Instrumented every silent exit in the completion path; caught
  `CapacityExceeded` live.
- Fix 1: `PostEvent` reserves the last 8 queue slots against TimerDue
  ticks (droppable by design, ~50/s; completions are load-bearing).
- Fix 2: files-op completion-post failure now propagates as
  `CapacityExceeded` from `FilesList/Read/Write/Append/Remove` so the
  binding cancels the cb and JS sees a nonzero rc (scanApps prints).
- Kept the diagnostics permanently (they cost a log line only when
  something is actually lost).
- Validation: cold boot + 3x `js pulp` = 4/4 "scanned 27", hits=13.
- Walked all 12 demos over HTTP (200 each), screenshot-verified, copied
  to sources/shots/; finalized guide §7/§8.

### Why
- A lost completion strands any async JS chain forever with zero
  evidence; the catalog scan was just the first victim large enough to
  notice. http, uploads, and wifi completions share the same queue.

### What worked
- Making every silent path loud, then letting the device talk. The
  smoking-gun log line existed for one build cycle before the cause was
  obvious.
- `js pulp` as a deterministic reproducer for a race that was
  intermittent at boot.

### What didn't work
- Theorizing from memory about resetTree/__cbs interactions — three
  plausible mechanisms died on contact with the logs (the appsWatch
  cancel at line "resetTree cancels module=5" is real but benign; the
  Apps cb is re-registered by enter()).

### What I learned
- `FilesList` and friends are synchronous on the owner: their "async"
  contract is purely the queue round-trip, so they compete with every
  other producer for queue slots *while the owner cannot drain*.
- The touch-tick producer's "a full queue just drops this tick" comment
  was correct for ticks — the design error was letting ticks and
  completions share the same fate.

### What was tricky to build
- The failure had three layers of masking: scanApps' instrumented error
  paths (JS) never ran because the callback never fired; JsModuleDone's
  drop path never ran because no event arrived; and the files module
  warned but returned Ok, so the JS binding reported rc=0. Each layer was
  individually reasonable; the composition was silence. The hunt only
  converged by instrumenting *every* exit and trusting that a zero-print
  run meant the loss was earlier still.
- Owner-posts-to-owner-queue means the obvious fix (retry with timeout)
  deadlocks: the owner is the only consumer. Reservation is the correct
  shape; blocking is not available.

### What warrants a second pair of eyes
- The 8-slot reservation constant: sized for ~160 ms of tick backlog
  headroom at 20 ms cadence; a longer owner stall (big CleanFull + eval)
  still fits because ticks stop being admitted at 8 free slots, leaving
  all 8 for completions — but a burst of >8 *completions* during one
  stall would still lose some (nothing produces that today; the modules
  are single-flight).
- Whether net_http/net_serve/wifi (void)-cast PostModuleDone sites should
  also propagate; the reservation protects them, but their failure would
  still be silent-ish (warn only). Follow-up candidate.

### What should be done in the future
- Consider a dedicated completion queue (or priority slot) if module
  concurrency ever grows beyond single-flight.
- The d-sound scale row clips key 7 at 540 px; drop to 6 keys in a
  cosmetic pass.

### Code review instructions
- `git show 147507c7`: `app_owner.cpp` (PostEvent reservation),
  `app_files.cpp` (Post propagation — check every op's early-error path
  returns the post status too), `app_js.cpp` (diagnostics only).
- Validate: flash, then `js pulp` three times with the console attached;
  expect "scanned 27" each time and no "completion post failed".

### Technical details
- Reproducer timeline (hunt2 log): `js pulp` at 18100 ms → "booting"
  print → resetTree cancels module=5 cb=3 (benign appsWatch cancel) →
  home present (render_us=254301) → `W (18990) files: completion post
  failed: CapacityExceeded` → silence. 32-slot queue, 20 ms ticks,
  ~700 ms owner occupancy ≈ 35 ticks.
