---
Title: Investigation diary
Ticket: ESP-47-CARDPUTER-ADV-ANIMATION-UI
Status: active
Topics:
    - cardputer-adv
    - cardputer
    - ui
    - animation
    - keyboard
    - display
    - m5gfx
    - esp-idf
    - firmware
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.cpp
      Note: Key evidence for ADV keyboard behavior recorded in the diary
    - Path: ttmp/2026/03/29/ESP-47-CARDPUTER-ADV-ANIMATION-UI--cardputer-adv-modern-dynamic-animation-ui-with-keyboard-controlled-scroll-motion/imports/retro_macos_line_minimap.html
      Note: Imported donor artifact reviewed during the analysis
ExternalSources: []
Summary: Chronological diary for creating the ESP-47 ticket, importing the donor prototype, analyzing the existing firmwares, and writing the intern-facing implementation guide.
LastUpdated: 2026-03-29T17:05:00-04:00
WhatFor: Use this diary to review the exact investigation path, commands, and rationale used to produce the design deliverable.
WhenToUse: Read this when continuing the ticket later, reviewing the evidence chain, or checking how the design conclusions were formed.
---


# Investigation diary

## Goal

Capture the exact steps used to create the new docmgr ticket, import the donor HTML prototype, analyze the existing Cardputer and Cardputer ADV firmwares, and write a detailed implementation guide for a new animation-oriented UI.

## Step 1: Create the ticket, import the donor, and assemble the implementation guide

This step created the new `ESP-47-CARDPUTER-ADV-ANIMATION-UI` ticket workspace and imported the browser donor prototype into `imports/`. The main engineering goal was not only to collect files, but to establish a reliable evidence base for how a new Cardputer ADV UI should be structured so that an intern can implement it without rediscovering input, animation, and rendering architecture from scratch.

The analysis focused on existing local firmware rather than speculative design. `0066` was treated as the ADV-specific reference for input and UI-task ownership, `0022` as the reference for sprite composition and navigation, and `0030` as the reference for scroll-state handling and event-driven UI updates. The resulting design document explains how to combine those patterns into a new standalone firmware instead of copying any one existing project wholesale.

### Prompt Context

**User prompt (verbatim):**

```text
Create a new docmgr ticket where I want to build a modern dynamic animation UI on the cardputer ADV. Import the file ~/Downloads/retro_macos_line_minimap.html to the ticket imports/ folder. 

Keep a diary. 

Analyze the other firmwares on how to build a UI on the cardputer ADV with keyboard control to start an animation to move the scroll position around. 

Create a detailed analysis / design / implementation guide that is very detailed for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file
  references.
  It should be very clear and detailed. Store in the ticket and the nupload to remarkable.
```

**Assistant interpretation:** Create a new docmgr ticket, import the provided HTML prototype into the ticket, analyze the existing Cardputer/Cardputer ADV firmware patterns relevant to keyboard-driven animated scroll UI, write a highly detailed intern-facing design guide, maintain a diary, and deliver the documentation bundle to reMarkable.

**Inferred user intent:** Build a new Cardputer ADV UI from a strong architectural base instead of improvising, and preserve enough documentation that a new contributor can implement the firmware confidently.

**Commit (code):** N/A

### What I did

- Created the ticket workspace:
  - `docmgr ticket create-ticket --ticket ESP-47-CARDPUTER-ADV-ANIMATION-UI --title "Cardputer ADV modern dynamic animation UI with keyboard-controlled scroll motion" --topics cardputer-adv,cardputer,ui,animation,keyboard,display,m5gfx,esp-idf,firmware`
- Added the main design doc and diary doc:
  - `docmgr doc add --ticket ESP-47-CARDPUTER-ADV-ANIMATION-UI --doc-type design-doc --title "Cardputer ADV dynamic animation UI analysis, design, and implementation guide"`
  - `docmgr doc add --ticket ESP-47-CARDPUTER-ADV-ANIMATION-UI --doc-type reference --title "Investigation diary"`
- Imported the donor HTML into the ticket:
  - `mkdir -p ttmp/2026/03/29/ESP-47-CARDPUTER-ADV-ANIMATION-UI--cardputer-adv-modern-dynamic-animation-ui-with-keyboard-controlled-scroll-motion/imports`
  - `cp /home/manuel/Downloads/retro_macos_line_minimap.html ttmp/2026/03/29/ESP-47-CARDPUTER-ADV-ANIMATION-UI--cardputer-adv-modern-dynamic-animation-ui-with-keyboard-controlled-scroll-motion/imports/`
- Inspected the docmgr and diary skills plus ticket-research workflow skill to follow the expected ticket structure and delivery path.
- Gathered evidence from:
  - `0066-cardputer-adv-ledchain-gfx-sim/main/app_main.cpp`
  - `0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp`
  - `0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.h`
  - `0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.cpp`
  - `0066-cardputer-adv-ledchain-gfx-sim/main/ui_overlay.h`
  - `0066-cardputer-adv-ledchain-gfx-sim/main/ui_overlay.cpp`
  - `0066-cardputer-adv-ledchain-gfx-sim/main/sim_engine.h`
  - `0066-cardputer-adv-ledchain-gfx-sim/main/sim_engine.cpp`
  - `0022-cardputer-m5gfx-demo-suite/main/app_main.cpp`
  - `0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp`
  - `0022-cardputer-m5gfx-demo-suite/main/ui_list_view.h`
  - `0022-cardputer-m5gfx-demo-suite/main/ui_list_view.cpp`
  - `0022-cardputer-m5gfx-demo-suite/main/ui_console.cpp`
  - `0030-cardputer-console-eventbus/main/app_main.cpp`
  - `imports/retro_macos_line_minimap.html`
- Replaced the generated placeholder docs with:
  - a detailed ticket index
  - a full design/analysis/implementation guide
  - this diary entry

### Why

- The request was specifically for a new ticket and a very detailed guide, so the documentation had to be first-class deliverable work rather than a quick note dump.
- The Cardputer ADV part of the request makes keyboard backend handling important, which is why `0066` became the primary code reference.
- The requested "animation to move the scroll position around" is conceptually present in the donor HTML as `scrollPos` easing toward `scrollTarget`, so the guide needed to show how that idea maps into ESP-IDF and `M5Canvas`.

### What worked

- `docmgr` was already initialized in this workspace, so ticket creation and document creation were straightforward.
- The donor HTML existed at `/home/manuel/Downloads/retro_macos_line_minimap.html` and was imported without issue.
- The repository already contains enough local prior art to write an evidence-backed guide:
  - `0066` for ADV input and modal UI
  - `0022` for scene composition and navigation
  - `0030` for scroll-state handling and single-task display ownership
- The donor HTML is small enough to analyze directly and clearly exposes the important porting concept: current scroll position plus target scroll position with easing.

### What didn't work

- No functional blocker occurred during the investigation.
- One generated task scaffold line was irrelevant:
  - `- [ ] Add tasks here`
  - This was removed manually while cleaning the ticket task list.

### What I learned

- The most reusable Cardputer ADV-specific asset in this repo is not a full app, but the `ui_kb` layer from `0066`. It already normalizes ADV versus non-ADV keyboard backends and emits semantic edge events that are ideal for animation triggers.
- The donor HTML is useful as a motion-design reference, not as a software architecture template.
- The existing firmwares consistently reward one principle: keep display ownership in one task and let other components feed that task with already-decoded state or events.

### What was tricky to build

- The hardest part was not understanding any single firmware, but deciding which existing project should be treated as the architectural base. `0066`, `0022`, and `0030` each solve different pieces of the problem, and the wrong conclusion would have been to simply pick one as-is.
- The underlying cause of that ambiguity is that the requested feature crosses three concerns at once:
  - ADV keyboard hardware compatibility
  - animated visual rendering
  - scroll/viewport behavior
- I resolved that by separating the evidence into layers:
  - hardware/input layer from `0066`
  - render-loop and sprite composition from `0022`
  - scrollback/state ownership from `0030`

### What warrants a second pair of eyes

- Whether the first production version should remain one-axis only or expose two-axis camera motion.
- Whether the new firmware should be scaffolded from `0066` with aggressive pruning, or created as a smaller fresh project that only copies the relevant input/display modules.
- Whether the final visual language should preserve the donor's retro monochrome feel or only preserve its minimap/scroll mechanics.

### What should be done in the future

- Implement the new firmware described in the design doc.
- Add a small validation playbook once flashing and on-device testing begin.
- Revisit whether a synthetic "virtual document" is the right first content model, or whether the UI should immediately target a real application domain.

### Code review instructions

- Start with the design doc:
  - `design-doc/01-cardputer-adv-dynamic-animation-ui-analysis-design-and-implementation-guide.md`
- Then inspect the local evidence in this order:
  - `0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.cpp`
  - `0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp`
  - `0022-cardputer-m5gfx-demo-suite/main/app_main.cpp`
  - `0030-cardputer-console-eventbus/main/app_main.cpp`
  - `imports/retro_macos_line_minimap.html`
- Validate the bookkeeping with:
  - `docmgr doctor --ticket ESP-47-CARDPUTER-ADV-ANIMATION-UI --stale-after 30`

### Technical details

- Key orientation commands used during this step:
  - `docmgr status --summary-only`
  - `docmgr vocab list`
  - `rg -n "CardputerADV|ADV|animation|scroll|keyboard|key|M5Cardputer|M5GFX|draw|viewport|offset" 0022-cardputer-m5gfx-demo-suite 0066-cardputer-adv-ledchain-gfx-sim 0015-cardputer-serial-terminal 0030-cardputer-console-eventbus -S`
  - `nl -ba 0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.cpp | sed -n '1,420p'`
  - `nl -ba 0030-cardputer-console-eventbus/main/app_main.cpp | sed -n '260,560p'`
  - `nl -ba ttmp/2026/03/29/ESP-47-CARDPUTER-ADV-ANIMATION-UI--cardputer-adv-modern-dynamic-animation-ui-with-keyboard-controlled-scroll-motion/imports/retro_macos_line_minimap.html | sed -n '300,540p'`

## Related

- `../design-doc/01-cardputer-adv-dynamic-animation-ui-analysis-design-and-implementation-guide.md`
- `../index.md`
- `../imports/retro_macos_line_minimap.html`

## Step 2: Add a task plan, scaffold the new firmware, and get the first build green

This step moved the ticket from analysis into implementation. The goal was to turn the design guidance into a concrete, buildable Cardputer ADV firmware while keeping the work split into reviewable milestones. I started by expanding the ticket task list into a real execution plan, then created a fresh `0083-cardputer-adv-animation-ui` project that combines the ADV bring-up style from `0038`, the semantic keyboard layer from `0066`, and a new animation-focused model/render loop tailored to the donor minimap idea.

The implementation deliberately avoided reusing a full existing firmware wholesale. `0038-cardputer-adv-serial-terminal` is strong on ADV board bring-up and tmux flashing workflow, but it is not a scroll-animation app. `0066-cardputer-adv-ledchain-gfx-sim` already solves the hard input-normalization problem, so I reused that pattern for semantic keyboard events. The new code then adds a dedicated `UiState`, a target-based easing model, and a renderer that keeps the minimap, viewport bars, and scrollbar indicator derived from the same horizontal scroll position.

### Prompt Context

**User prompt (verbatim):**

```text
alright, add a detailed list of tasks, and then work on them one by one, committing at appropriate intervals, keeping a detailed diary. Use tmux to idf.py flash / monitor.
```

**Assistant interpretation:** Expand the ticket into an explicit task list, start implementing the new firmware immediately, commit in clean milestones, maintain the diary as implementation progresses, and use a tmux-based flash/monitor workflow for on-device work.

**Inferred user intent:** Replace the design-only deliverable with a real implementation workflow that is easy to audit later and safe to continue on hardware.

**Commit (code):** `44798eb` (`Add initial Cardputer ADV animation UI firmware scaffold`)

### What I did

- Surveyed the existing ADV-oriented firmware projects to choose the correct implementation base:
  - `0038-cardputer-adv-serial-terminal`
  - `0047-cardputer-adv-lvgl-chain-encoder-list`
  - `0066-cardputer-adv-ledchain-gfx-sim`
- Confirmed that the existing worktree contained unrelated changes and avoided them:
  - `ttmp/2026/03/23/ESP-37-EREADER-EPD-CRASH--papers3-e-reader-crashes-on-epd-fillscreen-null-framebuffer-pointer/...`
  - `esper/`
- Replaced the placeholder ticket checklist with a detailed execution plan in:
  - `ttmp/2026/03/29/ESP-47-CARDPUTER-ADV-ANIMATION-UI--cardputer-adv-modern-dynamic-animation-ui-with-keyboard-controlled-scroll-motion/tasks.md`
- Created the new firmware project:
  - `0083-cardputer-adv-animation-ui/CMakeLists.txt`
  - `0083-cardputer-adv-animation-ui/main/CMakeLists.txt`
  - `0083-cardputer-adv-animation-ui/main/Kconfig.projbuild`
  - `0083-cardputer-adv-animation-ui/sdkconfig.defaults`
  - `0083-cardputer-adv-animation-ui/partitions.csv`
  - `0083-cardputer-adv-animation-ui/build.sh`
  - `0083-cardputer-adv-animation-ui/README.md`
  - `0083-cardputer-adv-animation-ui/main/app_main.cpp`
  - `0083-cardputer-adv-animation-ui/main/ui_kb.h`
  - `0083-cardputer-adv-animation-ui/main/ui_kb.cpp`
  - `0083-cardputer-adv-animation-ui/main/ui_model.h`
  - `0083-cardputer-adv-animation-ui/main/ui_model.cpp`
  - `0083-cardputer-adv-animation-ui/main/ui_render.h`
  - `0083-cardputer-adv-animation-ui/main/ui_render.cpp`
- Wired the project to the ADV vendor components:
  - `../../M5Cardputer-UserDemo-ADV/components/M5GFX`
  - `../../M5Cardputer-UserDemo-ADV/components/M5Unified`
  - local `../components/cardputer_kb`
- Added a tmux-oriented helper script modeled after the ADV projects so flashing and monitoring can stay single-owner on the serial device.
- Implemented the first app behavior:
  - semantic keyboard events for left/right/up/down/back/tab/enter/delete/space/text
  - target-based scroll animation with easing and snap threshold
  - modifier-aware movement policy for normal, Fn, Alt, and Ctrl
  - autoplay/demo motion
  - full-screen canvas rendering of header chrome, minimap bars, viewport bars, scrollbar thumb, and help overlay
- Ran the first build:
  - `./build.sh build`
- Fixed the only build error:
  - `ui_render.cpp` used `char label[8]` with `snprintf("%02d", i + 1)`, which tripped `-Werror=format-truncation`
  - increased the buffer to `char label[16]`
- Rebuilt successfully:
  - `./build.sh build`
- Committed the first firmware milestone:
  - `git commit -m "Add initial Cardputer ADV animation UI firmware scaffold"`

### Why

- The user explicitly asked for work to proceed task by task with commits, so the first implementation needed to land as a self-contained scaffold rather than a half-built experimental tree.
- Starting from a new `0083` project keeps the new animation UI easy to review and avoids contaminating the previously analyzed example projects.
- Using the semantic keyboard approach from `0066` reduces hardware-risk early, because the ADV keyboard backend distinction is already handled in one place.
- Keeping a dedicated `build.sh` with tmux support aligns with the repo’s existing ADV workflows and the serial single-owner requirement in `AGENTS.md`.

### What worked

- The vendor ADV components were consumable from a new local project once the component paths were corrected.
- The semantic keyboard layer adapted cleanly into the new app and already logs the detected backend.
- The target/easing animation model was small enough to implement directly without needing LVGL or a heavier framework.
- The full project built successfully after one renderer warning fix.

### What didn't work

- The first build failed under `-Werror` because of a potential `snprintf` truncation warning in `ui_render.cpp`.
- The original top-level vendor component path assumption was wrong at first. The project had to point two directories up to `../../M5Cardputer-UserDemo-ADV/...`, not one directory up.

### What I learned

- The most practical architecture for this app is still single-task display ownership in `app_main.cpp`, with the keyboard scanner isolated in its own FreeRTOS task and a queue boundary between input and rendering.
- The donor prototype maps naturally onto a single scalar scroll state. Once that scalar is correct, the minimap, scrollbar thumb, and viewport bars all become simple derived views.
- Strict compiler warnings are useful here because they force the renderer to be safe before device testing even begins.

### What was tricky to build

- The tricky part was choosing what to inherit from prior art without dragging in unrelated UI or protocol concerns.
- The underlying reason is that the ADV-specific examples in this repo solve different categories of problem:
  - board bring-up
  - keyboard backend compatibility
  - rendering structure
  - application state
- The solution was to compose those layers into a new project rather than fork one existing firmware directly.

### What warrants a second pair of eyes

- Whether the horizontal-only motion is enough for the first device demo, or whether the viewport should expose vertical movement too.
- Whether the modifier mapping for Fn/Alt/Ctrl movement feels good on the physical keyboard once tested on hardware.
- Whether the current synthetic bars should later be replaced by richer content primitives from the donor HTML concept.

### What should be done in the future

- Flash the new firmware through tmux and confirm the ADV board boots cleanly into the app.
- Validate keyboard navigation, autoplay toggling, and help overlay behavior on real hardware.
- Adjust rendering sizes and step constants based on real display feel rather than simulator-style estimates.
- Update the design doc if the hardware-tested implementation diverges from the original plan.

### Code review instructions

- Start with the new firmware entry point:
  - `0083-cardputer-adv-animation-ui/main/app_main.cpp`
- Then review the implementation layers in this order:
  - `0083-cardputer-adv-animation-ui/main/ui_kb.cpp`
  - `0083-cardputer-adv-animation-ui/main/ui_model.cpp`
  - `0083-cardputer-adv-animation-ui/main/ui_render.cpp`
  - `0083-cardputer-adv-animation-ui/build.sh`
- Confirm the current milestone compiles with:
  - `cd 0083-cardputer-adv-animation-ui && ./build.sh build`

### Technical details

- Orientation commands used at the start of this step:
  - `git status --short`
  - `ls -1`
  - `sed -n '1,240p' 0038-cardputer-adv-serial-terminal/build.sh`
  - `sed -n '1,260p' 0047-cardputer-adv-lvgl-chain-encoder-list/build.sh`
  - `sed -n '1,260p' 0038-cardputer-adv-serial-terminal/main/hello_world_main.cpp`
  - `sed -n '1,260p' 0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.cpp`
- Build and validation commands used during the scaffold milestone:
  - `cd 0083-cardputer-adv-animation-ui && ./build.sh build`
  - `git add 0083-cardputer-adv-animation-ui`
  - `git commit -m "Add initial Cardputer ADV animation UI firmware scaffold"`
