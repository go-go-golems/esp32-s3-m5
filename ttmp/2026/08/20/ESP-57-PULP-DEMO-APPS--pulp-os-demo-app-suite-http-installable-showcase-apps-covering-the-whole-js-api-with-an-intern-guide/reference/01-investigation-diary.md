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
RelatedFiles: []
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
