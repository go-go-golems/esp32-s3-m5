---
Title: Investigation diary
Ticket: ESP-58-PULP-APP-DISCOVERY
Status: active
Topics:
    - papers3
    - esp32s3
    - microquickjs
    - eink
    - javascript
    - mdns
    - networking
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: repo://0114-papers3-pulp-os/main/net_mdns.cpp
      Note: Unused query half of the component was the step-1 finding
ExternalSources: []
Summary: ""
LastUpdated: 2026-08-20T12:56:04.053466189-04:00
WhatFor: ""
WhenToUse: ""
---


# Investigation diary — ESP-58 app-server discovery

## Goal

Chronological record of the discovery work: mDNS browse on the device, the
`_pulp-apps._tcp` + index-endpoint convention, the store screen, and the
host server script — so app installs stop requiring hand-typed URLs.

## Step 1: Evidence pass and the intern guide

The ticket opened with a study pass, not code: the ESP-57 diary (the
encoding trap, the write-once-manifest rule change, the open scanApps
regression), the design-system project report, and every file the design
touches. The finding that shaped the whole design: the firmware already
ships the mDNS component with a full query API — `net_mdns.cpp` just never
calls it. Discovery is not a new subsystem; it is the unused half of an
existing one, plus a convention.

The guide (design-doc/01) was then written around five line-anchored
precedents: the advertise-only mDNS wrapper, the ModuleId/kDone async-verb
grammar, the HttpWorker throwaway-task pattern for blocking calls, the
wifi.scan count/indexed-accessor idiom, and the /apps/list JSON shape the
index contract extends. Three decision records: R-SVCTYPE (dedicated
`_pulp-apps._tcp` + TXT `path=`, not `_http._tcp` filtering), R-BROWSEAPI
(async verb + `mdns.count/name/indexUrl` accessors, URL assembly in C),
R-BROWSEEXEC (blocking `mdns_query_ptr` on a priority-4 worker task,
`ModuleId::Mdns`, `kDoneMdnsBrowse = 50`).

### Prompt Context

**User prompt (verbatim):** "I now want to create a new docmgr ticket, where one can discover webservers with apps, and then can hit an index endpoint. That way we don't need to enter URLs by hand. / Create  a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and the nupload to remarkable. / Study the last diary and the created project reports (like go-go-parc/Projects/2026/08/20/PROJ - PULP Design System and Demo Suite - Screenshot-Driven UI Engineering.md)"

**Assistant interpretation:** New ticket (ESP-58) for mDNS-based discovery of app servers plus a machine-readable index endpoint, with an intern-grade analysis/design/implementation guide stored in the ticket and uploaded to reMarkable; ground it in the prior diaries and reports.

**Inferred user intent:** Kill the last manual step in the app-install loop (typing URLs on an e-ink keyboard) and leave a document a new engineer could implement from.

### What I did
- Studied ESP-57 diary + the ESP-56/57 vault report; read `net_mdns.{h,cpp}`,
  `js_mdns.cpp`, `net_http.cpp` (worker pattern), `app_events.h` (ModuleId /
  kDone enums), `js_wifi.cpp` (accessor idiom), `20-catalog.js`
  (idFromUrl/installFromUrl/scanApps), `00-kernel.js` (/apps/list),
  `settings.js` (URL screen), the managed mdns component's query API, and
  the stdlib generation chain (`pulp_stdlib.c` → `gen_pulp_stdlib.sh` →
  `js_stdlib.h` + atoms → bytecode recoupling).
- Created ticket ESP-58-PULP-APP-DISCOVERY, wrote the guide
  (design-doc/01, 9 sections, 3 decision records, sequence diagram,
  5-phase file-level plan), added P1–P5 tasks.

### Why
- Both existing install paths (push, pull) assume the URL problem is
  solved out-of-band; ESP-54's mDNS solved host→device, nothing solved
  device→host. The guide names that asymmetry as the gap.

### What worked
- Every mechanism downstream of discovery already exists and is
  hardware-proven; the guide could anchor all of §3 to exact lines, and
  the new firmware surface reduced to one verb + three accessors + one
  enum slot + one screen.

### What didn't work
- N/A — documentation step; no code executed against hardware (device is
  still off USB from the ESP-57 gate).

### What I learned
- `espressif/mdns ~1.2.0` carries both blocking (`mdns_query_ptr`, :680)
  and async (`mdns_query_async_new`, :610) query families — the firmware
  binary already pays for browse; nobody calls it.
- The mdns singleton is the only JS module with zero async verbs, so this
  ticket also sets the precedent for its verb grammar.

### What was tricky to build
- Choosing where the blocking query runs. The owner must not block, and
  the async component API would introduce a second completion style (poll
  or notifier) unlike every other verb. Resolution: notice that
  `HttpSend` already solved the identical problem — blocking network call,
  single-flight, mailbox, ModuleDone from a throwaway pinned task — and
  copy it byte-for-byte (R-BROWSEEXEC). The diary of ESP-55 P8 (owner-
  tagged ModuleCb) is what makes this safe across app switches.
- The MdnsStop/browse interplay: `serve.stop()` calls `mdns_free()` while
  a worker could be inside `mdns_query_ptr`. Flagged in §8 as a P1
  decision (defer free vs. refuse stop with Busy) rather than silently
  designed away.

### What warrants a second pair of eyes
- The UI-sandbox rule: four new natives must each land in
  `js_stdlib_table_ui.c`'s deny list; the guide bakes a grep gate into §8.
- The index contract's absolute-URL decision (R-SVCTYPE consequences) —
  breaks for servers behind NAT/port-forward; accepted for v1, `v` field
  reserved for relaxing it.

### What should be done in the future
- Implement P1–P5 (tasks zmu3/6xn6/5kmu/ypn9/hpd2). P5's hardware walk
  shares the ESP-57 gate: scanApps regression fix + USB return.
- Decide at P2 whether `installFromUrl` grows an optional metadata arg so
  store installs get real titles instead of "installed from url".

### Code review instructions
- Read design-doc/01 top to bottom; check §3 claims against the cited
  lines (all in `0114-papers3-pulp-os/`); check the ModuleId append rule
  against `app_events.h:40`.

### Technical details
- Proposed JS surface: `mdns.browse(fn)` → `fn(50, count, err)`;
  `mdns.count()`, `mdns.name(i)`, `mdns.indexUrl(i)`.
- Index contract: `{"v":1,"name":...,"apps":[{"id","title","subtitle","url"}]}`,
  ids `[a-z0-9_-]{1,24}`, plain-ASCII titles, absolute URLs, ≤ 8 KiB.

## Step 2: P3 + P1 — the server exists, and the device can find it

Implementation began at the two ends of the wire and met in the middle:
first the host server (P3, so the browse verb would have something real
to find), then the browse verb (P1). By the end of the step the PaperS3
printed `Demo Shelf -> http://192.168.0.39:8123/pulp/index.json` from a
console probe — discovery works end to end, no URL typed anywhere.

### Prompt Context

**User prompt (verbatim):** "Ok, build ESP-58, commit at appropriate intervals and keep a detailed diary as you work (using the diary format from the skill)"

**Assistant interpretation:** Implement the ESP-58 guide phases P1–P5 with per-phase commits and diary steps.

**Inferred user intent:** Working zero-typing app discovery on the device, built to the guide's decision records.

**Commit (code):** dd592140 (P3 server script), d8c13f2f (P1 browse verb)

### What I did
- P3: `scripts/01-app-index-server.py` — http.server + zeroconf,
  `_pulp-apps._tcp` with TXT `path=`/`name=`, index built per request from
  the directory (drop a file in, it appears), sidecar `<id>.json`
  metadata, fail-fast plain-ASCII validation. Verified: 12-app index over
  HTTP; record browsable with an independent zeroconf client.
- P1: `ModuleId::Mdns` + `kDoneMdnsBrowse=50` (`app_events.h`);
  `MdnsBrowse` + worker + snapshot + accessors (`net_mdns.{h,cpp}`); four
  natives (`js_mdns.cpp`); registration in `pulp_stdlib.c` + regen +
  bytecode rebuild; host STUBs in `pulpjsc.c`; UI-sandbox denies; owner
  hook `MdnsOnBrowseDone` in `app_owner.cpp`; probe 29.
- Hardware gates: wifi-down → `rc=0` then `done k=50 n=0 e=1` (exact
  contract); live browse → 1 server, correct name and assembled URL;
  second browse during window → "module busy" throw.

### Why
- P3-before-P1 made the browse verb verifiable the moment it flashed —
  the same reason ESP-55 built the page server before the browser.

### What worked
- The HttpWorker copy-exactly plan: single-flight atomic, mailbox,
  ModuleDone-as-last-act transplanted without surprises.
- Probe 29 as the P1 gate: all three contract paths exercised from the
  console before any UI existed.

### What didn't work
1. `Zeroconf()` on all interfaces: `OSError: [Errno 105] No buffer space
   available` (IP_ADD_MEMBERSHIP; igmp_max_memberships exhausted by the
   host's many virtual interfaces). Fix: `Zeroconf(interfaces=[ip])`.
2. `avahi-browse` cannot see the python-zeroconf record on the SAME host
   (two mDNS stacks sharing 5353; a known rivalry). An independent
   zeroconf client — and then the device itself — sees it fine. Do not
   use avahi-browse as the gate for this script.
3. First build: `-Werror=format-truncation` on the URL assembly
   (host 63 + path 47 > url[96]). Fix: url[128] sized for the worst case
   (124) with a sizing comment, `%.31s` on the name fallback.
4. First flash: `mdns_init failed` with wifi down — `mdns_init` needs a
   live netif, so the wifi gate had to move BEFORE `MdnsInit()` for the
   deterministic wifi-down contract (rc=0 + immediate err=1 completion)
   instead of an opaque init error.
5. `gen_pulp_stdlib.sh` prints "Too many properties, consider increasing
   ATOM_ALIGN" — verified pre-existing (same warning on HEAD without the
   four new properties): the builder clamps a hash size, correctness
   preserved. Not ours, left alone.

### What I learned
- The espressif mdns component's query half coexists with the announce
  half with no extra init — MdnsInit covers both.
- The "second browse" guard is the module-cb "module busy" throw, not
  the Busy rc: RegisterModuleCb runs first and throws while a completion
  is pending. Same semantics as wifi join-during-scan (probe 18).

### What was tricky to build
- The MdnsStop/browse interplay (guide §8): `serve.stop`/`wifi.off`/power
  quiesce call MdnsStop, which frees the component; a live
  `mdns_query_ptr` on the worker must not have the rug pulled. Chosen
  shape: MdnsStop defers (flag) when the worker is in flight; the owner
  applies the stop in `MdnsOnBrowseDone` when ModuleDone(Mdns) is
  processed — worst case one 3 s window later. A lost completion would
  leave the deferral pending until the next explicit stop; the ESP-57
  queue reservation makes that loss unlikely.

### What warrants a second pair of eyes
- The deferred-stop path is code-reviewed but not hardware-exercised
  (timing a stop inside the 3 s window from the console is awkward);
  worth a targeted probe if it ever matters.
- BrowseWorker reads TXT/hostname/addr fields of mdns_result_t on the
  worker task after mdns_query_ptr returns — safe (results are a
  caller-owned copy freed by mdns_query_results_free), but confirm on
  component upgrades.

### What should be done in the future
- P2 store screen, P4 self-index + announce, P5 validation (next steps).

### Code review instructions
- `git show d8c13f2f`; start at `net_mdns.cpp` (BrowseWorker, MdnsBrowse,
  MdnsStop deferral), then the four-file registration checklist
  (`app_js_bindings.h`, `pulp_stdlib.c`, `js_stdlib_table_ui.c`,
  `pulpjsc.c` STUBs — the guide §3.6 list plus the host compiler).
- Validate: run the P3 script, `js probe 29` on the console; expect the
  three contract lines.

### Technical details
- Probe 29 live output: `browse _pulp-apps._tcp (3000 ms window)` →
  `browse done: 1 server(s)` → `[0] Demo Shelf ->
  http://192.168.0.39:8123/pulp/index.json`.

## Step 3: P2 — the store screens, and the widget-arena ceiling

The store UI went in as designed (three screens, all launch-mediated),
but its first flash crashed the settings app with `TypeError: widget
arena full` — the ticket's first genuinely new platform finding: the
retained widget tree caps at `WidgetArena::kCapacity = 128`, and the
Settings apps screen with a full catalog (4 action rows + 16 visible
apps = 20 menuRows) already sits at the edge. My 21st row ("Get apps")
pushed it over.

The fix was scoping, not surgery: raising kCapacity is NOT a drive-by —
`widget_diff.cpp` keeps two `CurrentSlot current[kCapacity]` arrays and
`widget_render.cpp` a `Rect clips[kCapacity]` ON THE STACK of the 8 KiB
owner task; doubling the capacity means ~10 KiB of new stack pressure or
a static/heap rework. So: "Get apps" moved to the MAIN settings screen
(7 rows, headroom, arguably better placement anyway), and the shelf
screen defensively caps at 14 rows + a "+N more (not shown)" note.

### Prompt Context

**User prompt (verbatim):** (see Step 2)

**Assistant interpretation / intent:** (see Step 2)

**Commit (code):** 6e1b6acb — "ESP-58 P2: 'Get apps' store screens"

### What I did
- `settings.js`: storeScreen (browse placeholder → server rows on
  completion; empty state names the script and the AP-isolation
  fallback), storeAppsScreen (installed marks via catalogFind,
  id/url-mismatch rows rendered inert, install via installFromUrl, index
  object rides the os.launch arg), dispatch entries, "Get apps" row on
  the main screen.
- On-panel walk (shots in sources/shots/): wifi-down → footer toast "no
  network - join wifi first" on the Apps screen; live → GET APPS lists
  "Demo Shelf" + URL; tap → DEMO SHELF, 12 rows all "- installed"; tap
  d-books → "installed d-books (1970B)".

### What worked
- The scanScreen async-rebuild shape transplanted cleanly to browse and
  index-fetch; no new UI machinery.
- Passing the parsed index through the os.launch arg (plain object,
  same-context relaunch) — no serialization, no os.state bloat.

### What didn't work
- First flash: `crashed: TypeError: widget arena full` opening the Apps
  screen (21 menuRows). See above; relocation + cap. The error page
  itself worked as designed (the ESP-55 errorPage caught it).

### What I learned
- Widget budget arithmetic: a menuRow costs ~5 retained widgets; page
  chrome ~8; the 128 cap supports roughly 22 rows. The launcher (13
  rows) has headroom; the apps screen is the first real pressure point.

### What was tricky to build
- Choosing NOT to bump kCapacity: the stack-allocated diff/render
  scratch arrays scale with it on the owner's 8 KiB stack. A capacity
  pass needs those arrays moved to statics or the render heap first —
  follow-up ticket material, recorded in the guide's open questions.

### What warrants a second pair of eyes
- storeAppsScreen trusts idx.name for the header after toUpperCase
  (plain-ASCII by contract; a hostile shelf name is only ever text — but
  worth eyeballing).
- The 14-row shelf cap: silent-truncation rule says the remainder note
  must always render; verify with a >14-app shelf when one exists.

### What should be done in the future
- Widget-arena capacity pass (kCapacity + stack arrays); shelf paging.

### Code review instructions
- `git show 6e1b6acb` — settings.js only; walk: main → Get apps with and
  without wifi; shelf tap; install tap.
