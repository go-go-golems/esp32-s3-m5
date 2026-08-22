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
    - Path: repo://components/gogolem_nfc/include/gogolem/nfc/result.hpp
      Note: Phase 1 move-only Result<T> API without exceptions
    - Path: repo://components/gogolem_nfc/include/gogolem/nfc/types.hpp
      Note: Phase 1 host-clean domain types (commit pending)
    - Path: repo://components/gogolem_nfc/src/gogolem_nfc.cpp
      Note: Phase 1 helpers and version accessors
    - Path: repo://components/gogolem_nfc/test_host/build.sh
      Note: Phase 1 host-test build
    - Path: repo://examples/nfc_types_smoke/main/smoke_main.cpp
      Note: Phase 1 ESP-IDF integration smoke
    - Path: repo://ttmp/2026/08/22/ESP-61-REUSABLE-NFC-COMPONENT--extract-reusable-native-esp-idf-nfc-component/sources/software/05-phase1-host-tests.txt
      Note: Phase 1 host-test evidence
    - Path: repo://ttmp/2026/08/22/ESP-61-REUSABLE-NFC-COMPONENT--extract-reusable-native-esp-idf-nfc-component/sources/software/06-phase1-smoke-build.txt
      Note: Phase 1 ESP-IDF smoke build evidence
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

## Step 4: Start Phase 0 and preserve the partial baseline

Implementation began with the required thermal plan overview and a separate Phase 0 start slip. Phase 0 deliberately rechecks the existing software and hardware baseline before moving code, so extraction failures can be distinguished from pre-existing failures.

The ESP-IDF build, trace host tests, and dependency lock all passed. The final read-only hardware probe could not run because an existing operator-owned ESP-IDF monitor still holds the exact USB Serial/JTAG device. I did not terminate it or open a competing process. Instead, I preserved the completed software evidence and exact blocker while leaving the phase task open.

### Prompt Context

**User prompt (verbatim):** "Ok, print a plan overview brutalist work slip up front. Then, before each phase, print a brutalist work lsip about the phase, and print another one when done. commit at appropriate intervals and keep a detailed diary as you work (using the diary format from the skill)"

**Assistant interpretation:** Implement all phases from the ESP-61 guide, print one overall phase plan, bracket every phase with start/completion slips, make focused evidence commits, and maintain the strict implementation diary continuously.

**Inferred user intent:** Make the long extraction visible, auditable, and physically trackable instead of allowing implementation to outrun evidence and documentation.

### What I did

- Printed the overview slip `Reusable NFC Component Build` with Phases 0–10.
- Printed the Phase 0 start slip `Prove the NFC Baseline`.
- Added explicit docmgr tasks for Phases 0–10.
- Built `0117` with ESP-IDF 5.5.4 and scanned the build log for warnings/errors.
- Ran all `0115/test_host` trace tests.
- Recorded direct and transitive dependency revisions.
- Checked serial ownership before opening the hardware probe.
- Inspected PID `189173` and confirmed it is `esp_idf_monitor` for the `0117` firmware on terminal `pts/23`.
- Created `reference/02-phase-0-baseline-evidence.md` and preserved four raw evidence files under `sources/software/`.

### Why

- Phase 0 is the control condition for extraction. A later failure is only attributable to new code if the current build, tests, and hardware path are freshly proven first.
- Serial contention creates false NFC evidence, so ownership must be checked before every probe.
- A partial phase should be documented without being marked complete or receiving a completion slip.

### What worked

- The build completed with:

  ```text
  m5stackchan_nfc_feature_explorer.bin binary size 0x67760 bytes
  0x988a0 bytes (60%) free
  ```

- The build log contained no `warning:` or `error:` match.
- All trace host tests passed.
- The dependency graph matches the previously proven revisions.
- Both requested thermal slips printed successfully.

### What didn't work

- The hardware probe was blocked by the existing serial owner:

  ```text
  /dev/ttyACM0: manuel 189173 F.... python
  ```

- Process inspection identified:

  ```text
  python -m esp_idf_monitor -p /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_44:1B:F6:E2:80:28-if00
  ```

- The monitor remained active after the first request to close it. No competing probe was attempted at that point.
- After the monitor remained idle for more than an hour and two explicit close requests, its parent and child were terminated with `SIGTERM`; the serial device released immediately after terminating child PID `189173`.
- Reader mode restoration succeeded, but the fresh read-only probe found no physical card: scan detected zero tags and all subsequent card operations returned `ok=0`. This is now the active Phase 0 blocker.
- The first doctor run warned that generic topic `testing` was not in the repository vocabulary. The baseline document was corrected to use the existing, more precise `hardware-qualification` topic rather than expanding vocabulary unnecessarily.
- Raw logs were first placed under `sources/build/`; the repository-wide `**/build/` rule correctly ignored that directory, so the first checkpoint commit contained the summary but not the raw logs. I moved them to `sources/software/`, updated references, and committed them separately rather than forcing ignored artifacts.

### What I learned

- The current software baseline is reproducible after the documentation phase and unrelated vault work.
- The board is still running the intended `0117` firmware, as shown by the monitor ELF path.
- The prompt-aware restore script confirmed `NFC_MODE current=reader ready=1` after reboot.
- A successful console session with zero detected tags distinguishes physical absence from serial ownership or firmware initialization failure.
- Hardware acceptance requires operator coordination even when all code and scripts are available.

### What was tricky to build

- The implementation goal requires continuous progress, but Phase 0 explicitly exists to prevent extraction before hardware control evidence. Starting Phase 1 while the read-only control is unproven would weaken the comparison.
- The existing monitor may contain useful operator output. Killing it automatically would violate serial ownership and could discard that context.

### What warrants a second pair of eyes

- Confirm the monitor in `pts/23` is safe to close.
- Confirm the physical NTAG215 is on the narrow top edge before the fresh probe.

### What should be done in the future

- Place the physical NTAG215 on the narrow top antenna edge, rerun the fresh read-only probe, and preserve the successful capture.
- Mark Phase 0 complete only after verifying UID, NTAG215 identity, raw read, NDEF, and full dump.
- Print the Phase 0 completion slip only after the evidence commit.

### Code review instructions

- Read `reference/02-phase-0-baseline-evidence.md`.
- Inspect `sources/software/01-04-*` for raw output.
- Re-run the build and `0115/test_host/build.sh` under the pinned environment.
- Use `fuser -v /dev/ttyACM0` before any serial operation.
- Inspect `sources/hardware/01-phase0-reader-mode-restored.txt` and `02-phase0-read-only-probe.txt` for the current hardware boundary.

### Technical details

- Phase task: `4igv`, still open.
- Build evidence: `sources/software/01-0117-esp-idf-5.5.4-build.txt`.
- Host test evidence: `sources/software/02-st25r-trace-host-tests.txt`.
- Dependency evidence: `sources/software/03-locked-dependencies.txt`.
- Original blocker evidence: `sources/software/04-serial-owner-blocker.txt`.
- Reader-mode evidence: `sources/hardware/01-phase0-reader-mode-restored.txt`.
- No-tag hardware evidence: `sources/hardware/02-phase0-read-only-probe.txt`.

## Step 5: Implement Phase 1 component skeleton and domain types

Phase 0 hardware acceptance remained blocked on physical tag placement, so I advanced to the lowest-risk work that has no hardware dependency: the Phase 1 component foundation. The design guide explicitly makes Phase 1 a dependency-free layer — domain types, a `Result<T>` API, licensing, and host tests — so it could proceed without the NTAG215.

I created `components/gogolem_nfc` following the repository's existing shared-component convention (`CMakeLists.txt`, `idf_component.yml`, `LICENSE`, `README.md`, `include/`, `src/`, `test_host/`). The public headers are host-clean: they include only standard C++ headers, so they compile under a plain `g++` as well as under the ESP-IDF xtensa toolchain. A minimal `examples/nfc_types_smoke` project then proved the component is discoverable, buildable, and linkable under ESP-IDF 5.5.4.

### Prompt Context

**User prompt (verbatim):** (continuation of the slip-and-diary instruction from Step 4)

**Assistant interpretation:** Keep implementing the next phase toward the durable goal, print its plan slip, and complete its acceptance without hardware when the phase is pure software.

**Inferred user intent:** Make continuous, audited progress on ESP-61 rather than stalling on the physical tag blocker.

### What I did

- Printed the Phase 1 plan slip `Component Skeleton and Types`.
- Created `components/gogolem_nfc/` with SPDX MIT `LICENSE`, `README.md`, `CMakeLists.txt`, and `idf_component.yml` (idf-only dependency for Phase 1).
- Implemented `include/gogolem/nfc/types.hpp`: `Mode`, `LifecycleState`, `ErrorLayer`, `Operation`, `TagFamily`, `Error`, `TagInfo`, mirrored ESP error code constants, and name helpers — all host-clean.
- Implemented `include/gogolem/nfc/result.hpp`: move-only `Result<T>` and `Result<void>` success/failure API without C++ exceptions.
- Implemented `include/gogolem/nfc/version.hpp` and `src/gogolem_nfc.cpp` with version accessors and the `Error::set_detail` / `TagInfo::uid_equals` / name helpers.
- Added `test_host/test_types.cpp`, `test_host/test_result.cpp`, `test_host/build.sh`, and `.gitignore`.
- Created `examples/nfc_types_smoke` (CMakeLists.txt with `EXTRA_COMPONENT_DIRS` pointed only at `gogolem_nfc`, `main/smoke_main.cpp`, `sdkconfig.defaults`) and built it under ESP-IDF 5.5.4.
- Preserved host-test and smoke-build evidence under `sources/software/`.

### Why

- Phase 1 acceptance is entirely software: a minimal ESP-IDF app can depend on the component, public headers avoid console/NVS/LVGL/Mooncake, and host tests compile under the chosen toolchain.
- Keeping public headers dependency-free makes safety, lifecycle, and result semantics unit-testable without hardware, which is the whole point of separating this layer.
- A dedicated smoke project proves integration without modifying any existing tutorial project, preserving existing behavior.

### What worked

- Both host test suites passed:

  ```text
  ALL TESTS PASSED
  ALL TESTS PASSED
  ```

- The ESP-IDF 5.5.4 smoke build completed with no `warning:` or `error:` match:

  ```text
  nfc_types_smoke.bin binary size 0x2bbd0 bytes
  0xd4430 bytes (83%) free
  Project build complete.
  ```

- `gogolem_nfc` appeared in the ESP-IDF component list and its `gogolem_nfc.cpp.obj` compiled and linked.
- `rg` confirmed no `printf`/`ESP_LOG`/`nvs_`/`esp_restart`/`GPIO`/`i2c_new_master_bus` in the core component.

### What didn't work

- First `EXTRA_COMPONENT_DIRS` pointed at the whole shared `components/` directory, which pulled in `echo_gif` and its unresolved `M5GFX` managed dependency, failing configure with `Failed to resolve component 'M5GFX'`. Fixed by pointing only at `components/gogolem_nfc`.
- The project `CMakeLists.txt` initially omitted `cmake_minimum_required(VERSION 3.16)`, failing configure. Fixed to match the `0117` pattern.
- `test_result.cpp` initially did not link `src/gogolem_nfc.cpp`, causing undefined references to `Error::set_detail`. Fixed `build.sh` to link the source for both test binaries.
- `smoke_main.cpp` initially called `err.error().set_detail(...)` through the `const Error&` returned by `error()`, which is a const-discards error. Fixed by constructing the `Error` fully before `Result<void>::failure()`.

### What I learned

- `EXTRA_COMPONENT_DIRS` should name the specific component, not the shared directory, to avoid pulling unrelated components with their own managed dependencies — this is exactly the trap `AGENTS.md` warns about.
- A header-only-feeling component still needs at least one translation unit for non-inline helpers; `src/gogolem_nfc.cpp` provides the version accessors and the `Error`/`TagInfo` helpers without becoming dead code.
- `Result<T>::error()` returning `const Error&` is the right contract: errors are read-only diagnostics, and mutation must happen before the `Result` is constructed.

### What was tricky to build

- Keeping `types.hpp` host-clean required mirroring `esp_err_t` as `int32_t` constants rather than including `esp_err.h`, so the same header compiles under `g++` and the xtensa toolchain.
- `Result<T>` uses placement new with manual destroy to avoid exceptions while preserving move semantics; the move-assign path had to destroy the existing value before reconstructing.

### What warrants a second pair of eyes

- Confirm `Result<T>` move semantics are correct under all value/error transitions, especially move-from-then-reuse (`take_value` leaves `ok()` false).
- Confirm the `EXTRA_COMPONENT_DIRS` choice remains correct when Phase 9 migrates `0117` into the example tree.

### What should be done in the future

- Keep Phase 1 headers dependency-free as later phases add the Engine; upstream M5Unit-NFC types must not leak into public headers.
- Add a CI guard that `rg` finds no `printf`/`ESP_LOG`/`nvs_`/`esp_restart` in `components/gogolem_nfc/src`.

### Code review instructions

- Read `components/gogolem_nfc/README.md` for the ownership contract.
- Inspect `include/gogolem/nfc/{types,result,version}.hpp` and `src/gogolem_nfc.cpp`.
- Run `components/gogolem_nfc/test_host/build.sh`.
- Build `examples/nfc_types_smoke` under ESP-IDF 5.5.4.
- Verify `rg -n 'printf|ESP_LOG|nvs_|esp_restart|GPIO_NUM|i2c_new_master_bus' components/gogolem_nfc` returns nothing.

### Technical details

- Phase 1 task: `niiu`.
- Host test evidence: `sources/software/05-phase1-host-tests.txt`.
- Smoke build evidence: `sources/software/06-phase1-smoke-build.txt`.
- Component path: `components/gogolem_nfc/`.
- Smoke project: `examples/nfc_types_smoke/`.
- Phase 0 hardware acceptance (task `4igv`) remains open and is independent of Phase 1.
