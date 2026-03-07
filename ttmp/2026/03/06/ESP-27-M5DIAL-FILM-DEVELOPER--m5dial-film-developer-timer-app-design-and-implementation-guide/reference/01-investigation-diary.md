---
Title: Investigation diary
Ticket: ESP-27-M5DIAL-FILM-DEVELOPER
Status: active
Topics:
    - esp32s3
    - m5stack
    - firmware
    - photo-development
    - ui
    - timer
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0072-m5dial-timer-demo/main/app_main.cpp
      Note: Evidence for the reusable task split discovered during research
    - Path: 0072-m5dial-timer-demo/main/m5dial_board.cpp
      Note: Evidence for the reusable board layer discovered during research
    - Path: 0073-m5dial-film-developer-timer/CMakeLists.txt
      Note: Evidence that the scaffold app was created and renamed
    - Path: 0073-m5dial-film-developer-timer/main/app_main.cpp
      Note: |-
        Evidence that the scaffold app received a distinct visible identity
        Diary evidence for runtime catalog initialization and boot log validation
    - Path: 0073-m5dial-film-developer-timer/main/generated_film_catalog.cpp
      Note: Diary evidence for the generated starter recipe dataset
    - Path: film_dev_times.json
      Note: Evidence for source-data size and schema constraints
    - Path: ttmp/2026/03/06/ESP-27-M5DIAL-FILM-DEVELOPER--m5dial-film-developer-timer-app-design-and-implementation-guide/scripts/analyze_film_dev_times.py
      Note: Diary-backed analysis script for dataset facts
    - Path: ttmp/2026/03/06/ESP-27-M5DIAL-FILM-DEVELOPER--m5dial-film-developer-timer-app-design-and-implementation-guide/scripts/generate_film_catalog_cpp.py
      Note: Diary evidence for the catalog generator
    - Path: ttmp/2026/03/06/ESP-27-M5DIAL-FILM-DEVELOPER--m5dial-film-developer-timer-app-design-and-implementation-guide/scripts/scope_subset_report.py
      Note: Diary-backed script for the narrowed v1 catalog scope
ExternalSources: []
Summary: Chronological diary of the research and design work for the proposed M5Dial film developer timer app.
LastUpdated: 2026-03-06T21:02:48.647041728-05:00
WhatFor: Use this diary to understand what was investigated, what evidence was gathered, and why the design recommendations landed where they did.
WhenToUse: Use when continuing the ticket, reviewing the rationale, or implementing the proposed 0073 application.
---




# Diary

## Goal

This diary captures the research and design work for a new M5Dial film developer timer application. It records the initial problem framing, the evidence gathered from the existing M5Dial timer demo and the film-development dataset, the scripts written during investigation, and the resulting design decisions.

## Step 1: Create the film-developer ticket and define a realistic v1 scope

The user asked for a new ticket and a new app concept based on the current M5Dial timer demo, but with a different domain: film development. The immediate job was not to start coding a new app blindly. It was to inspect the already-working `0072` hardware/software shell, inspect `film_dev_times.json`, and decide whether the new app should browse the full source dataset or start from a reduced, curated subset.

That investigation quickly showed that the hardware foundation is already strong, but the data source is not a drop-in runtime database. The current M5Dial app architecture is reusable. The source film JSON is large and uneven. That led to the core design decision for this ticket: copy `0072`, keep its board and LVGL architecture, and introduce a curated catalog layer between the raw source JSON and the on-device UI.

### Prompt Context

**User prompt (verbatim):**

> Ok, now create a new ticket and create a new app by copying this one, and let's turn it into a film developer timer. I want to be able to select a film and a temperature and a push / pull, potentially a developer if there is the option, based on esp32-s3-m5/film_dev_times.json .
>
> Use the current setup but then design a UI and make a full plan . Create a detailed analysis / design / implementation guide that is very detailed for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file references.
>
> It should be very clear and detailed. Store in the ticket and the nupload to remarkable.

**Assistant interpretation:** Create a new structured research ticket, inspect the current M5Dial timer app and the film-development dataset, and write a detailed onboarding-quality design and implementation guide for a future film developer timer app derived from the current one.

**Inferred user intent:** Turn the now-working M5Dial timer project into a useful, domain-specific product concept with enough technical guidance that an intern can implement it safely without rediscovering the architecture.

### What I did
- Created a new ticket workspace:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/06/ESP-27-M5DIAL-FILM-DEVELOPER--m5dial-film-developer-timer-app-design-and-implementation-guide`
- Added:
  - a primary design document
  - this diary
- Inspected the existing M5Dial timer app:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/app_main.cpp`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/input_events.h`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/m5dial_board.cpp`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/timer_model.h`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/timer_controller.cpp`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/ui_timer_screen.cpp`
- Inspected the film dataset:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/film_dev_times.json`
- Wrote ticket-local reusable scripts:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/06/ESP-27-M5DIAL-FILM-DEVELOPER--m5dial-film-developer-timer-app-design-and-implementation-guide/scripts/analyze_film_dev_times.py`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/06/ESP-27-M5DIAL-FILM-DEVELOPER--m5dial-film-developer-timer-app-design-and-implementation-guide/scripts/scope_subset_report.py`
- Ran the scripts to establish:
  - file size
  - category counts
  - developer frequency
  - temperature shape
  - push/pull variety
  - the feasibility of a reduced starter scope
- Adjusted the intended v1 scope after the follow-up user constraint:
  - start with color C-41 and common B/W developers, not all films

### Why
- The new app should be built on working hardware/application scaffolding, not from scratch.
- The raw source data is too large and inconsistent to use directly as an encoder-driven menu database on a small round display.
- A constrained v1 scope is the only realistic way to ship a useful first version quickly and cleanly.

### What worked
- The `0072` app already provides the correct architectural shell:
  - queue-backed event bus
  - dedicated LVGL-owning UI task
  - stable board bring-up
  - working encoder, button, and touch
- The source JSON was easy to inspect with small Python scripts.
- The scripts confirmed the most important design facts:
  - `14,293` entries
  - `340` films
  - heavy B/W skew
  - dominant `20 C` temperature
  - mixed temperature schema (`celsius`/`fahrenheit` versus `raw`)
- Saving the scripts directly in the ticket made the analysis reproducible, which is important for future implementation work.

### What didn't work
- The first broad text search across the repo was too noisy because it matched thousands of irrelevant strings and huge swaths of the raw JSON file.
- The first attempt to summarize temperatures assumed every record had `temperature.celsius`, which immediately failed with:

```text
KeyError: 'celsius'
```

- The initial assumption that there would be a clean `c41` category in the source data was wrong. The file actually uses `color_negative`, and explicit `C-41` developer rows are much rarer than expected.

### What I learned
- The data work is not optional. The most important architecture decision in this app is how to curate and normalize the catalog before the UI sees it.
- The current M5Dial timer demo is a strong reusable foundation because it already has the right concurrency boundary and hardware behavior.
- “Color C-41” in the product request does not map cleanly to the source file categories, so the guide needs to document that limitation instead of pretending it is already solved.

### What was tricky to build
- The hardest part of this investigation was not technical coding complexity but scope discipline. It would have been easy to promise “all films, all developers, all variations,” but the evidence from the source file does not support that cleanly on a tiny dial UI. The correct sequence was:
  - inspect the current app architecture first
  - inspect the data source second
  - save analysis scripts in the ticket instead of using throwaway one-liners only
  - narrow the initial recommendation to a curated starter scope
- The second tricky point was schema trust. The raw JSON looks structured, but fields like `temperature` are not normalized enough to drive a polished selector without cleanup.

### What warrants a second pair of eyes
- The proposed “common B/W developers plus explicit C-41-like rows” scope is sensible, but it should be reviewed by someone who cares deeply about film workflow expectations.
- The current dataset’s color-negative support looks thin, so the product language in the future app should not over-promise coverage.
- The eventual preprocessing strategy should be reviewed before implementation starts, especially if someone prefers generated JSON over generated C++ arrays.

### What should be done in the future
- Implement the proposed `0073` app by cloning `0072`.
- Add a real preprocessing script under the new app’s own `scripts/` directory.
- Generate a compact starter catalog from the raw JSON instead of parsing the full source at runtime.
- Decide whether color-negative support should remain minimal in v1 or be expanded with additional curated rules.

### Code review instructions
- Start with the architecture base that will be cloned:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/app_main.cpp`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/m5dial_board.cpp`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/input_events.h`
- Then inspect the source data and the analysis tooling:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/film_dev_times.json`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/06/ESP-27-M5DIAL-FILM-DEVELOPER--m5dial-film-developer-timer-app-design-and-implementation-guide/scripts/analyze_film_dev_times.py`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/06/ESP-27-M5DIAL-FILM-DEVELOPER--m5dial-film-developer-timer-app-design-and-implementation-guide/scripts/scope_subset_report.py`

### Technical details

Key source-data facts established during this step:

```text
file size: 7,911,909 bytes
lines: 324,602
entries: 14,293
films: 340
dominant category: bw
dominant temperature: 20.0 C
temperature schema variants: ('celsius', 'fahrenheit') and ('raw',)
```

Example commands run:

```bash
wc -c /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/film_dev_times.json
wc -l /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/film_dev_times.json
python /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/06/ESP-27-M5DIAL-FILM-DEVELOPER--m5dial-film-developer-timer-app-design-and-implementation-guide/scripts/analyze_film_dev_times.py /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/film_dev_times.json
python /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/06/ESP-27-M5DIAL-FILM-DEVELOPER--m5dial-film-developer-timer-app-design-and-implementation-guide/scripts/scope_subset_report.py /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/film_dev_times.json
```

## Step 2: Create and verify the 0073 scaffold by copying the current timer app

The original user request also asked for a new app copied from the current one. The first ticket deliverable focused on design and data strategy because that was the highest-risk unknown, but the scaffold itself still needed to exist on disk so future implementation work would start from a real project rather than a plan only.

This step created `0073-m5dial-film-developer-timer` as a direct copy of `0072`, then performed only a light rename pass. The goal was not to start the full film-domain rewrite yet. The goal was to create a clean, buildable landing zone with its own path, project name, README, and visible log identity.

### Prompt Context

**User prompt (verbatim):** (same as Step 1)

**Assistant interpretation:** The new ticket should be accompanied by a real cloned app directory, not just design notes.

**Inferred user intent:** Make the planning work actionable immediately by leaving behind a concrete project scaffold that implementation can begin from without another setup step.

### What I did
- Copied:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo`
  - to
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer`
- Updated visible scaffold identifiers:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/CMakeLists.txt`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/README.md`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/sdkconfig.defaults`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/app_main.cpp`
- Verified the copied scaffold builds with:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer
source ../.envrc
idf.py set-target esp32s3 build
```

### Why
- A ticket that proposes a new app should leave behind the app skeleton when the cost is this low.
- Keeping the rename pass small avoids premature churn in filenames and namespaces before the actual domain model is introduced.
- A successful build proves that the copied scaffold is a real starting point rather than a stale snapshot.

### What worked
- The directory copy was straightforward.
- The scaffold built successfully under IDF `5.4.1`.
- The new app now has its own project name:
  - `m5dial_film_developer_timer_0073`
- The binary still fits comfortably in the configured partition:
  - `0x9e490` bytes into a `0x400000` app partition

### What didn't work
- Nothing failed in the scaffold creation itself.
- `idf.py set-target esp32s3` did regenerate `sdkconfig` and rename the copied `sdkconfig` to `sdkconfig.old`, but that is expected behavior in this repo and not a blocker.

### What I learned
- The right time to make the copy is before the domain rewrite begins, not after. That keeps the planning ticket honest.
- A minimal rename pass is enough for a scaffold; deeper renames should wait until the film-specific modules replace the inherited timer modules.

### What was tricky to build
- The trick was scope restraint. After copying the app, it would have been easy to start renaming every namespace, class, and identifier from `0072` to `0073`, but that would create a lot of noise before the actual film-domain architecture exists. The more disciplined path was:
  - copy the app
  - rename only top-level visible identity
  - verify the build
  - leave the deeper internals to the real implementation phase

### What warrants a second pair of eyes
- The copied scaffold still contains inherited `tutorial_0072` namespaces and generic timer modules internally. That is fine for now, but it should not be mistaken for a finished architectural rename.
- If someone strongly prefers a full early rename, they should weigh that against the churn cost before any feature code lands.

### What should be done in the future
- Replace the inherited generic timer modules with the planned film-catalog, selector, and film-timer modules.
- Decide when to do the deeper namespace and file rename pass.

### Code review instructions
- Review the copied app root:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer`
- Start with the visible identity changes:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/CMakeLists.txt`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/README.md`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/app_main.cpp`

### Technical details

Build result from this step:

```text
Generated .../build/m5dial_film_developer_timer_0073.bin
m5dial_film_developer_timer_0073.bin binary size 0x9e490 bytes
Smallest app partition is 0x400000 bytes
0x361b70 bytes (85%) free
```

## Step 3: Add the starter catalog generator and prove it initializes on the dial

With the scaffold committed, the next highest-value task was the data foundation. The app cannot become a film developer timer until it has a runtime catalog that the UI can browse cheaply. That made the first real implementation step a catalog-pipeline step, not a UI step. The right move was to build a generator, decide the runtime format, emit a first starter catalog, and confirm the device can load it before changing any screens.

This step intentionally kept the existing `0072` UI and hardware plumbing intact. The goal was narrower and more important: prove that `0073` can carry a curated film dataset at runtime without boot regressions, and establish the data contracts that the later selector UI will consume.

### Prompt Context

**User prompt (verbatim):** "create tasks in the ticket, then do them one by one, committing at appropriate intervals, keeping a detailed diary.."

**Assistant interpretation:** Turn the plan into sequential implementation work, checkpoint each step with focused commits, and keep the ticket diary current as the code evolves.

**Inferred user intent:** Make steady, reviewable progress on the new film-developer app rather than jumping straight into a large UI rewrite.

**Commit (code):** eee5fd63a12e9d679be0283a888af3a980d64cb3 — "Add starter film catalog pipeline"

### What I did
- Added a generator script in the ticket scripts folder:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/06/ESP-27-M5DIAL-FILM-DEVELOPER--m5dial-film-developer-timer-app-design-and-implementation-guide/scripts/generate_film_catalog_cpp.py`
- Chose generated C++ as the runtime catalog format instead of runtime JSON parsing.
- Implemented a starter-scope filter in the generator:
  - a curated film whitelist of `19` films
  - common B/W developers only
  - the explicit `C-41` row that actually exists in the source file
- Normalized:
  - temperature to tenths of Celsius
  - push/pull display labels
  - runtime time selection to seconds using best-available film format time
- Aggregated duplicate source rows by selector key and used the median time where multiple rows survived for the same recipe combination.
- Generated:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/generated_film_catalog.cpp`
- Added the runtime catalog module:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/film_catalog.h`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/film_catalog.cpp`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/generated_film_catalog.h`
- Wired the module into:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/CMakeLists.txt`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/app_main.cpp`
- Built the app.
- Flashed `/dev/ttyACM0`.
- Captured a boot log proving runtime initialization:

```text
I (...) m5dial_film_0073: film catalog ready: recipes=1548 films=19 developers=9
I (...) m5dial_film_0073: film developer timer scaffold started
```

### Why
- The UI cannot be designed responsibly until the runtime option space is real.
- Generated C++ avoids runtime JSON parsing on the dial and gives deterministic startup behavior.
- A small curated starter catalog is consistent with the user’s scope correction: not all films, but common B/W plus limited explicit color/C-41 support.

### What worked
- The generator script produced a compact runtime catalog with `1548` normalized recipe rows.
- The app built cleanly after adding the catalog module.
- Flash succeeded to `/dev/ttyACM0`.
- The boot log confirmed the catalog initializes on-device without disturbing the inherited board/UI startup path.
- The existing M5Dial board, LVGL port, and event-bus architecture remained intact.

### What didn't work
- A quick follow-up file read right after generation initially reported “No such file or directory” for the generated C++ file, even though the generator had succeeded. That turned out to be a path-checking hiccup in the follow-up command sequence, not an actual generation failure.
- A simple post-flash serial-read trick missed the early boot log window, so I switched to a short TTY-backed `idf.py monitor` session to capture the startup lines reliably.

### What I learned
- The scoped whitelist was still large enough to be useful but small enough to reason about: `19` films and `9` developers is a good first selector target.
- Aggregating by recipe key and taking the median time is a practical v1 compromise for user-contributed source data with duplicates.
- The runtime C++ catalog approach fits this firmware much better than pretending the raw JSON is an on-device database.

### What was tricky to build
- The tricky part was data scope, not syntax. The earlier “common developer” filter still left too many films, so I had to tighten it further with an explicit film whitelist that matched the user’s request for a smaller starter set. The sequence was:
  - sample the surviving films under the broad common-developer filter
  - identify a recognizable starter film list
  - confirm the explicit `C-41` coverage was extremely small
  - accept that limitation in v1 rather than silently broadening the data semantics
  - generate a deterministic C++ artifact from that narrower set
- The other tricky point was making the step meaningful without touching the UI yet. The runtime log line solved that by giving a hardware-verified sign of progress before the selector screen exists.

### What warrants a second pair of eyes
- The starter film whitelist is intentionally opinionated. Someone with stronger film-process preferences may want a different first set.
- The median-time aggregation rule is sensible but still a product choice, not an absolute truth.
- The generated catalog file is large enough that future growth should be watched carefully.

### What should be done in the future
- Build the selector model on top of the new catalog contract.
- Decide how the UI should expose developer/dilution fields when only one option exists.
- Revisit the starter whitelist only after the selector UX exists and can be judged on real hardware.

### Code review instructions
- Start with the generator:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/06/ESP-27-M5DIAL-FILM-DEVELOPER--m5dial-film-developer-timer-app-design-and-implementation-guide/scripts/generate_film_catalog_cpp.py`
- Then inspect the runtime catalog contract:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/film_catalog.h`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/film_catalog.cpp`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/generated_film_catalog.cpp`
- Then confirm the app integration is intentionally shallow for this step:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/app_main.cpp`

### Technical details

Generator result for this step:

```text
generated 1548 recipe rows
starter films: 19
developers: 9
```

Build and device validation commands:

```bash
python /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/06/ESP-27-M5DIAL-FILM-DEVELOPER--m5dial-film-developer-timer-app-design-and-implementation-guide/scripts/generate_film_catalog_cpp.py \
  /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/film_dev_times.json \
  /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/generated_film_catalog.cpp

cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer
source ../.envrc
idf.py build
idf.py -p /dev/ttyACM0 -b 115200 flash
timeout 8s idf.py -p /dev/ttyACM0 monitor
```
