---
Title: Investigation diary
Ticket: 0102-JS-API-REPL-OS-SIM
Status: active
Topics:
    - esp32-p4
    - quickjs
    - javascript
    - visual-repl
    - picocalc
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: .gitmodules
      Note: Tracks upstream QuickJS as an explicit submodule dependency for the smoke-test fallback
    - Path: 0102-esp32-p4-visual-quickjs-repl/js/README.md
      Note: |-
        Prompt-required first-read file that shaped the diary conclusions
        Documents submodule initialization and corrected qjs -I invocation
    - Path: 0102-esp32-p4-visual-quickjs-repl/js/examples-native/layout-native.js
      Note: Native layout binding example (commit 267e113)
    - Path: 0102-esp32-p4-visual-quickjs-repl/js/lib/00-core.js
      Note: Phase 1 portable helpers and deterministic RNG (commit c4ffe35)
    - Path: 0102-esp32-p4-visual-quickjs-repl/js/lib/10-screen.js
      Note: Phase 1 fixed-cell screen buffer (commit c4ffe35)
    - Path: 0102-esp32-p4-visual-quickjs-repl/js/lib/20-os-sim.js
      Note: Phase 2 deterministic OS simulator (commit be1285d)
    - Path: 0102-esp32-p4-visual-quickjs-repl/js/lib/30-ui-runtime.js
      Note: Phase 3 fluent TUI runtime (commit 713f19e)
    - Path: 0102-esp32-p4-visual-quickjs-repl/js/tests/bundle-example.sh
      Note: Bundle/paste workflow script (commit c91c593)
    - Path: 0102-esp32-p4-visual-quickjs-repl/js/tests/run-api-tests.sh
      Note: Portable API regression runner
    - Path: 0102-esp32-p4-visual-quickjs-repl/js/tests/run-examples.sh
      Note: Example gallery runner (commit bf8faab)
    - Path: 0102-esp32-p4-visual-quickjs-repl/js/tests/run-interactive.sh
      Note: Launcher for qjs --std interactive emulator
    - Path: 0102-esp32-p4-visual-quickjs-repl/js/tests/run-native-host.sh
      Note: Wrapper for building/running native host examples
    - Path: 0102-esp32-p4-visual-quickjs-repl/js/tests/run-native-smoke.sh
      Note: Automated native host smoke for clean exit and key/layout behavior (commits 6a6d7b7
    - Path: 0102-esp32-p4-visual-quickjs-repl/js/tests/run-smoke.sh
      Note: |-
        Smoke test command attempted and failure recorded
        Fixed to preload host-shim.js with qjs -I before running smoke.js
    - Path: 0102-esp32-p4-visual-quickjs-repl/js/tools/interactive-host.js
      Note: Host-only interactive QuickJS emulator (commit 4e0979f)
    - Path: 0102-esp32-p4-visual-quickjs-repl/js/tools/native-host/README.md
      Note: Documents firmware-portable API boundary vs host glue
    - Path: 0102-esp32-p4-visual-quickjs-repl/js/tools/native-host/src/main.cpp
      Note: Host-only raw terminal event loop for native host
    - Path: 0102-esp32-p4-visual-quickjs-repl/js/tools/native-host/src/pico_native_api.cpp
      Note: C++ QuickJS native picoOS API prototype (commit 48b6e0a)
ExternalSources: []
Summary: Chronological investigation diary for the 0102 JS API REPL and QuickJS OS simulator design ticket.
LastUpdated: 2026-06-24T23:35:00Z
WhatFor: Use this to resume or review the design work for the portable picoOS JavaScript runtime and simulator.
WhenToUse: Read before continuing implementation or ticket documentation for 0102-JS-API-REPL-OS-SIM.
---







# Diary

## Goal

This diary captures the setup, evidence gathering, design decisions, and delivery steps for the ticket that will build a portable JavaScript API REPL and simulated picoOS runtime for the ESP32-P4 PicoCalc visual QuickJS REPL.

## Step 1: Create the ticket and design package

I created a docmgr ticket for the JS-side picoOS runtime and simulator work, then mapped the existing JS playbook, smoke-test harness, host shim, and firmware integration points. The goal of this step was to produce an implementation guide a new intern can follow without needing to reverse-engineer the active firmware branch first.

The main finding is that the JS work can move independently, but only if it remains strictly inside the portable contract: `print`, `millis`, and `gc` are the only globals examples may assume. The current firmware already installs those globals and has a visual input editor, but submit still records a placeholder status line instead of evaluating typed JavaScript.

### Prompt Context

**User prompt (verbatim):** "You are working in this git worktree:                                                                                                                      
                                                                                                                                                              
   /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5-js                                                                                  
                                                                                                                                                              
   This worktree is intentionally separate from the active firmware worktree. Stay on branch:                                                                 
                                                                                                                                                              
   feature/0102-js-scripts                                                                                                                                    
                                                                                                                                                              
   Your task is to develop portable JavaScript scripts for the ESP32-P4 PicoCalc visual QuickJS REPL. First read:                                             
                                                                                                                                                              
   0102-esp32-p4-visual-quickjs-repl/js/README.md                                                                                                             
                                                                                                                                                              
   Only edit files under:                                                                                                                                     
                                                                                                                                                              
   0102-esp32-p4-visual-quickjs-repl/js/**                                                                                                                    
                                                                                                                                                              
   Do not edit firmware files under main/, components/qjs_service, components/visual_repl, or components/picocalc_* unless explicitly asked.                  
                                                                                                                                                              
   Runtime contract:                                                                                                                                          
   - Available globals: print(...args), millis(), gc()                                                                                                        
   - Avoid: console.log, require, import, fs/path/process/Buffer, QuickJS std/os, browser APIs, network APIs.                                                 
                                                                                                                                                              
   Start by running or making runnable the smoke test:                                                                                                        
                                                                                                                                                              
   0102-esp32-p4-visual-quickjs-repl/js/tests/run-smoke.sh                                                                                                    
                                                                                                                                                              
   If qjs is missing, build the vendored QuickJS CLI:                                                                                                         
                                                                                                                                                              
   make -C 0100-esp32-p4-quickjs-wasm/wasm-src/quickjs qjs                                                                                                    
                                                                                                                                                              
   Then run:                                                                                                                                                  
                                                                                                                                                              
   0102-esp32-p4-visual-quickjs-repl/js/tests/run-smoke.sh                                                                                                    
                                                                                                                                                              
   Add small portable examples and self-tests that can later be pasted into or embedded in the device visual REPL. Keep output suitable for a 40-column       
 display. Commit focused JS-only changes with messages like:                                                                                                  
                                                                                                                                                              
   0102 js: add portable QuickJS examples    

I want you to build the JS API REPL and simulate the OS with quickjs.

[User supplied a long React picoOS devkit prototype and API gallery.]

---

create a new docmgr ticket to build it. Create  a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and the nupload to remarkable."

**Assistant interpretation:** Create a docmgr ticket for implementing the portable QuickJS picoOS API/runtime simulator, investigate current JS and firmware boundaries, write a detailed design/implementation guide for an intern, keep a diary, validate, and upload the resulting document bundle to reMarkable.

**Inferred user intent:** The user wants a structured handoff package before implementation starts, so a future contributor can build the JS-only runtime and examples safely without touching active firmware work.

**Commit (code):** N/A — documentation-only ticket setup and analysis; no source code changes were made.

### What I did

- Confirmed the worktree branch with `git status --short --branch` and `git branch --show-current`; it is on `feature/0102-js-scripts`.
- Read `0102-esp32-p4-visual-quickjs-repl/js/README.md` to capture the JS-side contract and edit boundaries.
- Read the existing JS files:
  - `js/host-shim.js`
  - `js/examples/smoke.js`
  - `js/tests/run-smoke.sh`
- Ran the smoke test:
  - Command: `0102-esp32-p4-visual-quickjs-repl/js/tests/run-smoke.sh`
  - Result: failed because no usable `qjs` binary was available in this worktree.
- Tried the requested vendored build command:
  - Command: `make -C 0100-esp32-p4-quickjs-wasm/wasm-src/quickjs qjs`
  - Result: failed because that directory is absent in this separate JS worktree.
- Located a sibling active firmware worktree containing `../esp32-s3-m5/0100-esp32-p4-quickjs-wasm/wasm-src/quickjs`, but did not use it to modify this worktree.
- Read firmware files as evidence only:
  - `0102-esp32-p4-visual-quickjs-repl/README.md`
  - `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp`
  - `components/qjs_service/include/qjs_service.h`
  - `components/qjs_service/qjs_service.cpp`
  - `components/visual_repl/include/visual_repl.h`
  - `components/visual_repl/visual_repl.cpp`
  - `components/visual_repl/README.md`
  - `components/picocalc_lcd/README.md`
  - `components/picocalc_keyboard/README.md`
- Created docmgr ticket `0102-JS-API-REPL-OS-SIM`.
- Added the design document and investigation diary.
- Wrote the detailed design/implementation guide with architecture diagrams, API sketches, pseudocode, phased implementation plan, testing strategy, risks, open questions, and file references.

### Why

- The user explicitly asked for a new docmgr ticket and a detailed intern-facing design/implementation guide before building the JS API REPL.
- The source code constraints require JS-side planning to avoid accidental edits to firmware components.
- The React prototype is a strong UX/API sketch but cannot run as-is in the portable QuickJS environment, so the design needed to separate behavior from browser-specific implementation.

### What worked

- The JS README clearly states the portable runtime contract, forbidden APIs, and JS-only edit scope.
- The existing host shim and smoke script provide a minimal pattern for future tests.
- The firmware QuickJS service already installs the same globals required by the JS playbook: `print`, `millis`, and `gc`.
- The visual REPL and keyboard code provide a clear future integration path: semantic keys, input line, row history, and row rendering already exist.
- `docmgr ticket create-ticket` and `docmgr doc add` created the ticket workspace and documents successfully.

### What didn't work

- The initial smoke test failed:

```text
qjs not found. Either install qjs or build vendored QuickJS:
  make -C /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5-js/0100-esp32-p4-quickjs-wasm/wasm-src/quickjs qjs
```

- The requested vendored build command failed in this worktree:

```text
make: *** 0100-esp32-p4-quickjs-wasm/wasm-src/quickjs: No such file or directory.  Stop.
```

- This indicates an environment/repository-content issue rather than a JS smoke-script failure. The local `0100-esp32-p4-quickjs-wasm/wasm-src` exists but does not contain the `quickjs` source directory in this JS worktree.

### What I learned

- Current firmware visual REPL geometry is 40×20 at 8×16 cells, while the requested React prototype assumes approximately 40×30 at 6×8. The JS simulator should support configurable dimensions.
- Firmware submit currently appends `ENTER captured; QuickJS eval is Phase 5`, so the JS design must be prepared for future eval integration but should not assume it already exists.
- The QuickJS service captures printed output and appends non-undefined eval return values to output, so JS examples should be output-oriented and bounded.
- The portable JS runtime should avoid `new Function`, `Date`, `Math.random`, DOM, and module mechanics in application code where possible.

### What was tricky to build

- The main tricky part was reconciling three different models:
  - the user’s browser/React 40×30 devkit prototype,
  - the current JS playbook’s minimal portable QuickJS contract,
  - the firmware’s current 40×20 row-oriented visual terminal.
- The symptom is that a direct port of the React code would look attractive but would violate the runtime contract through React, DOM rendering, browser events, `performance`, `requestAnimationFrame`, and HTML/CSS.
- The solution in the design is to preserve the fluent API shape and examples, but rewrite the runtime as ordered plain QuickJS scripts with a deterministic screen buffer, semantic key dispatch, and text snapshots.

### What warrants a second pair of eyes

- The proposed split between 40×30 default and 40×20 compatibility mode should be reviewed against the real PicoCalc font plan.
- The `OS.eval()` calculator API needs review because a direct eval-like implementation is convenient but may be inappropriate for a constrained/safe runtime.
- The eventual bundle size may exceed the current firmware input limit if pasted rather than embedded.
- The smoke-test environment issue should be reviewed by someone who knows whether this JS worktree should include the vendored QuickJS source or intentionally rely on a system `qjs`.

### What should be done in the future

- Install or restore a usable `qjs` in this worktree and rerun `js/tests/run-smoke.sh`.
- Implement Phase 1: core utilities and screen buffer.
- Add `run-api-tests.sh` and a small API smoke test before adding many widgets.
- Keep implementation commits focused and limited to `0102-esp32-p4-visual-quickjs-repl/js/**`.

### Code review instructions

- Start with the design document:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/24/0102-JS-API-REPL-OS-SIM--0102-js-api-repl-and-quickjs-os-simulator/design-doc/01-js-api-repl-and-os-simulator-design-and-implementation-guide.md`
- Check the evidence sections against these files:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5-js/0102-esp32-p4-visual-quickjs-repl/js/README.md`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5-js/components/qjs_service/qjs_service.cpp`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5-js/components/visual_repl/include/visual_repl.h`
- Validate with:
  - `docmgr doctor --ticket 0102-JS-API-REPL-OS-SIM --stale-after 30`
  - reMarkable dry-run and upload commands from the ticket-research workflow.

### Technical details

Key commands and outcomes:

```bash
git status --short --branch
# ## feature/0102-js-scripts

git branch --show-current
# feature/0102-js-scripts

0102-esp32-p4-visual-quickjs-repl/js/tests/run-smoke.sh
# qjs not found. Either install qjs or build vendored QuickJS:
#   make -C /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5-js/0100-esp32-p4-quickjs-wasm/wasm-src/quickjs qjs

make -C 0100-esp32-p4-quickjs-wasm/wasm-src/quickjs qjs
# make: *** 0100-esp32-p4-quickjs-wasm/wasm-src/quickjs: No such file or directory.  Stop.

docmgr ticket create-ticket --ticket 0102-JS-API-REPL-OS-SIM --title "0102 JS API REPL and QuickJS OS Simulator" --topics esp32-p4,quickjs,javascript,visual-repl,picocalc

docmgr doc add --ticket 0102-JS-API-REPL-OS-SIM --doc-type design-doc --title "JS API REPL and OS Simulator Design and Implementation Guide"

docmgr doc add --ticket 0102-JS-API-REPL-OS-SIM --doc-type reference --title "Investigation diary"
```

## Step 2: Add QuickJS as an explicit submodule and fix the smoke runner

I clarified that the missing `quickjs/` directory was not a tracked source mismatch between worktrees. It was an ignored local vendor checkout that existed in the sibling `../esp32-s3-m5` worktree but not in this separate JS worktree, even though the smoke runner relies on a vendored `qjs` fallback.

To make that dependency explicit, I added upstream QuickJS as a git submodule at the path the runner already expects, built the desktop `qjs` CLI there, and corrected the runner to include the host shim with QuickJS's `-I` option. After this change, the smoke test actually executes both the shim and the smoke example and prints `SMOKE PASS`.

### Prompt Context

**User prompt (verbatim):** "add it as submodule i think? since we are actually relying on it"

**Assistant interpretation:** Make the vendored QuickJS dependency explicit in git instead of relying on an ignored local checkout copied from another worktree, then verify the smoke-test path.

**Inferred user intent:** The user wants the JS worktree to be reproducible: if the smoke runner depends on `0100/.../wasm-src/quickjs/qjs`, that QuickJS source checkout should be represented as a real repository dependency.

**Commit (code):** 69873159b3186bcdcd577c4616e88051386e45f3 — "0102 js: add QuickJS submodule for smoke tests"

### What I did

- Checked the sibling QuickJS checkout remote and commit:
  - Remote: `ssh://git@github.com/bellard/quickjs.git`
  - Commit: `04be246001599f5995fa2f2d8c91a0f198d3f34c`
- Added QuickJS as a submodule using a portable HTTPS URL:
  - `git submodule add --force https://github.com/bellard/quickjs.git 0100-esp32-p4-quickjs-wasm/wasm-src/quickjs`
- Built the CLI:
  - `make -C 0100-esp32-p4-quickjs-wasm/wasm-src/quickjs qjs`
- Fixed `js/tests/run-smoke.sh` so QuickJS includes the shim with `-I` instead of treating the smoke file as an argv argument.
- Updated `js/README.md` command examples to match the working `qjs -I host-shim.js example.js` invocation.
- Ran the smoke test successfully:
  - `0102-esp32-p4-visual-quickjs-repl/js/tests/run-smoke.sh`

### Why

- The repository already documents a vendored QuickJS source directory and the smoke runner already falls back to that path, so an untracked ignored checkout made the developer environment fragile.
- Upstream `qjs` accepts one script file plus arguments; the existing command form did not execute `examples/smoke.js`. The correct way to preload `host-shim.js` is `qjs -I host-shim.js examples/smoke.js`.

### What worked

- The submodule resolved to the same upstream commit as the sibling worktree: `04be246001599f5995fa2f2d8c91a0f198d3f34c`.
- Building `qjs` completed successfully.
- The smoke test now prints:

```text
PicoCalc QuickJS smoke start 0
PASS arithmetic
PASS array
PASS object
SMOKE PASS 0
```

### What didn't work

- `git submodule status` for the whole repository still reports an older repository hygiene issue: there are pre-existing gitlink entries with no `.gitmodules` mappings, for example `0040-atoms3r-cam-streaming/.../esp32-camera`. This was not introduced by the QuickJS submodule change.
- The built `qjs`, `qjsc`, `repl.c`, and `.obj/` are ignored inside the QuickJS submodule. They are local build artifacts and should not be committed.

### What I learned

- The JS files themselves were not behind the sibling worktree; `0102-esp32-p4-visual-quickjs-repl/js/**` compared cleanly.
- The missing dependency was the ignored QuickJS vendor checkout under `0100-esp32-p4-quickjs-wasm/wasm-src/quickjs/`.
- The smoke runner had a second issue independent of the missing directory: it needed `-I` to preload the shim.

### What was tricky to build

- The tricky part was that the missing QuickJS directory is ignored by `.gitignore`, so it looked like a worktree sync problem but was actually an untracked dependency problem.
- Another sharp edge is that QuickJS command-line semantics are not “execute all files.” The first positional file is the script and later positional arguments become script arguments. The symptom was a zero-exit smoke run with no output after `qjs` was built. Using `-I` fixed that by explicitly including the shim before running the smoke script.

### What warrants a second pair of eyes

- Review whether the submodule should use the upstream `bellard/quickjs` `master` commit or a pinned release/tag.
- Review whether `.gitmodules` should also be repaired for the repository's older gitlinks without mappings; that is outside this ticket but affects `git submodule status`.
- Review whether the `.gitignore` rule `**/wasm-src/quickjs/` should be narrowed now that this path is a submodule. The submodule is tracked despite the ignore rule, but the rule can be confusing.

### What should be done in the future

- Commit the submodule addition and smoke-runner fix together or split into two focused commits, depending on reviewer preference.
- Consider documenting `git submodule update --init --recursive 0100-esp32-p4-quickjs-wasm/wasm-src/quickjs` in the JS README.
- Decide whether to pin QuickJS to a release tag instead of upstream `master`.

### Code review instructions

- Review `.gitmodules` and the gitlink at `0100-esp32-p4-quickjs-wasm/wasm-src/quickjs` first.
- Review `0102-esp32-p4-visual-quickjs-repl/js/tests/run-smoke.sh` for the `qjs -I` invocation.
- Review `0102-esp32-p4-visual-quickjs-repl/js/README.md` for matching commands.
- Validate with:
  - `make -C 0100-esp32-p4-quickjs-wasm/wasm-src/quickjs qjs`
  - `0102-esp32-p4-visual-quickjs-repl/js/tests/run-smoke.sh`

### Technical details

```bash
git submodule add --force https://github.com/bellard/quickjs.git 0100-esp32-p4-quickjs-wasm/wasm-src/quickjs
make -C 0100-esp32-p4-quickjs-wasm/wasm-src/quickjs qjs
0102-esp32-p4-visual-quickjs-repl/js/tests/run-smoke.sh
```

## Step 3: Implement Phase 1 core helpers and screen buffer

I implemented the first portable JS runtime layer: small core helpers, a deterministic RNG, a fixed-cell screen buffer, and a reusable API test runner. This gives later OS and widget work a stable rendering target before any higher-level DSL code is introduced.

The screen buffer is intentionally plain text first. It can store style metadata (`fg`, `bold`, `dim`) for future renderers, but its primary validation output is a 40-column text snapshot that works with only `print`, `millis`, and `gc` available.

### Prompt Context

**User prompt (verbatim):** "go ahead, task by task, commit at appropriate intervals, keeping a detailed diary as you work"

**Assistant interpretation:** Begin implementing the planned JS runtime phases, commit focused chunks as they become testable, and update this diary with the work and validation after each chunk.

**Inferred user intent:** The user wants steady incremental progress with reviewable commits and enough narrative detail that another engineer can resume the work safely.

**Commit (code):** c4ffe358b1d2547d43990b3f78cca7f5564e3442 — "0102 js: add portable screen test harness"

### What I did

- Added `0102-esp32-p4-visual-quickjs-repl/js/lib/00-core.js`:
  - assertion helpers,
  - `runTest`,
  - `clamp`, `pad`, `repeat`, `resolve`, `resolveX`,
  - deterministic `Pico.Lcg` RNG.
- Added `0102-esp32-p4-visual-quickjs-repl/js/lib/10-screen.js`:
  - `Pico.makeScreen(cols, rows)`,
  - clipped `set/get/text/hline/vline/box`,
  - `toLines()` and `toText()` snapshot output.
- Added `0102-esp32-p4-visual-quickjs-repl/js/tests/screen-snapshot.js` for core/screen self-tests.
- Added executable `0102-esp32-p4-visual-quickjs-repl/js/tests/run-api-tests.sh` to run host shim + libraries + API tests through QuickJS.
- Ran:
  - `0102-esp32-p4-visual-quickjs-repl/js/tests/run-api-tests.sh`
  - `0102-esp32-p4-visual-quickjs-repl/js/tests/run-smoke.sh`
- Committed the Phase 1 files.

### Why

- The design guide calls for a reliable screen buffer before building OS simulation or widgets.
- A deterministic core layer prevents later snapshot tests from becoming flaky.
- The test runner establishes the pattern that later phases can extend with more `-I` includes and test files.

### What worked

- API tests passed:

```text
PASS core pad clamp rng
PASS screen text and clipping
PASS screen boxes
PASS screen row widths
API TESTS PASS 2
```

- The existing smoke test still passed:

```text
PicoCalc QuickJS smoke start 0
PASS arithmetic
PASS array
PASS object
SMOKE PASS 0
```

### What didn't work

- N/A. This phase was straightforward after the QuickJS runner fix from Step 2.

### What I learned

- Multiple QuickJS `-I` includes work well for loading plain script libraries before a test script.
- Keeping `toLines(false)` as the exact-width form and `toText()` as the trimmed snapshot form gives both strict tests and readable output.

### What was tricky to build

- The main sharp edge was Unicode frame handling. The implementation uses `String(...).charAt(0)` and frame strings such as `╭╮╰╯─│`; this is fine for BMP box-drawing characters but should not be reused for emoji or multi-codepoint grapheme clusters without a wider text model.
- Another subtle point was clipping. The implementation clips per-cell writes, so partially off-screen text does not throw and does not wrap.

### What warrants a second pair of eyes

- Review whether `toText()` should trim trailing spaces by default. It makes terminal output readable, while `toLines(false)` preserves fixed-width snapshots for tests.
- Review whether the global namespace should remain `Pico` or eventually become `picoOS` before examples proliferate.

### What should be done in the future

- Add the deterministic OS simulator as Phase 2.
- Extend `run-api-tests.sh` with new libraries and test files as each phase lands.
- Add README documentation for `run-api-tests.sh` once the broader runtime is in place.

### Code review instructions

- Start with `js/lib/00-core.js` to review helper names and deterministic RNG behavior.
- Then review `js/lib/10-screen.js`, especially clipping and frame rendering.
- Validate with:
  - `0102-esp32-p4-visual-quickjs-repl/js/tests/run-api-tests.sh`
  - `0102-esp32-p4-visual-quickjs-repl/js/tests/run-smoke.sh`

### Technical details

```bash
0102-esp32-p4-visual-quickjs-repl/js/tests/run-api-tests.sh
0102-esp32-p4-visual-quickjs-repl/js/tests/run-smoke.sh
git commit -m "0102 js: add portable screen test harness"
```

## Step 4: Implement Phase 2 deterministic OS simulator

I added the simulated picoOS device model that the UI DSL examples will use. It provides deterministic metrics, clock, process list, music player, chat room, tiny file list, settings object, calculator expression parser, and snake game state without using Node, browser APIs, modules, filesystem access, network access, or JavaScript `eval`.

This step turns the runtime from a drawing-only test harness into a small simulated operating environment. Later widgets can now render realistic data and tests can assert stable behavior because all pseudo-random data comes from the seedable `Pico.Lcg` helper.

### Prompt Context

**User prompt (verbatim):** (same as Step 3)

**Assistant interpretation:** Continue the implementation plan with the next focused phase, commit after tests pass, and record what changed.

**Inferred user intent:** The user wants the simulator built incrementally with stable checkpoints and enough diary detail to review or resume.

**Commit (code):** be1285d2b9a6b013ad981969aa86389396ab4eb0 — "0102 js: add deterministic OS simulator"

### What I did

- Added `0102-esp32-p4-visual-quickjs-repl/js/lib/20-os-sim.js`:
  - `Pico.createOS(options)`,
  - deterministic history/metrics evolution,
  - `OS.clock`, `OS.launch`, `OS.toast`,
  - process list copy-out behavior,
  - music player state and `fft(n)`,
  - snake `reset`, `turn`, and `step`,
  - chat `send` and user colors,
  - tiny in-memory file listing and selection,
  - settings object,
  - calculator expression parser supporting arithmetic, `^`, `×`, `÷`, `√`, `π`, and degree-based trig.
- Added `0102-esp32-p4-visual-quickjs-repl/js/tests/os-sim-test.js`.
- Updated `0102-esp32-p4-visual-quickjs-repl/js/tests/run-api-tests.sh` to run both screen and OS simulator tests with the common library includes.
- Re-ran API tests and the original smoke test.
- Committed the Phase 2 files.

### Why

- The user’s target API examples depend on an `OS` object with live-ish device data, but the portable QuickJS scripts cannot depend on real firmware bindings yet.
- A deterministic simulator lets examples be useful on desktop and later paste/embed into the device visual REPL.
- Implementing a tiny parser avoids using `new Function` or `eval` for calculator examples.

### What worked

- API tests passed:

```text
PASS core pad clamp rng
PASS screen text and clipping
PASS screen boxes
PASS screen row widths
API TESTS PASS 3
PASS os clock and metrics evolve
PASS os processes are copied
PASS os calculator parser
PASS os snake and launch
PASS os chat files music settings
OS SIM TESTS PASS 2
```

- Smoke test still passed:

```text
PicoCalc QuickJS smoke start 0
PASS arithmetic
PASS array
PASS object
SMOKE PASS 0
```

### What didn't work

- The first version of the API test runner printed an absolute path in the `git submodule update --init ...` hint. I corrected it to the relative path so it is copy/pasteable from the repository root.

### What I learned

- The simulator can cover most of the React prototype’s data model without adding host dependencies.
- A simple recursive-descent parser is enough for the calculator examples and keeps the runtime safer than `new Function`.
- Running each test file as the main QuickJS script while loading libraries via `-I` keeps tests isolated and readable.

### What was tricky to build

- The calculator parser had to handle right-associative power (`2^3^2`), unicode operator normalization (`×`, `÷`, `√`, `π`), and degree-based trig while staying small.
- The OS model returns copies for mutable lists like processes and files where tests should not mutate internal state. Some nested objects such as selected files remain intentionally simple and may need stronger copy semantics later.
- Snake movement uses the same simple collision rule as the prototype; it checks the existing body before shifting the tail, which is acceptable for the small demo but worth reviewing for game-accurate behavior.

### What warrants a second pair of eyes

- Review the expression parser for edge cases before relying on it beyond examples.
- Review whether `OS.library.current` should return a copy or a live track object. The prototype used live state for toggles such as `liked`.
- Review the `OS.mode` getter, which returns an accessor object over internal player state; it is convenient but slightly unusual.

### What should be done in the future

- Implement Phase 3: `App`, `Layout`, `Panel`, base widgets, frame loop, and key dispatch.
- Add widget tests that consume `Pico.createOS()` instead of static fixtures.
- Keep examples small enough to paste into the visual REPL once firmware eval is connected.

### Code review instructions

- Start with `js/lib/20-os-sim.js`, especially `createOS`, `ExprParser`, and `_evolve`.
- Then review `js/tests/os-sim-test.js` to see the intended public behavior.
- Validate with:
  - `0102-esp32-p4-visual-quickjs-repl/js/tests/run-api-tests.sh`
  - `0102-esp32-p4-visual-quickjs-repl/js/tests/run-smoke.sh`

### Technical details

```bash
0102-esp32-p4-visual-quickjs-repl/js/tests/run-api-tests.sh
0102-esp32-p4-visual-quickjs-repl/js/tests/run-smoke.sh
git commit -m "0102 js: add deterministic OS simulator"
```

## Step 5: Implement Phase 3 portable TUI runtime

I added the fluent UI runtime layer that turns the screen buffer and OS simulator into the beginning of the requested picoOS DSL. The runtime now supports `Pico.createRuntime()`, `OS.app()`, app state, layout rows/columns, panels, frames, titles, status bars, timers, loops, key dispatch, and a first set of widgets.

This is the first step where the API begins to resemble the supplied React prototype, but it remains a QuickJS script runtime: no React, DOM, browser animation, modules, filesystem, or host APIs are required.

### Prompt Context

**User prompt (verbatim):** (same as Step 3)

**Assistant interpretation:** Continue implementing the next planned runtime layer, validate it with QuickJS tests, and commit a focused checkpoint.

**Inferred user intent:** The user wants the picoOS API to become usable through the same fluent builder shape demonstrated in the prototype, while preserving portability.

**Commit (code):** 713f19ea2f129b6f8986a50f74af6c18ba62b3d3 — "0102 js: add portable TUI runtime"

### What I did

- Added `0102-esp32-p4-visual-quickjs-repl/js/lib/30-ui-runtime.js`:
  - `Pico.createRuntime(options)`,
  - `OS.app(name)`,
  - `App.state`, `layout`, `panel`, `key`, `on('tick')`, `loop`, `compute`, `statusbar`, `mount`,
  - `Layout.row` and `Layout.col`,
  - `Panel.frame`, `title`, `titleRight`, `footer`, and content bounds,
  - widgets: `Text`, `Gauge`, `Spark`, `Menu/List`, `Table`, `Grid`, and `Progress`.
- Added `0102-esp32-p4-visual-quickjs-repl/js/tests/ui-runtime-test.js`.
- Extended `run-api-tests.sh` to include `30-ui-runtime.js` and the UI runtime test file.
- Ran API tests and the original smoke test.
- Committed the Phase 3 runtime.

### Why

- The OS simulator alone cannot express the fluent API the user requested.
- A small but working runtime unlocks examples such as hello, dashboard, sysmon, and snake.
- Testing timers, layout, gauges, menus, key dispatch, and grid layers now reduces risk before adding example scripts.

### What worked

- API tests passed, including the new UI tests:

```text
PASS runtime renders hello panel
PASS runtime layout gauge menu
PASS runtime grid follows OS snake
UI RUNTIME TESTS PASS 9
```

- The smoke test still passed:

```text
PicoCalc QuickJS smoke start 0
PASS arithmetic
PASS array
PASS object
SMOKE PASS 0
```

### What didn't work

- The first implementation of `runFrame()` accidentally called `currentApp._frame(screen, dt)` even though `_frame` takes only `dt`. The symptom was timer accumulator corruption (`0[object Object]`) and a failed assertion looking for `ticks: 1`.
- I fixed it by changing the call to `currentApp._frame(dt || 0)`.

### What I learned

- The runtime API is easiest to test through real mini-apps rather than isolated widget constructors.
- The screen-buffer approach makes runtime bugs obvious: the failed timer test showed a stale rendered value immediately.
- The existing user prototype maps well to a draw-list architecture sorted by z-order.

### What was tricky to build

- Method chaining had to preserve intuitive return values. For example, `Panel.text()` returns a `Text` widget, while `Panel.frame()` returns the panel.
- Key dispatch needs to route explicit app bindings first, then focusable widget movement/activation/type handlers.
- Layout reserves the last row for the statusbar, so panel defaults and row calculations must use `screen.rows - app._reserved`.

### What warrants a second pair of eyes

- Review the early widget set for API compatibility with the prototype. Some widgets are intentionally minimal and do not yet implement all prototype methods.
- Review `Gauge` string padding and whether to avoid `String.prototype.padEnd` entirely for style consistency.
- Review `Table.sortBy()`, which is currently a no-op placeholder to preserve chain compatibility.

### What should be done in the future

- Implement Phase 4 example scripts using this runtime.
- Add a bundle/paste workflow so examples can be flattened into a single script for device use.
- Add more widgets only when an example or test needs them.

### Code review instructions

- Start with `Pico.createRuntime()` in `js/lib/30-ui-runtime.js`.
- Review `App._frame`, `App._fireKey`, `Layout._recompute`, and `Panel.content` next.
- Validate with:
  - `0102-esp32-p4-visual-quickjs-repl/js/tests/run-api-tests.sh`
  - `0102-esp32-p4-visual-quickjs-repl/js/tests/run-smoke.sh`

### Technical details

```bash
0102-esp32-p4-visual-quickjs-repl/js/tests/run-api-tests.sh
0102-esp32-p4-visual-quickjs-repl/js/tests/run-smoke.sh
git commit -m "0102 js: add portable TUI runtime"
```

## Step 6: Implement Phase 4 portable examples

I added a small gallery of portable picoOS examples that exercise the runtime under QuickJS and print fixed-cell screen snapshots. These examples are intentionally small and self-contained: each one creates a runtime, builds an app through the fluent API, advances a frame or two, and prints the resulting screen.

The examples are not browser demos. They are desktop/device-oriented scripts that can later be bundled or pasted after the runtime libraries, and their output is designed for a 40-column PicoCalc-style display.

### Prompt Context

**User prompt (verbatim):** (same as Step 3)

**Assistant interpretation:** Continue the task list by adding usable examples and validating them through QuickJS, then commit the example checkpoint.

**Inferred user intent:** The user wants concrete scripts that demonstrate the API and can become paste/embed candidates for the visual REPL.

**Commit (code):** bf8faab6ca7fed02724c0367d05d6915b73e3554 — "0102 js: add portable picoOS examples"

### What I did

- Added example scripts:
  - `js/examples/hello-api.js`
  - `js/examples/dashboard.js`
  - `js/examples/sysmon.js`
  - `js/examples/snake.js`
  - `js/examples/calc.js`
- Added `js/tests/run-examples.sh` to preload the portable libraries and run every example through QuickJS.
- Updated `js/README.md` to document:
  - `js/tests/run-api-tests.sh`
  - `js/tests/run-examples.sh`
- Ran examples and tests before committing:
  - `js/tests/run-examples.sh`
  - `js/tests/run-api-tests.sh`
  - `js/tests/run-smoke.sh`

### Why

- The runtime needs concrete scripts to prove the API shape is usable outside unit-style tests.
- The example gallery maps directly to the user’s requested prototype categories: hello, dashboard, sysmon, snake, and calculator.
- The example runner gives future contributors a quick visual smoke test for 40-column rendering.

### What worked

- All examples executed without throwing and printed snapshots.
- The snake example initially overwrote the right border because a 19-cell grid with two-character cells was placed one column too far right. I changed the grid origin from `.at(1, 1)` to `.at(0, 1)` so the 38-character grid fits inside the framed panel content.
- API tests and smoke tests still passed:

```text
PASS runtime renders hello panel
PASS runtime layout gauge menu
PASS runtime grid follows OS snake
UI RUNTIME TESTS PASS 9
PicoCalc QuickJS smoke start 0
PASS arithmetic
PASS array
PASS object
SMOKE PASS 0
```

### What didn't work

- The first snake snapshot had no right border on grid rows because the grid wrote through the frame column. This was an example layout bug, not a renderer bug.

### What I learned

- The current API is already enough for simple but recognizable apps.
- 40-column layout is tight: two-character grid cells need exact placement inside framed panels.
- Printed snapshots are useful for both human review and future grep/snapshot assertions.

### What was tricky to build

- The examples have to balance looking like the user’s richer React prototype while staying within the smaller widget set currently implemented.
- The dashboard uses the existing menu widget directly instead of a framed nested menu because `Menu.frame/title` is not implemented yet.
- The calculator example uses the safe parser from `OS.eval()` rather than JavaScript eval, which keeps it portable but means the supported expression language must remain documented.

### What warrants a second pair of eyes

- Review whether these examples should target 40×30 or 40×20 by default. Current examples use 40×30 for hello/dashboard/sysmon and 40×20 for snake/calc.
- Review whether output snapshots should become formal golden files or remain smoke/demo output for now.
- Review whether missing prototype widgets (`row`, `keypad`, `pad`, `form`, `feed`, `input`, `editor`, `viewer`) should be implemented next or deferred until needed.

### What should be done in the future

- Implement Phase 5 bundle/paste workflow to flatten libs plus one example into a single script.
- Add formal example assertions if snapshot stability becomes important.
- Continue adding widgets only when an example needs them.

### Code review instructions

- Start with `js/tests/run-examples.sh` to see how examples are loaded.
- Review each `js/examples/*.js` header and app construction.
- Validate with:
  - `0102-esp32-p4-visual-quickjs-repl/js/tests/run-examples.sh`
  - `0102-esp32-p4-visual-quickjs-repl/js/tests/run-api-tests.sh`
  - `0102-esp32-p4-visual-quickjs-repl/js/tests/run-smoke.sh`

### Technical details

```bash
0102-esp32-p4-visual-quickjs-repl/js/tests/run-examples.sh
0102-esp32-p4-visual-quickjs-repl/js/tests/run-api-tests.sh
0102-esp32-p4-visual-quickjs-repl/js/tests/run-smoke.sh
git commit -m "0102 js: add portable picoOS examples"
```

## Step 7: Implement Phase 5 bundle/paste workflow

I added the bundle workflow that flattens the portable runtime libraries plus one example into a single JavaScript file. This is the handoff path for examples that should later be pasted into the visual REPL or embedded as firmware strings once device eval is wired.

The bundle deliberately excludes `host-shim.js`, because the device firmware already provides `print`, `millis`, and `gc`. Desktop validation still preloads `host-shim.js` with `qjs -I` before running the generated bundle.

### Prompt Context

**User prompt (verbatim):** (same as Step 3)

**Assistant interpretation:** Complete the planned implementation tasks by adding the paste/embed workflow and validating generated bundles under QuickJS.

**Inferred user intent:** The user wants examples that are not only runnable as separate files, but also easy to turn into a single REPL/device candidate.

**Commit (code):** c91c59347c624eb6c079e41314e087294e3e470f — "0102 js: add example bundle workflow"

### What I did

- Added `0102-esp32-p4-visual-quickjs-repl/js/tests/bundle-example.sh`:
  - accepts an example name or path,
  - emits `00-core.js`, `10-screen.js`, `20-os-sim.js`, `30-ui-runtime.js`, and the selected example as one script.
- Added `0102-esp32-p4-visual-quickjs-repl/js/tests/run-bundle-smoke.sh`:
  - bundles `hello-api` and `calc`,
  - runs each generated bundle with desktop QuickJS and `host-shim.js`,
  - greps for expected output.
- Updated `js/README.md` with bundle and bundle-smoke commands.
- Ran bundle smoke, API tests, example runner, and original smoke test.
- Committed the bundle workflow.

### Why

- The device visual REPL paste/embed path needs a single script, not a directory of ordered libraries.
- Keeping the bundler as a tiny shell script avoids introducing Node, npm, or a module system.
- Bundle smoke tests catch ordering problems between libs and examples.

### What worked

- Bundle smoke passed:

```text
PASS bundle hello-api
PASS bundle calc
```

- API and smoke tests still passed after adding the bundle scripts.

### What didn't work

- N/A. The library order was already explicit from `run-examples.sh`, so the bundle script used the same order.

### What I learned

- The runtime is already concatenation-safe because each library augments the `Pico` namespace and examples use the public API.
- Separating host shim from bundle output keeps the generated script closer to the device contract.

### What was tricky to build

- The bundle script has to accept both short example names and paths while remaining shell-only and copy/pasteable.
- The desktop smoke runner needs `host-shim.js`, but bundling it into device candidates would blur the contract. The scripts therefore keep host shim as a desktop-only preload.

### What warrants a second pair of eyes

- Review whether bundles should include a generated timestamp or avoid one for deterministic diffs. The current header avoids timestamps.
- Review whether the bundle should support omitting unused libraries for smaller paste payloads.
- Review bundle size against the current firmware input limit before attempting device paste.

### What should be done in the future

- Measure generated bundle sizes for each example.
- Once firmware submit calls `qjs_service_eval()`, try the smallest bundle on hardware.
- Add a firmware-side command table only after the JS examples stabilize.

### Code review instructions

- Review `js/tests/bundle-example.sh` first; it defines the runtime ordering for paste/embed candidates.
- Review `js/tests/run-bundle-smoke.sh` for desktop validation.
- Validate with:
  - `0102-esp32-p4-visual-quickjs-repl/js/tests/run-bundle-smoke.sh`
  - `0102-esp32-p4-visual-quickjs-repl/js/tests/run-api-tests.sh`
  - `0102-esp32-p4-visual-quickjs-repl/js/tests/run-examples.sh`
  - `0102-esp32-p4-visual-quickjs-repl/js/tests/run-smoke.sh`

### Technical details

```bash
0102-esp32-p4-visual-quickjs-repl/js/tests/bundle-example.sh hello-api > /tmp/hello-api.bundle.js
qjs -I 0102-esp32-p4-visual-quickjs-repl/js/host-shim.js /tmp/hello-api.bundle.js
0102-esp32-p4-visual-quickjs-repl/js/tests/run-bundle-smoke.sh
git commit -m "0102 js: add example bundle workflow"
```

## Step 8: Add an interactive desktop host emulator

I added a host-only interactive loop that runs under desktop QuickJS with `--std` and lets a developer load examples, render the current screen, send semantic keys, and advance simulated time. This provides a lightweight device-emulation workflow without introducing Node, a browser, or firmware dependencies.

The important boundary is that only the host shell uses `qjs --std` for stdin. The runtime libraries and examples remain portable and still depend only on the Pico APIs plus the firmware-compatible `print`, `millis`, and `gc` globals.

### Prompt Context

**User prompt (verbatim):** "can we test them interactively?"

**Assistant interpretation:** Add a desktop interaction harness so examples can be driven like a simple emulated device rather than only printed once.

**Inferred user intent:** The user wants to manually explore examples, send key input, and step frames before the firmware visual REPL can run the same scripts interactively on hardware.

**Commit (code):** 4e0979f20b39cd79941f95c8971f2fc8d7cb48e4 — "0102 js: add interactive host emulator"

### What I did

- Added `0102-esp32-p4-visual-quickjs-repl/js/tools/interactive-host.js`:
  - host-only QuickJS script using `std.in.getline()`,
  - loads examples by name,
  - renders `rt.renderText()`,
  - sends semantic key tokens,
  - advances frames with `frame` and `run` commands.
- Added `0102-esp32-p4-visual-quickjs-repl/js/tests/run-interactive.sh`:
  - preloads host shim and portable runtime libraries,
  - runs `interactive-host.js` with `qjs --std`,
  - accepts an initial example name.
- Updated `js/README.md` with interactive usage and commands.
- Tested the host with piped commands:
  - `show`
  - `key a`
  - `frame 1000`
  - `open snake`
  - `key down`
  - `frame 130`
  - `quit`
- Re-ran smoke/API/bundle checks.

### Why

- One-shot snapshots are useful for regression, but manual UI development needs a loop where state persists across key presses and frames.
- The device firmware is not yet wired to evaluate visual REPL submissions, so a host emulator lets JS-side API work continue.

### What worked

- The host starts with `hello-api`, shows a 40-column screen, accepts commands, and can switch to `snake`.
- Example auto-printing is suppressed during `open` so the host controls rendering explicitly.
- Existing tests still passed:
  - `run-smoke.sh`
  - `run-api-tests.sh`
  - `run-bundle-smoke.sh`

### What didn't work

- The first host implementation allowed examples to print their initial snapshots while loading, then the host printed another explicit snapshot. I fixed that by temporarily replacing `globalThis.print` with a no-op while `std.loadScript()` loads an example, then restoring it.

### What I learned

- QuickJS `--std` gives enough stdin support for a practical host emulator through `std.in.getline()` and `std.loadScript()`.
- Keeping the host in `js/tools/` makes the portability boundary clearer than mixing stdin behavior into runtime libraries.

### What was tricky to build

- The host needs to use non-portable `std` APIs while not contaminating the portable examples. The solution is to keep `--std` usage only in `interactive-host.js` and keep app scripts unchanged.
- Loading examples reuses global names such as `rt`, `OS`, and `app`. That is acceptable for a single-process host loop, but a stronger future host might load examples in separate contexts.

### What warrants a second pair of eyes

- Review whether `tools/interactive-host.js` should live under `tests/` instead of `tools/`.
- Review whether the host should support raw single-key mode later; currently it is line-oriented commands.
- Review whether examples should expose factory functions instead of relying on global `rt` after `std.loadScript()`.

### What should be done in the future

- Add a richer host command for repeated animation, such as `watch snake 8fps`, if manual UI tuning needs it.
- Consider adding formal scripted interactive tests if the host becomes part of CI.
- Try matching the firmware's 40×20 mode by opening `snake` and `calc`, which already use 40×20 snapshots.

### Code review instructions

- Start with `js/tests/run-interactive.sh` to see the host preload boundary.
- Review `js/tools/interactive-host.js` command dispatch and example loading.
- Validate manually with:
  - `0102-esp32-p4-visual-quickjs-repl/js/tests/run-interactive.sh hello-api`
  - commands: `open snake`, `key down`, `frame 130`, `show`, `quit`.

### Technical details

```bash
printf 'show\nkey a\nframe 1000\nopen snake\nkey down\nframe 130\nquit\n' | \
  0102-esp32-p4-visual-quickjs-repl/js/tests/run-interactive.sh hello-api

git commit -m "0102 js: add interactive host emulator"
```

## Step 9: Add C++ native QuickJS host prototype

I added the first C++ host program that embeds QuickJS directly and implements the beginning of the picoOS JavaScript API on the C++ side. This is a different layer from the earlier JS-only simulator: the new host exposes `OS`, `App`, `Panel`, `Text`, and `Gauge` bindings through QuickJS C APIs, while C++ owns screen state, timers, keyboard dispatch, and ANSI terminal rendering.

The layout intentionally separates firmware-portable pieces from host-only glue. `pico_native_api.hpp/.cpp` is the part designed to migrate into ESP-IDF components later; `main.cpp` is only the desktop terminal event loop using termios/raw keyboard and ANSI redraws.

### Prompt Context

**User prompt (verbatim):** "ok, yes. And I want to build it so that that c++ program can later be built (in parts) onto the firmware itself. the JS API should be implemented for the most part on the C++ side, like we do for say, libraries in ~/code/wesen/go-go-golems/go-go-goja/ or maybe more precisely like ~/code/wesen/go-go-golems/goja-text,  except it's C++ not go"

**Assistant interpretation:** Build a native C++ QuickJS host with the picoOS API implemented as native bindings, structured so the API/runtime portion can later be reused in firmware, analogous to native module providers in the Go/goja ecosystem but using C++ and QuickJS.

**Inferred user intent:** The user wants to move the API ownership from JavaScript shims toward C++ bindings, so the desktop emulator and firmware eventually share the same native implementation strategy.

**Commit (code):** 48b6e0af9b7f4d76445e7cbb79541cf032d2f520 — "0102 js: add native QuickJS host prototype"

### What I did

- Added `js/tools/native-host/`:
  - `Makefile` builds a desktop `picojs-host` binary against the QuickJS submodule.
  - `README.md` documents the API/host split and usage.
  - `src/pico_native_api.hpp` exposes the runtime boundary.
  - `src/pico_native_api.cpp` implements native QuickJS bindings and screen/app/widget state.
  - `src/main.cpp` implements the host-only terminal loop.
- Added native-host examples:
  - `js/examples-native/hello-native.js`
  - `js/examples-native/dashboard-native.js`
- Added wrapper:
  - `js/tests/run-native-host.sh`
- Updated `js/README.md` with native host commands.
- Built and validated the host.

### Why

- The earlier JS-side runtime proves the API shape, but firmware should not necessarily implement the visual API as a large JavaScript library.
- C++ native bindings are closer to the final ESP32-P4 architecture: QuickJS evaluates user scripts, while native code owns rendering, input, stateful services, and constrained memory behavior.
- Keeping host-only terminal code separate from `pico_native_api.*` makes it clearer which pieces can later become firmware components.

### What worked

- Native host builds cleanly with the local QuickJS submodule:

```bash
make -C 0102-esp32-p4-visual-quickjs-repl/js/tools/native-host all
```

- `hello-native` accepts key input and updates native-rendered state:

```bash
(printf 'a'; sleep 0.2; printf 'q') | \
  0102-esp32-p4-visual-quickjs-repl/js/tests/run-native-host.sh hello-native
```

- `dashboard-native` can call `OS.launch('notes')` through the native OS binding:

```bash
(printf 'o'; sleep 0.2; printf 'q') | \
  0102-esp32-p4-visual-quickjs-repl/js/tests/run-native-host.sh dashboard-native
```

- Existing JS tests still pass:
  - `run-smoke.sh`
  - `run-api-tests.sh`
  - `run-bundle-smoke.sh`

### What didn't work

- The first build used the wrong relative path to the QuickJS submodule in the native-host Makefile. I corrected `ROOT` from one directory too high to `../../../..`.
- The initial C++ type layout defined `Runtime` in an anonymous namespace while the public header forward-declared `pico_native::Runtime`, producing ambiguous/incompatible type errors. I moved the implementation type into the `pico_native` namespace.
- A first attempt to mirror panels in a map accidentally risked double ownership of `Panel*`; I removed that ownership path and kept the first checkpoint panel order as the owner.
- Freeing the QuickJS runtime at process exit asserted because native structs still hold duplicated JS callback values. For this host checkpoint I intentionally leak the QuickJS runtime on exit and documented that explicit finalizers/value release are required before firmware migration.

### What I learned

- The native binding shape maps well to the goja provider analogy: JavaScript gets fluent objects, while native code owns object state and method implementations.
- QuickJS command-line hosting and embedded QuickJS hosting are quite different: embedding requires deliberate ownership of JSValue callbacks and class opaque pointers.
- A minimal native API can already support useful scripts: `OS.app`, `OS.clock`, `OS.launch`, `App.state/panel/on/key/statusbar/mount/exit`, `Panel.frame/title/titleRight/text/gauge`, and `Text/Gauge` methods.

### What was tricky to build

- JSValue lifetime is the main sharp edge. Native widgets and timers store callbacks/literals with `JS_DupValue`, but the first checkpoint does not yet free them. That is acceptable for the short-lived desktop host but not for firmware.
- Terminal raw input is host-only and must not leak into the firmware-portable API layer.
- The native host currently uses a basic UTF-8 cell advance model. It handles current box drawing well enough, but it is not a full wcwidth/grapheme implementation.

### What warrants a second pair of eyes

- Review the native QuickJS ownership model before porting to firmware. Finalizers or explicit teardown are mandatory.
- Review whether the C++ object model should move to smaller component-style files before it grows.
- Review whether `q` should quit the host or be forwarded to the app; current host uses `q` as host quit.
- Review whether `pico_native_api.*` should be placed under a future firmware component once the API stabilizes.

### What should be done in the future

- Add proper JSValue cleanup and QuickJS class finalizers.
- Add more native widgets and layout support to match the JS-side prototype.
- Add a non-interactive native-host smoke mode for CI instead of testing through piped raw keys.
- Begin planning the ESP-IDF component boundary for `pico_native_api.*`.

### Code review instructions

- Start with `js/tools/native-host/README.md` for the boundary between portable native API and host-only terminal code.
- Review `src/pico_native_api.hpp/.cpp` next, especially JSValue storage and C++ object ownership.
- Review `src/main.cpp` last; it is desktop-only raw terminal glue.
- Validate with:
  - `make -C 0102-esp32-p4-visual-quickjs-repl/js/tools/native-host all`
  - `0102-esp32-p4-visual-quickjs-repl/js/tests/run-native-host.sh hello-native`
  - `0102-esp32-p4-visual-quickjs-repl/js/tests/run-native-host.sh dashboard-native`

### Technical details

```bash
make -C 0102-esp32-p4-visual-quickjs-repl/js/tools/native-host all
(printf 'a'; sleep 0.2; printf 'q') | 0102-esp32-p4-visual-quickjs-repl/js/tests/run-native-host.sh hello-native
(printf 'o'; sleep 0.2; printf 'q') | 0102-esp32-p4-visual-quickjs-repl/js/tests/run-native-host.sh dashboard-native
git commit -m "0102 js: add native QuickJS host prototype"
```

## Step 10: Clean up native host QuickJS values

I replaced the native host's deliberate process-exit QuickJS leak with a real teardown path for the first native API objects. Native timers, widgets, panels, and app fields now use move-only RAII storage for duplicated `JSValue` handles, and `runtime_destroy()` releases native objects before freeing the QuickJS context/runtime.

This step matters because the C++ API layer is intended to migrate toward firmware. Firmware cannot rely on process exit to reclaim leaked JS callbacks, so the native API needs deterministic ownership even in the host prototype.

### Prompt Context

**User prompt (verbatim):** "continue"

**Assistant interpretation:** Continue the native-host work by addressing the next risk called out in the previous diary step: proper QuickJS value ownership and cleanup.

**Inferred user intent:** The user wants steady progress toward a firmware-reusable C++ implementation, not just a desktop proof of concept.

**Commit (code):** 6a6d7b74c48641640728395ae650ea219e2757de — "0102 js: clean up native host QuickJS values"

### What I did

- Made `StoredValue` move-only and RAII-managed:
  - duplicates values with `JS_DupValue`,
  - frees values with `JS_FreeValue`,
  - prevents accidental shallow copies.
- Made `Timer` move-only so vectors can own callback handles safely.
- Changed `runtime_destroy()` to:
  - clear `g_current_runtime` if needed,
  - reset the native app before freeing the context,
  - free `JSContext`,
  - free `JSRuntime`,
  - delete the runtime object.
- Added `run-native-smoke.sh` to exercise the native host with scripted key input and verify clean exits.
- Updated native-host docs to describe the non-owning JS wrapper objects and native-owned app/panel/widget state.

### Why

- The previous native host intentionally leaked the QuickJS runtime at process exit because native structs held duplicated JS callbacks.
- That was acceptable only as a first checkpoint. A firmware-portable API layer needs deterministic release ordering.
- A smoke script catches regressions where `JS_FreeRuntime` asserts due to live GC objects.

### What worked

- Native smoke passed:

```text
PASS native hello-native
PASS native dashboard-native
```

- Existing JS tests still passed:
  - `run-smoke.sh`
  - `run-api-tests.sh`
  - `run-bundle-smoke.sh`

### What didn't work

- N/A for this step. The cleanup path was validated with the same piped-key smoke that previously exposed the `JS_FreeRuntime` assertion.

### What I learned

- Most native QuickJS ownership bugs come from accidental value copies. Making `StoredValue` move-only prevents that class of bug.
- Native wrapper JS objects can remain non-owning as long as native state outlives the context and is released before `JS_FreeContext`.

### What was tricky to build

- Containers need move-friendly value types. `Timer` had to become explicitly move-only because it owns a `StoredValue` callback.
- Teardown ordering matters: native `StoredValue` fields must free before the context disappears, but after JS execution is finished.

### What warrants a second pair of eyes

- Review whether non-owning JS wrapper objects are acceptable long-term, or whether class finalizers should mark/detach native pointers.
- Review exception paths: callbacks that throw currently do not have rich error reporting in the native event loop.

### What should be done in the future

- Add native layout support next so the C++ API catches up with the JS-side fluent runtime.
- Add explicit handling for `JS_IsException` after native `JS_Call` sites.

### Code review instructions

- Review `StoredValue` and `runtime_destroy()` in `js/tools/native-host/src/pico_native_api.cpp`.
- Validate with:
  - `0102-esp32-p4-visual-quickjs-repl/js/tests/run-native-smoke.sh`
  - `0102-esp32-p4-visual-quickjs-repl/js/tests/run-smoke.sh`
  - `0102-esp32-p4-visual-quickjs-repl/js/tests/run-api-tests.sh`

### Technical details

```bash
0102-esp32-p4-visual-quickjs-repl/js/tests/run-native-smoke.sh
0102-esp32-p4-visual-quickjs-repl/js/tests/run-smoke.sh
0102-esp32-p4-visual-quickjs-repl/js/tests/run-api-tests.sh
git commit -m "0102 js: clean up native host QuickJS values"
```

## Step 11: Add native layout binding

I added native `App.layout()` and `Layout.row()/col()` support so JavaScript can define named panel regions while C++ computes and owns the actual rectangles. This moves another part of the fluent picoOS API from JavaScript into the C++ QuickJS binding layer.

I also fixed missing `JS_FreeValue()` calls for callback return values. The layout callback exposed this leak because the callback chain returns a layout object; not freeing that return value left QuickJS GC objects alive at runtime teardown.

### Prompt Context

**User prompt (verbatim):** (same as Step 10)

**Assistant interpretation:** Continue expanding the native C++ API toward parity with the JS-side runtime while keeping the host testable and firmware-oriented.

**Inferred user intent:** The user wants the C++ implementation to become the main API surface, with JavaScript examples becoming thin scripts over native objects.

**Commit (code):** 267e1133ded2d7eedf3aca8def7eba20b0054772 — "0102 js: add native layout binding"

### What I did

- Added a native `Layout` struct and `PicoLayout` QuickJS class.
- Added `App.layout(fn)`.
- Added `Layout.row(size, id)` and `Layout.col(size, id)`.
- Added C++ region recomputation and made `App.panel(id)` use named layout regions.
- Added `examples-native/layout-native.js`.
- Extended `run-native-smoke.sh` to validate `layout-native`.
- Updated `README.md` and native-host docs with layout/native-smoke commands.
- Freed callback return values from:
  - layout callback invocation,
  - tick timer callbacks,
  - key callbacks.

### Why

- Layout is a central part of the requested picoOS fluent API.
- C++-owned layout rectangles are a good firmware boundary: JS describes structure, native code owns geometry and rendering.
- Freeing `JS_Call` return values is required for clean native runtime teardown.

### What worked

- Native smoke now validates all three native examples:

```text
PASS native hello-native
PASS native dashboard-native
PASS native layout-native
```

- Full validation passed:
  - `run-native-smoke.sh`
  - `run-smoke.sh`
  - `run-api-tests.sh`
  - `run-bundle-smoke.sh`

### What didn't work

- The first `layout-native` run aborted on exit with `JS_FreeRuntime: Assertion list_empty(&rt->gc_obj_list) failed`.
- The cause was not `StoredValue`; it was unfreed return values from `JS_Call`, especially the layout callback returning the layout object from `.row(...).row(...)` chaining.
- I fixed this by storing each callback return value and calling `JS_FreeValue()`.

### What I learned

- Fluent APIs produce many intermediate JS return values. Native bindings must free those values even when the return value is ignored by C++.
- Layout can be implemented in a firmware-friendly way with only named `Rect` regions on the app.

### What was tricky to build

- `App.layout()` currently creates a stack-owned `Layout` and passes a non-owning JS wrapper into the callback. This is fine for synchronous builder use but would be unsafe if user code stored the layout object for later.
- Single-axis layout mirrors the JS-side runtime and intentionally does not support mixed row/column nesting yet.

### What warrants a second pair of eyes

- Review whether layout wrappers should be heap-owned with finalizers if the API might ever allow storing them.
- Review the single-axis-only limitation against upcoming examples.
- Review the region ownership model before porting to firmware.

### What should be done in the future

- Add native `Menu` and focus/key routing to exercise arrows and enter without manual app key bindings.
- Add native exception reporting after callback calls.
- Split `pico_native_api.cpp` into smaller files as more native widgets are added.

### Code review instructions

- Review `Layout`, `js_app_layout`, `layout_recompute`, and `js_layout_add` in `pico_native_api.cpp`.
- Review `examples-native/layout-native.js` for the intended JS-facing API.
- Validate with:
  - `0102-esp32-p4-visual-quickjs-repl/js/tests/run-native-smoke.sh`
  - `make -C 0102-esp32-p4-visual-quickjs-repl/js/tools/native-host all`

### Technical details

```bash
0102-esp32-p4-visual-quickjs-repl/js/tests/run-native-smoke.sh
0102-esp32-p4-visual-quickjs-repl/js/tests/run-smoke.sh
0102-esp32-p4-visual-quickjs-repl/js/tests/run-api-tests.sh
0102-esp32-p4-visual-quickjs-repl/js/tests/run-bundle-smoke.sh
git commit -m "0102 js: add native layout binding"
```
