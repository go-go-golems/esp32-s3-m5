---
Title: Native C++ PBUI subset for the ESP32-P4 PicoCalc
Ticket: PBUI-HANDHELD-1
Status: active
Topics:
    - pbui
    - embedded
    - architecture
    - design
    - onboarding
    - research
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: abs:///home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/picocalc_lcd/include/picocalc_lcd.h
      Note: LCD row-blit boundary and geometry
    - Path: abs:///home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/visual_repl/include/visual_repl.h
      Note: Proven 40x20 renderer and dump API used as the UI-framework baseline
    - Path: abs:///home/manuel/workspaces/2026-09-01/add-plot-editor/pbui/src/presentation/actions/resolve.ts
      Note: Semantic resolver and precedence contract translated into the native design
    - Path: abs:///home/manuel/workspaces/2026-09-01/add-plot-editor/pbui/src/presentation/interaction/accept.ts
      Note: Pure accept state machine and fuzzed invariants
    - Path: abs:///home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/09/04/PBUI-HANDHELD-1--conceptual-port-of-pbui-to-an-embedded-esp32-p4-handheld-core-engines-and-keyboard-navigation/sources/pbui-handheld.jsx
      Note: Keyboard reducer and presentation-bearing line model specification
ExternalSources: []
Summary: Native C++ architecture and implementation plan for porting PBUI's semantic kernel, keyboard shell, and row-based LCD UI to the 32 MB ESP32-P4 PicoCalc.
LastUpdated: 2026-09-04T13:52:20.114888608-04:00
WhatFor: Use to implement the native C++ PBUI subset and keyboard-only PicoCalc firmware project 0104 without carrying React or QuickJS.
WhenToUse: Read before creating the firmware project, translating PBUI engine tests, defining a product declaration, or implementing keyboard and LCD behavior.
---


# Native C++ PBUI subset for the ESP32-P4 PicoCalc

> This document complements `01-pbui-handheld-port-analysis-design-and-implementation-guide.md`. That guide recommends running the existing TypeScript kernel under QuickJS. This document deliberately explores and specifies the other route requested by the handoff: a native C++20 subset, preserving PBUI behavior while omitting React, DOM, pointer interaction, and initially the workbench tree and link kernel.

## 1. Executive summary

Build `0104-esp32-p4-pbui-handheld` as an ESP-IDF C++ application with three separately testable layers:

1. **`pbui_core`**, a platform-independent C++ library implementing the closed nominal type graph, shared selector, four-operation condition algebra, four-state availability, action resolution, fresh perform checks, typed acceptance, the accept state machine, and the Enter activation ladder.
2. **`pbui_handheld`**, a pure reducer implementing the PBUI/HB keyboard shell: navigation, command completion, accept slots, hints, search, menus, overview, tray pronouns, histories, and the Esc ladder. It consumes `pbui_core`; it does not duplicate action or acceptance policy.
3. **`pbui_rows`**, a retained 40×20 cell UI framework derived from `visual_repl`. Product views emit rows containing styled segments and an optional typed presentation. A compositor adds chrome, caret, type-tone bars, accept lighting, overlays, and dirty-row detection before the LCD adapter calls `picocalc_lcd_blit_row`.

The product supplies one compiled declaration and four narrow adapters: immutable facts, `catalog(types)`, `describe(reference) -> rows`, and `execute(verb)`. The engine owns selection; the product owns facts and effects. This keeps the PBUI invariant intact: a rendered menu is not authority, conditions alone do not authorize an operation, and execution always re-resolves against a fresh snapshot and requires the same candidate to win.

The ESP32-P4's 32 MB PSRAM is ample, but the design avoids dynamic allocation in the hot key/render path. Declaration data is compiled once at boot into bounded vectors and interned string IDs; transient resolution uses reusable scratch storage. DMA row buffers remain internal RAM. The initial deck is flat, one full-screen card at a time. `workbench-core` trees, links/wiring, dynamic plugin loading, arbitrary condition expressions, proportional fonts, and networked products are deferred.

The behavioral contract is not a fresh C++ interpretation of the prose. Translate the TypeScript tests alongside each engine, including permutation and fuzz/property tests, and replay the six tutorials from `pbui-handheld-manual.md` as golden key scripts against desktop-native C++ screen dumps before flashing hardware.

## 2. Scope and success criteria

### 2.1 In scope

- Native C++ implementations of the PBUI semantic subset named in the handoff.
- A validated, closed product declaration containing types, scopes, predicates, descriptors, action rules, relations, catalog providers, and view descriptors.
- Keyboard-only interaction matching PBUI/HB's documented modes and Esc behavior.
- A compact row/cell UI toolkit for the 320×320 LCD.
- A flat workspace/deck/session model sufficient for multiple full-screen cards.
- Host tests on Linux, firmware tests where hardware matters, text dumps, `/key` script injection, and timing/memory telemetry.
- A first static “grace-period” product proving files, hunks, tasks, memories, context segments, steps, cards, and apps.

### 2.2 Deferred

- React, DOM, browser components, `createPbui.tsx`, chrome, and pointer gestures.
- QuickJS and JavaScript applications in the initial firmware.
- Recursive split layouts and the full `workbench-core` mutation protocol.
- Link visualization and interactive wiring. Acceptance-exposed relations are included; the larger links subsystem is not.
- Runtime-loaded declarations. The first declaration is linked into firmware and compiled at boot.
- Unicode shaping, images, touch, animation beyond low-rate accept highlighting, networking, and persistence.

### 2.3 Success criteria

1. All translated core unit tests pass on the host, including order-independence and state-machine invariants.
2. Manual tutorial key scripts produce stable expected text dumps on the host and through device `/key` injection.
3. A key that changes one row reaches the panel within 50 ms; full-card transitions target 100 ms or less.
4. Resolution and acceptance allocate no heap memory after model compilation in the normal path.
5. A stale action whose candidate changes, disappears, becomes unavailable, or becomes ambiguous never executes.
6. Unknown reference types, parent types, predicate IDs, scopes, relation endpoints, and duplicate IDs fail model compilation rather than silently degrading.
7. One task owns the LCD and one task owns keyboard I2C; exactly one host process owns `/dev/ttyACM0`.

## 3. Behavioral source of truth

The port has two specifications:

- **Semantic specification:** headers and tests in `src/presentation`. In particular, `actions/resolve.ts` defines precedence as smallest type distance, nearest active scope, highest priority, then ambiguity as data. `availability.ts` distinguishes `inapplicable` (withdraw, allowing fallback) from `hidden` (compete, suppress fallback, render no row). `perform.ts` requires candidate identity to survive fresh resolution.
- **Shell specification:** `sources/pbui-handheld.jsx` and the tutorials in `sources/pbui-handheld-manual.md`. The reducer and manual define modes, key behavior, command completion, lit targets, tray pronouns, histories, deck navigation, and chrome.

The previous design guide and project report explain intent and tradeoffs, but where prose and executable tests differ, tests win. Record intentional divergences as explicit compatibility decisions and change the golden scripts.

## 4. Current-state evidence and implications

### 4.1 PBUI core

The type graph is closed, validates duplicate IDs, unknown parents and cycles, and caches breadth-first ancestor lists. Its only semantic queries are reflexive `isSubtype` and shortest `distance`. This should remain a small utility, not become a payload converter or RTTI hierarchy.

`matchSelector` is shared by actions, help, and relations. It processes type, then nearest active scope, then condition, returning provenance (`declaredType`, concrete type, distance, scope/index, priority) or a staged rejection. The action resolver intentionally evaluates a rule's condition as availability rather than selector rejection because hidden and unavailable candidates must suppress generic fallbacks.

The action resolver partitions by action ID only after candidate collection and availability evaluation. Registration order, labels, and menu ordering are forbidden tie-breakers. Binding happens only for the unique available winner. These details must be reflected in C++ data ownership: a candidate may point at a binding factory, but a concrete executable verb should only be produced after selection.

Acceptance first checks direct subtyping without converting the reference, then considers only relations exposed to acceptance. Relation ties are reduced by nearest scope and priority; unresolved ties become chooser data. The accept flow and activation ladder are already pure state machines and map directly to `std::variant` states/events/outcomes.

### 4.2 Hardware and reusable firmware

`picocalc_lcd` exposes 320×320 RGB565 fills, rectangles, and row blits. Existing measurements summarized in the first design document are about 1.8 ms per 8×16 row and roughly 46 ms for a 40×20 screen. This supports dirty row repainting with occasional full-screen card redraws.

`picocalc_keyboard` reads the STM32 southbridge at I2C address `0x1F`, reports pressed/repeated/released states, and names arrow, function, navigation, and printable ASCII keys. Release events make hold-to-peek possible. `0102/.../app_main.cpp:657-679` already translates key codes into stable tokens; retain that boundary rather than passing device scan codes into the shell.

`visual_repl` proves a 40×20, 8×16 cell renderer and text dump. It is not itself a general UI model: it has six transcript styles and mutable history/input concepts. Reuse glyph rendering and row blits, but create `pbui_rows` with retained cells, presentation-bearing rows, overlays, and dirty comparison.

### 4.3 Device constraints

The P4 has limited internal SRAM and 32 MB PSRAM. Pixel DMA buffers must remain internal/DMA-capable. Semantic state, declaration arrays, labels, deck state, histories, and row models can live in PSRAM, but predictable latency still argues for bounded structures and reuse. A full 320×320 RGB565 framebuffer costs 204,800 bytes; two cost 409,600 bytes. The proposed renderer needs only two 320×16 RGB565 row buffers (20,480 bytes) plus compact cell planes, avoiding framebuffer pressure and allowing queued or synchronous row blits.

## 5. Proposed architecture

```text
keyboard/I2C task
  -> KeyEvent queue (pressed/repeat/released + KeyToken)
  -> UI owner task
       -> reduce(HandheldState, KeyEvent, RuntimeView)
       -> resolve/accept/evaluateFresh in pbui_core
       -> product execute(Verb) -> new immutable facts + revision
       -> product describe/catalog -> ScreenDocument rows
       -> compose chrome + overlays -> CellFrame
       -> dirty row list
       -> picocalc_lcd_blit_row

serial console
  -> /key TOKEN and /keys script -> same KeyEvent queue
  <- /dump, /state, /resolve, /mem, /timing
```

Only the UI owner mutates shell state, product state, and retained frames. The keyboard task performs I2C polling and enqueues normalized events. The initial renderer executes synchronously on the UI task because the row blit cost is already measured and safe; a later display-owner task may consume immutable rendered rows if profiling proves it useful. No engine function touches FreeRTOS, LCD, I2C, serial, or wall-clock time.

### 5.1 Suggested project layout

```text
0104-esp32-p4-pbui-handheld/
  CMakeLists.txt
  sdkconfig.defaults                  # copied from 0102, QuickJS options removed
  main/
    app_main.cpp                      # tasks, queues, console commands
    keymap.cpp                        # scan code -> KeyToken
    demo_product.cpp                  # linked declaration and grace-period world
  components/
    pbui_core/
      include/pbui/{ids,reference,type_graph,condition,selector,
                    availability,action,resolve,perform,relation,
                    acceptance,accept_machine,activation,model}.hpp
      src/*.cpp
    pbui_handheld/
      include/pbui_handheld/{state,event,reducer,catalog,history,deck}.hpp
      src/*.cpp
    pbui_rows/
      include/pbui_rows/{style,cell,row,document,layout,compose,renderer}.hpp
      src/*.cpp
    picocalc_lcd/                     # reuse shared component
    picocalc_keyboard/                # reuse shared component
  test/                               # host-native CMake target
    core/*_test.cpp
    handheld/*_test.cpp
    golden/*.keys
    golden/*.screen
```

Prefer symlinking or `EXTRA_COMPONENT_DIRS` to copying shared hardware components if the repository conventions permit it. Do not fork the LCD and keyboard drivers inside 0104.

## 6. Native core data model

### 6.1 IDs, values, and references

Use interned 16-bit IDs for declaration vocabulary and stable 32-bit or 64-bit product object handles for values. Avoid `std::any`, RTTI, ownership-bearing pointers in references, and pervasive strings.

```cpp
using TypeId = uint16_t;
using ScopeId = uint16_t;
using ActionId = uint16_t;
using PredicateId = uint16_t;
using RelationId = uint16_t;
using CandidateId = uint32_t;

struct Reference {
  TypeId type;
  uint32_t value;       // product-managed stable handle
  friend bool operator==(const Reference&, const Reference&) = default;
};
```

The product resolves a handle through typed accessors. An inherited rule sees the concrete `Reference`; subtyping never rewrites it. Debug builds retain reverse ID-to-string tables for dumps and diagnostics. Candidate identity should be deterministic from contribution ID plus family instance key, not an address.

If a first product truly needs values larger than 32 bits, replace `value` with a fixed tagged payload (`uint64_t`) while keeping references trivially copyable. Do not introduce a heap-owning variant until evidence requires it.

### 6.2 Snapshots

```cpp
struct SelectionSnapshot {
  Revision revision;
  std::span<const ScopeId> scopes;       // inner -> outer
  BitSet modes;
  BitSet capabilities;
  const ProductFacts* product;           // immutable for one resolve
};
```

The product increments or recomputes `revision` whenever a fact relevant to resolution changes. The snapshot borrows immutable product facts for the duration of a synchronous call. It must never escape into a menu row or verb. Resolved actions retain revision and candidate identity, not pointers into old facts.

### 6.3 Errors

Model construction returns a structured `CompileResult` with diagnostics suitable for serial output, then firmware boot fails into a diagnostic screen if any error exists. Runtime closed-world violations are programmer faults: return an error result in release builds and assert in host/debug builds. User-level ambiguity, unavailable actions, and acceptance failure are ordinary result variants, never exceptions.

ESP-IDF often disables C++ exceptions and RTTI. The core therefore uses `Result<T, Error>`/`std::variant`, not throws. The translation must preserve TypeScript's fail-closed behavior even though the mechanism changes.

## 7. Engine specifications and C++ APIs

### 7.1 Type graph

```cpp
struct TypeDef { TypeId id; std::span<const TypeId> parents; bool abstract; };

class TypeGraph {
 public:
  static Result<TypeGraph, Diagnostics> compile(std::span<const TypeDef>);
  bool has(TypeId) const;
  bool is_abstract(TypeId) const;
  bool is_subtype(TypeId type, TypeId supertype) const;
  std::optional<uint16_t> distance(TypeId type, TypeId supertype) const;
  std::span<const Ancestor> ancestors(TypeId) const;
};
```

Compile steps: reject duplicates; reject undeclared parents; iterative DFS color check for cycles; BFS from each type to produce shortest ancestor distances in deterministic parent declaration order. With a small closed universe, precomputing all ancestor lists is preferable to lazy caches and locks.

### 7.2 Conditions and availability

```cpp
struct Available {};
struct Unavailable { StringId because; StringId code; };
struct Inapplicable { enum Reason { NotRelevant, NotApplicable } because; };
struct Hidden { enum Reason { NotDisclosed, Policy } because; };
using Availability = std::variant<Available, Unavailable, Inapplicable, Hidden>;

enum class ConditionOp : uint8_t { All, Mode, Capability, Predicate };
struct ConditionNode { ConditionOp op; uint16_t a; uint16_t b; Availability on_fail; };
```

Represent condition trees as flat immutable node arrays with child ranges, avoiding recursive ownership. `all` evaluates children in declaration order and returns the first non-available state. Only named predicates receive `ProductFacts`; mode and capability nodes read snapshot bitsets. Unknown operations and predicate IDs are compile errors and runtime failures in defense depth.

### 7.3 Shared selector

```cpp
struct SelectorMatch {
  std::optional<TypeId> declared_type;
  TypeId concrete_type;
  uint16_t type_distance;
  std::optional<ScopeId> scope;
  std::optional<uint16_t> scope_index;
  int16_t priority;
};
struct SelectorReject { enum Stage { Type, Scope, Condition } stage; StringId reason; };
using SelectorResult = std::variant<SelectorMatch, SelectorReject, RuntimeError>;
```

Preserve stage order: type, scope, condition. The nearest active scope is the lowest index in the snapshot's inner-to-outer stack among scopes declared by the selector. Return provenance, not a bool. Actions call the common selector without the `when` condition and evaluate action conditions as statuses; relations may use the complete selector.

### 7.4 Action declarations and resolver

Use non-owning function pointers plus an opaque product context rather than `std::function`, which may allocate:

```cpp
struct RuleContext { Reference subject; const SelectionSnapshot& snapshot; };
using TestFn = Availability (*)(const RuleContext&, const void* rule_data);
using BindFn = Verb (*)(const RuleContext&, const void* rule_data);

struct ActionRule {
  ContributionId contribution;
  ActionId action;
  Selector selector;
  InvocationMask invocations;
  TestFn test;
  BindFn bind;
  const void* data;
  ActionMetadata metadata;
};
```

Resolution algorithm per query:

1. Validate subject type is declared.
2. For each contribution, check type reach. Type-unreachable rules emit no trace.
3. Check invocation before handling scope rejection, preserving trace semantics.
4. Require an active declared scope.
5. Evaluate `when`, then `test` only if still available.
6. Drop `inapplicable`; retain available, unavailable, and hidden candidates.
7. Expand bounded families, rejecting duplicate instance keys.
8. Partition candidates by action ID.
9. Keep minimum type distance.
10. Keep minimum scope index.
11. Keep maximum priority.
12. If multiple remain, emit sorted ambiguity data and select none.
13. Record the winner; hidden winners produce trace but no action row.
14. Bind only an available unique winner.
15. Sort visible rows by group, order, label, action strictly for presentation.

Families should initially have a compile-time maximum expansion count (for example 32) and write into caller-provided scratch storage. Report overflow as a structured resolver error; never truncate silently.

Trace collection is optional per call via a bounded sink. Normal menu resolution can disable it; `/resolve` and tests enable it. Selection and trace still share the same branches, avoiding a second diagnostic resolver.

### 7.5 Fresh perform

A menu row stores `{action_id, candidate_id, original_query, snapshot_revision, registry_version}` plus display metadata. On Enter:

```cpp
PerformDecision perform_fresh(const ResolvedAction& stale) {
  auto snapshot = product.snapshot();
  auto fresh = model.actions.resolve(stale.query, snapshot);
  auto decision = evaluate_fresh(stale, fresh);
  if (auto* proceed = std::get_if<Proceed>(&decision))
    return product.execute(proceed->verb); // authorization remains here
  return decision;
}
```

Refuse on ambiguity, no current row, changed candidate, or unavailable winner. Do not accept action-ID equality as sufficient. `product.execute` must validate state again because fresh resolve is UX safety, not authorization.

### 7.6 Relations and acceptance

The subset needs explicitly declared one-step relations exposed to acceptance. Each relation has source selector, declared codomain, exposure bits, and an apply function returning zero or one reference. Do not infer graph paths or chain relations in Phase 1.

Acceptance order:

1. If offered reference subtype-reaches a requested type, apply filter and preserve the same reference.
2. Otherwise discover only acceptance-exposed relations whose declared codomain reaches a wanted type.
3. Apply candidates; validate concrete output type and filter.
4. Reduce by nearest scope then highest priority.
5. Return accepted, none, or sorted ambiguous options.

### 7.7 Accept machine and activation ladder

Translate the TypeScript discriminated unions to variants. State is `Idle`, `Pending`, or `Choosing`; effects are `CloseMenu`, `Settle`, or `ResolveNull`. Exactly one reducer function handles all transitions. The promise concept from React is unnecessary: the device associates `request_id` with a command continuation and consumes terminal effects synchronously or through a small pending operation table.

Enter on a presentation follows exactly this order:

1. attempt acceptance if a request is pending and the object fits;
2. invoke a host activation callback if the row explicitly owns one;
3. perform exactly one available primary action;
4. otherwise open the action menu.

For the native shell, “bubble” means return `UnhandledByPresentation` so the enclosing view may consume Enter. It is not DOM propagation.

## 8. Compiled product declaration

A native declaration should look declarative even though C++ lacks TypeScript's generic value map:

```cpp
constexpr TypeDef kTypes[] = { inspectable, file, hunk, task, mem, ctxseg, step, card, app };
constexpr ScopeId kScopes[] = { scope_view, scope_product, scope_global };
constexpr Descriptor kDescriptors[] = { /* label, describe, tone */ };
constexpr ActionRule kRules[] = { /* selectors, status, bind */ };
constexpr Relation kRelations[] = { /* source -> target, exposure */ };
constexpr ViewDef kViews[] = { /* title, root rows, catalog provider */ };

const PresentationDeclaration kDemo {
  .id = sid("grace-period.presentation"),
  .types = kTypes,
  .known_scopes = kScopes,
  .default_active_scopes = kDefaultScopes,
  .predicates = kPredicates,
  .descriptors = kDescriptors,
  .actions = kRules,
  .relations = kRelations,
  .version = 1,
};
```

Compilation validates all cross-references and builds graph tables, predicate lookup, action partitions, and relation indexes. Prefer generated numeric IDs from one checked vocabulary header once the demo stabilizes. Until then, a constexpr string hash is acceptable only if compilation detects collisions and reverse maps retain original names.

Descriptors are narrowed for the device:

```cpp
struct Descriptor {
  TypeId type;
  void (*label)(Reference, const ProductFacts&, TextBuffer&);
  void (*describe)(Reference, const ProductFacts&, RowSink&);
  Tone tone;
};
```

The two explicit shell extensions are:

- `Catalog::enumerate(span<TypeId>, facts, ReferenceSink&)`: all live references eligible for off-screen completion.
- `Descriptor::describe`: presentation-bearing rows for inspectors.

Catalog order must be deterministic and documented. It affects cycling and default display but must not break semantic ties.

## 9. Keyboard shell

### 9.1 State and events

```cpp
enum class Mode { Nav, Cmd, Accept, Hint, Search, Menu, Overview, Help, Peek };
struct HandheldState {
  Mode mode;
  DeckSession session;
  Caret caret;
  SmallVector<Reference, 16> tray;
  FlatMap<TypeId, Reference> history;
  AcceptState accept;
  CommandEditor command;
  OverlayState overlay;
  std::optional<RepeatRecord> last_command;
  uint64_t ui_revision;
};

struct KeyEvent { KeyToken token; KeyState state; char printable; };
ReduceResult reduce(const HandheldState&, const KeyEvent&, Runtime&);
```

All user-visible policy resides in `reduce` and mode-specific helpers. Hardware translation is outside. Press/repeat events navigate or edit; released events matter for quasimodes such as hold-`i` peek. Ignore repeats for destructive confirmation and one-shot mode switches unless specifically allowed.

### 9.2 Navigation and screen index

Every composed `ScreenDocument` yields a screen index in reading order:

```cpp
struct PresentationLocation { Reference ref; uint8_t row; Rect cell_bounds; };
```

The caret stores identity where possible, not merely a row number. After rerender, preserve the same reference; if absent, choose the nearest previous reading-order position, then first object, then no caret. Up/down walks object rows only. Hints assign deterministic labels to visible locations. Search filters visible labels. Typed cycling filters by type.

### 9.3 Command mode and `availCmds`

Command completion is computed from action resolution, not a second command/type table:

1. Collect unique visible references from the screen index.
2. Add the current card reference as an implicit visible object.
3. Resolve `invocation=introspection` or a dedicated shell discovery query for each reference.
4. Union action IDs with available or unavailable visible rows; do not expose hidden winners.
5. Add nullary shell actions whose explicit availability passes.
6. Apply tray-dependent conditions through normal product facts/predicates.

Typing a full known verb remains legal even if discovery does not offer it; execution still requires a typed receiver and normal resolution. Prefix completion sorts only for display. Ambiguous prefixes are refused with candidates.

### 9.4 Accept slot

Entering a unary command creates an `AcceptRequest`. Candidate sources are composed in this order:

- visible acceptable references (for digit chips);
- the current caret as `it`, if acceptable;
- tray `$n`, type-checked through `model.accept`;
- catalog references, each checked through `model.accept` so relations work;
- defaults: tray-specific newest, caret, current card for card actions, then per-type history.

Deduplicate by `(type,value)` after acceptance output; preserve provenance when two relations produce distinct meaningful choices and open the chooser rather than collapsing semantic ambiguity.

Digits choose lit visible targets only while the input buffer is empty. `$n` and `it` are parsed inside accept mode. Typed text filters labels case-insensitively. Tab cycles. Enter accepts the selected candidate or printed default. Escape closes a relation chooser first, then aborts the pending slot and returns to command/nav as specified.

### 9.5 Menus, overview, help, and Esc

- **Menu:** rows come directly from `resolve(menu)`, including unavailable rows with reasons. Hidden rows are absent. Ambiguities appear in a diagnostic footer and cannot execute.
- **Overview:** a product view of workspace/card references, not a privileged renderer. Enter therefore uses the same activation ladder.
- **Help:** initially static rows plus resolved action/refusal text. A future additive help interpreter can be ported without changing the row renderer.
- **Esc ladder:** dismiss chooser → close menu/hint/search/help/peek → abort accept → clear command mode → pop inspector → remain in Nav. Encode this as reducer tests, not nested widget callbacks.

### 9.6 Flat deck

Start with:

```cpp
struct Card { CardId id; AppId app; SmallVector<Reference, 8> breadcrumbs; };
struct Workspace { WorkspaceId id; SmallVector<Card, 12> cards; uint8_t active; };
struct DeckSession { SmallVector<Workspace, 6> spaces; uint8_t workspace; };
```

Commands include create card after current, close unless last card, activate card, select workspace, and push/pop inspector references. Cards and apps are presentation references with normal descriptors/actions/catalog entries. If the complete workbench tree is later needed, replace the deck backend behind these commands rather than changing shell modes.

## 10. Row-based UI framework

### 10.1 Logical model

The physical baseline is 40 columns × 20 rows at 8×16. Keep one status row, one title row, 16 content rows, one tray row, and one documentation row. The 53×32 prototype cannot be copied literally without introducing a 6×10 font; first preserve behavior, not density. A later font experiment can change layout constants without changing presentation semantics.

```cpp
struct TextStyle { Color fg, bg; bool bold, dim, inverse, underline; };
struct Segment { StringView text; StyleId style; };
struct Row {
  SmallVector<Segment, 8> segments;
  std::optional<Reference> presentation;
  Tone tone;
  RowRole role;
  uint32_t semantic_hash;
};
struct ScreenDocument { std::array<Row, 20> rows; };
```

Product code emits content rows only. The shell owns status/title/tray/doc rows and overlays. Layout truncates on cell boundaries with a visible ellipsis; it never permits a UTF-8 byte split. Phase 1 may constrain display strings to printable ASCII plus a small glyph table.

### 10.2 Composition order

For each row:

1. base background and product segments;
2. type-tone bar for presentation rows;
3. caret fill/outline;
4. accept-compatible lighting and digit chip;
5. hint label;
6. menu/peek/help overlay;
7. chrome rows above all content.

Interaction metadata remains attached through layout; decoration must not create fake presentation rows. Overlays explicitly define whether underlying rows remain addressable (accept lighting may show through; a modal menu captures keys).

### 10.3 Rendering and dirty rows

Maintain previous and next cell frames. Hash the final cells and presentation decorations per row. Rasterize and blit only changed rows. Use two internal-RAM DMA-capable buffers, never PSRAM buffers for SPI DMA. A simple synchronous sequence is sufficient initially:

```cpp
for (row : dirty_rows) {
  auto* buf = row_buffers.acquire();
  rasterize(frame[row], buf, 320, 16);
  ESP_ERROR_CHECK(picocalc_lcd_blit_row(row * 16, 16, buf, 320 * 16));
}
```

Animation should be state-based and low rate (for example 4 Hz accept pulse), disabled when no lit rows exist. Do not run a general 60 FPS loop.

### 10.4 Test and diagnostic surfaces

`pbui_rows::dump_text` emits 20 fixed-width lines plus an optional sidecar mapping row to `<type:value>`, caret, acceptable index, and style role. `/dump` prints it. `/resolve TYPE VALUE` prints action rows, ambiguities, and trace. `/mem` reports internal heap, PSRAM, scratch high-water marks, row-buffer use, and largest free block. `/timing` reports key-to-render, resolve, compose, raster, and blit timings.

## 11. Tasking, ownership, and lifecycle

### 11.1 Tasks

- **UI owner task:** owns product model/state, compiled presentation, shell reducer, frames, and LCD calls. Suggested stack 16–24 KB, pin only after measurement.
- **Keyboard task:** owns keyboard polling and enqueues normalized events. It also serializes future backlight/battery I2C operations or uses the driver's existing lock.
- **Console task:** parses slash commands and injects events into the same queue; it never calls the reducer directly.

Use a bounded event queue. If it fills, coalesce repeated navigation keys but never drop pressed/released transitions required by a held quasimode. Expose overflow counts.

### 11.2 Boot

1. Initialize logging and memory diagnostics.
2. Initialize LCD and show a minimal boot row.
3. Initialize keyboard.
4. Compile the linked presentation declaration.
5. On diagnostics, show a boot failure screen and keep console alive.
6. Construct demo facts, deck, shell state, and first snapshot.
7. Render full screen.
8. Start polling and console injection.

### 11.3 Serial ownership

Follow the established rule: flash first, then run one monitor in one named tmux pane. All other observation uses `tmux capture-pane -p`. Never start a second `idf.py monitor`, `screen`, or serial reader against `/dev/ttyACM0`. Automated replay sends `/keys` through the existing monitor/session or takes ownership only after it is stopped.

## 12. Memory plan

Initial budgets are guardrails to verify, not promises:

| Area | Target | Placement |
|---|---:|---|
| Two 320×16 RGB565 row buffers | 20 KB | internal DMA RAM |
| Current/next 40×20 cell frames | <32 KB | internal or PSRAM |
| Task stacks and queues | <96 KB | internal RAM |
| Compiled declarations/indexes | <256 KB | PSRAM/flash-backed constants |
| Product world and deck | <1 MB | PSRAM |
| Resolver/family/accept scratch | <128 KB | preallocated PSRAM |
| Labels, rows, logs, diagnostics | <1 MB | PSRAM |
| Contingency | >24 MB remains | PSRAM |

Use fixed-capacity vectors (`etl`, `inplace_vector` if available, or a small local implementation) in state and hot paths. Standard containers are acceptable during model compilation if allocated explicitly in PSRAM and frozen afterward. Make overflow visible and tested. Avoid global `new` routing changes until allocation telemetry proves necessary; ESP-IDF capability allocators should be wrapped in explicit arenas.

## 13. Decision records

### Decision 1: Native C++ semantic kernel, not QuickJS

- **Context:** The earlier guide chose reuse through QuickJS; this handoff asks for implementing/porting PBUI to C++ and permits a subset.
- **Options considered:** Bundle the existing TypeScript in QuickJS; native C++; hybrid C++ shell with JS semantics.
- **Decision:** Implement the specified semantic subset in platform-independent C++ and omit QuickJS from 0104 initially.
- **Rationale:** It yields deterministic ownership, smaller runtime surface, direct host/device tests, and a firmware architecture suitable for products that do not want JS. Translating tests controls semantic drift.
- **Consequences:** More implementation effort and permanent conformance responsibility. The TS implementation remains the oracle; differential fixtures should be retained.
- **Status:** proposed.

### Decision 2: Preserve semantics, narrow extensibility

- **Context:** PBUI's semantics are small, but its generic TypeScript APIs and React labels are not appropriate on firmware.
- **Options considered:** Literal API port; ad hoc product-specific switches; typed fixed declaration plus callbacks.
- **Decision:** Preserve result variants and precedence while narrowing IDs, values, labels, callbacks, family bounds, and relation composition.
- **Rationale:** Behavioral compatibility matters more than source-level resemblance. Fixed representations improve memory predictability.
- **Consequences:** Dynamic plugins and arbitrary value payloads are deferred; new needs require explicit API evolution.
- **Status:** proposed.

### Decision 3: One owner task for semantic state and rendering

- **Context:** Concurrent mutation would make snapshots, stale action checks, caret repair, and dirty frames difficult to reason about.
- **Options considered:** Locks around shared state; event-sourced multi-task actors; one UI owner.
- **Decision:** Normalize external input into a queue and perform reduce, execute, compose, and initial synchronous blits on one UI task.
- **Rationale:** This provides a total event order and avoids locks in the semantic core.
- **Consequences:** Long product effects must be asynchronous and return completion events. A display task may be added behind immutable row messages.
- **Status:** proposed.

### Decision 4: Retained row/cell IR rather than LVGL

- **Context:** The UI is text-dense, keyboard-only, and every object occupies a logical row; `visual_repl` already proves row blits and dumps.
- **Options considered:** LVGL widgets; immediate custom drawing; retained row/cell model.
- **Decision:** Build `pbui_rows` as a retained row/cell compositor derived from `visual_repl` primitives.
- **Rationale:** Presentation metadata, golden text dumps, and dirty-row repainting are natural in this model. LVGL would add focus/widget policy that the shell already defines.
- **Consequences:** Rich graphics and proportional layout require later extensions. Accessibility is through semantic dumps rather than a widget tree.
- **Status:** proposed.

### Decision 5: 40×20 baseline before a 6×10 font

- **Context:** The prototype assumes 53×32; proven firmware uses 40×20.
- **Options considered:** Implement 6×10 first; preserve 8×16; runtime-selectable fonts immediately.
- **Decision:** Ship behavior on 40×20 first, then evaluate a 6×10 font.
- **Rationale:** It isolates semantic/shell risk from font legibility and rasterizer risk and reuses measured performance.
- **Consequences:** Fewer content rows and more truncation than the manual screenshots. Golden expectations must target the device layout while preserving tutorial flows.
- **Status:** proposed.

### Decision 6: Flat deck before workbench-core trees

- **Context:** A 320-pixel display presents one useful card at a time, while full workbench-core includes splits, persistence, links, and protocols.
- **Options considered:** Port workbench-core; hard-code screens; a small flat deck command model.
- **Decision:** Implement workspace/card arrays and stable IDs behind command methods.
- **Rationale:** It supports the prototype's actual navigation and keeps migration to a richer backend possible.
- **Consequences:** No split panes, rebalance, or protocol compatibility initially.
- **Status:** proposed.

### Decision 7: Product effects remain an authorization boundary

- **Context:** `evaluateFresh` prevents stale UI actions but cannot prevent state changes after resolution or callers bypassing menus.
- **Options considered:** Treat resolver availability as authorization; execute directly from stale verbs; revalidate then authorize in product router.
- **Decision:** Re-resolve and verify candidate identity, then call a product router that validates the operation again.
- **Rationale:** This preserves PBUI's documented security boundary.
- **Consequences:** Some checks appear both as user-facing predicates and product invariants; tests should ensure reasons stay coherent.
- **Status:** proposed.

## 14. Key flows

### 14.1 One key to pixels

```text
poll_event -> normalize token -> enqueue
UI receives event
  -> reducer dispatches by mode
  -> optional resolver/acceptance call
  -> apply pure state transition and effects
  -> product effect updates facts + revision
  -> build product rows
  -> compose shell/chrome/overlays
  -> repair caret by reference identity
  -> compare row hashes
  -> rasterize/blit dirty rows
  -> record timings
```

### 14.2 Open and execute a menu row

```text
m on caret
  snapshot := product.snapshot()
  menu := resolve({caret.ref, invocation=menu}, snapshot)
  retain display rows + stale action identity; do not retain snapshot pointers
Enter on row
  fresh_snapshot := product.snapshot()
  fresh := resolve(stale.query, fresh_snapshot)
  decision := evaluateFresh(stale, fresh)
  refused -> doc line/toast with reason
  proceed -> product.authorize_and_execute(verb)
  update revision, rerender, close menu
```

### 14.3 Verb-first command and accept

```text
: -> Cmd
letters -> filter offered verbs
space on unique unary verb
  build AcceptRequest from action signature
  dispatch accept request; close menu; mode=Accept
  enumerate visible + tray + catalog references
  call model.accept for each; build lit and completion sets
input changes -> filter labels, recompute lit subset and printed default
Enter
  accepted unique/default -> settle -> resolve action for chosen subject -> fresh perform
  ambiguous relation -> Choosing overlay
  none -> remain pending and show refusal
Escape -> chooser to pending; pending to aborted; then Cmd/Nav
```

### 14.4 Enter ladder

```text
if pending request accepts caret: offer it to accept machine
else if row has host activation: return to view handler
else resolve invocation=primary
     if exactly one available primary: fresh-perform it
     else open menu
```

### 14.5 Caret repair after world mutation

```text
old := caret.reference + old reading index
render new rows and index
if old reference exists: caret = its new index
else if index nonempty: caret = clamp(old reading index)
else: caret = none
update history only on explicit activation, not repair
```

## 15. Implementation phases

### Phase 0: Scaffold and host test harness

- Create `0104-esp32-p4-pbui-handheld`.
- Copy `0102.../sdkconfig.defaults`; remove QuickJS components/options and JS assets.
- Reuse `picocalc_lcd` and `picocalc_keyboard` through component paths.
- Add a top-level host CMake target so `pbui_core`, `pbui_handheld`, and `pbui_rows` compile without ESP-IDF.
- Select a small test framework or implement a dependency-free assertion runner.
- Add CI/local commands for host tests and formatting.

**Exit:** hello screen, keyboard dump, `/key`, `/dump`, and one host golden test.

### Phase 1: Type graph, availability, conditions, selector

Translate implementation and tests in dependency order. Include duplicate, unknown parent, cycle, diamond shortest-distance, closed-world, nearest-scope, condition short-circuit, unknown predicate, and provenance tests.

**Exit:** no ESP headers in core; translated suites pass; compile diagnostics print stable IDs.

### Phase 2: Actions and fresh perform

Implement rules, bounded families, partition/precedence, hidden/inapplicable behavior, ambiguity, optional trace, metadata sorting, candidate identity, and `evaluateFresh`. Translate resolver freeze/permutation and perform tests.

**Exit:** declaration order permutations produce the same selected actions and ambiguities; all stale scenarios refuse correctly.

### Phase 3: Relations, acceptance, and interaction machines

Implement acceptance-exposed single-step relations, direct subtype acceptance, ranking, chooser ambiguity, accept reducer, and activation ladder. Port fuzzed invariants using deterministic random event streams.

**Exit:** acceptance and activation tests pass under sanitizers on host.

### Phase 4: Row framework and LCD adapter

Extract font/raster logic from `visual_repl`, implement cells/rows/segments, presentation metadata, chrome composition, overlays, row hashes, dumps, and dirty blits. Add snapshot tests for clipping, tone bars, caret, unavailable menu rows, accept chips, and overlays.

**Exit:** host SVG/PPM preview and device display match; one-row and full-screen timings are recorded.

### Phase 5: Keyboard reducer

Port modes in slices: Nav and Esc; Menu; Cmd; Accept; Hint/Search; Overview/Help/Peek; tray/history/repeat. Translate reducer-focused tests from the prototype and add released-key tests for peek.

**Exit:** reducer is deterministic and hardware-free; mode matrix and Esc ladder pass.

### Phase 6: Demo declaration and product

Model the grace-period types, scopes, descriptors, actions, predicates, relations, catalog, describe rows, timeline fold, overrides, and flat deck. Replace prototype switches with declaration entries wherever PBUI owns semantics. Preserve product-specific world updates in the product router.

**Exit:** all six manual tutorials are executable as host golden key scripts.

### Phase 7: Device integration and evidence

Wire tasks, normalized keyboard queue, serial commands, memory/timing counters, and boot diagnostics. Flash once, start one tmux monitor, replay scripts with `/keys`, capture with `tmux capture-pane -p`, and compare `/dump` output.

**Exit:** success criteria for latency, memory, stale refusal, and tutorial replay are met on hardware.

### Phase 8: First real product

Choose one bounded product such as SD-card files or PicoOS app/process objects. Define its declaration and product router without changing the core. Exercise asynchronous effects through completion events.

**Exit:** the framework proves reuse beyond the scripted demo.

## 16. Test strategy

### 16.1 Conformance matrix

Translate tests from:

- `actions/typeGraph.test.ts`
- `context/selector.test.ts`
- `actions/conditions.test.ts`
- `actions/resolve.test.ts` and `resolve.freeze.test.ts`
- `actions/perform.test.ts`
- `acceptance/resolve.test.ts`
- `interaction/accept.test.ts`
- `interaction/activation.test.ts`

Maintain fixture names close to TypeScript names and annotate intentional representation changes. Generate common JSON or compact text fixtures where feasible and run them against both implementations to detect drift.

### 16.2 Properties

- Type graph distance is reflexive at zero and implies subtype reachability.
- Resolver output is invariant under contribution permutation.
- A hidden specific winner never leaks a generic fallback.
- An inapplicable specific candidate permits a generic fallback.
- No ambiguous partition binds or executes a verb.
- At most one accept request is pending; each accepted request produces exactly one terminal effect.
- Any displayed action executed after state change passes `evaluateFresh` with the same candidate.
- Any row marked as a presentation appears exactly once in screen reading order.
- Caret always points to a current screen-index entry or is empty.
- Lit targets are a subset of acceptable visible references.

### 16.3 Golden scripts

Convert each manual tutorial into token files, for example:

```text
# tutorial-5-command-line.keys
key :
text rev
key space
key 1
expect-toast reverted
expect-mode nav
dump
```

The runner should support reset, key press/repeat/release, text, time advance, state assertion, dump, and expected refusal. Time is virtual on host so accept pulse and peek behavior are deterministic.

### 16.4 Tooling

Run host tests with compiler warnings as errors, AddressSanitizer and UndefinedBehaviorSanitizer. Add allocation counters around hot-path tests. On target, use heap integrity checks and high-water marks after every tutorial. Fuzz state machines on host, not on the device.

## 17. Risks and mitigations

| Risk | Symptom | Mitigation |
|---|---|---|
| Semantic drift from TypeScript | Different winner or fallback behavior | Translate tests first; differential fixtures; preserve header algorithms literally |
| C++ callback lifetime bugs | stale pointers after resolve | freeze declaration for firmware lifetime; never retain snapshot/product pointers in rows |
| Heap fragmentation in PSRAM | latency grows after navigation | bounded vectors, arenas, allocation counters, resettable scratch |
| ID collision or unstable IDs | wrong descriptor/action selected | generated IDs or checked intern table; collision validation |
| Too few cells at 40×20 | labels and doc line truncate | concise descriptors, horizontal policy, later 6×10 experiment |
| Renderer policy leaks into product | inconsistent caret/lights | product emits rows; compositor alone owns interaction decoration |
| Repeat/release event loss | stuck peek or repeated destructive action | bounded queue policy, transition counters, timeout recovery for held modes |
| Long product effect blocks UI | keyboard lag | asynchronous product command and completion event |
| Hidden action leaks via discovery | secret generic verb shown | discover through resolver output only; hidden-winner tests |
| Family overflow | missing actions | structured error and diagnostic row; never truncate |
| Serial contention | corrupt monitor/failed flashing | flash then one tmux monitor; capture pane only |

## 18. Alternatives considered

### Existing TypeScript under QuickJS

This minimizes semantic porting and was the recommendation in the first guide. It remains the fastest proof path and a useful oracle. It is not selected here because the requested design is explicitly for a C++ port/subset. If delivery schedule dominates native ownership, revisit Decision 1 rather than producing a rushed, weakly tested C++ resolver.

### Product-specific command switches

The prototype's `CMDS`, `labelFor`, and `runCmd` could be copied into C++. This is simpler initially but loses the one-declaration property, scope/type precedence, hidden fallback suppression, acceptance relations, provenance, and fresh candidate identity. It would be a port of the demo rather than PBUI.

### LVGL

LVGL provides widgets, focus groups, styling, and drawing, but PBUI/HB's fundamental unit is a semantic row carrying a presentation. A row compositor is smaller, easier to dump, and already matched to measured hardware. LVGL remains suitable if future products become graphical rather than line-oriented.

### Full framebuffer

A framebuffer simplifies arbitrary composition but costs 200 KB per RGB565 buffer and still needs dirty transfer. The line renderer matches current content and keeps DMA use bounded. Add a framebuffer only if overlays or graphics become sufficiently complex.

### Full workbench-core port

It would provide validated trees and commands, but includes capabilities the first device cannot display and expands scope substantially. The flat deck keeps stable command concepts and can be replaced behind an interface.

## 19. Open questions

1. Should IDs be generated from a checked YAML/JSON vocabulary shared with TypeScript, enabling exact cross-language fixtures, or maintained in C++ constexpr tables?
2. Does the first release need action families, or can the implementation and tests land while product declarations use only fixed rules?
3. Is one-step acceptance relation support enough for the first real product, or does an existing PBUI fixture require named compositions?
4. Which test framework is already acceptable in this ESP-IDF repository for host-native C++?
5. Is 40×20 legible and sufficient in physical testing, or should 6×10 be brought forward after Phase 4?
6. Should unavailable commands appear in verb-first completion when a visible receiver resolves them, or only in object menus? The resolver can support either; the doc-line contract should make the policy explicit.
7. Which first real product best validates reuse: SD-card files, PicoOS apps/processes, battery/backlight objects, or a small plot editor?
8. How should dangling tray references be represented by the product handle table: tombstones with labels, or missing values rendered generically?

## 20. Review instructions

Start with the semantic boundary (§6–§8), especially the distinction between selector rejection and action availability, candidate identity in fresh perform, and borrowed snapshot lifetimes. Then review the shell's use of resolver and acceptance (§9): no secondary type-command policy should contradict the compiled declaration. Finally inspect task ownership and row-buffer placement (§10–§12).

Validation commands once Phase 0 exists should be standardized as:

```bash
cmake -S . -B build-host -DPBUI_HOST_TESTS=ON
cmake --build build-host -j
ctest --test-dir build-host --output-on-failure
idf.py build
idf.py -p /dev/ttyACM0 flash
# Then exactly one tmux monitor; use capture-pane for evidence.
```

## 21. References

### PBUI semantic source

- `src/presentation/actions/typeGraph.ts` and `typeGraph.test.ts`
- `src/presentation/context/selector.ts` and `selector.test.ts`
- `src/presentation/actions/conditions.ts`, `availability.ts`, and tests
- `src/presentation/actions/resolve.ts`, `resolve.test.ts`, `resolve.freeze.test.ts`
- `src/presentation/actions/perform.ts` and `perform.test.ts`
- `src/presentation/acceptance/resolve.ts` and `resolve.test.ts`
- `src/presentation/interaction/accept.ts`, `activation.ts`, and tests
- `src/presentation/model/types.ts`
- `packages/pbui-ecommerce/src/presentation/actions.ts`
- `packages/pbui-ecommerce/src/presentation/runtime.tsx`

### Handheld behavior

- `sources/pbui-handheld.jsx`
- `sources/pbui-handheld-manual.md`
- `sources/pbui-handheld-project-report.md`
- `design-doc/01-pbui-handheld-port-analysis-design-and-implementation-guide.md`

### Firmware

- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/picocalc_lcd/`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/picocalc_keyboard/`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/visual_repl/`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp`
