---
Title: PBUI on PicoCalc from first principles - semantic kernel keyboard protocol and LCD architecture
Ticket: PBUI-HANDHELD-1
Status: active
Topics:
    - pbui
    - architecture
    - cpp
    - picocalc
    - esp32-p4
    - design
    - intern-guide
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: abs:///home/manuel/workspaces/2026-09-01/add-plot-editor/pbui/src/presentation/acceptance/resolve.ts
      Note: Direct-first acceptance and relation provenance
    - Path: abs:///home/manuel/workspaces/2026-09-01/add-plot-editor/pbui/src/presentation/actions/resolve.ts
      Note: Lexicographic precedence and four-state competition contract
    - Path: abs:///home/manuel/workspaces/2026-09-01/add-plot-editor/pbui/src/presentation/interaction/accept.ts
      Note: Trusted event reducer requiring native boundary correlation
    - Path: repo://components/picocalc_lcd/picocalc_lcd.c
      Note: Current 40 MHz polling driver and internal staging ownership
    - Path: repo://components/visual_repl/visual_repl.cpp
      Note: Raster mechanics and lowercase glyph limitation
    - Path: repo://ttmp/2026/09/04/PBUI-HANDHELD-1--conceptual-port-of-pbui-to-an-embedded-esp32-p4-handheld-core-engines-and-keyboard-navigation/scripts/01-probe-pbui-contracts.mts
      Note: Actual-source counterexamples supporting review
    - Path: repo://ttmp/2026/09/04/PBUI-HANDHELD-1--conceptual-port-of-pbui-to-an-embedded-esp32-p4-handheld-core-engines-and-keyboard-navigation/scripts/02-selection-algebra.cpp
      Note: Sanitized algebraic selection experiment
ExternalSources: []
Summary: Evidence-backed replacement of the first native proposal, deriving a C++ presentation kernel, command language, interaction state machine, and row UI from order theory, algebra, relational queries, identity, and transactional state transitions.
LastUpdated: 2026-09-04T17:45:00-04:00
WhatFor: Implementation baseline for native firmware 0104; explains fundamentals, reviews earlier decisions, defines contracts and failures, and gives a staged intern implementation plan.
WhenToUse: Read before implementing PBUI on the PicoCalc; this guide supersedes the implementation recommendations of guides 01 and 02 while preserving them as historical analysis.
---


# PBUI on PicoCalc, from first principles

## 0. How to use this guide

This guide specifies a native C++ presentation system for the keyboard-only ESP32-P4 PicoCalc with 32 MB PSRAM. It is not a translation checklist for React components. It derives the required behavior from the existing PBUI semantic engines, the handheld interaction prototype, and the current firmware drivers, then chooses an implementation small enough to understand and test on a desktop.

The ticket originally lived in the pbui repository. It now lives in the firmware repository, `esp32-s3-m5/ttmp/2026/09/04/PBUI-HANDHELD-1--conceptual-port-of-pbui-to-an-embedded-esp32-p4-handheld-core-engines-and-keyboard-navigation`. This is document 03 because two previous design documents already exist; it is the independent review and replacement native design requested after document 02. Neither earlier implementation plan is an additional backlog.

Read §§1–3 for the review and conceptual model, §§4–8 for semantics and native interfaces, §§9–12 for interaction and pixels, and §§13–17 for implementation and verification. Work through the examples before implementing the corresponding module. The source references in §18 distinguish three roots:

- **P**: `/home/manuel/workspaces/2026-09-01/add-plot-editor/pbui`.
- **F**: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5`.
- **T**: this ticket directory under F.

Pseudocode is explicitly instructional unless a file under `T/scripts/` is named. Those scripts are executable research probes, not production components. Source evidence was collected on 2026-09-04. No firmware was built or flashed during this review, and no new on-device latency or heap measurement is claimed.

### Writing and reasoning conventions

A **source fact** names a file or retained experiment. A **design decision** changes or narrows behavior and states why. A **target** is something a later implementation must measure, not an achieved result. Mathematical definitions below are used to derive tests and algorithms, not to imply that the application has been formally verified.

The theoretical tools are deliberately limited: finite directed graphs and partial orders, algebraic data types, an ordered first-failure algebra, lexicographic optimization with explicit ties, relational query planning, hierarchical state machines, stable identity, and serialized transactions. No theorem prover, general logic language, distributed event system, or full widget framework is required.

## 1. Executive decision

Keep the native direction, but replace several important parts of the earlier architecture. Build a **small semantic interpreter plus a keyboard protocol and a row renderer**, not a miniaturized desktop workbench and not a firmware-wide inheritance hierarchy.

The proposed boundaries are:

```text
compiled declaration + immutable turn context
              |
              v
 pbui_core: types / selectors / conditions / resolution / acceptance
              |
              v
 pbui_handheld: command schema / typed slots / focus / surfaces / deck
              |
              v
 pbui_rows: document -> viewport -> interaction frame -> raster
              |
              v
 picocalc_lcd: synchronous copy / byte swap / SPI transfer

keyboard owner -> ordered input queue -> application owner
console owner  -> ordered requests    -> application owner
workers        -> correlated results -> application owner
```

The application owner is the only writer of product state, session state, and interaction state. Queries read a stable turn context. Effects are explicit commands, not closures stored in menu rows. The display adapter is synchronous initially and is called only by this owner. The keyboard task remains independent because I2C reads and recovery may block for much longer than a UI frame.

The first useful vertical slice is much smaller than the complete prototype: a few files and edits, one protected edit, two cards, an inspector, an object menu, `:revert`, a typed argument slot, a tray, and a single acceptance relation. This exercises the important semantic distinctions before anyone ports the complete timeline demo.

### The principal changes from document 02

1. Introduce a **command schema** in the declaration. An action rule is not a command signature; a current card cannot stand in for an app argument to `newtile`.
2. Distinguish **object identity**, **presentation occurrence identity**, and **view identity**. The same object may appear twice with different scopes and positions.
3. Make one **interaction frame** the shared input for rendering and key decoding. Do not compute hints, digits, defaults, and lighting independently.
4. Keep semantic acceptance separate from action executability; keep relation provenance until the user has resolved translation ambiguity.
5. Add request/choice IDs and fresh acceptance validation at the native host boundary. Do not treat the TypeScript accept reducer as an untrusted-input validator.
6. Plan catalog enumeration from requested types **and relation source types**, with an explicit completeness argument.
7. Use a checked, bounded memory policy rather than promising zero allocation everywhere or assuming ordinary `malloc` always returns PSRAM.
8. Start from the **current 40 MHz synchronous LCD driver with its existing 32 KiB internal staging buffer**, not a hypothetical queued 80 MHz path.
9. Port behavior through a declared compatibility profile. Generated families and relation compositions are deferred by rejection, not half-implemented with silently different rules.
10. Replace blanket “byte-identical prototype screens” with semantic tutorial assertions plus layout-specific goldens, because the prototype has bugs and is not a 320-pixel raster implementation.

## 2. What the fresh review established

### 2.1 What was independently inspected

The review read the pure graph, selector, conditions, availability, resolver, freshness function, acceptance resolver, interaction reducers, action registry, model compiler, relation system, and representative tests. It also read the complete 1,133-line JSX prototype, the manual and report, workbench session/close contracts, ecommerce declaration examples, and actual LCD/keyboard/visual REPL implementations and 0102 build files.

The selected baseline ran **307 tests across 11 files**, including 200 seeded accept-machine cases. The old “166 tests” number was not a useful current acceptance criterion. `T/sources/pbui-conformance-baseline.json` is the actual run output. The baseline is only those selected suites, not every PBUI test.

`T/scripts/01-probe-pbui-contracts.mts` imports the actual P modules and demonstrates additional edge cases. `T/scripts/02-selection-algebra.cpp` checks a proposed mathematical selection fold against a sorted oracle for 5,000 generated worlds with eight permutations each, under AddressSanitizer and UndefinedBehaviorSanitizer. These are evidence for specific propositions, not native firmware conformance.

### 2.2 Review findings and disposition

| Earlier assertion or design | Fresh evidence | Disposition |
|---|---|---|
| LCD is an 80 MHz, 1.8 ms/row baseline | `F/components/picocalc_lcd/picocalc_lcd.c:25-34` selects 40 MHz and documents ghosting at 80 MHz | Keep 40 MHz; older 80 MHz timings are historical, not a current SLA |
| Two DMA row buffers allow overlap through `blit_row` | `lcd_tx` uses polling transfers; `blit_rect` copies and byte-swaps through a private 32 KiB buffer | Use one CPU RGB565 row buffer; no overlap claim |
| DMA requires all row input buffers to be internal | This API copies caller memory into an internal buffer | Caller row may be ordinary byte-addressable memory, including PSRAM while accessible; DMA restrictions remain driver-specific |
| `visual_repl` provides a reusable complete font | `glyph5x7` uppercases lowercase characters | Reuse raster mechanics, supply distinguishable lowercase glyphs before validating `r`/`R` UI |
| Typed acceptance and command execution are interchangeable | Acceptance resolves a value; action rules evaluate availability on a query | Separate the two and explain disabled targets |
| Catalog of requested types covers acceptance | A memory can satisfy a segment slot through a relation but is not in a segment-only catalog | Enumerate the conservative relation preimage, then run the exact resolver |
| Deduplicate accepted results by `(type,value)` | Actual resolver returns two options for two equally ranked relations even with identical results | Deduplicate source references for catalog display, never erase route ambiguity |
| Accept reducer validates choices | `accept.ts:90-92` settles any supplied option while choosing | Native adapter validates request, choice identity, and fresh route/result |
| Same type graph reachability means same semantics | Adding a redundant parent edge changes shortest distances and can change winner to tie | Preserve declared edges; no silent transitive reduction |
| `newtile` is discovered by current-card actions | Prototype explicitly adds both `card` and `app`; signature accepts an app | Define session receiver plus app slot, not imaginary visible app objects |
| Prototype is already a pure exported reducer | Handler is nested in React effect, mirrors mutable `SP`, and duplicates acceptance logic | Treat it as behavior evidence; implement a new pure state transition |
| Hints/digits target what is visible | Handler indexes all object rows; renderer clips later | Derive local shortcuts only from committed visible occurrences |
| Caret can be stored as reference alone | Same reference can occur multiple times and have different local contexts | Focus by occurrence, retain object reference for semantic operations |
| Copy 0102 task wiring | It drops release events and invokes UI paths from the keyboard task | Reuse normalized key vocabulary, replace ownership/wiring |
| 32 MB implies large guaranteed free headroom | Allocator preferences and internal reservations matter; static arrays do not automatically move | Account by memory class; measure largest free blocks and peaks |
| Exact manual screenshots are goldens | Prototype has 29 timeline entries, cursor maximum 28, while manual shows `ev29/29` | Use actual fixture-derived counts and intentional compatibility notes |

### 2.3 What remains valuable

The previous work correctly identified the small React-free semantic kernel, four availability states, candidate identity checks, the keyboard prototype as a starting point, and catalog/line-description as missing host capabilities. It also correctly resisted importing pointer shells and preferred a flat deck.

These are retained. What changes is the precision of their composition: who is the receiver, which object is offered, which occurrence carries context, which snapshot was displayed, and which route is selected. Many UI defects are disagreements among those identities, not failures of drawing code.

## 3. The basic model: references, presentations, and interpreters

### 3.1 A reference is not a widget

Let `T` be a finite set of nominal type names. For each concrete type `t`, let `V_t` be the set of product values of that type. A presentation reference belongs to the disjoint union

$$
  R = \coprod_{t \in T_{\mathrm{concrete}}} \{t\} \times V_t.
$$

“Disjoint union” means the type tag is part of the identity. File handle 17 and task handle 17 are different references. The type graph does not know the bytes stored in `V_t`; it answers semantic reachability questions. It cannot justify casting one C++ payload structure to another.

A **presentation occurrence** adds a location and context to a reference:

```text
Occurrence = (occurrenceId, reference, contextAnchor, rowKey)
```

A file reference can occur in its directory list, an inspector link, and two listener entries. These are three or more occurrences of one object. The tray stores references. The caret stores an occurrence. Histories store references and usage order. Menus retain the occurrence's context anchor and the reference on which the action was displayed.

This distinction allows a generic renderer to remain product-independent without forcing every row into a C++ class hierarchy.

### 3.2 One declaration, several questions

A compiled declaration `D` answers several different questions about one reference and one context `S`:

```text
label_D(r,S)                 -> display text
resolve_D(query,S)           -> action winners / unavailable rows / ties
accept_D(request,r,S)        -> none / direct or related value / route choices
describe_D(r,S,viewState)     -> logical rows
catalog_D(sourceTypes,S)     -> enumerable reference sources
```

Only the product's command gateway changes facts or performs I/O. Descriptors, predicates, relations, and binders must be referentially transparent within a turn: repeated calls with equal inputs return equal results, without changing product state. C++ `const` alone cannot enforce this; tests and ownership do.

This is the **Interpreter pattern** over a small declarative language. Action selection, acceptance, and later additive help are separate interpreters sharing type and scope semantics. The display does not need a second semantic implementation.

### 3.3 Why not use C++ inheritance for runtime types?

A runtime subtype relationship expresses which declarations may apply. It need not mirror C++ memory layout, and multiple nominal parents are common. In P, an inherited rule receives the original concrete reference. A C++ base pointer would instead imply layout/accessor contracts and tempt unsafe downcasts or virtual effect methods.

Use nominal IDs and product-owned checked accessors. C++ variants and templates describe the implementation's data representation; the PBUI graph describes the product's semantic type vocabulary. They are related only through explicit codecs/accessors.

## 4. Types and selectors: graph theory before optimization

### 4.1 Reachability is a partial order

Let `G=(T,E)` be a finite directed acyclic graph with edges from child to parent. Define `a <= b` when `a=b` or a directed path exists from `a` to `b`. This relation is reflexive, transitive, and antisymmetric because cycles are forbidden; it is therefore a partial order.

It is not necessarily a lattice. Two types may have several incomparable common ancestors, or no declared common ancestor. Do not invent a least upper bound or universal root unless the product declares one.

PBUI also observes shortest path length:

$$
 d(a,b) = \min\{|p| : p \text{ is a path from } a \text{ to } b\},
$$

with `d(a,a)=0` and an explicit `Unreachable` result when there is no path. This is a directed distance, not a symmetric metric: an image may reach file while file cannot reach image.

### 4.2 A subtle consequence: edges are part of policy

Consider:

```text
image -> file -> object
```

For an image, a rule on file has distance 1 and one on object has distance 2. Now add the redundant edge `image -> object`. Reachability is unchanged, but both distances become 1 and equally scoped/prioritized rules can tie. The actual-source probe produces exactly this change. P labels this tie `incomparable-types` because the declared types differ, even though file and object are comparable in the partial order; that diagnostic string is not a proof of mathematical incomparability.

Therefore:

- Preserve parent edges, including redundant edges, in the port.
- Do not “normalize” the graph by transitive reduction.
- Do not call PBUI's distance ranking the mathematically unique most-specific ordering. It is an explicit policy over a graph representation.
- Document changes to type parents as behavior changes and test them.

An alternative would prefer a rule whose declared type is below another's declared type in the partial order. That would choose file in the redundant-edge example. It is a plausible different resolver, but not this port's compatibility contract. The native core preserves PBUI shortest-distance selection.

### 4.3 Compilation algorithm and cost

For `n` types and `e` edges:

1. Intern and validate unique names.
2. Validate every parent reference.
3. Run iterative color DFS or Kahn topological validation in `O(n+e)`.
4. Run BFS from each type to compute shortest ancestor distances in `O(n(n+e))`.
5. Store a dense distance matrix if `n` is small; at `n=64`, 16-bit entries occupy 8,192 bytes.

Use `UINT16_MAX` as unreachable only if compilation proves all valid distances are smaller; never add to the sentinel. An undeclared subject is an error. An unknown requested supertype should be rejected at declaration/request validation, while the lower-level graph query may return unreachable to mirror P's tests.

Compile-time validation does not imply compile-time C++ metaprogramming. A straightforward boot-time compiler gives better diagnostics than a complex template DSL and runs once.

### 4.4 Scope is a sequence, not a set

A query context has an inner-to-outer scope sequence `S.scopes`. A selector declares an eligible scope set `Q.scopes`. Its proximity is

$$
 s(Q,S)=\min\{i : S.\mathrm{scopes}[i] \in Q.\mathrm{scopes}\}.
$$

If a nonempty selector scope set has no member in the active stack, matching rejects instead of evaluating an empty minimum. The declaration order of eligible scopes does not decide proximity. Empty selector scopes mean universal scope for relations, with no proximity claim; action rules require explicit nonempty scopes. Unknown and duplicate active scopes are rejected at context construction, matching `model/compile.ts`.

Scopes describe semantic context such as `file-inspector`, `triage`, `global`, not keyboard surfaces such as `menu`. Opening an object menu must not replace its object's scope stack with an unrelated menu scope. Store a stable `ContextAnchor` and reconstruct its current scopes when revalidating.

### 4.5 Match results carry evidence

```cpp
struct Match {
    TypeId concrete;
    std::optional<TypeId> declared;
    uint16_t distance;
    std::optional<ScopeId> scope;
    std::optional<uint16_t> scope_index;
    int32_t priority;
};
using MatchResult = Result<std::variant<Match, Rejection>, CoreError>;
```

The selector runs type, scope, then optional condition. Return a typed rejection stage and reason. Actions use the same type/scope logic but **do not pass action availability conditions into selector rejection**. That distinction is the subject of the next section.

## 5. Conditions and actions: two different algebras

### 5.1 Availability is not four-valued Boolean logic

The four tags encode distinct override behavior:

| Status | Competes for winner? | Shown if unique winner? | May execute? |
|---|---:|---:|---:|
| Available | yes | enabled | yes |
| Unavailable(reason) | yes | disabled, with reason | no |
| Inapplicable | no | no | no |
| Hidden | yes | no | no |

A hidden specific rule must suppress a generic fallback. An unavailable protected-edit rule must not permit a generic edit action to bypass its policy. An inapplicable restore rule on a live file should withdraw and allow another applicable implementation.

`Hidden` is not a universal authorization mechanism and does not make the object itself invisible. A publicly exposed diagnostic trace may still name hidden candidates. Normal UI discovery must omit hidden details; privileged development traces can include them. Product access control remains at the product boundary.

### 5.2 Conditions form an ordered first-failure algebra

For statuses `x,y`, define

$$
 x \triangleright y =
 \begin{cases}
 y & x=\mathrm{Available},\\
 x & \text{otherwise}.
 \end{cases}
$$

`Available` is the identity and this operation is associative. It is not commutative: `Unavailable("read only") ▷ Hidden` returns the reason, while `Hidden ▷ Unavailable("read only")` returns Hidden. An `all` condition is a left-to-right fold with this operation, lazily evaluating the next child only while the result is Available.

This has an engineering consequence. An optimizer may flatten nested `all` nodes without changing leaf order, but must not sort them, run them eagerly for side effects, or treat them as an unordered Boolean conjunction. If disclosure policy must dominate a read-only reason, the declaration must place it first. Test this explicitly.

The condition language stays small:

```text
Condition := All(children)
           | Mode(id, requiredActive, failure)
           | Capability(id, failure)
           | Predicate(id)
```

Named predicates are the only condition nodes that inspect product facts. No generic `not` or `any` is added. Product predicates remain the escape hatch for real domain logic. This is an application of a small embedded DSL with a closed AST and a total interpreter; it is not an attempt to construct a theorem prover over arbitrary C++ callbacks.

### 5.3 Action selection is lexicographic minimization with ties

For a collected candidate `c`, define rank

$$
 k(c)=(d(c),s(c),-p(c)).
$$

The minus sign explains the order: smaller distance, smaller scope index, larger priority. In C++, compare priority in the opposite direction rather than negating a potentially minimum signed integer.

For each action ID `a`, let `C_a` be the eligible candidates after dropping only Inapplicable. If it is empty, set `W_a` to the empty set without evaluating a minimum. Otherwise define

$$
 W_a=\{c \in C_a : k(c)=\min_{x\in C_a} k(x)\}.
$$

- `|W_a|=0`: no result.
- `|W_a|=1`: one winner; its status determines rendering and binding.
- `|W_a|>1`: ambiguity; select no candidate, bind no verb.

Labels, declaration order, action menu order, and candidate IDs never break a semantic tie. IDs only provide stable diagnostic/display ordering. A hidden and available candidate at equal rank are ambiguous; do not hide the hidden candidate first and let the other win.

### 5.4 Deriving a bounded selection algorithm

A summary for one partition is either Empty or `(bestRank, tiedCandidates)`. To merge two summaries:

```text
merge(Empty, b) = b
merge(a, Empty) = a
merge(a, b):
    if a.rank better than b.rank: return a
    if b.rank better than a.rank: return b
    return (a.rank, union(a.tiedCandidates, b.tiedCandidates))
```

Given unique candidate identities and set union, merge is associative, commutative, and idempotent. This proves that streaming candidate collection in any order produces the same winner set. It also explains why “keep first minimum” is wrong: that operation discards tie information and becomes order-dependent.

The research C++ experiment verifies this merge against a sort/filter oracle. Production can retain bounded candidate indices or use two passes: first determine best rank/count, then gather tied IDs and trace details. If scratch storage is insufficient, return `CapacityExceeded` and **no executable rows**. Truncating before selection can remove a hidden winner and expose a generic action; this is a correctness failure, not a harmless degraded menu.

### 5.5 A worked example

Suppose `image -> file -> inspectable`, the scope stack is `[editor,global]`, and the query is `open(image#7)`:

| Candidate | Declared type | Scope | Priority | Status | Rank |
|---|---|---|---:|---|---|
| generic.open | inspectable | global | 100 | Available | (2,1,-100) |
| file.open | file | editor | 0 | Available | (1,0,0) |
| image.open | image | global | -5 | Unavailable("locked") | (0,1,5) |

`image.open` wins despite its more distant scope and lower priority. Its type distance dominates. The UI shows “locked”; it must not execute file.open. Change its status to Inapplicable and file.open wins. Change it to Hidden and no open row appears. Add an equally ranked image.open2 and neither executes.

### 5.6 Resolver API and the binder boundary

```cpp
Result<Resolution, CoreError> resolve(
    const CompiledModel&, const Query&, const TurnContext&,
    ResolutionScratch&, TraceSink* optional_trace);
```

`Resolution` contains decisions and provenance. A selected available candidate may bind a small command value. Binding is pure: it cannot close a card, allocate a context segment, write a file, or launch I/O. In discovery mode the native implementation may skip materializing unused bound values, provided the decision result is unchanged and binders are guaranteed pure. This is a documented API narrowing, not a promise that binder call counts match every P debug fixture.

For the first conformance port, implement ordinary selected-only binding exactly as P does. Optimize discovery allocation only after the baseline passes. Debug trace generation shares selection branches; it is not a second resolver. Canonicalized trace content, not emission order under permutation, is the semantic test target.

## 6. Acceptance: typed partial functions and query planning

### 6.1 Subtyping and relations are different graphs

A subtype edge says an unchanged reference may substitute for a requested nominal supertype. A relation is a named partial function on values in a context:

$$
 f_{\rho,S}:R_{\mathrm{source}}\rightharpoonup R_{\mathrm{target}}.
$$

For example, a memory can expose its existing context segment, but memory is not a subtype of context segment. A relation cannot silently create a segment while the user types a filter. Relations are queries; creation belongs to commands.

Do not implement `pin(mem)` by translating memory to its segment if pinning is the operation that creates that segment. The relation would be undefined exactly when the command is useful. Use separate memory and segment action rules under one conceptual pin action. Demonstrate a relation with an inspection slot such as “show existing context segment,” where the segment already exists.

This corrects an attractive but circular mapping in the earlier guide.

### 6.2 Preserve the acceptance algorithm precisely

For request `A=(wantedTypes,filter)` and offered reference `r`:

1. Validate the offered runtime reference and requested vocabulary.
2. If `r.type` reaches any wanted type, apply the filter to `r`. Return Accepted(r) or None. **Do not try relations after a direct filter rejection.**
3. Otherwise discover only relations exposed to acceptance whose declared codomain reaches a wanted type.
4. Match each source selector using the shared matcher; evaluate applicable relations as partial queries.
5. Validate that any concrete result is a declared, nonabstract type, reaches the relation codomain, reaches a requested type, and passes the request filter.
6. Rank successful routes by nearest scope and then greatest priority. Universal scope ranks after a matched explicit scope.
7. Return None, one accepted option, or the tied route choices sorted by stable relation name.

Acceptance does not use action type-distance precedence to rank routes. The shared selector computes type reach/provenance, but the acceptance interpreter has its own documented reduction. This is why “share the selector” does not mean “share every interpreter's winner algorithm.”

### 6.3 Keep translation evidence

```cpp
struct AcceptOption {
    OptionId id;
    Reference source;
    std::optional<RelationId> relation; // null means direct
    Reference result;
    ContextAnchor anchor;
};
```

Two routes can produce the same result and remain ambiguous. The actual-source probe creates `r1` and `r2`, both returning `segment#s1`; P returns two chooser options. Coalescing them by result would change semantics. Equality of values is not equality of the interpretation the user is choosing.

A menu or chooser must retain source, route, and expected result identity. At confirmation, recompute the route under current facts. If the route now returns a different object, refresh and ask again; do not silently redirect an old digit choice.

### 6.4 Relation-aware catalog completeness

A slot cannot enumerate only requested output types. Let `Wanted` be its type set and let `down(X)` be all concrete types that reach at least one member of `X`.

For each acceptance-exposed relation whose declared codomain reaches Wanted, include source types compatible with its source selector. The conservative enumeration set is

$$
 \begin{aligned}
 E_A &= \{\rho : \mathrm{exposed}(\rho)\ \land\ \mathrm{codomain}(\rho)\le Wanted\},\\
 \mathrm{SourceTypes}(A) &= \mathrm{down}(Wanted)\ \cup\
 \bigcup_{\rho\in E_A}\mathrm{SourceDomain}(\rho).
 \end{aligned}
$$

Here `SourceDomain` is the exact source type for an exact selector or its concrete descendants for a subtypes selector. Enumerate product catalog entries in those types, deduplicate identical source references, then call the exact acceptance resolver on each. Conditions and filters are evaluated in the exact stage, not guessed by the static planner.

**Completeness argument:** every accepted reference either reaches Wanted directly, and is included by the first term, or succeeds via a discoverable relation, whose source selector includes its type in the second term. The proof assumes product catalog providers enumerate every live reference in the requested source type set. It proves candidate inclusion, not that all candidates will succeed.

For small products, enumerating all live references and applying acceptance is the simplest complete implementation. Use the conservative type plan when catalog size warrants it. For paged catalogs, carry revision and `complete/more` state. Never display “0 candidates” while an unfinished scan merely has not found one yet, and never auto-accept the sole current hit before enumeration is complete. Digits may still choose a displayed, validated occurrence explicitly.

### 6.5 Acceptance versus execution status

An object can fit a `hunk` slot yet be protected from revert. These are two facts:

```text
fits(request, source)          -> acceptance result, possibly a route chooser
executable(command, args)      -> action resolution + gateway preconditions
```

The accept UI should show compatible-but-disabled candidates with a reason rather than claim they have the wrong type. Use separate decorations: typed fit, enabled confirm, and route ambiguity. Only a candidate with a unique valid route and an available action may be committed directly. Choosing a compatible disabled candidate shows its reason and keeps the slot open.

The core acceptance machine remains domain-neutral. The shell's action slot composes it with an action check. Generic value acquisition that has no command may settle a typed value without an action check.

### 6.6 Errors are not empty results

The existing relation system can report invalid outputs or thrown callbacks through `evaluate`; `matches` keeps only successful values. The native profile has no exceptions in callbacks and uses `Result<optional<Reference>,RelationError>`.

A legitimate partial relation returning no value is None. A bad type, missing callback, or exhausted buffer is a diagnostic error. An interactive command slot must fail closed for that query rather than hide the error behind “nothing matched.” This is a deliberate stronger diagnostic policy; differential tests compare valid relation behavior separately from error handling.

## 7. Commands: a grammar cannot be inferred from action rules

### 7.1 Separate operation, implementation, receiver, and arguments

An **action ID** names a conceptual operation. A **rule ID** names one implementation selected by type/scope/priority. A **command ID** names an entry in the keyboard language. A **receiver** is the reference on which an action is resolved. A **slot argument** is a value the command still needs.

For `revert hunk`, the sole argument is also the receiver. For `newtile app`, the receiver is the current session and the argument is the app kind. For `close card`, the receiver is the selected card. For `clear`, the receiver can be the session even though the syntax has no arguments.

Conflating these roles is why adding current-card to the visible set cannot make an app action appear automatically.

### 7.2 Put syntax in the same declaration, not in another policy switch

```cpp
struct SlotSpec {
    SlotId id;
    TypeSet wanted;
    FilterId filter;
    SourcePolicy sources;       // World, TrayOnly
    DefaultPolicy defaults;    // ordered policy IDs, not callbacks with effects
};
struct CommandSpec {
    CommandId id;
    StringId spelling;
    ActionId action;
    ReceiverPolicy receiver;   // Slot(0), CurrentSession, CurrentCard
    Span<const SlotSpec> slots;
    DiscoveryPolicy discovery; // VisibleReceiver or Contextual
    Invocation invocation;     // e.g. Command, explicit mask in native rules
};
```

The compiler validates spellings/IDs, slot types, receiver index, filters, action references, and invocation masks. There is one source of command syntax and one source of action policy inside the same compiled declaration. Keyboard aliases such as `R` produce the same command/action intent, not independent mutation code.

The binding contract must also say where acquired arguments go. V1 leaves the semantic action query as receiver plus invocation/context. A binder returns a product operation seed such as `NewTile{sessionId}` or `Revert{hunkRef}`; the shell combines that fresh seed with the validated CommandSpec arguments into `CommandInvocation{commandId, seed, arguments}`. The gateway checks the seed/argument consistency and product preconditions. For a receiver-from-slot command, slot 0 must equal the resolved receiver. For `newtile`, the seed names the session and slot 0 names the app. Argument-dependent checks belong to the slot filter and gateway in this profile; no callback is allowed to secretly read the slot editor's mutable buffer from global state.

A menu entry for an operation requiring an unfilled slot starts that CommandSpec's acquisition rather than calling its final gateway with missing arguments. Enter or a menu can thus begin a command, but only a fully acquired invocation can mutate the product. If later commands need arguments to influence rule precedence, add an explicit immutable argument environment to Query and expand the conformance profile; do not smuggle it into unrelated product facts.

PBUI currently has `menu`, `primary`, `agent`, `introspection`, and `accept` invocation kinds. The native shell adds `command` explicitly rather than using introspection as accidental execution authority. Fixtures without explicit invocation filters allow it; filtered declarations must opt in. This is a documented vocabulary extension. `newtile` is contextual on session; a generic `open` command remains visible-receiver driven.

### 7.3 Define discovery with an existential predicate

For visible occurrences `V` and explicitly declared context receivers `C`, define

$$
 \mathrm{Offered}(cmd,S) = \exists o \in V\cup C(cmd):\mathrm{Discoverable}(Q(cmd,o,S)).
$$

Here `Q(cmd,o,S)` resolves `cmd.action` on `o.reference` with the occurrence's context and the command's invocation.

Discoverable means a unique available or unavailable visible action row exists. Hidden-only winners and unresolved action ambiguity do not produce an enabled command completion. A known command typed in full may still enter its argument workflow even when not suggested, but it receives no exemption from visibility policy, slot validation, or final gateway checks.

Command discovery depends on actual receiver queries, not just visible type names. Caching by type alone is unsound when facts or occurrence scopes differ. A conservative cache key includes model epoch, semantic revision, reference identity, context identity/revision, and invocation. The first implementation should recompute for the small visible set and measure before adding a cache.

### 7.4 Deterministic command parsing

The v1 grammar supports zero or one argument. It is a mode-sensitive input protocol, not a shell string parser:

```text
Nav ':' -> CommandEditing(prefix="")
CommandEditing printable -> append to prefix
CommandEditing Tab -> cycle the saved matching-name list
CommandEditing Space/Enter:
    exact known spelling -> select it
    else exactly one offered prefix match -> select it
    else zero -> show unknown, stay editing
    else many -> show ambiguity, stay editing
selected nullary command -> resolve and dispatch
selected unary command -> open typed slot
```

Keep the user's search prefix separate from the completion's selected spelling. The prototype overwrites its buffer with a completed name, which then narrows future Tab cycles to that exact name. The new `prefix + selectedIndex` state avoids that trap.

Do not preserve the prototype's unreachable old inline `verb arg` parser as a second grammar. Serial `text` events feed the same mode transitions as physical typing. A future multi-slot command advances through explicit slot states; v1 declarations with multiple slots produce `UnsupportedFeature` rather than silently discarding arguments.

### 7.5 One source for the documentation line

Build a `KeyAffordances` value from the current interaction state and resolved decisions. It lists which tokens are handled and what they mean now. The documentation line and help view project that value; the key dispatcher uses the same definitions. A stale toast must never replace the accept prompt and conceal the command's target.

The design invariant is not “every sentence fits on one line.” It is that every displayed executable affordance is backed by an intent in the same interaction frame, and every advertised default is the exact target Enter will attempt.

## 8. Native representation, ownership, and compilation

### 8.1 Strong IDs without overengineering

Use distinct wrapper types, not aliases of the same integer:

```cpp
struct TypeId    { uint16_t raw; auto operator<=>(const TypeId&) const = default; };
struct ActionId  { uint16_t raw; auto operator<=>(const ActionId&) const = default; };
struct RuleId    { uint16_t raw; auto operator<=>(const RuleId&) const = default; };
struct ViewId    { uint32_t raw; auto operator<=>(const ViewId&) const = default; };
struct ObjectKey { uint32_t slot; uint32_t generation; };
struct Reference { TypeId type; ObjectKey object; };
struct OccurrenceKey { ViewId view; uint32_t local; };
```

These sketches need explicit equality/ordering implementations for compound keys in real code. Static assertions verify trivial copyability and measured sizes; do not assume a packed structure. Strong wrappers prevent passing a ScopeId to an ActionId API and cost no dynamic allocation.

The simplest vocabulary compiler interns full strings, assigns dense local IDs, and retains a reverse table. Use stable names in serialized tests and diagnostics; local numeric order is not a cross-language identity. Do not add a YAML generator or constexpr hash scheme until multiple products demonstrate a benefit. Hash collisions should not be introduced merely to avoid comparing a few dozen names at boot.

### 8.2 Liveness is distinct from identity

A reusable object slot must carry a generation to prevent the ABA problem: delete object A in slot 7, create unrelated B in slot 7, then accidentally execute an old tray reference against B. Generation mismatch makes the old reference stale.

The timeline demo needs another distinction. An edit not yet reached at the playhead is temporarily unavailable, not a new object every time the user scrubs. Assign stable object keys to timeline entities for the whole session; temporal existence is a product fact. Reserve generation changes for true identity replacement. Tray entries can retain a last-known label for missing objects without pretending they are live.

The product validates `(type,key)` on every lookup. Neither a type tag nor an in-range slot proves the payload is live or belongs to that type.

### 8.3 Identity hierarchy

| Identity | Purpose | Lifetime |
|---|---|---|
| Type/Action/Rule name | semantic vocabulary | declaration/version |
| ModelEpoch | distinguish compiled model replacement | one compiled model |
| ObjectKey | referent identity, including generation | product session or persistent domain |
| ViewId | independent card/inspector state | view lifetime |
| OccurrenceKey | focusable instance in a view | stable logical row lifetime |
| RequestId | pending argument acquisition | one request |
| ChoiceId | one displayed option under a request/frame | one choice generation |
| FrameId | presented interaction map | one completed UI frame |
| OperationId | async effect correlation | one submitted operation |

Menu arrays, row positions, vector addresses, labels, and type names alone are none of these identities.

### 8.4 A coherent turn context

The application owner processes one event at a time and forms:

```cpp
struct TurnContext {
    const DomainFacts& domain;
    const SessionFacts& session; // cards, app kinds, tray membership when relevant
    SemanticRevision revision;
    ModelEpoch model;
};
struct QueryContext {
    ContextAnchor anchor;
    Span<const ScopeId> scopes;
    ModeBits semantic_modes;
    CapabilityBits capabilities;
};
```

The core can be templated on a product-specific `Facts` and `Verb` type. Avoid an unstructured global `void* ProductFacts` store. Product predicates receive typed const facts. Type-erased callback userdata, if needed for a fixed descriptor table, is immutable data owned by the compiled model and hidden behind a typed factory.

A turn-local borrow may refer to owned model/state because no other task mutates it during the call. It cannot escape in a result, callback, or queue message. Results contain value IDs, owned bounded strings, or string-pool offsets with a stated arena lifetime.

Separate semantic revision from frame revision. A caret move can change the frame without changing product facts; a different occurrence scope can change query context without a domain mutation. Freshness checks always re-resolve; revision tokens are cache/diagnostic aids, not substitutes for validation.

### 8.5 Compilation is a checked construction boundary

```text
compile(declaration, supportedProfile, budget):
    validate unique names and feature support
    merge fragments with origin diagnostics
    validate type DAG and concrete descriptors
    validate scope/mode/capability/predicate vocabulary
    validate condition AST node references and depth
    validate action IDs, selectors, invocation masks and metadata
    diagnose unconditional overlap conservatively
    validate relation endpoints/exposure
    validate command signatures and receiver mappings
    validate catalog provider coverage for concrete types
    calculate storage requirements
    allocate once, build indexes, freeze
    return CompiledModel or Diagnostics; never a partial model
```

The first profile supports fixed rules and direct relations. Unsupported family or composition tags are rejected explicitly. Do not claim all P model tests will pass under that profile; maintain a per-test mapping of shared behavior, deferred feature, or deliberate extension.

Unconditional collision validation must consider invocation intersection. P's current registry overlap check does not use invocation masks in its “guaranteed” pair check. The native compiler should reject only a demonstrated overlapping invocation/type/scope case and warn for possible query-dependent collisions. Arbitrary predicates cannot be proven disjoint by this compiler; the runtime ambiguity result remains authoritative.

### 8.6 Memory and error representation

Do not pass C++ ownership-bearing objects through a raw FreeRTOS queue, which copies bytes and does not run C++ constructors/destructors. Queue events must be trivially copyable envelopes containing IDs and bounded values or handles into an explicitly owned message pool.

Use `std::variant`, `std::optional`, and `std::span` in the portable core under C++20. Do not require `std::expected` or C++26 `std::inplace_vector`; implement a small `Result<T,E>` wrapper and a **narrow checked buffer** abstraction if necessary. For trivially copyable candidate/event records, a caller-owned span plus length and `try_push` is enough; there is no need to invent a general STL replacement.

General product containers may use normal vectors/maps with declared budgets. Core/frame scratch is allocated at startup and reset per turn. Boot-time allocations use capability-aware adapters on device and normal aligned memory on host. Every push/append returns an explicit error on overflow. With exceptions disabled, ordinary `std::vector::push_back` is not a recoverable out-of-memory strategy; reserve/freeze or stay within checked buffers in paths that must return a refusal.

Callback outputs use owned values or offsets into caller-provided text arenas. `StringView` into a temporary local formatted string is forbidden. Keep previous and next frame arenas alive until rendering and input publication release them.

## 9. Interaction as a hierarchical transition system

### 9.1 Model legal states, not combinations of flags

The prototype and first C++ sketch store `mode`, `accept`, `menu`, `peek`, and edit buffers independently. A mode can say Accept while accept state is Idle, or Menu can outlive its receiver. Replacing flags with a sum type removes those impossible combinations.

A practical state decomposition is:

```text
AppState = DomainState x DeckState x InteractionState x DeviceStatus

InteractionState = Browsing(BrowseState)
                 | EditingCommand(CommandEditor, ReturnPoint)
                 | Acquiring(Acquisition)
                 | ObjectMenu(MenuSession, ReturnPoint)
                 | Help(HelpView, ReturnPoint)

Acquisition = Pending(request, slotEditor, ReturnPoint)
            | Choosing(request, source, choiceSet, ReturnPoint)

BrowseState = (ViewId, Focus, Viewport, Transient)
Transient = None | HintSession | SearchEditor | TypeCyclePrefix | Peek
```

The deck and playback are orthogonal product state; the acquiring/choosing relationship is nested alternative state. This follows the useful part of Harel's statecharts: represent independent dimensions as a product and mutually exclusive states as alternatives, rather than enumerating or loosely flagging every combination. It does not require implementing full statechart broadcast semantics.

### 9.2 Transition function and effects

Let state be `s`, input event `e`, and stable query context `q`. Define

$$
 \delta(s,e,q)=(s',\mathrm{effects}).
$$

Effects are explicit values such as `AttemptAction(ticket)`, `SubmitOperation(command)`, `RequestCatalogPage(id)`, or `Notice(reason)`. The reducer does not read the clock or launch I/O. Timers arrive as events with IDs and deadlines. The effect interpreter runs on the owner after the transition and dispatches further results as ordered events.

In a resource-constrained implementation the owner may mutate its private storage while computing this transition, but observability must remain equivalent to the pure function: no publication of partially repaired session state, no callback into the reducer during a transition, and no frame built from half-installed facts.

### 9.3 Safety and liveness claims

Safety properties describe what never happens:

- At most one acquisition is pending.
- Choosing implies a pending request with the same RequestId.
- Each admitted request emits at most one terminal result.
- Terminal result IDs name an admitted/refused request, never another request.
- A rejected foreign choice changes no state.
- A positional key decodes only a visible occurrence in its interaction frame.

Liveness needs assumptions. “Every admitted request eventually terminates” is false if the user never chooses or cancels. The correct statement is: if the environment eventually supplies a valid choice, abort, or configured timeout, the request terminates exactly once. P's generated tests explicitly abort at the end of each sequence to establish terminal counts. Do not misdescribe a drained safety test as unconditional liveness proof.

### 9.4 Strengthen the native event boundary

P's accept reducer receives precomputed resolutions and choice objects, trusts them, and attaches terminal effects to the current request. That is sufficient inside its synchronous trusted caller architecture. Firmware queues and future workers need an explicit boundary:

```cpp
struct OfferEvent {
    RequestId request;
    FrameId frame;
    OccurrenceKey occurrence;
};
struct ChooseEvent {
    RequestId request;
    ChoiceGeneration generation;
    ChoiceId choice;
};
```

On Offer, verify request and frame, look up the reference and context in the retained frame, then recompute acceptance. On Choose, find the option in the pending choice set, re-resolve its source under current context, verify the chosen route is still among the best routes and still yields the expected result, then settle. An external event never supplies an arbitrary result reference for direct installation.

A second request while one is pending gets Refused for its own ID and leaves the first intact. Stale completions are ignored/refused with diagnostics. Reused IDs are not allowed within a boot session; use a wide monotonically increasing counter and fail safely on wrap rather than assuming it cannot happen.

### 9.5 Esc is a transition relation, not a global reset

Use explicit return points and this table:

| Current state | Esc result |
|---|---|
| Choosing | dismiss choices, keep the same pending request |
| Pending acquisition | abort once, restore its return point |
| ObjectMenu | close menu, return to origin view/focus |
| Hint/Search/TypeCycle/Peek | close only that transient |
| Help | restore origin |
| EditingCommand | cancel buffer, return to browsing |
| Browsing inspector | pop one inspector view |
| Browsing root | no-op |

The prototype sends every non-nav mode directly to nav. That is not copied blindly: it loses overview/menu origin and cannot express PBUI's chooser-preserves-request rule. The native table is the chosen replacement behavior. A release of `i` closes only a peek created by that physical key; it must not dismiss a later menu or acquisition.

### 9.6 Key-state policy

Normalize pressed, repeated, and released states before shell dispatch. Repeats are accepted for arrows, text editing, and deletion; destructive confirmation, command submission, mode entry, yank, and toggles trigger on press only. The hardware's composed printable byte is preserved: `r` and `R` are distinct.

Unknown keys are ignored with optional diagnostics; unsupported codes must not be interpreted as printable bytes. Add Tab, Home, End, PageUp/PageDown and their physical mappings explicitly; 0102's token helper is a starting point, not complete shell coverage. Pass releases through instead of copying 0102's pressed/repeated-only filter.

## 10. Navigation, completion, and deck identity

### 10.1 Three sets, not one overloaded screen list

Let `O` be all presentation occurrences in the logical document. Let `V` be occurrences intersecting the visible content viewport after clipping. Let `C` be the world/catalog reference set. These have different uses:

- Up/down and typed cycling navigate `O`, adjusting the viewport to reveal focus.
- Hints and empty-buffer digit picks address only `V`.
- Label search can search `O`, revealing a result even if it was off-screen.
- Slot completion draws from `C` plus explicit tray/context sources and runs exact acceptance.
- Verb suggestion uses `V` plus explicitly declared contextual receivers.

The prototype derives digits from all object rows and then clips display, so a digit can target an object whose chip is not visible. The new design makes this impossible by construction: both chip rendering and key decoding consume the same ordered `visiblePicks` vector.

### 10.2 Prefix-free hint labels

For an alphabet of `b > 1` symbols and `m >= 1` visible targets, fixed-width labels need

$$
 \ell=\max(1,\lceil\log_b m\rceil).
$$

When no target is visible, do not open a hint session; show a no-target notice.

At most sixteen object rows are visible in the initial layout; one letter from a 26-letter alphabet suffices. If later inline presentations exceed that, use a fixed width for the whole hint session. Mixing a one-character label `a` with a longer label `aa` makes immediate selection ambiguous; a prefix-free code or explicit terminator is required.

Freeze the hint session's occurrence mapping for its frame. If the view changes, invalidate the session rather than silently renumber its targets. This is an application of naming stability, not a need for globally permanent hint letters.

### 10.3 Pronouns and defaults as named evidence

`$n` denotes the nth tray reference at the time the displayed slot interpretation was built. `it` denotes the focused occurrence's reference; its context and direct/related acceptance still matter. Source discovery and parsing do not bypass liveness or action checks.

The default chain is an ordered policy with explicit provenance:

1. Tray removal: caret reference if it is in the tray; otherwise newest tray entry.
2. Otherwise the focused reference if acceptable.
3. For close/open card slots, the current card if acceptable.
4. Most recently successfully used acceptable reference from history.
5. No implicit default; a first completion can be visibly selected, but must not be mislabeled as history or `it`.

History is an ordered bounded list with sequence numbers, not only a map from type to last value. The map cannot choose the most recent among multiple requested types. Record successful acquisition/action use; ordinary caret repair does not rewrite history.

Store the chosen default as `{source,route?,result,reason,frame}`. If it disappears, Enter refuses and refreshes; it does not silently take the next catalog entry. Tab explicitly changes the selected target, even when the buffer is empty. This corrects the prototype's default-overrides-Tab behavior.

Digits are shortcuts only when the argument buffer is empty. After `$`, `1` is literal input. Exact `$n` parses before substring search. `it` may be prepended to matching labels, with duplicate source entries removed but route ambiguity retained. The printed choice and Enter attempt must be derived from the same slot projection.

### 10.4 Flat deck, stable views

A workspace owns an ordered sequence of CardIds; a card owns an AppId and independent view state. A session holds active WorkspaceId and CardId, not mutable array indexes as durable identity.

```text
Deck invariant:
  at least one workspace
  each workspace has at least one card
  card IDs are globally unique within the session
  active card belongs to active workspace

close(card):
  reject absent card or last card of workspace
  remove card in a planned next deck
  if active was removed, choose successor or previous deterministically
  close/repair dependent view sessions
  install deck and session together
```

Inspectors receive new ViewIds and explicit return points. Do not key view state by workspace index, card index, and stack depth: two objects inspected at the same depth would inherit each other's caret/scroll state, and deletion before an active card would shift identities.

Reuse the conceptual `canClose` rule from `P/packages/workbench-core/src/queries.ts:69`, but not the full workbench tree, generated protocol, links, or persistence. The first profile chooses a flat deck deliberately, not as a partial implementation of workbench-core.

## 11. LCD UI: treat rendering as a compilation pipeline

### 11.1 Logical rows are not the physical frame

A `ScreenDocument` with exactly twenty rows cannot represent scrollable inspectors or help. Product views produce a logical document of any supported bounded length, containing stable row keys and optional occurrences. The shell then selects a viewport and composes chrome and overlays into a fixed-size frame.

```text
Product facts + view state
    -> logical RowDocument (stable RowKeys / OccurrenceKeys)
    -> focus repair and optional detail projection
    -> viewport and clipping
    -> shared InteractionFrame (picks / choices / affordances)
    -> CellFrame + decorations
    -> RGB565 row raster
    -> LCD transfer
```

This is a compiler pipeline: each stage lowers one representation to another while preserving the information required by later stages. Presentation identity must survive until interaction mapping; it need not survive in the final pixel buffer.

### 11.2 Proposed APIs

```cpp
Result<RowDocument, ViewError> describe(
    Reference subject, const TurnContext&, const ViewState&, DocumentArena&);

Result<ViewProjection, ViewError> project_view(
    const RowDocument&, Focus, Viewport, const LayoutMetrics&);

Result<InteractionFrame, UiError> build_interaction(
    const ViewProjection&, const InteractionState&,
    const CompiledModel&, const TurnContext&, FrameArena&);

Result<CellFrame, UiError> compose_cells(
    const ViewProjection&, const InteractionFrame&, const Theme&, FrameArena&);

Result<void, DisplayError> paint_dirty(
    const CellFrame& desired, PresentedFrame& actual, LcdSink&);
```

The native modules should not depend on ESP-IDF except `LcdSink`'s firmware adapter. Host raster output can be a PPM image or a small SVG preview; text and semantic dumps derive from the same cell/interaction frame.

### 11.3 Layout at 40 by 20

Start with 8×16 cells: `floor(320/8)=40`, `floor(320/16)=20`. The font glyph can be smaller within a cell. Reserve two cells in object content rows for hint/digit/gutter decorations, leaving 38 text cells. Draw the tone strip inside the reserved gutter rather than adding unaccounted pixels to a 320-pixel-wide row.

Normal frame:

```text
row 0       status / device or product summary
row 1       workspace / card / breadcrumb
rows 2..17  sixteen content rows, each at most forty cells
row 18      tray summary (or blank, geometry stays stable)
row 19      mode and concise key help
```

During command/accept input, use the last two content rows for command text and the selected/default target. This leaves fourteen content rows and keeps mode, input, target, and cancellation instructions visible. Do not cram the prototype's long prompt into a single clipped footer. Selection/refusal messages have a dedicated area or precedence within those rows; transient success messages may not obscure what Enter will do.

Help must scroll or paginate. “Any key returns” is unsuitable when the help document exceeds one screen; native Help uses arrows/PageUp/PageDown to read and Esc to exit. This is an intentional improvement with its own golden test.

### 11.4 Focus repair must precede decoration

Focus stores an OccurrenceKey plus previous reading-order position as a fallback. After rebuilding the logical document:

1. Keep the same occurrence if it exists.
2. Otherwise prefer another occurrence of the same reference in this view when that preserves user intent.
3. Otherwise choose the nearest surviving reading-order neighbor.
4. Otherwise focus is empty.

Then compute the viewport needed for focus, then key mappings and decorations. The earlier diagram repaired focus after composition, risking one frame of stale highlights. The new ordering prevents that.

A list/detail view has an apparent dependency cycle: rows depend on focus and focus depends on rows. Break it explicitly into primary rows, focus repair on primary occurrences, and detail rows derived from the repaired focus. Initially make the detail region read-only; opening its inspector gives independent focusable occurrences. Do not iterate uncontrolled layout until it “stabilizes.”

### 11.5 Plain-text scrolling and object navigation

A long diff or help page may contain no object rows. Up/down object navigation cannot reach its text. Provide a manual viewport mode using PageUp/PageDown (and arrows in Help). In browsing, manual scrolling may leave the focused occurrence off-screen; then suppress `it`/Enter-on-caret and show “Pg scroll; arrows return to objects,” or clear focus explicitly. Choose one tested behavior: v1 clears active occurrence when it leaves the viewport during manual scroll, retaining the last occurrence as a restoration hint. Do not keep an invisible actionable caret.

Search scans the whole logical document and reveals its chosen occurrence. Hints never reach off-screen rows. These distinctions make both text reading and safe positional selection possible.

### 11.6 Font and encoding contract

`visual_repl` uses a 5×7 bitmap expanded vertically into 8×16 cells and maps lowercase to uppercase. Preserve the cell geometry but add a tested lowercase glyph table before presenting case-sensitive key instructions. A device showing `R` for both repeat and revert would be behaviorally misleading despite a perfect text dump.

Use UTF-8 at product boundaries, decode into glyph IDs in layout, and define a fixed one-cell repertoire for v1: printable ASCII, lower/uppercase, arrows, type symbols, and a replacement glyph. Unsupported Unicode code points become one replacement cell; combining sequences and wide characters are explicitly unsupported, not counted by byte length. Clip on decoded glyph boundaries. A glyph is not a byte and `strlen` is not a layout width function.

Tone and style use enumerated tokens resolved to RGB565. Product views do not provide arbitrary CSS or raw pointers to style objects. A custom graph renderer can be added later as another display primitive; it does not justify building a general widget tree now.

### 11.7 Dirty rendering is exact incremental computation

Let `F_old` be the last successfully painted cell/decor frame and `F_new` the desired one. Dirty row `i` means their raster inputs differ. Compare cells and decoration fields exactly. A hash may be a quick rejection, but equality of hashes is not a proof of equality; with only 800 cells, direct comparison is inexpensive. Avoid `memcmp` on uninitialized C++ padding; compare initialized packed scalar fields or explicit operators.

```text
for each dirty row:
    rasterize desired row into one CPU buffer
    result = lcd.blit_row(row_y, height, buffer)
    if result succeeded:
        update presented row cache
    else:
        keep row dirty; mark display/input map uncertain; report failure
publish interaction FrameId only when its required rows succeeded
```

Do not mark the whole frame clean before SPI succeeds. The display is not atomic: a full refresh can be visibly scanned row by row. A coherent input mapping nevertheless requires a completion point. While a frame transition is in progress, positional confirmation/hints from an older frame are refused or deferred by frame ID, never decoded against a new unseen list.

The keyboard task can tag events with the last published FrameId at polling time. This is a conservative software epoch, not a timestamp of the physical press; hardware FIFO events have no original frame identity. If an event crosses a frame change, refuse positional activation and invite retry. Ordinary text or navigation events can be serialized under explicit policy. This sacrifices occasional keys during a full refresh rather than silently targeting another object.

### 11.8 The actual LCD API boundary

The current `picocalc_lcd_blit_row(y,h,pixels,count)` delegates to `blit_rect`. It reads host-endian RGB565 values, converts them to big-endian byte order in a private internal buffer, and calls `spi_device_polling_transmit`. The call returns after transmission. There is no asynchronous completion callback and no benefit from alternating caller buffers to “overlap” with this synchronous API.

Budget one 320×16×2 = 10,240-byte caller row buffer and the driver's existing 32,768-byte internal DMA staging buffer. The caller buffer may be in PSRAM because the driver reads it on the CPU before transfer; no blanket claim is made about every P4 DMA peripheral. Do not double-swap RGB565 before calling the existing API.

Start at the source's 40 MHz SPLL configuration. The payload-only lower bounds are:

$$
 \begin{aligned}
 t_{\mathrm{row}}&\ge\frac{320\cdot16\cdot16}{40\cdot10^6}=2.048\text{ ms},\\
 t_{\mathrm{screen}}&\ge\frac{320\cdot320\cdot16}{40\cdot10^6}=40.96\text{ ms}.
 \end{aligned}
$$

Command transactions, byte swapping, rasterization, task scheduling, and wiring margin add time. Older measurements at 80 MHz do not override this bound. Initial acceptance targets are p95 under 75 ms from dequeued navigation event to completed small update, under 150 ms for full-screen transitions, and responsive independent keyboard acquisition. Report acquisition delay separately from owner-to-paint delay. Tighten after hardware evidence, not before.

A future queued display adapter may optimize transfers, but it must define ownership, endian format, completion/failure, in-flight frame mapping, and memory budgets. It is a separate measured change, not the default implementation.

## 12. Fresh actions, transactions, and asynchronous work

### 12.1 Displayed actions are optimistic observations

A displayed action is a statement about a past query. Store an action ticket:

```cpp
struct ActionTicket {
    ModelEpoch model;
    ActionId action;
    RuleId candidate;      // fixed-rule v1; later tagged family key
    Reference receiver;
    ContextAnchor anchor;
    Invocation invocation;
    SemanticRevision displayed_revision;
};
```

The ticket is not an authorization token. On activation, validate receiver liveness and context, capture current facts, re-resolve the same query, and require the same candidate to remain uniquely available. Use only the newly bound command value.

P's `evaluateFresh` ignores revision equality and permits the same candidate to bind a changed verb, which its tests explicitly require. The port preserves that basic behavior. For commands whose human confirmation includes a material operand (delete path, destination, cost), the shell additionally compares a `ConfirmationKey` built from the displayed immutable operands. If the key changes, re-present and request confirmation. Do not retrofit “all verb bytes must stay equal” into every PBUI action; that would break benign fresh binding.

### 12.2 Serial owner as a linearization boundary

The owner performs validation and a synchronous product state install without processing another mutation event in between. That gives a clear linearization point: the instant the command's state changes become visible. Gateway preconditions remain authoritative even if UI resolution allowed the command.

```text
attempt(ticket, args):
    reject old model epoch or dead receiver/context
    fresh = resolve(same query, current turn)
    decision = evaluateFresh(ticket, fresh)
    refuse unless same unique available candidate
    compare confirmation operands if command requires them
    plan = gateway.prepare(fresh.command, args, current state)
    if rejected: show product reason, install nothing
    validate planned domain + deck + session invariants
    commit planned next state once
    publish outcome and append live-reference history
```

This uses the **Command pattern** for effect values and an optimistic validate/commit discipline for stale UI observations. It does not require persistent event sourcing or copying the entire 32 MB world on every key. A small command plan can contain validated deltas, and the owner can apply them in place once no failure-producing work remains.

### 12.3 Separate pure queries, state changes, and I/O

A callback returning a bool is not enough to classify its role. The boundaries are:

- **Query:** predicate, descriptor, relation, catalog page; no state mutation or blocking I/O.
- **State command:** validate and atomically change owner-held state.
- **External operation:** file I/O, network, lengthy compute; submit a bounded request and receive a correlated result.

A worker must not hold pointers into mutable owner state. It receives owned values or a retained immutable job input. Its result names `OperationId`, expected object generations, and relevant domain version/preconditions. On completion, the owner decides whether the result is still applicable. A completed operation cannot install into a reused card/object slot.

Closing an inspector cancels its interest, not necessarily an irreversible external operation. Operation records outlive views until completion/cancellation acknowledgment. Repeat commands store an action/command ID and explicit argument policy, never a stale closure or a prior bound verb.

### 12.4 Request validation before action validation

A relation chooser adds another stale boundary. The safe full path is:

```text
displayed source occurrence
    -> request/choice identity validation
    -> fresh typed acceptance and route/result check
    -> command argument construction
    -> fresh receiver/action selection
    -> product gateway preconditions
    -> atomic state install or correlated I/O
```

Skipping acceptance revalidation is not repaired by fresh action resolution: the action may still be available on the wrong newly translated object. The identities guard different facts.

## 13. Device ownership, budgets, and failures

### 13.1 Task plan

Start with three tasks and optional workers:

| Task | Owns | Must not do |
|---|---|---|
| Application owner | domain/session/interaction, model lifetime, frames, LCD calls | block on keyboard recovery or unbounded product I/O |
| Keyboard owner | keyboard init/poll/recovery and future southbridge operations | call product/router/renderer directly |
| Console | line parsing and serialized request/reply transport | mutate UI/model or start another serial reader |
| Optional worker | one job's owned input/result | access live UI state or LCD |

Initialize the keyboard driver before concurrent use. Its lock creation and global diagnostics were not designed as arbitrary multi-client lock-free APIs. Route southbridge diagnostic/battery/backlight requests through the keyboard owner. `picocalc_keyboard_recover` waits 3.1 seconds; send `InputLost` before recovery, release all held/quasimode state in the shell, and report `InputRestored` afterward.

0102's README pins **ESP-IDF 5.4.2**. Its toolchain defaults to C++23 (`gnu++2b` in local build code); explicitly request C++20 for this portable library and test the actual target compiler before relying on library features. Exceptions and RTTI are disabled by default and are not needed here. Do not infer stack sizes from generic FreeRTOS examples: ESP-IDF task creation stack depths are specified in bytes; verify the pinned API and measure high-water marks.

### 13.2 Bounded queues without silent semantics changes

A bounded queue cannot guarantee delivery under an unlimited producer. State the assumptions and overload behavior rather than saying “never drop keys.”

Use a small copied event envelope and a queue capacity chosen after burst measurements, initially 128 as a configurable experiment. Producers do not mutate the consumer's state or dequeue arbitrary events to make room. Repeated navigation may be dropped at the producer on full queue, with a counter. If a pressed/released transition cannot be delivered, set an atomic input-desynchronization flag. The owner consumes that flag before the next event, cancels held/positional modes, clears the affected physical-key state, and shows an input-loss notice. This is safe failure, not lossless input.

Console replay uses a separate bounded request path with acknowledgments after state/frame commit and explicit backpressure. It does not flood physical-key events without waiting. Worker completions use a request/result pool or blocking bounded delivery that preserves job ownership. No huge dump is constructed in a queue item; the owner copies a bounded diagnostic snapshot for the console to format/output.

A rough queue feasibility condition is `capacity >= burst + arrival_rate * maximum_service_pause`, under the declared burst/rate model. The keyboard's 10 kHz I2C bus rate is not an event arrival rate. Measure acquisition intervals, FIFO saturation, scheduling delay, and owner processing separately.

### 13.3 Memory classes and an initial budget

Thirty-two MB of PSRAM is useful, but it is not thirty-two MB of internal SRAM or a guarantee that every allocation lands externally. `CONFIG_SPIRAM_USE_MALLOC` permits ordinary malloc to use PSRAM; thresholds and fallback also permit internal allocations. Static arrays are internal unless explicitly placed/configured otherwise.

Use capability-aware startup allocation for large arenas. Keep task stacks, event queues/control blocks, and the LCD driver's DMA buffer internal. Keep model tables, row arenas, catalog results, and product data in PSRAM where appropriate. Ordinary code and immutable vocabulary strings can remain flash-backed. No PSRAM-backed code/data is accessed from an IRAM-only path or while unavailable during cache-disabled operations.

Illustrative starting allocations, to be recorded with actual `sizeof` values:

| Area | Initial bound | Memory class / behavior |
|---|---:|---|
| LCD private staging | 32,768 B | internal DMA, existing driver |
| CPU row raster | 10,240 B | PSRAM or ordinary CPU memory |
| Two cell frames, 800 cells each at a proposed 4 B/cell | 6,400 B plus decorations | PSRAM; assert actual cell size |
| Type distance matrix, up to 64 types | 8,192 B | model arena |
| Up to 256 fixed rules, 64 relations, 64 command entries | compute at boot | model arena, initial 256 KiB envelope |
| Frame/text/interaction arenas | 2 x 128 KiB | PSRAM, previous/next lifetime |
| Resolver scratch and diagnostic reserve | 128 KiB initial | PSRAM, checked; exact model-derived requirements |
| Product/deck/catalog working data | 4 MiB initial cap | PSRAM, explicit per-product budgets |
| Owner/keyboard/console stacks | initially 16/6/6 KiB | internal, measure and tune |
| Queue and message pool | computed from envelope sizes | internal control + bounded payload pool |

These numbers are provisional limits, not reported consumption. If a model exceeds a limit, boot compilation returns a diagnostic with required and available sizes. Do not silently reduce the type/rule set.

At boot and after every integration milestone record: total/free/minimum internal heap, internal largest free block, total/free/minimum PSRAM, PSRAM largest free block, task high-water marks, arena peaks, query latency percentiles, and queue-loss counters. A “remaining 24 MB” promise without those observations has no evidentiary value.

### 13.4 Failure taxonomy

| Failure | Response |
|---|---|
| Invalid declaration / unsupported profile | boot diagnostic screen + console; no partial model |
| Unknown/dead reference | user-facing stale/missing result, no cast or reuse |
| Ambiguous action | no execution; explain tied rules in developer diagnostics |
| Ambiguous relation | chooser if complete valid choices, otherwise diagnostic |
| Capacity exceeded during semantic query | invalidate query result; no truncated execution authority |
| Text clipped for display | explicit ellipsis/replacement; full detail in inspector/dump |
| Catalog scan incomplete | show progress/more, no false uniqueness/default |
| SPI transfer failure | do not update presented cache; disable positional confirms until coherent frame restored |
| Keyboard desynchronization | reset held modes, report loss/recovery |
| Worker late result | reject stale applicability; free its owned resources exactly once |
| OOM at startup | minimal static diagnostic path; no dependency on failed arenas |

Reserve enough internal static text/log capacity to report failures when PSRAM or model compilation fails. `ESP_ERROR_CHECK` is suitable for truly unrecoverable startup prerequisites; ordinary display/query failures should not reboot the device repeatedly.

## 14. Implementation shape and a worked product

### 14.1 File layout

Use the firmware repository's existing shared-component convention, rather than copying drivers into a new project directory:

```text
F/
  components/
    pbui_core/                 # portable compiler and semantic interpreters
      include/pbui/*.hpp
      src/{model,type_graph,condition,selector,resolve,acceptance}.cpp
      tests/
    pbui_handheld/             # portable command/session/interaction protocol
      include/pbui_handheld/*.hpp
      src/{commands,acquisition,interaction,deck,projection}.cpp
      tests/
    pbui_rows/                 # portable row/cell model and raster
      include/pbui_rows/*.hpp
      src/{document,layout,font,compose,raster}.cpp
      tests/
    picocalc_lcd/              # unchanged baseline API
    picocalc_keyboard/         # unchanged baseline API
  0104-esp32-p4-pbui-handheld/
    CMakeLists.txt             # ESP-IDF only, explicit EXTRA_COMPONENT_DIRS
    sdkconfig.defaults
    partitions.csv
    main/{app_main,key_adapter,lcd_adapter,console,product}.cpp
    main/CMakeLists.txt
    host/CMakeLists.txt        # standalone host build, no IDF required
    tests/golden/
```

Do not include the whole `../components` directory and accidentally build all unrelated firmware components. Name only the required paths. Drop `quickjs_native`, `qjs_service`, `picojs_runtime`, and `picoos_core` from 0104's initial dependency closure. There is no need to remove or modify them for other projects.

### 14.2 A small declaration that exercises real behavior

Start with types `inspectable` (abstract), `file`, `hunk`, `mem`, `segment`, `card`, `app-kind`, and `session`. Add one `global` scope and one `triage` scope. Every concrete type has a descriptor and catalog source where meaningful; session has exactly one live reference.

The first product table is:

| Rule ID | Action | Receiver | Status / command |
|---|---|---|---|
| inspectable.inspect | inspect | descendants of inspectable | push inspector view |
| hunk.revert | revert | hunk | unavailable if protected, otherwise add override |
| hunk.restore | restore | hunk | inapplicable unless reverted |
| mem.pin | pin | mem | set pin intent, create segment in gateway if needed |
| segment.pin | pin | segment | update segment pin intent |
| card.switch | switch | card | activate by stable CardId |
| card.close | close | card | unavailable if last; gateway checks again |
| session.newtile | newtile | session | acquire app-kind, allocate independent CardId |
| session.clear-tray | clear-tray | session | unavailable/inapplicable policy for empty tray explicitly declared |

Mark relevant actions primary through metadata. `task.cycle` is a real action rule if tasks are added; do not hard-code it in Enter's handler. `R` chooses revert or restore by resolved command/key binding metadata and current status, not by product type switches in the shell.

Add one relation `mem.current-segment` exposed to acceptance. It returns an existing segment or None. A `show-segment` command accepts a segment slot and demonstrates accepting a memory via that route. Test two equally ranked relations to show the chooser before any actual application requires ambiguity.

### 14.3 Native declaration sketch

This is an API teaching sketch, not code claimed to compile:

```cpp
auto d = DeclarationBuilder<DemoFacts, DemoVerb>{"demo"};
auto inspectable = d.abstract_type("inspectable");
auto hunk = d.concrete_type("hunk", {inspectable}, hunk_descriptor);
auto session = d.concrete_type("session", {}, session_descriptor);
auto app = d.concrete_type("app-kind", {}, app_descriptor);

d.predicate("hunk.can-revert", [](const DemoQuery& q) -> Availability {
    const Hunk* h = q.facts.lookup_hunk(q.subject);
    if (!h) return unavailable("edit is no longer present");
    if (h->protected_edit) return unavailable("edit is protected");
    return available();
});
d.rule("hunk.revert", "revert", hunk, /*scopes*/ {triage, global},
       predicate("hunk.can-revert"),
       [](const DemoQuery& q) { return DemoVerb{Revert{q.subject}}; });
d.command("revert", action("revert"), receiver_from_slot(0),
          {slot("edit", {hunk})}, visible_receiver());
d.command("newtile", action("newtile"), current_session(),
          {slot("app", {app})}, contextual());
```

The compile step checks and freezes builder-owned strings/tables; no initializer-list spans survive their temporary arrays. The builder should own/copy inputs until compilation, then compiled tables own or reference firmware-lifetime storage. A naive `span` into `{triage,global}` stored for later would dangle.

### 14.4 One complete command trace

Suppose the screen shows two occurrences of hunk H4, and the focused one is `files-row-4` in scope triage. The user types `:rev Space`:

1. Command parser resolves the unique offered spelling revert.
2. Slot construction requests hunk; its default nomination is focused source H4 with reason `it`.
3. Acceptance confirms direct subtype fit and preserves the reference.
4. Action resolution on H4 under triage returns `hunk.revert` available.
5. The interaction frame contains default ticket `(H4,direct,hunk.revert,frame=52)` and a digit map for visible occurrences.
6. Renderer paints the command and selected label. Only after required rows succeed does frame 52 become current.
7. The user presses Enter. The owner validates the request/frame and reconstructs current facts.
8. If H4 has become protected, acceptance still succeeds but fresh action becomes unavailable. Show the reason; no override is added and the slot remains active.
9. Otherwise gateway prepares `Revert(H4)`, validates the object and policy, installs the override, updates semantic revision, logs a successful outcome, and ends the acquisition.
10. The product view rebuilds. Both occurrences display reverted state; the tray remains a reference to H4, not a copy of its old state.

This trace explains why freshness cannot be collapsed into a Boolean type check, and why the same object identity must update across several occurrences.

## 15. Phased intern implementation plan

### Phase A: Host contracts before hardware

**Teaching goal:** distinguish semantics from representation. Create the component directories and the standalone `0104/host/CMakeLists.txt`, with C++20, warnings-as-errors, sanitizers, and a tiny test executable. Do not create a second ESP-IDF root for host tests.

Implement strong IDs, references, explicit errors, checked buffers, and a fake product table. Add generation/ABA tests and owned-string lifetime tests. Write the compatibility matrix before claiming any PBUI suite has been ported.

**Exit evidence:** clean host build, sanitizer run, invalid-reference tests, a reproducible compiler command, and no ESP/React dependencies in portable headers.

### Phase B: Graph, selector, conditions, action resolution

**Teaching goal:** derive selection from the graph/rank algebra. Implement type validation, distances, scope matching, conditions, fixed rules, partition selection, statuses, provenance, and fresh checks. Use simple linear scans over bounded tables first.

Translate fixtures from P's typeGraph, selector, conditions, resolver and freeze/perform suites. Add redundant-edge, extreme-priority, hidden-tie, action invocation, overflow and empty-registry unknown-subject cases. Keep an independent sort/filter oracle in host tests.

**Exit evidence:** per-fixture mapping, no binding of nonselected candidates, permutation tests, and error injection showing no partial executable results.

### Phase C: Acceptance and command schemas

**Teaching goal:** distinguish typed values, translation routes, and command receivers. Implement direct relations, relation-aware source planning, filter semantics, route choices, command vocabulary, slot and receiver policies, and exact/full-name parser behavior.

Use the minimal hunk/memory/segment/card/session product. Add the `newtile` contextual discovery case and the `pin(mem)` no-existing-segment case to prevent the earlier mapping errors. Test catalog source planning against an all-reference brute-force oracle.

**Exit evidence:** command and menu routes share action semantics; wrong-type pronouns refuse; direct-filter failure never falls through to relations; equal-result routes remain ambiguous.

### Phase D: Interaction state machine and logical views

**Teaching goal:** make invalid state combinations unrepresentable. Implement ViewIds/occurrences, deck invariants, return points, command editing, Pending/Choosing, Esc, hints/search, tray/history/defaults, repeat and peek events.

Do not render LCD pixels yet. Use logical/semantic dumps. Add stale request/choice IDs, route retargeting, duplicate occurrences, view deletion, same-depth inspectors, empty views, and input-loss tests. Assert successful settled requests have exactly one terminal outcome after explicit drain/cancel.

**Exit evidence:** deterministic event replay and state-machine property tests, including invalid external events rather than only trusted fixture events.

### Phase E: Cells, font, and host raster

**Teaching goal:** preserve semantic identity through layout and remove it only when generating pixels. Implement RowDocument, viewport, gutter geometry, cell encoding, lowercase glyphs, shared interaction frame, footer/accept layout, help paging, exact dirty-row comparison, and a fake failing LCD sink.

Add text goldens, semantic sidecars, and PPM previews. Test visible-digit decode equality, >viewport objects, hint code lengths, clipped Unicode, lowercase key labels, focus repair before paint, and partial paint failures.

**Exit evidence:** host replay with stable layout-specific goldens, no invisible positional targets, and failed-row retry without a false clean cache.

### Phase F: ESP32 wiring and real measurements

**Teaching goal:** an abstract contract must match the actual driver. Scaffold 0104's IDF project with explicit component paths; copy 0102 defaults and a deliberate partition table, remove its JS dependencies, and preserve UART/PSRAM configuration. Source IDF 5.4.2, set target once, build normally afterward.

Wire the keyboard owner, application owner, console, and synchronous LCD sink. Keep the current 40 MHz setting. Implement `/key`, `/text`, `/dump`, `/state`, `/resolve`, `/mem`, and `/timing` as owner requests with acknowledgments. Include press/repeat/release injection and input-loss/reset events.

**Exit evidence:** captured build log, memory class/stack/queue measurements, current SPI frequency, visual color/glyph checks, timing distributions, single-owner serial capture, and hardware replay of the minimal vertical slice. Do not call a host timing measurement device latency.

### Phase G: Full prototype scenarios and usability corrections

**Teaching goal:** validate behavior rather than cargo-cult the prototype. Port the timeline fold and overrides into the demo product, then the listener, files, edits, tasks, window, memory, overview, and inspectors. Derive event totals from the fixture. Test temporary temporal absence separately from deleted identities.

Replay the six tutorials as semantic assertions, then maintain 40×20 and optional 53×32 goldens separately. Record intentional changes: paged Help, visible-only shortcuts, meaningful Tab selection, no silent default replacement, safe stale menus, and preserved return points.

**Exit evidence:** all tutorials completed on host and hardware, readable physical labels, pin/unpin/forget/eviction temporal invariants, independent duplicate card focus/scroll, and a written usability issue list.

### Phase H: Reuse and only then optional optimization

**Teaching goal:** a framework is validated by a second product, not by additional abstraction. Add either a file browser or a small device-status product through declaration/query/gateway interfaces. Only then decide whether generated families, named relation compositions, a smaller font, asynchronous display transfer, persistence, or split panes are justified.

**Exit evidence:** second product requires no core key-handler type switches. Optimization patches retain differential, property, and failure tests and include before/after measurements.

## 16. Verification strategy and proof obligations

### 16.1 Compatibility profile

| Behavior | V1 commitment |
|---|---|
| Nominal DAG, shortest distance, scoped matching | preserve valid P behavior |
| Four conditions and first failure | preserve, with explicit runtime error for malformed opcode |
| Four action statuses / lexicographic ties / selected-only bind | preserve |
| Fresh same-candidate behavior | preserve; native liveness/context/model checks added |
| Fixed rules | implement |
| Generated families | reject as unsupported initially; keep original fixtures for later phase |
| Direct acceptance relations | implement valid-result behavior |
| Named relation compositions | reject as unsupported initially; no inferred paths |
| Relation error visibility | stronger native diagnostic policy |
| Accept trusted reducer transitions | preserve after native input validation |
| Foreign/stale offer/choice | new native refusal behavior |
| Full React host activation bubbling | no DOM; internal view-local activation intent only |
| Command syntax / command invocation | new explicit declaration and vocabulary |
| Help interpreter / link kernel / workbench tree | defer; simple help projection and flat deck instead |
| Menu collation | deterministic byte/ASCII order in v1, not localeCompare equivalence |
| Priority / menu-order values | signed 32-bit integers in native declarations; reject fractional or out-of-range imported values rather than rounding P's finite numbers |
| Prototype layouts and known bugs | semantic tutorials retained, explicit corrections tested |

### 16.2 Reference refinement, not blind source parity

Let `normalize` turn results from either language into stable names and value IDs, sorting only diagnostic sets and permitted display order. For valid shared-profile fixtures, require

$$
 \mathrm{normalize}(\mathrm{resolve}_{C++}(D,q,S))
 =\mathrm{normalize}(\mathrm{resolve}_{TS}(D,q,S)).
$$

Do not compare pointer addresses, local numeric ID assignments, arena offsets, or string collation artifacts. Do compare selected candidate, status/reason, ambiguity membership, direct versus relation acceptance, concrete result identity, and fresh refusal category.

Every difference gets a compatibility entry. A test marked unsupported is not a pass. Invalid-input diagnostics introduced by the native host have their own tests rather than pretending the TS reducer already promised them.

### 16.3 Property tests

- Graph reachability is reflexive/transitive/antisymmetric; distances match BFS and declared edges are preserved.
- Candidate order permutations leave winners and ambiguity sets unchanged.
- Adding unrelated action IDs leaves existing partitions unchanged.
- Inapplicable candidates can be dropped; hidden/unavailable candidates cannot be dropped before reduction.
- No incomplete/capacity-error resolution exposes an executable command.
- Conservative catalog enumeration followed by exact acceptance matches brute-force enumeration on finite test worlds.
- Equal-output distinct routes remain distinct choices until the route choice is resolved.
- Every request's terminal count is at most one; after draining admitted requests it is exactly one.
- Foreign request/choice/frame IDs produce no state mutation.
- Every displayed digit has exactly one decode target in the same visible frame, and every decode target has a displayed digit.
- Frame-cache rows are updated only for successful writes; failed writes remain dirty.
- Card/session invariants hold after any command sequence, including rejected closes and stale references.
- All resources owned by canceled/completed jobs are released once.

Use seeded generators with shrinkable event traces or a small exhaustive state space. Store failing seeds and minimal reproductions. Random tests support the algebraic reasoning; they do not replace it.

### 16.4 Tutorial replay protocol

A host/device replay script needs more than raw characters:

```text
reset minimal-demo
key press :
text rev
key press space
expect state Acquiring.Pending
expect selected hunk:H4 direct reason=it
expect visible-pick 1 occurrence=files:H4
mutate-test-fact protect H4
key press enter
expect refusal protected
expect state Acquiring.Pending
expect domain H4.reverted=false
key press escape
expect state Browsing
checkpoint protected-revert
```

`mutate-test-fact` exists only in the test product/diagnostic build; it is not a production authorization bypass. On device, each command is acknowledged after the owner has processed it and committed required display rows. A `checkpoint` captures text, semantic sidecar, state/revision, and counters. Pixel screenshots remain useful for hardware quality, but are not the sole oracle.

### 16.5 What the research already ran

- Eleven existing P suites: **307/307 tests passed**.
- Actual-source semantic probes: passed; output retained.
- C++ selection algebra experiment: 64 associativity combinations, 5,000 worlds x 8 permutations, summary laws, priority extremes, overflow and viewport cases; passed under ASan/UBSan.
- Cross-repository move: eleven original files verified by SHA-256 before reference repair.

What is **not** yet established: a production C++ core, native tutorial replay, an ESP-IDF 0104 build, LCD performance for this UI, physical lowercase legibility, or measured memory consumption. Those are phase exit criteria, not completed work.

## 17. Decision records and remaining questions

### ADR 1: Native core with differential oracle

- **Context:** Native C++ is the chosen target, but reimplementation risks semantic drift.
- **Options:** QuickJS reuse, native C++, or mixed JS/C++ semantics.
- **Decision:** Keep one native implementation per device and use TypeScript only as a development oracle. No QuickJS in 0104 v1.
- **Rationale:** Native ownership and toolchain fit the request; the small semantic profile is testable without importing React or the workbench.
- **Consequences:** Translation costs and ongoing compatibility maintenance remain real; no claim of native speedup without profiling.
- **Status:** proposed implementation baseline under the user's native direction.

### ADR 2: Explicit command schema and receiver policies

- **Context:** Rules do not encode command arity or context receivers; `newtile` demonstrates the gap.
- **Options:** Infer slots from subjects, maintain ad hoc CMDS, or declare command syntax with validated mappings.
- **Decision:** Add CommandSpec to the compiled root with typed slots and receiver policies.
- **Rationale:** Separates grammar from dispatch without duplicating action policy; makes contextual commands explicit.
- **Consequences:** One more small declaration table, but no product-specific shell switches. Multi-slot declarations are rejected in v1.
- **Status:** proposed; replaces document 02's inferred-signature approach.

### ADR 3: Reference, occurrence, and view identities

- **Context:** Duplicate presentations, reordered cards, temporal absence, and reused handles invalidate index-only/reference-only focus.
- **Options:** Row indices, raw pointers, reference equality, or stable layered identities.
- **Decision:** Strong IDs, stable view/occurrence keys, generational reusable object handles, explicit liveness.
- **Rationale:** Correct focus and stale execution require knowing which identity changed.
- **Consequences:** Product/view authors must supply stable row keys; snapshots may not retain raw object pointers.
- **Status:** proposed.

### ADR 4: One interaction frame and strict commit-time validation

- **Context:** Prototype key decoding and rendering independently recompute candidate/default state.
- **Options:** Duplicate logic, shared query helpers only, or one frame containing picks/defaults/affordances.
- **Decision:** Build one interaction frame, display it, decode by its IDs, and revalidate acceptance and action separately.
- **Rationale:** Establishes a testable relation between advertised keys and attempted operations, while handling stale facts safely.
- **Consequences:** A frame transition can refuse positional keys; previous/next frame lifetime is explicit.
- **Status:** proposed.

### ADR 5: Synchronous row rendering with current driver

- **Context:** Existing source is 40 MHz polling with private DMA staging; previous double-buffer claims were not supported.
- **Options:** New queued driver now, full framebuffer/LVGL, or reuse synchronous row API.
- **Decision:** One CPU row buffer, existing staging path, exact dirty rows, 40 MHz. New lowercase glyphs and host raster tests.
- **Rationale:** Minimizes new hardware risk and creates a trustworthy measurement baseline.
- **Consequences:** No CPU/SPI overlap initially; full-screen transitions are bounded but not atomic.
- **Status:** proposed; supersedes prior buffer/80 MHz assumptions.

### ADR 6: Bounded resources without blanket zero-allocation dogma

- **Context:** The board has plentiful PSRAM but limited internal RAM and disabled exceptions by default.
- **Options:** Arbitrary heap containers, a custom STL replacement, or checked arenas/buffers where needed and ordinary product containers with budgets.
- **Decision:** Preallocate core/frame scratch; explicit capability arenas and overflow results; use standard language facilities without building a generic container framework.
- **Rationale:** Protects safety and diagnostics while avoiding premature complexity.
- **Consequences:** Budgets must be measured and compiled requirements checked; a malformed/oversized query cannot return partial authority.
- **Status:** proposed.

### ADR 7: Fixed rules and direct relations first

- **Context:** Families and compositions expand the lifetime/validation surface before a useful device slice exists.
- **Options:** Port every API first, partially support all, or explicitly reject deferred forms.
- **Decision:** Ship a documented semantic profile with fixed rules and direct relations; keep deferred fixtures indexed.
- **Rationale:** A smaller correct interpreter is preferable to claiming compatibility with unimplemented semantics.
- **Consequences:** Some PBUI products cannot load unchanged; adding a feature requires expanding the compiler and tests, not silently adapting it.
- **Status:** proposed.

### ADR 8: Flat deck and small UI primitives, not a general window/widget system

- **Context:** Single-screen use needs stable cards, inspectors, selection, lists and overlays, not arbitrary spatial docking.
- **Options:** Port workbench-core and pointer shell, use LVGL, or a flat deck plus row primitives.
- **Decision:** Flat deck with CardId/ViewId, RowDocument/viewport/cell compositor, semantic menus and acquisition surfaces.
- **Rationale:** It matches the device and keeps declarations product-neutral while minimizing hidden widget focus state.
- **Consequences:** No split panes, CSS-like layout, wire geometry, or persistence protocol in v1. A graphics primitive can be added without replacing semantics.
- **Status:** proposed.

### Remaining decisions that require evidence

1. Tune queue, stack, model, frame, and product budgets on the exact board/build. The guide deliberately does not assert actual free RAM.
2. Physically test 8×16 lowercase and tone colors; try 6×10 only after the basic UI is readable and correct.
3. Choose the second product after the minimal demo, not before semantic interfaces have been exercised.
4. Decide whether saved decks/object references need persistence. Dense model IDs are not persistent IDs; a persistence format must use stable vocabulary names/domain keys and versions.
5. Revisit static relation compositions or families only when a real command cannot be expressed cleanly without them. Never emulate them with effectful accept callbacks.
6. Confirm physical flash size and the copied partition layout before allocating large assets; the 0102 defaults request 32 MB and use custom partitions kept below the relevant access constraints.

## 18. Source and API reference map

The reproducible inventory in `T/sources/source-inventory.json` records absolute roots, source hashes, repository commits, and symbol line anchors. Rerun `T/scripts/03-source-inventory.py P F` after changes before relying on line numbers.

### PBUI semantic files

| Source | Entry points / relevant region | Why read it |
|---|---|---|
| `P/src/presentation/actions/typeGraph.ts:55-135` | createPresentationTypeGraph | closed DAG, BFS distances, no payload conversion |
| `P/src/presentation/context/selector.ts:37-171` | activeScope, selectorOf, matchSelector | type/scope/condition order and provenance |
| `P/src/presentation/actions/conditions.ts:22-126` | Condition, evaluateCondition | four operations, first-failure behavior |
| `P/src/presentation/actions/availability.ts:23-52` | Availability factories | absence versus fallback suppression |
| `P/src/presentation/actions/registry.ts:86-191` | createActionRegistry, collision loop | construction validation and conservative diagnostic limits |
| `P/src/presentation/actions/resolve.ts:66-367` | resolveActions | collection, status, partition reduction, binding, display sort |
| `P/src/presentation/actions/perform.ts:25-47` | evaluateFresh | fresh verb with same candidate, not revision equality |
| `P/src/presentation/acceptance/resolve.ts:16-88` | finish, resolveAcceptance | direct-first, exposed relations, route ranking |
| `P/src/presentation/interaction/accept.ts:25-108` | AcceptState/Event, acceptStep | trusted-event transition machine |
| `P/src/presentation/interaction/activation.ts:41-54` | activationOutcome | acceptance, host, primary, menu order |
| `P/src/presentation/relations/system.ts` | discoverable, execute, evaluate, matches | codomain filtering, exposure, concrete result validation |
| `P/src/presentation/model/compile.ts` | mergeFragments, compilePresentation, snapshot | one declaration, structural diagnostics, context validation |
| `P/src/presentation/model/types.ts` | PresentationDeclaration, CompiledPresentation | root and fragment API shape |
| `P/packages/pbui-ecommerce/src/presentation/actions.ts:16-55` | SHOP_TYPES, shopRevision, shopContextFor | product-specific facts and scopes |
| `P/packages/pbui-ecommerce/src/presentation/runtime.tsx:22-43` | createShopPresentation | compiled declaration construction, without importing React host into firmware |
| `P/packages/workbench-core/src/session.ts:9-27` | WorkbenchSession, repairSession | stable IDs and repair discipline |
| `P/packages/workbench-core/src/queries.ts:69-73` | canClose | last-card invariant |

Do not port `P/src/presentation/createPbui.tsx`, `P/src/chrome`, `P/src/components`, or `P/packages/pbui-workbench`. They were not needed to derive the native shell design.

### Prototype evidence

- `T/sources/pbui-handheld.jsx:151-215`: timeline fold, overrides, memory/segment materialization.
- `:221-253`: labels, catalog, line helpers.
- `:319-351`: command signatures and `availCmds`'s explicit card/app additions.
- `:355-450`: product/shell mutations and menu callbacks capturing `S`.
- `:622-645`: primary occurrence indexing and position/depth view keys.
- `:683-709`: accept defaults, tray pronouns, catalog search.
- `:711-791`: Esc and command/accept dispatch, digit selection and default-overrides-Tab behavior.
- `:948-1027`: clipping, duplicated candidate/render logic, chips and documentation line.
- `T/sources/pbui-handheld-manual.md`: six tutorial scenarios, not a pixel-perfect native oracle.
- `T/sources/pbui-handheld-project-report.md`: interaction rationale; verify claims against code as done here.

### Firmware evidence

- `F/components/picocalc_lcd/picocalc_lcd.c:25-34`: 40 MHz default and signal-integrity comment.
- `:52-96`: synchronous polling transmit and internal DMA allocation.
- `:270-311`: blit copy/byte-swap and row wrapper.
- `F/components/picocalc_keyboard/include/picocalc_keyboard.h:16-32`: pins, address, 10 kHz bus rate, event states.
- `F/components/picocalc_keyboard/picocalc_keyboard.c:143-165`: 3.1-second recovery.
- `:188-278`: register delays and event polling.
- `:304-374`: current key-name mapping; old handoff line numbers referred to an earlier file layout.
- `F/components/visual_repl/visual_repl.cpp:29`: CPU row buffer; `:75-149` font lookup, including lowercase-to-uppercase conversion; `:151-202` raster and row call.
- `F/0102-esp32-p4-visual-quickjs-repl/README.md`: IDF 5.4.2 build recipe.
- `F/0102-esp32-p4-visual-quickjs-repl/{CMakeLists.txt,main/CMakeLists.txt,sdkconfig.defaults}`: explicit component closure, UART/PSRAM/custom partition settings.
- `F/0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp:653-679,729-815`: token helper, polling/recovery, pressed/repeated filter; reuse vocabulary, not ownership.
- `F/AGENTS.md`: pinned toolchain, config regeneration, serial ownership, artifacts policy.

### Foundational and platform references

1. LispWorks CLIM 2.0 User Guide §8.5, presentation translators and presentation-to-command translators: <https://www.lispworks.com/documentation/lw80/clim/clim-ch8-5.htm>. Extract in `T/sources/clim-translator-examples.md`. Used to distinguish value translation from command invocation; PBUI does not claim full CLIM compatibility.
2. David Harel, *Statecharts: A Visual Formalism for Complex Systems*, Science of Computer Programming 8 (1987), 231–274. Introduction and §3 explain hierarchical alternatives and orthogonal products: <https://www.zingstudio.io/static/statecharts.pdf>. Extract in `T/sources/harel-statecharts-1987.txt`. The native design uses those structuring ideas, not the full formalism.
3. ESP-IDF **v5.4.2, ESP32-P4**, C++ support: <https://docs.espressif.com/projects/esp-idf/en/v5.4.2/esp32p4/api-guides/cplusplus.html>. Extract in `T/sources/idf-5.4.2-p4-cplusplus.md`; local `~/esp/esp-idf-5.4.2/tools/cmake/build.cmake:159,197` confirms compiler dialect defaults.
4. ESP-IDF **v5.4.2, ESP32-P4**, external RAM: <https://docs.espressif.com/projects/esp-idf/en/v5.4.2/esp32p4/api-guides/external-ram.html>. Extract in `T/sources/idf-5.4.2-p4-external-ram.md`. Used for allocation preferences, internal reservations, stack/default restrictions, and cache constraints.

The graph, rank-fold, condition, catalog-completeness, and transaction arguments in this guide are derived explicitly from the code and definitions above. They do not depend on an unverified external citation.

## 19. Build, review, and handoff checklist

Once implementation exists, the intended command sequence is:

```bash
# Standalone host harness; paths are proposed Phase A outputs.
cmake -S 0104-esp32-p4-pbui-handheld/host -B /tmp/pbui-host \
  -DPBUI_SANITIZERS=ON
cmake --build /tmp/pbui-host -j
ctest --test-dir /tmp/pbui-host --output-on-failure

# Device; read F/docs/01-playbook-esp-idf-build-and-dev-environment.md first.
cd 0104-esp32-p4-pbui-handheld
source /home/manuel/esp/esp-idf-5.4.2/export.sh
idf.py set-target esp32p4  # once, not every rebuild
idf.py build
```

Use a stable `/dev/serial/by-id/...` path when available. Before flashing, establish that the exact port is free; do not kill unrelated monitors. Flash, then start **one** tmux monitor. Capture evidence with `tmux capture-pane -p`. Never open a second serial reader merely to obtain a dump. No live hardware access is required to review this design document.

If defaults need to override generated settings, remember that `sdkconfig.defaults` only seeds absent options: save any intentional local config changes, remove the generated `sdkconfig` deliberately, then rebuild. `idf.py fullclean` alone does not remove it. Copy a real partition table with the custom-partition defaults rather than creating a project that references a missing file.

Before approving each phase, ask:

- Which existing behavior is preserved, which is intentionally changed, and which remains unsupported?
- What exact identity is retained at each deferred boundary?
- Can a capacity error, stale event, or partial draw cause a different target to execute?
- Is every advertised key backed by the same frame that renders its target?
- Are observed timings and memory numbers clearly separated from targets?
- Can the host tests reproduce the failure without a camera or physical keyboard?

The first implementation should be judged by those answers, not by how closely its class names resemble TypeScript or how many abstract interfaces it introduces.
