---
Title: Investigation diary
Ticket: 0104-CARDCORE-MESHCORE
Status: active
Topics:
    - esp-idf
    - esp32-s3
    - cardputer
    - lora
    - meshcore
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: repo://0105-cardcore-mesh-terminal
      Note: Task 5ptk firmware scaffold built at commit eded12a317268664c7bb7b598e4e0b4ec8ae57a1
    - Path: repo://0105-cardcore-mesh-terminal/dependencies.lock
      Note: Resolved Arduino-ESP32 3.3.10 component graph
    - Path: repo://components/cardputer_kb
      Note: Local reusable keyboard evidence inspected during investigation
    - Path: repo://ttmp/2026/07/13/0104-CARDCORE-MESHCORE--cardputer-adv-meshcore-companion-terminal/design-doc/01-cardputer-adv-meshcore-companion-terminal-architecture-and-implementation-guide.md
      Note: Primary evidence-backed architecture guide produced during this investigation
    - Path: repo://ttmp/2026/07/13/0104-CARDCORE-MESHCORE--cardputer-adv-meshcore-companion-terminal/sources/plai/main/hal/hal_cardputer.cpp
      Note: Native-IDF Cap expander and SX1262 initialization evidence
ExternalSources: []
Summary: Chronological evidence and decisions for the Cardcore MeshCore terminal.
LastUpdated: 2026-07-13T19:56:19-04:00
WhatFor: Allow a future implementer to continue the research and bring-up without rediscovering constraints.
WhenToUse: Read before modifying the proposed Cardcore firmware or taking ownership of the attached board serial port.
---



# Diary

## Goal

This diary records the evidence-gathering and design work for a standalone ESP-IDF MeshCore Companion terminal on Cardputer-ADV plus Cap LoRa-1262, including what was deliberately not probed on the attached hardware.

## Step 1: Create the Cardcore research ticket

I created a dedicated active docmgr ticket rather than attaching this design to an unrelated Cardputer project. The ticket contains a design document and this diary so that implementation can begin from an evidence-backed plan and retain a chronological record.

The requested deliverable is a firmware plan and reMarkable bundle, not a flash of a new unreviewed image. The connected Cardputer's stable USB Serial/JTAG path was observed, but the port was not opened by a monitor/flasher in this step.

### Prompt Context

**User prompt (verbatim):** "Create a new docmgr ticket to build this firmware.

Create  a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and the nupload to remarkable.

Investigate eexisting firmwares and their tickets and see if we can reuse things. I connected a cardputer-adv over usb (with jtag and serial) with the hat on.

Keep a deteailed diary as you work."

**Assistant interpretation:** Create a ticketed, intern-oriented, evidence-backed design for an ESP-IDF MeshCore handheld and deliver its documentation to reMarkable; investigate both local and public donor work while preserving the connected device.

**Inferred user intent:** Establish a low-risk implementation foundation for a portfolio-quality standalone MeshCore terminal and avoid rediscovering hardware/protocol integration pitfalls.

### What I did
- Ran `docmgr status --summary-only` and inspected repository projects and ticket workspace structure.
- Created ticket `0104-CARDCORE-MESHCORE` at `ttmp/2026/07/13/0104-CARDCORE-MESHCORE--cardputer-adv-meshcore-companion-terminal`.
- Added the primary design doc and investigation diary.
- Observed `/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_AC:A7:04:04:88:F4-if00 -> ../../ttyACM0` without starting a monitor, probe, flash, or reset operation.

### Why
- A new ticket keeps the large design, donors, diary, tasks, validation, and delivery together.
- The board is shared serial hardware. Research did not require serial ownership, so avoiding it eliminates accidental monitor contention or unintended reset/flash activity.

### What worked
- `docmgr ticket create-ticket`, `docmgr doc add`, `docmgr ticket list`, and `docmgr doc list` completed successfully.
- The by-id serial device is present and provides a stable future target path.

### What didn't work
- `docmgr ticket list --ticket 0079`, `0082`, and `0102` returned `No tickets found.` These are local firmware directory numbers, not docmgr ticket IDs in this workspace.
- Invoking `/home/manuel/esp/esp-idf-5.5.4/tools/idf.py --version` without first correcting the current shell environment reported: `WARNING: IDF_PATH environment variable is set to /home/manuel/esp/esp-idf-5.4.1 but idf.py path indicates IDF directory /home/manuel/esp/esp-idf-5.5.4. Using the environment variable directory, but results may be unexpected...` followed by `ESP-IDF v5.4.1`.

### What I learned
- The next implementation shell must source `/home/manuel/esp/esp-idf-5.5.4/export.sh` before any `idf.py` operation; merely calling the desired version's script path is insufficient when `IDF_PATH` is already set.
- The ticket has no existing predecessor diary to resume, while multiple local Cardputer code projects are useful donors.

### What was tricky to build
- The ticket number sequence and docmgr ticket names do not correspond one-to-one with all numbered firmware directories. The symptom was the explicit `No tickets found` result; the solution was to create the new descriptive ticket rather than infer a relationship from project directory numbers.

### What warrants a second pair of eyes
- Confirm the project directory name and eventual git repository placement before source implementation starts; this ticket is documentation, not yet a firmware source tree.
- Confirm IDF 5.5.4 with the actual Arduino-ESP32 version selected in `main/idf_component.yml` before accepting the architecture spike.

### What should be done in the future
- Establish and document the board's current flashed firmware and reset/attach behaviour before any erase/flash operation.

### Code review instructions
- Review `index.md`, `tasks.md`, and the two initial documents in this ticket.
- Run `docmgr ticket list --ticket 0104-CARDCORE-MESHCORE` and `docmgr doc list --ticket 0104-CARDCORE-MESHCORE`.

### Technical details
```text
Ticket path:
ttmp/2026/07/13/0104-CARDCORE-MESHCORE--cardputer-adv-meshcore-companion-terminal

Observed stable console path:
/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_AC:A7:04:04:88:F4-if00
```

## Step 2: Inspect existing firmware and public protocol/hardware donors

I inspected local Cardputer-ADV applications and retrieved shallow source snapshots of upstream MeshCore, the Cardputer MeshCore fork, Plai, and meshcore-c. This changed the design from a generic pin-and-radio proposal into a concrete reuse plan with explicit boundaries and licensing constraints.

The most important evidence is that MeshCore's current core and helpers are Arduino-shaped, while Plai provides a native-IDF proof of the Cap's expander and SX1262 ownership model. The design therefore keeps native IDF ownership of the application and contains Arduino inside a compatibility component for the first interoperability milestone.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Determine what can safely be reused from existing Cardputer and MeshCore work, then select a lowest-risk implementation path.

**Inferred user intent:** Reuse proven board knowledge without accidentally copying incompatible or over-scoped firmware architecture.

### What I did
- Searched local projects and tickets for Cardputer-ADV, TCA8418, LoRa and MeshCore references.
- Read local sources: `components/cardputer_kb`, `0038-cardputer-adv-serial-terminal`, and `0083-cardputer-adv-animation-ui`.
- Cloned shallow snapshots into this ticket's `sources/`: MeshCore `219812b`, MeshCore-Cardputer-ADV `e341957`, Plai `fda03cf`, and meshcore-c `1e373d5`.
- Inspected MeshCore Companion/Secure Chat/Room Server flows, the Cardputer fork's variant and radio initialization, and Plai's expander/SX1262 HAL.

### Why
- The requested product depends on protocol interoperability and unusual Cap LoRa-1262 hardware control. Both must be source-verified before making architecture claims.

### What worked
- Local `cardputer_kb` already provides ADV TCA8418 discovery, event draining and physical-key mapping, even though one header comment is stale.
- `0083` demonstrates a useful non-blocking UI queue/render loop and semantic keyboard decoding.
- The Cardputer fork confirms the SX1262 pins and a reset → init → PA ordering.
- Plai's GPL-3.0 source provides a particularly clear native-IDF reference: expander address `0x43`; only P0 is configured push-pull/high; DIO2 selects RX/TX.
- Upstream MeshCore includes the exact Companion, Simple Secure Chat, and Simple Room Server code paths needed for MVP interop.

### What didn't work
- The broad first `rg` output was truncated at the tool's 50 KB limit: `[Showing lines 514-801 of 801 (50.0KB limit). Full output: /tmp/pi-bash-cf57aa558d5d4935.log]`. I replaced it with focused, line-range inspections rather than treating truncated search output as evidence.

### What I learned
- Do not use the Cardputer fork's whole-bank expander writes as a donor. Its board class writes `0xFF`; Plai's read-modify-write P0 method is the safer behavior reference.
- Plai is a Meshtastic implementation, so its radio/HAL structure is reusable as a conceptual/reference design but not its network protocol.
- Plai is GPL-3.0, whereas upstream MeshCore is MIT. Any code reuse requires a compatible licensing decision; reimplementation from public hardware behavior is the default.
- meshcore-c is explicitly WIP and not yet the safe foundation for this interoperability MVP.

### What was tricky to build
- Several sources appear to agree at a high level but disagree in low-level radio settings: the Cardputer fork has different base and Cap-specific TCXO/current values. The symptom is contradictory configuration stanzas, not a resolved electrical truth. The approach chosen is to treat oscillator and PA configuration as a Phase 1 hardware-validation result, rather than hard-code a copied value.

### What warrants a second pair of eyes
- Validate the exact Cap LoRa-1262 oscillator/TCXO, PA, current-limit, region, and power configuration on the real hardware before any long-range/high-power operation.
- Review Arduino component integration to guarantee it does not claim the I2C/SPI buses already owned by the native BSP.
- Review the ticket's retained third-party source snapshot policy before committing or redistributing source material.

### What should be done in the future
- Add a dedicated source-lock file or git submodule record when the firmware project is created, including upstream URL, commit, license and local patch series.
- Add raw radio packet/interop captures with secrets redacted once Phase 1 hardware work begins.

### Code review instructions
- Start with the design doc's “Current evidence and reusable work” and “Hardware model” sections.
- Inspect `sources/meshcore-cardputer-adv/variants/m5stack_cardputer/target.cpp:189-246`, `sources/plai/main/hal/hal_cardputer.cpp:331-392`, and `components/cardputer_kb/unified_scanner.cpp:56-227`.

### Technical details
```text
Snapshots retrieved:
MeshCore                    219812b  2026-07-13
MeshCore-Cardputer-ADV      e341957  2026-01-28
Plai                        fda03cf  2026-06-07
meshcore-c                  1e373d5  2026-05-19

Key native-IDF Cap sequence:
probe 0x43 → RMW P0 output/high → SX1262 init → DIO2 RF switch → RX
```

## Step 3: Write the intern implementation guide and prepare delivery

I wrote the primary design document as an implementation guide rather than a short architecture note. It defines scope, evidence, reuse boundaries, pin/bus ownership, task and transport contracts, persistence behavior, UI semantics, decisions, phases, tests, review gates, and open risks.

The guide keeps the initial claim appropriately narrow: it is a design for an ESP-IDF product architecture with an isolated Arduino compatibility component, not a claim that a pure-native MeshCore port already exists or that the attached device has already passed radio tests.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Produce a clear, technical, intern-ready design with prose, diagrams, pseudocode, APIs and file-backed references, then publish it through the requested documentation channel.

**Inferred user intent:** Make the first implementation executable by a new engineer and establish reviewable evidence before hardware changes begin.

### What I did
- Rewrote `design-doc/01-cardputer-adv-meshcore-companion-terminal-architecture-and-implementation-guide.md` with the complete technical plan.
- Included explicit architecture diagrams, interfaces, bring-up and recovery pseudocode, phase gates, test matrix, and decision records.
- Recorded both observed facts and unresolved verification work, including the attached board's unprobed state.

### Why
- This project has two high-risk seams—Cap radio hardware and MeshCore runtime compatibility—so an intern needs clear ownership and stopping points before adding UI features.

### What worked
- The resulting guide identifies immediately reusable local keyboard/UI/configuration work and makes the Arduino boundary enforceable through review.
- The guide ties critical claims to local or retrieved source file ranges and distinguishes licensing-safe reuse from reference-only sources.

### What didn't work
- No firmware build, flash, radio transmission, or live serial monitor was intentionally run in this research step. Therefore no hardware success claim is made.

### What I learned
- The correct initial definition of “ESP-IDF firmware” is native IDF application ownership, not an unrealistic requirement that every third-party protocol dependency already be Arduino-free.
- A stock MeshCore peer is required for the interop milestone; two new Cardcore images alone cannot prove compatibility.

### What was tricky to build
- The guide had to preserve a future pure-native path without creating a second protocol implementation now. The solution is a Cardcore-owned `MeshTransport` value-type contract: `MeshCoreArduinoTransport` is the initial implementation, while `MeshCoreNativeTransport` is a later replacement behind the same UI/model boundary.

### What warrants a second pair of eyes
- Check the proposed LittleFS partition details against the selected IDF LittleFS component and generated partition table before code starts.
- Check every radio/expander register assertion against M5Stack documentation and the physical Cap revision.
- Check Room Server client requirements against the pinned upstream revision before committing the public API as stable.

### What should be done in the future
- Create the actual firmware repository/project and complete Phases 0–1 before scheduling UI or Room Server implementation.
- Upload the validated document bundle to reMarkable after `docmgr doctor` passes.

### Code review instructions
- Read the design document from Executive Summary through “Phased implementation plan”; then use its references to inspect the donors.
- Validate documentation with `docmgr doctor --ticket 0104-CARDCORE-MESHCORE --stale-after 30` before delivery.

### Technical details
```text
Non-negotiable ownership rules:
- radio/MeshCore calls: mesh task only
- SX1262 ISR: notification only
- LittleFS writes: storage task only
- UI: no MeshCore/Arduino headers
- I2C: one native bus owner
- Cap RF enable: only PI4IOE5V6408 P0, read-modify-write
```

## Step 4: Validate the ticket and make retained evidence lightweight

The initial shallow source snapshots were useful for source inspection but totaled more than 200 MB, largely because Plai includes generated/asset content. I replaced those full clones with a 308 KB, license-preserving evidence subset containing only the files cited by the guide. This keeps the ticket reviewable without turning it into an unpinned copy of four upstream repositories.

The first `docmgr doctor` run also revealed two vocabulary terms missing from the workspace and a second `index.md` inside the full MeshCore source clone. After preserving the cited files only and registering the vocabulary terms, doctor passed cleanly.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Validate the ticket before delivery and keep the research material practical for a new engineer to consume.

**Inferred user intent:** Receive a clean, portable documentation package rather than an oversized source archive with unresolved metadata warnings.

### What I did
- Ran `docmgr doctor --ticket 0104-CARDCORE-MESHCORE --stale-after 30` and frontmatter validation on both authored documents.
- Reduced `sources/` from the four full shallow clones to cited, license-preserving files only (308 KB).
- Ran `docmgr vocab add --category topics --slug lora --description "Long-range low-power radio hardware and protocol work"`.
- Ran `docmgr vocab add --category topics --slug meshcore --description "MeshCore protocol and companion-node development"`.
- Re-ran doctor successfully.

### Why
- The ticket needs a clean validation result before reMarkable upload.
- Retaining only cited evidence makes the local source paths in the guide reviewable while avoiding 200+ MB of unrelated source/assets.

### What worked
- `docmgr doctor --ticket 0104-CARDCORE-MESHCORE --stale-after 30` ended with `✅ All checks passed`.
- Both authored documents previously reported `Frontmatter OK`.

### What didn't work
- The first doctor run reported `multiple_index — Multiple index.md files found (2)` because `sources/meshcore/docs/index.md` looked like a second ticket index.
- The same run reported unknown `lora` and `meshcore` topics in three documents each. Both findings were resolved as described above.

### What I learned
- Ticket-local source snapshots must be curated: docmgr recursively interprets conventional documentation names inside a full upstream checkout.
- Vocabulary belongs to the shared docmgr catalog, so new domain terms must be registered before a ticket can pass clean validation.

### What was tricky to build
- The evidence references in the design document use ticket-local paths and line ranges. Deleting all snapshots would have made the evidence non-reviewable; retaining full clones introduced a doctor conflict and excessive size. The chosen solution retained the exact cited files and their license texts, which preserves traceability without a nested project checkout.

### What warrants a second pair of eyes
- Confirm the selected evidence subset is sufficient for future review; full sources remain available from the URLs and pinned revisions listed in the design document.

### What should be done in the future
- When implementation begins, use a proper pinned Git submodule or dependency archive in the firmware project instead of expanding the documentation ticket's evidence subset.

### Code review instructions
- Run `du -sh ttmp/2026/07/13/0104-CARDCORE-MESHCORE--cardputer-adv-meshcore-companion-terminal/sources` and inspect the retained license files.
- Run `docmgr doctor --ticket 0104-CARDCORE-MESHCORE --stale-after 30`.

### Technical details
```text
Final retained evidence size: 308K
Validation: docmgr doctor passed cleanly
```

## Step 5: Deliver the validated ticket bundle to reMarkable

I rendered and uploaded a single PDF bundle containing the ticket overview, tasks, changelog, architecture guide, and diary. Bundling keeps the design and the investigation evidence together for an intern/reviewer, with a depth-two table of contents for navigation.

The upload followed a dry run and a clean docmgr doctor result. No serial or firmware operation was performed as part of documentation delivery.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Upload the completed ticket documentation to reMarkable after producing the requested guide and diary.

**Inferred user intent:** Make the implementation plan readable away from the development workstation while retaining ticket-local source evidence.

### What I did
- Ran a dry-run bundle upload over `index.md`, `tasks.md`, `changelog.md`, the design doc, and this diary.
- Uploaded `Cardcore MeshCore Companion Terminal Guide.pdf` to `/ai/2026/07/13/0104-CARDCORE-MESHCORE`.

### Why
- A single bundle prevents the diary from separating from its architecture guide and makes the delivery easy to navigate on reMarkable.

### What worked
- Dry run reported all five intended Markdown inputs, a bundle PDF render, and the selected remote directory.
- Real upload returned exactly: `OK: uploaded Cardcore MeshCore Companion Terminal Guide.pdf -> /ai/2026/07/13/0104-CARDCORE-MESHCORE`.

### What didn't work
- N/A.

### What I learned
- The `remarquee upload bundle` command itself reports successful remote delivery, so a routine remote listing is unnecessary.

### What was tricky to build
- The ticket includes source evidence and several metadata files, but only the review-facing Markdown documents belong in the PDF. The bundle explicitly lists the ticket overview, task state, changelog, design doc, and diary rather than dumping source code into the reading document.

### What warrants a second pair of eyes
- Confirm the rendered typography and code-block legibility on the physical reMarkable before using it as the sole field copy; the CLI upload success verifies delivery, not visual taste.

### What should be done in the future
- When implementation phases complete, append diary steps, update the changelog/tasks, validate doctor, then upload a replacement bundle intentionally (with `--force` only after preserving any annotations that matter).

### Code review instructions
- Open `Cardcore MeshCore Companion Terminal Guide.pdf` at `/ai/2026/07/13/0104-CARDCORE-MESHCORE` and use its table of contents.
- Compare the architecture guide's Phase 0/1 checklist with ticket task IDs `5ptk` and `ptlo`.

### Technical details
```text
Dry run: passed
Upload: passed
Remote directory: /ai/2026/07/13/0104-CARDCORE-MESHCORE
Bundle: Cardcore MeshCore Companion Terminal Guide.pdf
```

## Step 6: Scaffold project 0105 and prove the IDF compatibility build

I created `0105-cardcore-mesh-terminal` as the next numerical firmware project and committed the reproducible ESP-IDF scaffold in commit `eded12a317268664c7bb7b598e4e0b4ec8ae57a1`. The project has a native `app_main`, an explicit 8 MB partition table, USB Serial/JTAG defaults, and a small `meshcore_compat` component that is the only project code allowed to include `Arduino.h`.

The final clean-environment ESP-IDF 5.5.4 build succeeded and produced a 0x330e0-byte application, proving that Arduino-ESP32 3.3.10 can be resolved and linked behind the intended component boundary. The first flash attempt did not reach the bootloader because opening the connected USB serial device returned an I/O error; no additional live retry was made.

### Prompt Context

**User prompt (verbatim):** "create the new project in the next numerical id, then go ahead. Commit at appropriate intervals (including the ticket files), and keep a detailed diary as you work (so we can retrace your steps later on). You can use tmux to run idf.py flash monitor / reflash by sending Ctrl-A when you get there). 

Work on it task by task."

**Assistant interpretation:** Create project 0105, implement the ticket tasks sequentially, commit narrowly scoped source and ticket changes, record exact progress, and use a single-owner tmux serial workflow for hardware work.

**Inferred user intent:** Move from design into an auditable, reproducible firmware implementation without losing the ability to reconstruct decisions, failures, and device interactions.

**Commit (code):** eded12a317268664c7bb7b598e4e0b4ec8ae57a1 — "Feat: scaffold Cardcore ESP-IDF terminal"

### What I did
- Created `0105-cardcore-mesh-terminal` with root/main CMake files, `sdkconfig.defaults`, an 8 MB partition table, README, and `main/idf_component.yml` pinning `espressif/arduino-esp32 ^3.3.10`.
- Moved Arduino initialization out of `main/app_main.cpp` into `components/meshcore_compat/meshcore_compat.cpp`; `main` now depends only on the component's native `runtime.h` contract.
- Installed the missing ESP-IDF 5.5.4 Python environment from a clean shell, generated and committed `dependencies.lock`, and built with `IDF_PATH=/home/manuel/esp/esp-idf-5.5.4`.
- Started one `tmux` session, confirmed no process owned the serial port first, then attempted one `idf.py -p /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_AC:A7:04:04:88:F4-if00 flash monitor` operation.

### Why
- Task `5ptk` requires a real IDF/Arduino compatibility proof before adding BSP or MeshCore protocol code.
- The component boundary prevents Arduino includes from spreading into native board, UI, storage, and model code.
- The locked dependency file makes the resolved Arduino 3.3.10 component graph reproducible.

### What worked
- `env -u IDF_PATH -u IDF_PYTHON_ENV_PATH -u VIRTUAL_ENV -u PYTHONPATH bash --noprofile --norc -c 'source "$HOME/esp/esp-idf-5.5.4/export.sh" && idf.py build'` completed successfully.
- The final image was `0x330e0` bytes; the 4 MB factory partition has `0x3ccf20` bytes (95%) free.
- The successful configure reported ESP-IDF `5.5.4`, Arduino-ESP32 `3.3.10`, and the `meshcore_compat` component in the build graph.
- Arduino-ESP32's required FreeRTOS 1 kHz tick was satisfied by `CONFIG_FREERTOS_HZ=1000`.

### What didn't work
- The first `source ~/esp/esp-idf-5.5.4/export.sh` inherited `IDF_PYTHON_ENV_PATH=/home/manuel/.espressif/python_env/idf5.4_py3.13_env`; it failed dependency validation and continued with IDF 5.4.1. The exact diagnostic included: `ERROR: Python environment is set to /home/manuel/.espressif/python_env/idf5.4_py3.13_env which was generated for ESP-IDF 5.4 instead of the current 5.5.`
- Running `~/esp/esp-idf-5.5.4/install.sh esp32s3` in that contaminated environment failed for the same reason. Re-running it with `env -u IDF_PATH -u IDF_PYTHON_ENV_PATH -u VIRTUAL_ENV bash -lc ...` created the `idf5.5_py3.13_env` environment successfully.
- The initial `REQUIRES arduino` name was wrong for the Component Manager artifact. CMake reported `Failed to resolve component 'arduino' required by component 'main': unknown name.` The correct dependency name is `arduino-esp32`.
- Arduino CMake then reported `esp32-arduino requires CONFIG_FREERTOS_HZ=1000 (currently 100)`. Setting `CONFIG_FREERTOS_HZ=1000` and regenerating `sdkconfig` resolved it. An attempted `CONFIG_FREERTOS_HZ_1000=y` was an unknown Kconfig symbol and was removed before the final build.
- The one tmux flash attempt reached esptool but failed before writing: `A serial exception error occurred: Could not configure port: (5, 'Input/output error')`.

### What I learned
- On this host, the `.envrc` automatically activates IDF 5.4.1; invoking `export.sh` is not enough when the old Python environment variables remain inherited. Run all 5.5.4 project commands in the documented clean shell until the workspace environment is corrected.
- The managed Arduino component is named `arduino-esp32` at CMake dependency time, even though the registry package is `espressif/arduino-esp32`.
- The Arduino component compiles a broad component graph, including optional library code. Cardcore does not initialize Wi-Fi/BLE, but future dependency-size work should narrow the Arduino component feature surface before final MVP size/power claims.

### What was tricky to build
- ESP-IDF version selection had two independent failure modes: the active `IDF_PATH` and the active `IDF_PYTHON_ENV_PATH`. The symptom was CMake resolving `idf (5.4.1)` even after requesting the 5.5.4 script. The fix was to install 5.5.4 from a clean environment and run `idf.py` from an `env -u ... bash --noprofile --norc` shell. This must be preserved in the project runbook.
- The first flash failure is distinct from a firmware failure: esptool could not configure the OS serial device, so no image data was written and no monitor session was established. Retrying blindly would violate the single-owner/manual-reset guidance.

### What warrants a second pair of eyes
- Confirm whether the Cardputer needs a manual reset, USB replug, or boot-button sequence to make its USB Serial/JTAG interface writable; inspect the physical connection and attached cable before another flash attempt.
- Review whether Arduino's broad managed dependency graph is acceptable for the intended flash/RAM budget, and identify component configuration needed to avoid unused transport features.
- Verify the custom `storage` partition will be mounted as LittleFS when that component is introduced; it is not mounted in this task.

### What should be done in the future
- Complete Task `5ptk` only after the firmware is flashed and its `Cardcore boot` plus `Arduino compatibility runtime initialized` logs are observed through the single tmux monitor owner.
- After that proof, proceed to Task `ptlo` with shared I2C/TCA8418/Cap-expander diagnostics and raw SX1262 work.

### Code review instructions
- Start with `0105-cardcore-mesh-terminal/main/app_main.cpp` and `components/meshcore_compat/meshcore_compat.cpp`; verify the Arduino include remains isolated.
- Review `main/idf_component.yml`, `dependencies.lock`, and `sdkconfig.defaults` for the pinned IDF/Arduino and 1 kHz tick contract.
- Rebuild with the clean-environment command in the technical details; do not flash until the operator confirms the board is reset/replugged and ready.

### Technical details
```text
Successful final build:
ESP-IDF v5.5.4
Arduino-ESP32 3.3.10
binary: build/cardcore_mesh_terminal.bin
size:   0x330e0

Clean-shell build command:
env -u IDF_PATH -u IDF_PYTHON_ENV_PATH -u VIRTUAL_ENV -u PYTHONPATH \
  bash --noprofile --norc -c \
  'source "$HOME/esp/esp-idf-5.5.4/export.sh" && idf.py build'

Blocked live operation:
idf.py -p /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_AC:A7:04:04:88:F4-if00 flash monitor
→ Could not configure port: (5, 'Input/output error')
```
