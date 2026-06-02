---
Title: Investigation Diary
Ticket: ESP32-P4-DISPLAY-SERVER
Status: active
Topics:
    - esp32-p4
    - picocalc
    - display
    - firmware-port
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Chronological diary for the ESP32-P4 PicoCalc display server design ticket"
LastUpdated: 2026-06-01T21:45:00-04:00
WhatFor: "Track display server design work, evidence gathered, validation, and delivery"
WhenToUse: "Resume or review the ESP32-P4 PicoCalc display server design effort"
---

# Diary

## Goal

Document the creation of a new ticket and intern-facing design package for an ESP32-P4 PicoCalc display server. The display server is intended to turn the validated `0099` LCD benchmark code into a production-oriented FreeRTOS task and API that owns LCD protocol state, queued SPI transfer, DMA buffers, and dirty-region batching.

## Step 1: Ticket Created and Display Server Design Guide Written

Created a new docmgr ticket for the ESP32-P4 PicoCalc display server and wrote the primary design document. The guide explains the system from first principles for a new intern: current hardware mapping, LCD SPI clock constraints, current benchmark evidence, why a display server is needed, what the public API should look like, how queued/double-buffered transfer must be sequenced, and how implementation should proceed in phases.

The document uses the existing `0099` firmware and prior LCD optimization ticket as evidence. It treats the display server as an ownership boundary: application code submits display commands, while one FreeRTOS task owns LCD windows, DC GPIO transitions, SPI transactions, DMA buffers, and dirty-region batching.

### Prompt Context

**User prompt (verbatim):** "create a new ticket to create a display server on the picocalc / esp32-p4. Create  a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and the nupload to remarkable."

**Assistant interpretation:** Create a docmgr ticket for designing an ESP32-P4 PicoCalc display server, write a comprehensive intern-facing technical design/implementation guide with diagrams, pseudocode, API sketches, evidence, and file references, then upload the ticket bundle to reMarkable.

**Inferred user intent:** The user wants a durable implementation guide that can onboard a new engineer/intern to the display-server work without requiring them to reconstruct the prior LCD optimization investigation from terminal history.

**Commit (code):** N/A — design/documentation ticket only.

### What I did

- Created docmgr ticket `ESP32-P4-DISPLAY-SERVER`.
- Added primary design document `design-doc/01-display-server-design-and-implementation-guide.md`.
- Added diary document `reference/01-investigation-diary.md`.
- Gathered line-anchored evidence from:
  - `0099-esp32-p4-picocalc-display-keyboard/main/app_main.c`
  - `0099-esp32-p4-picocalc-display-keyboard/README.md`
  - prior LCD throughput optimization guide in the `ESP32-P4-PICOCALC` ticket.
- Wrote the design guide with:
  - executive summary;
  - problem statement and scope;
  - current-state analysis;
  - Mermaid architecture diagrams;
  - proposed public API;
  - internal data structures;
  - queued transfer pseudocode;
  - dirty-region coalescing plan;
  - text renderer plan;
  - implementation phases;
  - testing strategy;
  - risks and alternatives;
  - implementation checklist for an intern;
  - file references.
- Updated `tasks.md` with design completion and future implementation phases.

### Why

The display work has moved from isolated benchmarks into architecture planning. The current code proves the hardware and low-level primitives, but it is not yet safe as a production API because many future subsystems could otherwise touch LCD state directly. A display server centralizes ownership and makes queued SPI transfer safe by construction.

### What worked

- `docmgr ticket create-ticket` created the workspace cleanly.
- `docmgr doc add` created both design and diary documents.
- Existing `0099` firmware and prior ticket documentation contained enough evidence to write a concrete guide without speculation.
- The guide now defines an actionable phase plan rather than only a conceptual API.

### What didn't work

- I accidentally created an empty typo path under `ttmp/2026/06/01/ESP32-P4-DISPLAY-SERVER--esp32-p4-wifi6-as-picocalc-mcu-replacement-rp2350-swap/typo` while preparing to write the design document. I removed the empty file and accidental directory immediately.

### What I learned

- The display server design is primarily about ownership, not just speed. The key correctness problem is that queued SPI bytes depend on LCD address-window state and the GPIO-controlled DC line.
- The first display server should intentionally keep one pixel payload in flight. That is enough to overlap rendering with transfer while preserving the manual DC/window invariant.
- Dirty-region coalescing should begin with simple row-oriented rules. General rectangle union is not necessary for the first implementation and can transfer more pixels than it saves.

### What was tricky to build

The tricky part was writing the design so it is useful to a new intern without hiding the embedded constraints. The guide had to explain why the existing benchmark code is not yet a production architecture, and why a display task should own both protocol state and performance policy.

The strongest invariant is easy to state but easy to violate in code: no new LCD window command or DC transition may occur while a queued pixel payload is still in flight. The design document repeats this invariant in the API, task loop, queued-transfer pseudocode, and risk sections so implementers see it before writing code.

### What warrants a second pair of eyes

- Whether `DISPLAY_CMD_BLIT_RGB565` should use copy-before-return or borrow-until-flush semantics in the first implementation.
- Whether the first server should support arbitrary blits or only fills and text rows.
- Whether the public API should expose `DISPLAY_CMD_PRESENT`, or whether `display_flush()` is enough for the first implementation.
- Whether display-server implementation should happen inside `0099` first or in a new firmware directory.

### What should be done in the future

- Run `docmgr doctor` and resolve any vocabulary/frontmatter issues that affect this new ticket.
- Upload the design bundle to reMarkable.
- Implement Phase 1: extract low-level LCD code from `app_main.c`.
- Implement Phase 2: add `display_server.h/.c` and route simple fill/clear operations through the server.

### Code review instructions

- Start with the design guide:
  - `ttmp/2026/06/01/ESP32-P4-DISPLAY-SERVER--esp32-p4-picocalc-display-server-design/design-doc/01-display-server-design-and-implementation-guide.md`
- Cross-check against:
  - `0099-esp32-p4-picocalc-display-keyboard/main/app_main.c`
  - `0099-esp32-p4-picocalc-display-keyboard/README.md`
  - `ttmp/2026/06/01/ESP32-P4-PICOCALC--esp32-p4-wifi6-as-picocalc-mcu-replacement-rp2350-swap/design-doc/04-picocalc-lcd-spi-throughput-optimization-guide.md`
- Validate implementation later with:
  - `idf.py build`
  - `lcd perf full`
  - `lcd perf queued`
  - `lcd restorebench both 64 64 300`
  - `lcd mixedbench both 40 24 200 4`

### Technical details

The proposed first display-server API is:

```c
esp_err_t display_start(void);
esp_err_t display_stop(void);
esp_err_t display_submit(const display_cmd_t *cmd, TickType_t timeout);
esp_err_t display_submit_batch(const display_cmd_t *cmds, size_t count, TickType_t timeout);
esp_err_t display_flush(TickType_t timeout);
esp_err_t display_get_status(display_status_t *out);
```

The proposed safe queued-transfer loop is:

```text
render current operation into buffer A
program LCD window for current operation
set DC high
queue buffer A pixel payload
render next operation into buffer B
wait for buffer A transaction completion
program next LCD window
queue buffer B pixel payload
```
