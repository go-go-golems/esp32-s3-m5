---
Title: Investigation Diary
Ticket: ESP-61-REUSABLE-NFC-COMPONENT
Status: active
Topics:
    - nfc
    - esp-idf
    - st25r3916
    - m5stackchan
    - component-architecture
    - intern-guide
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: repo://0115-m5stackchan-nfc-reader/test_host/test_st25r_trace.c
      Note: Evidence for host-testable trace extraction
    - Path: repo://0116-m5stackchan-nfc-debug-ui/overlay/firmware/main/apps/app_nfc_debug/nfc_debug_service.h
      Note: Evidence for worker ownership decision
    - Path: repo://0117-m5stackchan-nfc-feature-explorer/dependencies.lock
      Note: Pinned upstream revisions used during architecture assessment
    - Path: repo://0117-m5stackchan-nfc-feature-explorer/main/nfc_console.cpp
      Note: Evidence of console, reboot, confirmation, and output policy to move out of core
ExternalSources: []
Summary: Chronological evidence and decisions used to design the reusable native ESP-IDF NFC component extraction.
LastUpdated: 2026-08-22T19:30:00-04:00
WhatFor: Preserve how the current implementations were assessed and how the extraction architecture was chosen.
WhenToUse: Read before implementing or reviewing ESP-61, especially when changing ownership, lifecycle, safety, or dependency boundaries.
---


# Investigation Diary

## Goal

This diary records the evidence gathering and architecture work for extracting the proven ESP-60 NFC implementations into a reusable ESP-IDF component. It is intended to let a new engineer continue without rediscovering which code is production-quality, which code is diagnostic, and which policies must remain outside the component.

## Step 1: Create the extraction ticket and map the existing implementations

The first step created a dedicated ticket rather than extending ESP-60. ESP-60 established reader correctness and broad NFC functionality; ESP-61 has a different objective: define a stable component boundary suitable for multiple applications and for an intern to implement safely.

I inspected `0115`, `0116`, `0117`, the pinned dependency lock, M5Unit-NFC, and M5UnitUnified. The evidence showed that no single first-party drop-in component exists yet. Instead, the repository contains a strong low-level trace module, a minimal singleton diagnostic driver, a correct production worker pattern, and a broad feature explorer that still contains application policy.

### Prompt Context

**User prompt (verbatim):** "Ok, Create a new docmgr ticket and create  a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and the nupload to remarkable."

**Assistant interpretation:** Create a new ticket dedicated to reusable-component extraction, document the current system and proposed implementation in enough depth for a new intern, maintain ticket bookkeeping and diary evidence, validate it, and deliver the documentation as a reMarkable bundle.

**Inferred user intent:** Convert the successful experimental NFC work into an implementation-ready architecture that can be reused across projects without copying application-specific code.

### What I did

- Created ticket `ESP-61-REUSABLE-NFC-COMPONENT` with topics `nfc`, `esp-idf`, `st25r3916`, `m5stackchan`, `component-architecture`, and `intern-guide`.
- Added the primary design doc and this investigation diary.
- Added six explicit tasks covering architecture mapping, API design, intern guide, testing, validation, and reMarkable delivery.
- Inspected `0115/main/st25r3916`, `0115/main/st25r_trace`, and host trace tests.
- Inspected `0116` service ownership and `hal_bridge::board_get_i2c_bus()` integration.
- Inspected `0117` application, explorer, console, manifest, and lockfile.
- Inspected upstream native ESP-IDF bus attachment and license/component files.

### Why

- Component design must be grounded in code that has already passed hardware tests.
- The three projects solve different problems, so treating one directory as the reusable component would preserve the wrong boundaries.
- The intern needs exact source locations and a reasoned separation of mechanism, policy, ownership, and presentation.

### What worked

- `NfcExplorer::begin(i2c_master_bus_handle_t, NfcBootMode)` already establishes the correct application-owned bus boundary.
- `0116` already demonstrates the correct queue/worker/snapshot pattern for UI integration.
- `st25r_trace` already has instance-based state and host tests.
- The pinned dependency graph provides exact upstream revisions rather than floating `main` behavior.

### What didn't work

- The first search used a nonexistent path:

  ```text
  rg: 0116-m5stackchan-nfc-debug-ui/main: No such file or directory (os error 2)
  ```

  The overlay actually stores NFC LAB under:

  ```text
  0116-m5stackchan-nfc-debug-ui/overlay/firmware/main/apps/app_nfc_debug/
  ```

- There was no existing `components/` implementation to evaluate as a finished extracted component.

### What I learned

- “ESP-IDF component” has two meanings in the current tree. `main/` is compiled as an ESP-IDF component, but it is an application component, not a reusable library boundary.
- Upstream M5Unit-NFC is already the correct reusable protocol implementation. The repository-specific value is lifecycle, safety, structured results, worker ownership, diagnostics, and integration policy.
- The cleanest asset is `st25r_trace`; the broadest asset is `NfcExplorer`; the strongest production ownership example is `nfc_debug::Service`.

### What was tricky to build

- The difficult part was separating code quality from reuse quality. `0117` is highly successful as a hardware feature explorer, but direct printing, global NVS functions, hard-coded records/keys, and reboot behavior make it a poor core library API.
- `0115` has a clean-looking C header but process-wide static device handles and trace state in the implementation. The public surface alone does not reveal instance and concurrency limits.

### What warrants a second pair of eyes

- Verify the upstream teardown capabilities before promising runtime reader-to-target transitions.
- Verify how precisely upstream failures distinguish no-card, transport, authentication, and protocol outcomes.
- Confirm licensing/notice requirements when wrapping and redistributing the pinned M5 components.

### What should be done in the future

- Keep the three evidence roles explicit: `0115` regression harness, `0117` component example, and `0116` integration consumer.
- Do not delete the proven examples during extraction.

### Code review instructions

- Start with `0117/main/nfc_explorer.hpp` and map each member to mechanism or policy.
- Compare `0116/.../nfc_debug_service.h` for ownership and snapshots.
- Inspect static state at the top of `0115/main/st25r3916/st25r3916.c`.
- Run `rg -n 'printf|nvs_|esp_restart' 0117-m5stackchan-nfc-feature-explorer/main` to see current coupling.

### Technical details

- Direct M5Unit-NFC revision: `93745b547364f310cd64b5155a870103a7800a5d`.
- Resolved M5UnitUnified revision: `bf711f370047cf16355b00005450ef615fab36e2`.
- Native integration API: `UnitUnified::add(Component&, i2c_master_bus_handle_t)`.
- Production shared-bus API: `hal_bridge::board_get_i2c_bus()`.

## Step 2: Design the layered component and intern implementation path

The second step converted the evidence into a concrete component architecture. The central design is a synchronous, instance-based Engine with an optional single-owner Service. This supports direct console programs and multi-task UI applications without imposing one runtime model on both.

The guide defines public domain types, typed errors, tag sessions, mutation permits, write reports, NDEF and Classic APIs, emulation profiles, queue ownership, shutdown ordering, adapters, a package tree, decision records, ten implementation phases, a test pyramid, migration mapping, and definition-of-done criteria.

### Prompt Context

**User prompt (verbatim):** (same as Step 1)

**Assistant interpretation:** (same as Step 1)

**Inferred user intent:** (same as Step 1)

### What I did

- Designed `components/gogolem_nfc` as the core C++17 component.
- Separated optional console, NVS, and transaction-trace adapters.
- Defined application-owned I²C bus and Engine-owned NFC device state.
- Defined synchronous Engine and asynchronous Service APIs.
- Defined `TagInfo`, `Error`, `Result<T>`, `MutationPermit`, `WriteReport`, NDEF, Classic, and emulation contracts.
- Added sequence, state, architecture, ownership, NDEF-write, and test-pyramid Mermaid diagrams.
- Added decision records for upstream wrapping, bus ownership, Engine/Service separation, typed results, mode lifecycle, confirmation policy, trace packaging, and migration compatibility.
- Added phased implementation and acceptance criteria from baseline through NFC LAB integration.

### Why

- A reusable component needs stable semantics independent of serial output and UI framework.
- Multi-task applications require serialization, while small test applications should not be forced to allocate a worker task.
- Mutation safety requires machine-checkable UID/family/address policy and detailed restoration outcomes, not only a human confirmation phrase.

### What worked

- Existing code maps cleanly into the proposed layers without requiring new NFC protocol research.
- The design preserves proven REQA/WUPA behavior and shared-bus ownership.
- The component can remain board-independent because pins and controller creation stay with the application.
- The staged plan keeps read-only behavior ahead of mutation and integration risk.

### What didn't work

- N/A — this step produced documentation and proposed APIs; implementation has not started.

### What I learned

- A TagSession object is useful because NFC activation creates an obligation to deactivate and because an upstream PICC should not leak through the public API.
- Restoration needs a report with separate attempted/succeeded fields; it cannot be compressed into the main operation result.
- The worker queue cannot safely carry arbitrary temporary vector pointers. NDEF mutation payload ownership must be designed explicitly.

### What was tricky to build

- The API must remain precise without pretending upstream teardown and error classification are already proven. The design therefore marks runtime mode switching as proposed and keeps open questions visible.
- Public NDEF types should avoid upstream coupling, but vectors have allocation implications. The guide records that tradeoff instead of choosing an undocumented memory model.

### What warrants a second pair of eyes

- Review whether `gogolem_nfc` is the preferred long-term component name.
- Review TagSession destructor semantics; explicit `close()` must remain the authoritative error-reporting path.
- Review whether Service should support NDEF writes in version one or reserve them for a synchronous owner until a bounded request pool exists.

### What should be done in the future

- Resolve upstream teardown behavior in Phase 2 before freezing lifecycle API promises.
- Add a C facade only after a real C consumer exists.
- Introduce a generic reader-backend abstraction only after a second proven front end exists.

### Code review instructions

- Read the design guide sections 7–14 for package/API/ownership decisions.
- Read sections 15–17 for file-level implementation and testing.
- Validate every proposed responsibility against the migration table in section 17.

### Technical details

- Core has no `esp_console`, NVS, LVGL, Mooncake, GPIO, bus creation, reboot, or demo-content dependency.
- Mode is selected before `begin()` and immutable while ready in version one.
- Engine is non-thread-safe by contract; Service is the supported multi-task owner.
- Mutation permits bind operation kind to the selected UID.

## Step 3: Validate, commit, and deliver the research bundle

The final step validated the ticket as a documentation product and delivered it to reMarkable. Frontmatter validation passed for the primary guide and diary. `docmgr doctor` initially identified one new vocabulary term, which was added with a concrete description. The clean rerun passed all checks.

The guide, index, and diary were bundled into one PDF with a depth-two table of contents. A dry run confirmed the exact files and destination before the real upload. The successful upload message is the delivery evidence.

### Prompt Context

**User prompt (verbatim):** (same as Step 1)

**Assistant interpretation:** (same as Step 1)

**Inferred user intent:** (same as Step 1)

**Commit (docs):** `3e275482b112b4e5a1b69a634721d28bfe187883` — "ESP-61: design reusable ESP-IDF NFC component"

### What I did

- Counted and inspected the 9,343-word primary guide and 1,590-word diary.
- Verified balanced Markdown fences and five Mermaid diagrams in the design guide.
- Ran `docmgr validate frontmatter` on the guide and diary.
- Added vocabulary topic `component-architecture`.
- Ran `docmgr doctor --ticket ESP-61-REUSABLE-NFC-COMPONENT --stale-after 30` to a clean result.
- Ran `git diff --cached --check`, fixed its one finding, and committed the ticket package.
- Ran the reMarkable bundle upload in dry-run mode.
- Uploaded the real bundle to `/ai/2026/08/22/ESP-61-REUSABLE-NFC-COMPONENT`.

### Why

- A design package is not complete until its metadata, vocabulary, file relations, task state, rendering inputs, and delivery are verified.
- The dry run prevents accidental destination or file-selection mistakes.

### What worked

- Both docmgr frontmatter validations passed.
- The clean doctor result was:

  ```text
  ESP-61-REUSABLE-NFC-COMPONENT
  - ✅ All checks passed
  ```

- The real upload returned:

  ```text
  OK: uploaded ESP-61 Reusable Native ESP-IDF NFC Component Guide.pdf -> /ai/2026/08/22/ESP-61-REUSABLE-NFC-COMPONENT
  ```

### What didn't work

- A custom validation script incorrectly assumed `tasks.md` had YAML frontmatter and stopped with:

  ```text
  AssertionError: tasks.md
  ```

  `tasks.md` and `changelog.md` are docmgr bookkeeping files and do not use the same frontmatter contract as index/design/reference docs. The authoritative `docmgr validate` and `docmgr doctor` commands were used instead.

- The first doctor run warned:

  ```text
  unknown topics value(s): component-architecture (3 docs)
  ```

  This was resolved with:

  ```bash
  docmgr vocab add --category topics --slug component-architecture \
    --description 'Architecture and packaging of reusable software components and their public integration boundaries'
  ```

- The first staged commit check stopped with:

  ```text
  changelog.md:16: new blank line at EOF.
  ```

  The extra generated trailing blank line was removed, then `git diff --cached --check` passed.

### What I learned

- Ticket document types and bookkeeping files have different metadata contracts; validation scripts should target docmgr document files rather than every Markdown file uniformly.
- Adding a precise vocabulary entry is preferable to replacing a useful topic with a less specific existing term.
- reMarkable can render the complete long-form package directly from the index, design guide, and diary without intermediate manual PDF work.

### What was tricky to build

- The main guide contains many nested C++ and pseudocode blocks plus Mermaid diagrams. Balanced-fence validation was necessary before Pandoc rendering.
- The ticket skill requires dry-run delivery, while the upload tool avoids redundant status/list calls. The final sequence used exactly the useful two calls: dry run and real upload.

### What warrants a second pair of eyes

- Review the rendered Mermaid diagrams and wide API tables on the reMarkable page size.
- Review the open lifecycle and allocation questions before implementation starts; documentation completeness does not resolve those empirical questions.

### What should be done in the future

- Begin implementation with Phase 0 baseline verification and Phase 1 domain types.
- Update this diary at every implementation evidence boundary.

### Code review instructions

- Review commit `3e275482` for ticket creation, design, diary, tasks, changelog, and vocabulary only.
- Run `docmgr doctor --ticket ESP-61-REUSABLE-NFC-COMPONENT --stale-after 30`.
- Render the same three-file bundle locally if PDF layout changes are needed.

### Technical details

- Bundle name: `ESP-61 Reusable Native ESP-IDF NFC Component Guide.pdf`.
- Remote path: `/ai/2026/08/22/ESP-61-REUSABLE-NFC-COMPONENT`.
- ToC depth: 2.
- Included files: `index.md`, primary design guide, and investigation diary.
