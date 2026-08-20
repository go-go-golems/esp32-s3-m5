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
