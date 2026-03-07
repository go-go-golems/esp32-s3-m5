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
    - Path: 0073-m5dial-film-developer-timer/main/film_catalog.cpp
      Note: Diary evidence for catalog query helpers used by the selector layer
    - Path: 0073-m5dial-film-developer-timer/main/recipe_selector_model.cpp
      Note: Diary evidence for the first selector state machine implementation
    - Path: film_dev_times.json
      Note: Evidence for source-data size and schema constraints
    - Path: ttmp/2026/03/06/ESP-27-M5DIAL-FILM-DEVELOPER--m5dial-film-developer-timer-app-design-and-implementation-guide/scripts/analyze_film_dev_times.py
      Note: Diary-backed analysis script for dataset facts
    - Path: ttmp/2026/03/06/ESP-27-M5DIAL-FILM-DEVELOPER--m5dial-film-developer-timer-app-design-and-implementation-guide/scripts/generate_film_catalog_cpp.py
      Note: Diary evidence for the catalog generator
    - Path: ttmp/2026/03/06/ESP-27-M5DIAL-FILM-DEVELOPER--m5dial-film-developer-timer-app-design-and-implementation-guide/scripts/scope_subset_report.py
      Note: Diary-backed script for the narrowed v1 catalog scope
    - Path: ttmp/2026/03/06/ESP-27-M5DIAL-FILM-DEVELOPER--m5dial-film-developer-timer-app-design-and-implementation-guide/scripts/validate_catalog_samples.py
      Note: Diary evidence for raw-source spot checks against the curated runtime catalog
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

## Step 4: Add selector queries and the first recipe-selector state machine

With the generated starter catalog in place, the next missing piece was the first real domain-level interaction layer: code that can turn the flat recipe table into a sequence of user-selectable fields. The runtime catalog already knew how many rows existed. It did not yet know how to answer product questions such as "which developers exist for this film?" or "what temperatures exist for this film/developer/dilution combination?".

This step added those query helpers to `FilmCatalog`, then introduced a dedicated `RecipeSelectorModel` that walks the user through a deterministic chain of selector fields: film, developer, dilution, temperature, push/pull, and finally a review state. The important point is that this was built as a pure state machine first, before any new on-screen UI was written. That keeps the selection logic testable and keeps the eventual LVGL work thinner.

### Prompt Context

**User prompt (verbatim):** "create tasks in the ticket, then do them one by one, committing at appropriate intervals, keeping a detailed diary.."

**Assistant interpretation:** Treat the guide as an implementation backlog, advance the next logical task, validate it on hardware, and record each milestone cleanly before moving to the next one.

**Inferred user intent:** Make steady, reviewable progress instead of bundling several architectural changes into one large, hard-to-audit commit.

**Commit (code):** 453cada — "Add film recipe selector model"

### What I did
- Expanded `FilmCatalog` so it can answer selector-oriented queries:
  - `films()`
  - `developers_for(...)`
  - `dilutions_for(...)`
  - `temperatures_for(...)`
  - `push_pull_for(...)`
  - `resolve(...)`
- Added:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/recipe_selector_model.h`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/recipe_selector_model.cpp`
- Registered the new selector module in:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/CMakeLists.txt`
- Wired the selector model into boot initialization in:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/app_main.cpp`
- Built and flashed the app:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer
source ../.envrc
idf.py build
idf.py -p /dev/ttyACM0 -b 115200 flash monitor
```

### Why
- The UI should not directly scan the raw generated recipe array every time the user turns the encoder.
- A dedicated selector state machine is the clean seam between domain data and LVGL rendering.
- Building the selector logic before the selector UI keeps future UI work focused on presentation and event wiring rather than catalog search details.

### What worked
- The generated catalog was already sorted in a way that made one-pass unique extraction practical for v1.
- The selector model initialized successfully on hardware and resolved an actual starter recipe at boot.
- The boot log confirmed end-to-end wiring from generated data through catalog queries to a resolved selection:

```text
I (...) m5dial_film_0073: film catalog ready: recipes=1548 films=19 developers=9
I (...) m5dial_film_0073: selector ready: film=Arista Premium 100 developer=D-76 dilution=1+1 temp=20.0C push=pull-1.7 time=465s
```

### What didn't work
- There was no code-level failure in this step.
- The one awkward detail in the first boot log was the generated push/pull label `pull-1.7`, which is valid source data but a reminder that later UI polish should normalize odd fractional labels more gracefully if they look confusing on screen.

### What I learned
- The dataset is structured enough that a thin query API plus linear scans is acceptable for the starter catalog size.
- The right next step is UI replacement, not more catalog abstraction. We now have enough domain logic to drive a real selector screen.
- Separating `RecipeSelectorModel` from the controller/UI code was the right call. It already made the boot validation straightforward.

### What was tricky to build
- The subtle part was deciding how much intelligence belongs in the catalog versus the selector model. The stable split that emerged was:
  - `FilmCatalog`: answer filtered lookup questions and resolve a concrete recipe
  - `RecipeSelectorModel`: manage current field, current indices, wrap-around behavior, and downstream option rebuilding
- Another small but important detail was avoiding unnecessary copies of raw string data. Using `std::string_view` backed by the generated static table keeps the selector cheap.

### What warrants a second pair of eyes
- The current query helpers rely on the generated catalog ordering to make `append_unique(...)` behave sensibly. That assumption is fine because the generator currently sorts the output, but it should stay documented if the generator ever changes.
- The choice to use linear scans is pragmatic for `1548` rows. If the starter catalog grows substantially, the next iteration may want precomputed indexes.

### What should be done in the future
- Replace the inherited timer UI with a selector-focused screen that renders the current field and current option clearly on the round display.
- Feed encoder and button events into `RecipeSelectorModel` through the existing queue-based controller boundary.
- Add a review screen that uses `resolved_recipe()` to present the chosen process before the countdown starts.

### Code review instructions
- Review the new domain-query seam first:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/film_catalog.h`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/film_catalog.cpp`
- Then review the selector state machine:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/recipe_selector_model.h`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/recipe_selector_model.cpp`
- Finally verify the boot wiring:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/app_main.cpp`

### Technical details

The selector state machine currently exposes these stages:

```text
Film -> Developer -> Dilution -> Temperature -> Push/Pull -> Review
```

The runtime behavior is:

```text
adjust(delta): mutate the active field index with wrap-around
confirm(): advance to the next meaningful field, skipping fields that only have one option
back(): retreat to the previous meaningful field
snapshot(): expose the current field, option count, current selection, and resolved recipe
```

That gives the next UI step a stable API without requiring LVGL code to know how recipes are filtered internally.

## Step 5: Replace the inherited timer screen with a film-selector UI

Once the selector model existed, the next logical step was to stop showing the inherited countdown timer UI. The app already had enough data and state to present a real film-selection flow, so keeping the old screen would only make later integration messier. The goal of this step was not yet to build the full process timer. It was to make the device show a selector-first interface and route the existing encoder, button, and swipe events into that selector flow.

This step introduced a new `FilmSelectorScreen` and `FilmSelectorController` rather than overloading the old timer classes. That preserves a cleaner path for the later review/countdown stages and avoids welding film-domain behavior into the generic timer leftovers from `0072`.

### Prompt Context

**User prompt (verbatim):** "continue"

**Assistant interpretation:** Move to the next ticket task without stopping at the selector-model backend, and keep committing/dairying each milestone.

**Inferred user intent:** Replace the placeholder UI with the first real film-selection experience on device.

**Commit (code):** dd86850 — "Add M5Dial film selector UI"

### What I did
- Added:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/film_selector_screen.h`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/film_selector_screen.cpp`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/film_selector_controller.h`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/film_selector_controller.cpp`
- Updated:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/CMakeLists.txt`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/app_main.cpp`
- Replaced the inherited `TimerScreen`/`TimerController` path in the UI task with:
  - `FilmSelectorScreen`
  - `FilmSelectorController`
  - `RecipeSelectorModel`
- Kept the existing architecture boundaries intact:
  - board polling still happens in the IO task
  - events still cross the FreeRTOS queue
  - LVGL still lives only in the UI task
- Built and flashed the app to `/dev/ttyACM0`.

### Why
- The inherited timer screen was the last major placeholder that made the app feel like a clone instead of a film tool.
- A dedicated selector UI/controller pair is cleaner than mutating the timer classes into a second job they were never designed for.
- This step proves the event path from hardware input to domain selector state to LVGL rendering without mixing in countdown logic yet.

### What worked
- The new selector screen compiled cleanly after adjusting to the set of LVGL fonts actually enabled in this project.
- Flash and boot succeeded on the dial.
- The boot log now shows the app entering the new selector UI path:

```text
I (...) m5dial_film_0073: selector ready: film=Arista Premium 100 developer=D-76 dilution=1+1 temp=20.0C push=pull-1.7 time=465s
I (...) m5dial_film_0073: film developer selector started
```

- The event wiring is now aligned with the film-selection flow:
  - encoder changes the current selector field value
  - short press advances/accepts
  - long press goes back
  - swipe changes visual theme

### What didn't work
- The first build failed because the new screen used `lv_font_montserrat_16` and `lv_font_montserrat_24`, which are not enabled in the current LVGL font configuration. Switching to already-enabled sizes fixed that immediately.
- I can confirm build, flash, and boot from the terminal, but I cannot independently confirm the visual quality of the on-screen layout without the user looking at the dial. That remains a separate validation step in the task list.

### What I learned
- A screen/controller pair specific to selection is the right seam. It let the old timer path drop out of the UI loop cleanly.
- The current ticket should treat "booted and flashed" and "human-validated on-screen behavior" as separate milestones. They are not the same thing.
- The remaining big feature work is now clearly the countdown/review side, not basic app identity.

### What was tricky to build
- The main trap was avoiding overreach. It would have been easy to start inventing the countdown/review state machine in the same patch. That would have mixed two milestones. The disciplined path was:
  - add a selector screen
  - add a selector controller
  - wire it into the existing task architecture
  - flash and confirm boot
  - leave countdown behavior for the next focused step
- The other small trap was LVGL font availability. The portable-looking API names do not mean every font size is compiled into this build.

### What warrants a second pair of eyes
- The review-stage layout is currently only the selector screen’s "review field", not yet a dedicated ready screen. That is acceptable for now but should be revisited when the countdown flow exists.
- The user-facing text for odd push/pull values such as `pull-1.7` is still too raw.

### What should be done in the future
- Verify the selector layout and interaction quality on the physical dial with human feedback.
- Split the current review field into an actual process-ready screen if that reads better on hardware.
- Build the film countdown model/controller next so the confirmed recipe can transition into a running process timer.

### Code review instructions
- Start with the new UI classes:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/film_selector_screen.cpp`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/film_selector_controller.cpp`
- Then inspect the integration point:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/app_main.cpp`
- Confirm that the old timer-model path is no longer driving the UI loop, even though the old files still exist in the repo for later cleanup.

### Technical details

The selector screen currently renders:

```text
title / theme
active field name
current selected value
supporting detail line
summary line
interaction hint
```

The controller mapping is:

```text
encoder delta -> RecipeSelectorModel::adjust(...)
short press -> RecipeSelectorModel::confirm()
long press -> RecipeSelectorModel::back()
swipe -> FilmSelectorScreen::cycle_theme(...)
```

That is enough to prove the selector-first product shape before any film countdown state has been introduced.

## Step 6: Add the actual film process timer flow and close out the v1 app shape

After the selector UI landed, the last missing product layer was obvious: the app could choose a recipe, but it still could not become that recipe. This step added the process-timer side of the application so a confirmed selection transitions into a ready/running/done countdown flow with film-specific metadata on screen instead of stopping at selection.

The implementation deliberately kept the selector state machine and the process timer separate. The selector still owns catalog browsing. A new timer model owns the chosen recipe and countdown state. `app_main.cpp` now provides the thin top-level mode switch between those two subsystems.

### Prompt Context

**User prompt (verbatim):** "continue until the full app is built, i'm gonna go swimming in the meantime."

**Assistant interpretation:** Finish the remaining implementation work autonomously, including the countdown side, validation, and diary updates.

**Inferred user intent:** Treat `0073` as a product implementation task now, not a partial experiment.

**Commit (code):** f2000a2 — "Add film process timer flow"

### What I did
- Added the process-timer domain model:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/film_timer_model.h`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/film_timer_model.cpp`
- Added the process controller:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/film_timer_controller.h`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/film_timer_controller.cpp`
- Added the process screen:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/film_process_screen.h`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/film_process_screen.cpp`
- Extended the selector controller so a short press on the review stage enters process mode instead of doing nothing.
- Reworked:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/app_main.cpp`
  so the UI task now switches between:
  - selector mode
  - process mode
- Removed the old generic timer sources from the active component source list in:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/CMakeLists.txt`
- Updated the app README to match the actual control flow.
- Added a raw-source validation helper:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/06/ESP-27-M5DIAL-FILM-DEVELOPER--m5dial-film-developer-timer-app-design-and-implementation-guide/scripts/validate_catalog_samples.py`
- Built and flashed the full app to `/dev/ttyACM0`.

### Why
- The app was not “full” until a chosen recipe could drive a countdown.
- Keeping a dedicated timer model separate from the selector model keeps the system easier to reason about:
  - selector logic stays data-driven
  - timer logic stays time-driven
  - `app_main.cpp` only coordinates the mode switch
- The README had drifted behind reality and still described the inherited timer demo; that needed to be corrected before this step could reasonably be called complete.

### What worked
- The process-timer model compiled and integrated cleanly on the first build.
- Flash to `/dev/ttyACM0` succeeded.
- Boot remained healthy after the new mode-switch logic:

```text
I (...) m5dial_film_0073: film catalog ready: recipes=1548 films=19 developers=9
I (...) m5dial_film_0073: selector ready: film=Arista Premium 100 developer=D-76 dilution=1+1 temp=20.0C push=pull-1.7 time=465s
I (...) m5dial_film_0073: film developer selector started
```

- The new process flow now has a clear behavior model:
  - short press on selector review enters process mode
  - short press in process mode starts, pauses, resumes, or reruns
  - long press while running or paused resets the current recipe to ready
  - long press while ready or done returns to selector mode

- The source-data spot-check script validated that the default boot recipe is not an accidental artifact. It comes from four raw source rows with times `[510, 420, 510, 420]`, which correctly collapse to a `465s` median in the curated runtime catalog.

### What didn't work
- There was no code-level integration failure in the process-timer step itself.
- There is still one known product-polish issue: some raw push/pull labels from the source dataset remain awkward on screen, such as fractional `pull-*` variants.
- I can prove build, flash, boot, and the raw-data normalization path from the terminal, but I still cannot independently confirm the exact on-screen aesthetics without the human looking at the dial.

### What I learned
- The clean split is now stable:
  - `FilmCatalog` answers lookup questions
  - `RecipeSelectorModel` drives staged selection
  - `FilmTimerModel` owns the chosen recipe countdown
  - `app_main.cpp` coordinates which screen/controller pair is currently active
- That split is good enough for an intern to extend without needing to restructure the app first.
- The data-validation script was worth adding. It turns a potentially hand-wavy “trust the generator” step into something reviewable.

### What was tricky to build
- The only meaningful architecture decision here was resisting the temptation to let the selector model also own countdown state. That would have produced a confused object with two jobs. The better sequence was:
  - leave selector state in `RecipeSelectorModel`
  - build a new timer model for the chosen recipe
  - switch modes at the UI-task level
- The other subtle point was screen ownership. Both selector and process screens now defer `lv_scr_load(...)` until `apply(...)` so the active mode, not initialization order, decides what is visible.

### What warrants a second pair of eyes
- The exact “hold to reset” versus “hold to return to selector” semantics in process mode are pragmatic but still product choices.
- A later cleanup pass should remove the now-unused legacy `timer_*` and `ui_timer_screen*` files from the directory, not just from the active build list.
- The raw push/pull labels should be reviewed by someone who cares about photo-lab wording quality.

### What should be done in the future
- Normalize or remap awkward fractional push/pull labels into friendlier UI copy.
- Capture a photo of the selector screen and the process screen for the ticket/README.
- Decide whether sound or stage chains belong in v2.

### Code review instructions
- Start with the top-level flow switch:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/app_main.cpp`
- Then review the new timer-domain files:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/film_timer_model.cpp`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/film_timer_controller.cpp`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/film_process_screen.cpp`
- Finally inspect the data-validation helper:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/06/ESP-27-M5DIAL-FILM-DEVELOPER--m5dial-film-developer-timer-app-design-and-implementation-guide/scripts/validate_catalog_samples.py`

### Technical details

The v1 mode structure is now:

```text
Selecting -> Process Ready -> Running -> Paused -> Complete
      ^             |
      |-------------|
```

With current input behavior:

```text
selector review + short press -> load recipe into FilmTimerModel and enter process mode
process ready + short press -> start timer
process running + short press -> pause
process paused + short press -> resume
process complete + short press -> rerun same recipe
process running/paused + long press -> reset to ready
process ready/complete + long press -> return to selector
```

That is enough to call the app functional as a single-stage film developer timer.
