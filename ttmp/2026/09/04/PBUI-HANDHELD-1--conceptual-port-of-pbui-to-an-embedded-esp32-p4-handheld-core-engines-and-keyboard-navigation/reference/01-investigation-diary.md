---
Title: Investigation diary
Ticket: PBUI-HANDHELD-1
Status: active
Topics:
    - pbui
    - embedded
    - architecture
    - design
    - onboarding
    - research
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: abs:///home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp
      Note: key mapping region read
    - Path: abs:///home/manuel/workspaces/2026-09-01/add-plot-editor/pbui/src/presentation/createPbui.tsx
      Note: Provider/Presentation/ObjectMenu/performAction regions read
    - Path: abs:///home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/09/04/PBUI-HANDHELD-1--conceptual-port-of-pbui-to-an-embedded-esp32-p4-handheld-core-engines-and-keyboard-navigation/design-doc/01-pbui-handheld-port-analysis-design-and-implementation-guide.md
      Note: the deliverable this diary explains
    - Path: abs:///home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/09/04/PBUI-HANDHELD-1--conceptual-port-of-pbui-to-an-embedded-esp32-p4-handheld-core-engines-and-keyboard-navigation/sources/pbui-handheld.jsx
      Note: read in full with nl -ba
ExternalSources: []
Summary: Chronological diary of the PBUI-HANDHELD-1 investigation - source import, pbui kernel and workbench-core mapping, ESP32-P4 firmware evidence, design decisions, and delivery.
LastUpdated: 2026-09-04T12:30:44.717641888-04:00
WhatFor: Continue the handheld-port investigation or review how the design guide was derived and validated.
WhenToUse: Read when picking up PBUI-HANDHELD-1 after this session or when a decision in the design guide needs its evidence trail.
---


# Diary

## Goal

Record how the `PBUI-HANDHELD-1` design guide was produced: which sources were imported, which pbui and ESP32-P4 files were read as evidence, what was decided and why, what was tricky, and how the deliverable was validated and delivered.

## Step 1: Create the ticket, import the handheld sources, and map three code bases

The ticket was created in the pbui repository with a new `embedded` topic, the three handheld files were copied from `~/Downloads` into `sources/`, and the investigation proceeded in three passes: the handheld prototype and its report and manual; the pbui presentation kernel, React runtime and workbench-core; and the ESP32-P4 PicoCalc firmware tree with its docmgr tickets. The design guide was then written in one pass with decision records, pseudocode and phased plan.

The single most useful finding was that the three code bases already agree on the shape of the solution. pbui's kernel is React-free since KERNEL-1; the handheld prototype's interaction core is a pure reducer that was validated headlessly; and the ESP32 tree's 0102 project already hosts JavaScript behind an owner task with key tokens, a text-cell renderer and a text dump. The port is therefore a shell rewrite plus two small product contracts (catalog, line-describe), not a kernel port.

### Prompt Context

**User prompt (verbatim):** "Create a new docmgr ticket in pbui to analyze how to do a "port" (conceptual port) of pbui/ to an embedded device, which will require porting the core engines and then implementing a keyboard based navigation. Import from ~/Downloads: [pbui-handheld(3).jsx, pbui-handheld-manual(1).md, pbui-handheld-project-report(1).md] into the sources/ folder of the ticket, then analyze them, analyze the pbui project, and Create  a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and the nupload to remarkable. This will be targetting a esp32p4 like in /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0099-esp32-p4-picocalc-display-keyboard and the corresponding tickets in ttmp/ in that repository: [19 ticket document paths]"

**Assistant interpretation:** Produce an intern-facing analysis/design/implementation guide for porting pbui's engines and building keyboard navigation on an ESP32-P4 PicoCalc, grounded in the imported handheld prototype, the pbui code, and the existing ESP32-P4 tickets; keep it in a docmgr ticket and upload to reMarkable.

**Inferred user intent:** Decide, with evidence, what of pbui can travel to a keyboard-only embedded device and what has to be built, and hand someone a document they can implement from.

### What I did

- `docmgr vocab add --category topics --slug embedded`, `docmgr ticket create-ticket --ticket PBUI-HANDHELD-1 …`, `docmgr doc add` for the design doc and this diary.
- Copied the three Downloads files into `sources/` without the `(n)` suffixes: `pbui-handheld.jsx` (1,133 lines), `pbui-handheld-manual.md`, `pbui-handheld-project-report.md`.
- Read the prototype in full with `nl -ba` and the report and manual in full.
- Read the pbui kernel: `src/presentation/{types,model/*,actions/{types,resolve,availability,perform,conditions,typeGraph,registry},context/*,acceptance/*,interaction/*,relations/types,help/types,links/{terms,snapshot,index}}.ts`, `src/link-kernel.ts`, `src/focus.ts`, `src/chrome/shortcutRouting.ts`, and the `Provider`/`Presentation`/`ObjectMenu`/`performAction` regions of `createPbui.tsx`.
- Read workbench-core: `README.md`, `index.ts`, `commands.ts`, `createWorkbenchCore.ts`, `session.ts`, `document.ts` exports, `apps.ts` exports, `describe.ts` head; the workbench proto message list; `pbui-workbench` `launcherRows.ts`, `links/contributions.ts` ids, `types.ts` `WorkbenchVerb`; `pbui-ecommerce` `runtime.tsx` and `actions.ts`.
- Read the ESP32 tree: `0099/README.md`, `main/picocalc_keyboard.{h,c}`, `sdkconfig.defaults`; `components/{picocalc_lcd,qjs_service,visual_repl,picoos_core,picojs_runtime}/include/*.h`; `0102/README.md`, `0102/js/{README.md,host-shim.js,lib/10-screen.js,lib/30-ui-runtime.js,examples/calc.js}`, the key-mapping region of `0102/main/app_main.cpp`; the native QuickJS guide (service API, flows) and its diary's measured numbers; the visual QuickJS REPL guide in full; the display-server guide outline and summary; the LVGL guide's architecture section; the SPI throughput guide's benchmark table; executive summaries of the power, sleep, audio, SD card, backlight and pin-map docs; the WASM guide's hardware section.
- Counted evidence for the guide: `wc -l` of the kernel (9,295 non-test lines in `src/presentation`, of which `createPbui.tsx` is 1,392) and workbench-core (6,564); `it(` counts (166 kernel, 215 core).
- Wrote the design guide in five appended sections (0–3, 4–5, 6–7, 8, 9–13), 1,313 lines, with Mermaid diagrams, ten decision records, pseudocode for nine flows, a nine-phase plan and a file reference map.

### Why

- The user asked for an intern guide that explains "all the parts of the system needed to understand what it is"; that required reading the engines rather than summarising their READMEs, and quoting file and line evidence for each contract.
- The ESP32 tickets were listed by the user as the hardware context; reading their measured results (SPI row throughput, QuickJS timings, key codes) let the design carry a latency budget and a task model instead of guesses.

### What worked

- `grep -n -E '^export'` and targeted `sed -n` ranges gave the API surfaces of large files (`createPbui.tsx`, `resolve.ts`, `createWorkbenchCore.ts`) without reading all of them.
- The prototype report's own "mapping tables" (pointer affordance → handheld replacement; REPL vs. palette/hints/magit) translated almost one-to-one into the guide's pbui mapping (§5) and gap analysis (§6).
- The 0102 project turned out to already contain the host plumbing the port needs (`picoos_key` tokens, frame loop, `visual_repl_dump_text`), which made Phase 4 a clone-and-trim rather than new firmware.

### What didn't work

- A `grep -rn -l … --include=*.tsx` search failed under zsh with `(eval):1: no matches found: --include=*.tsx` because the glob was expanded by the shell; rerunning without `--include` (and with `grep -rn` over explicit paths) worked. No other command failed.
- The generated design-doc and diary templates had placeholder bodies; the frontmatter was kept and the bodies replaced with a small Python splice on the second `---` so `LastUpdated`/`Topics` stayed docmgr-managed.

### What I learned

- pbui has **no catalog**: nothing in the kernel enumerates instances of a type. Every pointer interaction starts from an object already on screen. A keyboard accept slot needs the enumeration for completion, so it becomes a product contract next to `contextFor`.
- pbui's `describe()` returns `unknown` for agents; the handheld needs a *line-producing* describe. These are different things and should stay two members.
- `workbench-core` already gives the handheld a deck engine (session = two pointers; leaves = cards; `canClose` = the last-tile guard) and an agent-readable screen description, which is the nearest existing thing to the prototype's "screen is a query result".
- The device keyboard delivers printable characters already shifted, and emits `repeat`/`released` events, so `:` `;` `/` `?` `$` need no modifier machine and hold-`i` peek is implementable.
- The flash size is stated differently in two ESP32 documents (16 MB in the WASM guide, 32 MB in 0099's `sdkconfig.defaults`); recorded as an open question rather than resolved.

### What was tricky to build

- **Deciding the language of the engines on the device.** The obvious embedded instinct is a C port. The evidence pointed the other way: the kernel is defined by permutation tests, four-state availability and a trace format, and reimplementing it would create the second implementation KERNEL-1 exists to prevent; the device's measured QuickJS speed (100k loop 133 ms) covers a per-keystroke resolve with headroom. The decision record (Decision 1) states the fallback (a C `matchSelector`) so a reviewer who disagrees has a concrete alternative to argue for.
- **Separating shell state from domain state in the prototype.** `runCmd` mixes domain overrides (skip, pins) with shell effects (push the inspector, toast, tray). The guide splits them: verbs carry domain meaning only; the shell owns stack, tray, histories and toasts. This is what makes `onPerform` a pure reducer over `{ cursor, overrides }` and lets the listener render `PerformEnvelope`s.
- **Keeping the REPL's "offered ⇔ receivable on screen" rule while fixing its known friction.** The prototype's `goto` problem is answered by pbui's scopes: actions declared in a `"shell"` scope are offered regardless of the screen. That is a declaration, not a special case.
- **The JS→C frame contract.** JSON per keystroke would cost a stringify and a parse per frame; the guide specifies typed arrays (`Uint16Array` cells with glyph+style, `Uint8Array` row metadata) plus a dirty bitmask, and keeps the text dump for tests only.

### What warrants a second pair of eyes

- Decision 1 (JS engines in QuickJS) rests on extrapolating from loop benchmarks to `matchSelector` over Map/Set-heavy objects; Phase 2's speed report is the real check.
- Decision 5 (workbench-core as the deck engine) assumes `createWorkbenchPresentationFragment` from `pbui-workbench` is React-free enough to bundle; if it is not, the tile/port/link types and rules need lifting into the new package, which duplicates declarations.
- The `slots` convention (which action ids take an argument) is a shell-side map rather than kernel metadata; a reviewer may prefer an `ActionMetadata` extension.
- The key table assigns `F10` to the menu and `F1` to help; check against the PicoCalc's physical key legends before committing.

### What should be done in the future

- Phase 1 of the plan: create `packages/pbui-handheld`, extract the prototype's reducer and line model, and make the six manual tutorials pass as screen-dump goldens.
- Confirm the board's flash size from `esptool` before writing the 0104 partition table.
- Enumerate ES2021+ usages in the kernel (`Object.hasOwn` at least) against the vendored QuickJS version.

### Code review instructions

- Start with the design guide: `design-doc/01-pbui-handheld-port-analysis-design-and-implementation-guide.md`, sections 4.9 (the pure/pointer-bound split), 6 (gaps), 7.3 (decisions), 8 (flows).
- Cross-check the pbui claims against `src/presentation/actions/resolve.ts` (header comment), `src/presentation/interaction/{accept,activation}.ts`, `src/presentation/acceptance/resolve.ts`, and `packages/workbench-core/src/{commands,session}.ts`.
- Cross-check the hardware numbers against `…/esp32-s3-m5/0099-esp32-p4-picocalc-display-keyboard/README.md`, the SPI throughput guide's table, and the native QuickJS diary Step 6.
- Validate bookkeeping with `docmgr doctor --ticket PBUI-HANDHELD-1 --stale-after 30`.

### Technical details

Key evidence quoted in the guide, with locations:

- Precedence ladder and "never a tie-breaker": `src/presentation/actions/resolve.ts:24-43`.
- Four availability states: `src/presentation/actions/availability.ts:1-27`.
- Closed type world: `src/presentation/context/selector.ts:107-112`.
- Accept machine invariants: `src/presentation/interaction/accept.ts:4-24`.
- Activation ladder: `src/presentation/interaction/activation.ts:4-24`.
- Session as two pointers: `packages/workbench-core/src/session.ts:9-14`.
- Command algebra: `packages/workbench-core/src/commands.ts:46-70`.
- Prototype: `availCmds` `jsx:343-351`; `enterAccept` `jsx:683-693`; `acceptCands` `jsx:694-709`; digit gate `jsx:768-773`; Esc ladder `jsx:714-719`; line helpers `jsx:251-253`; `SP` mirror `jsx:464`.
- Device: key codes `picocalc_keyboard.c:180-250`; registers `picocalc_keyboard.h:16-30`; `qjs_service.h` config and eval API; `visual_repl.h` geometry constants; 0102 token map `app_main.cpp:657-668`.

### Delivery evidence

- `docmgr doctor --ticket PBUI-HANDHELD-1 --stale-after 30` → `All checks passed`.
- `remarquee upload bundle --dry-run …` then the real upload: `OK: uploaded PBUI-HANDHELD-1 pbui handheld port guide.pdf -> /ai/2026/09/04/PBUI-HANDHELD-1`; `remarquee cloud ls /ai/2026/09/04/PBUI-HANDHELD-1 --long --non-interactive` lists `[f] PBUI-HANDHELD-1 pbui handheld port guide`. The bundle contains the design guide, this diary, the project report and the manual, with a depth-2 table of contents.

## Step 2: Specify the native C++ port and LCD UI framework

The direction changed from embedding the TypeScript kernel in QuickJS to implementing a bounded native C++ subset. I created a second design document rather than rewriting the original, so the two approaches and their tradeoffs remain reviewable. The new document turns the handoff into concrete C++ types, APIs, ownership boundaries, key flows, memory budgets, tests, and eight implementation phases.

The proposed firmware has three independently host-testable layers: `pbui_core` for semantic interpretation, `pbui_handheld` for the pure keyboard reducer, and `pbui_rows` for presentation-bearing retained rows and dirty LCD blits. The product still supplies one declaration plus facts, catalog, describe, and execute adapters.

### Prompt Context

**User prompt (verbatim):** See `reference/02-handoff-to-the-c-engine-port.md`, section “The handoff”, which preserves the handoff text verbatim; the final request was: “Create a new design doc in the same folder that actually goes over implementing / porting the PBUI framework / a subset of the framework to the picocal esp32-p4 (32MB of RAM) and using keyboard navigation, and rendering using the LCD (so probably building some UI framework as well)”

**Assistant interpretation:** Add a second, implementation-oriented design document for a native C++ PBUI subset on the 32 MB ESP32-P4 PicoCalc, including the keyboard shell and an LCD UI framework.

**Inferred user intent:** Give the C++ firmware implementer an actionable architecture that preserves PBUI semantics without carrying React or QuickJS.

### What I did

- Read the core engines in the requested dependency order and treated their header comments and tests as the behavioral contract.
- Re-read the existing handheld guide, project report, manual, product declaration example, and the reusable LCD, keyboard, and visual REPL APIs.
- Created `design-doc/02-native-c-pbui-subset-for-the-esp32-p4-picocalc.md` with native representations, API sketches, resolver and accept algorithms, task ownership, a 40×20 retained row UI, memory budgets, decision records, key-flow pseudocode, implementation phases, testing, risks, and review instructions.

### Why

- A native port creates a second semantic implementation, so ambiguity handling, hidden/inapplicable behavior, and stale action refusal must be specified precisely and defended by translated tests.
- The keyboard shell and LCD renderer need separate boundaries from product semantics so they remain reusable and camera-free testable.

### What worked

- The React-free modules map cleanly to closed C++ data and `std::variant` result/state types.
- The measured row-blit path supports a simple dirty-row renderer without a full framebuffer.
- The existing handoff already isolated catalog and line-producing `describe` as the only new product contracts.

### What didn't work

- Running `docmgr` from the workspace parent failed with `Error: root directory does not exist: /home/manuel/workspaces/2026-09-01/add-plot-editor/ttmp`; changing into `pbui/` fixed it.
- Initial `nl` commands against `src/...` from the workspace parent failed with `No such file or directory`; absolute paths and the `pbui/` working directory are required.

### What I learned

- The most important native representation rule is that resolved rows retain candidate identity and revisions, never borrowed pointers into an old snapshot.
- The action selector must not evaluate `when` as selector rejection: action conditions produce availability states because hidden and unavailable candidates suppress fallbacks.
- A two-row-buffer renderer needs about 20 KB of DMA-capable internal RAM; most semantic and retained UI state can safely use PSRAM.

### What was tricky to build

- The shell's `availCmds` must be backed by actual resolver results rather than recreating the prototype's command/type table, otherwise the “one declaration” invariant is lost. The design therefore unions introspection results over visible references and the implicit current card.
- Translating exception-based TypeScript validation to ESP-IDF C++ requires structured compile/runtime errors because exceptions and RTTI may be disabled. Fail-closed behavior is retained without depending on throws.
- The 53×32 prototype does not match the proven 40×20 renderer. The design prioritizes behavioral parity on 8×16 cells, then treats 6×10 as a measured later phase.

### What warrants a second pair of eyes

- Review the proposed narrow reference payload (`TypeId + uint32_t handle`) against the first real product.
- Verify whether action families and composed relations are needed in the initial milestone or can be delayed.
- Review unavailable-command discovery policy and the exact Esc ladder before implementation freezes habits.

### What should be done in the future

- Scaffold 0104 and the host CMake harness, then translate type graph tests before implementing device UI code.
- Decide whether vocabulary IDs are generated from a shared schema or maintained as checked constexpr tables.

### Code review instructions

- Start at design doc §§6–9 for semantic lifetimes, resolver behavior, and shell integration.
- Validate the implementation plan against §§14–16 and compare each translated suite with its TypeScript source.
- Run `docmgr doctor --ticket PBUI-HANDHELD-1 --stale-after 30` from the `pbui` repository.

### Technical details

- Proposed layers: `pbui_core`, `pbui_handheld`, `pbui_rows`.
- Proposed runtime order: keyboard queue → pure reducer/core → product router → row composition → dirty row blits.
- Proposed baseline geometry: 40×20 at 8×16, with two 320×16 RGB565 DMA buffers.
