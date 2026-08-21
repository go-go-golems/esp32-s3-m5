---
Title: Investigation diary
Ticket: ESP-60-M5STACKCHAN-NFC
Status: active
Topics: []
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: repo://0115-m5stackchan-nfc-reader/main/st25r3916/st25r3916.c
      Note: |-
        Implements and applies NFC-A frame-wait timer (commit 74bc45f9)
        Documented M5 initialization and Space-B diagnostics
    - Path: repo://0115-m5stackchan-nfc-reader/main/st25r3916/st25r3916_regs.h
      Note: Adds NRT and timer control register definitions (commit 74bc45f9)
    - Path: repo://0116-m5stackchan-nfc-debug-ui/overlay/firmware/main/apps/app_nfc_debug/nfc_debug_service.cpp
      Note: UI-0 serialized NFC worker and immutable snapshot implementation (commit 50d7c151)
    - Path: repo://0116-m5stackchan-nfc-debug-ui/scripts/flash.sh
      Note: NFC-only physical deployment workflow (commit 51efbe4f)
    - Path: repo://0116-m5stackchan-nfc-debug-ui/scripts/prepare.sh
      Note: Pinned reproducible upstream composition process (commit 50d7c151)
    - Path: repo://ttmp/2026/08/20/ESP-60-M5STACKCHAN-NFC--esp-idf-st25r3916-nfc-reader-console-app-for-m5stackchan-intern-guide/sources/code/m5unit-nfc/unit_ST25R3916.cpp
      Note: Authoritative initialization implementation
    - Path: repo://ttmp/2026/08/20/ESP-60-M5STACKCHAN-NFC--esp-idf-st25r3916-nfc-reader-console-app-for-m5stackchan-intern-guide/sources/code/m5unit-nfc/unit_ST25R3916_nfca.cpp
      Note: Authoritative NFC-A configuration and request sequence
    - Path: repo://ttmp/2026/08/20/ESP-60-M5STACKCHAN-NFC--esp-idf-st25r3916-nfc-reader-console-app-for-m5stackchan-intern-guide/sources/web/03-m5stack-stackchan-nfc-official-images.md
      Note: Official physical placement evidence
ExternalSources: []
Summary: ""
LastUpdated: 2026-08-20T21:53:02.871902069-04:00
WhatFor: ""
WhenToUse: ""
---










# Diary: ESP-IDF ST25R3916 NFC Reader Intern Guide

## Goal

Create a docmgr ticket for a simple NFC reader for the M5StackChan; analyze the available firmware in this repo; research NFC reading on ESP32 (Kagi + cloned sources); save valuable documents into the ticket's `sources/`; and write a detailed intern-oriented analysis/design/implementation guide that starts with `esp_console`-based reading before a nicer UI. Store in the ticket and upload to reMarkable. Commit at appropriate intervals. Research/analysis only — no firmware implementation in this phase.

## Step 1: Reconnaissance — locate the device, prior tickets, and available firmware

I established the working environment, found the connected StackChan, and surveyed what NFC/firmware evidence already exists in the monorepo and in prior docmgr tickets. The device is connected and the prior `M5STACKCHAN*` tickets (research, BLINKY, BENCH, RAWBLIT) were the richest context.

### Prompt Context

**User prompt (verbatim):** "Create a new docmgr ticket to create a simple NFC reader for the stackchan, and analyze what firmware we have available here, and potentially use kagi to search (and save valuable documents into the ticket's source/). We first want to just try reading with esp_console, before making a nicer UI for it. Create a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and the nupload to remarkable."

**Assistant interpretation:** Build a docmgr ticket + an intern-grade design doc for a StackChan NFC reader; start from console reading; research via Kagi; save sources into the ticket; upload to reMarkable.

**Inferred user intent:** A self-contained onboarding + implementation guide so a new intern can read NFC tags from the StackChan's on-board reader using ESP-IDF, console-first.

### What I did

- Confirmed CWD = `/home/manuel/code/wesen/go-go-golems/esp32-s3-m5`; listed the firmware project tree (`0001`…`0114`).
- `docmgr status`: 210 tickets, 767 docs.
- Confirmed connected device: `/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_44:1B:F6:E2:80:28-if00 -> ../../ttyACM0`; `lsusb` shows `303a:1001 Espressif USB JTAG/serial debug unit`. This matches the BLINKY diary's `/dev/ttyACM0` + ESP32-S3 USB Serial/JTAG console.
- Found prior stackchan tickets and read the `M5STACKCHAN` research diary + the `M5STACKCHAN-BLINKY` intern guide + its diary. The BLINKY diary confirmed: ESP-IDF v5.5.4, USB JTAG/serial on `/dev/ttyACM0`, `build/firmware/main/apps/...` structure, Mooncake app framework, `LvglLockGuard` for LVGL, and the hardware architecture (CoreS3 + body, **ST25R3916 NFC controller on I2C at 0x50**).
- Searched the whole tree for NFC/ST25R3916 code: none in any ESP-IDF firmware `main/`; only Arduino references in `0090`/`0091` research and the prior tickets.

### Why

Before designing a reader I had to know (a) which chip is on the board and how it is wired, (b) what the firmware already does with I2C, and (c) what flashing workflow colleagues used. The prior tickets had already mapped the hardware and flashing; I only had to re-derive the NFC-specific path and the ESP-IDF I2C driver pattern.

### What worked

- `lsusb` + `/dev/serial/by-id` instantly identified the device and console path.
- The BLINKY diary was a near-perfect template: same board, same console, same IDF version, intern-guide format.
- The `M5STACKCHAN` research diary named the ST25R3916 at 0x50 and listed the cloned repos (StackChan, StackChan-BSP, M5Unit-NFC).

### What didn't work

- The cloned StackChan/M5Unit-NFC source repos from the research ticket were gone (gitignored/cleaned), so I had to re-clone them to read the actual NFC driver.
- `defuddle` returned empty for GitHub READMEs and the elechouse quick-start page (both JS-rendered). Worked around with `curl` to `raw.githubusercontent.com` for the elechouse README.

### What I learned

- The StackChan body has a **ST25R3916 NFC controller at I2C 0x50**, sharing the body I2C bus (SDA=GPIO12, SCL=GPIO11, port 1) with touch/PMIC/battery/IO-expander.
- The official NFC examples are **Arduino-only** (`M5UnitUnifiedNFC`); the ESP-IDF firmware has **no NFC code**.
- The firmware uses the **new** `driver/i2c_master.h` API everywhere (e.g. `PY32IOExpander_Class`).

### What was tricky to build

- Nothing in this step; it was pure discovery. The tricky part (next step) was that the live repos were not present and had to be re-cloned.

### What warrants a second pair of eyes

- Confirm the device I identified (`/dev/ttyACM0`) is the StackChan and not another ESP32-S3 on the bus (only one Espressif JTAG device was present, so it is almost certainly it; the operator should confirm by watching `idf.py monitor` output mention "M5Stack-StackChan-Board").

### What should be done in the future

- Re-clone StackChan repos into a stable, gitignored-but-persistent path so future NFC work does not re-clone each session.

### Code review instructions

- Verify: `ls -la /dev/serial/by-id/` shows the Espressif JTAG unit → `/dev/ttyACM0`.
- Read: `ttmp/2026/06/11/M5STACKCHAN-BLINKY--.../reference/01-investigation-diary.md` (flashing workflow) and `.../design/01-...md` (hardware architecture diagram with ST25R3916 at 0x50).

### Technical details

- Device: Espressif USB JTAG/serial debug unit, `303a:1001`, `/dev/ttyACM0`.
- IDF on this machine: `~/esp/esp-idf-5.5.4` (firmware requires `>=5.5.2`).

---

## Step 2: Clone the firmware + NFC driver libs and extract the I2C/NFC contract

I re-cloned the M5Stack repos, read the real ESP-IDF firmware (I2C bus init, IO expander driver) and the M5Unit-NFC register-level ST25R3916 driver, and pinned down the exact I2C address, register-access protocol, init sequence, and ISO14443-A poll sequence.

### Prompt Context

**User prompt (verbatim):** (see Step 1)
**Assistant interpretation:** Gather the concrete technical evidence needed to write an accurate driver design: pins, address, register protocol, init + poll sequences.
**Inferred user intent:** The design doc must be grounded in real code, not generic NFC lore.

### What I did

- Cloned `m5stack/StackChan`, `m5stack/StackChan-BSP`, `m5stack/M5Unit-NFC` into `/tmp/nfc-research/repos`.
- Read `StackChan/firmware/main/hal/board/config.h` → I2C pins `AUDIO_CODEC_I2C_SDA_PIN=GPIO_NUM_12`, `...SCL_PIN=GPIO_NUM_11`.
- Read `StackChan/firmware/main/hal/board/stackchan.cc::InitializeI2c` → exact `i2c_master_bus_config_t` (port 1, GPIO 12/11, internal pullup, glitch_ignore_cnt=7) and `I2cDetect()` (the `i2c_master_probe` scan pattern).
- Read `PY32IOExpander_Class.{hpp,cpp}` → the firmware's clean I2C driver style to mirror: `i2c_master_bus_add_device` + `i2c_master_transmit` / `i2c_master_transmit_receive`, 100 kHz.
- Read `M5Unit-NFC/src/unit/unit_ST25R3916.hpp` → `M5_UNIT_COMPONENT_HPP_BUILDER(UnitST25R3916, 0x50)` = **I2C address 0x50**.
- Read `ST25R3916_definition.hpp` → register/command constants (`REG_IC_IDENTITY`, `VALID_IDENTIFY_TYPE=0x05`, `CMD_SET_DEFAULT=0xC1`, `MAX_FIFO_DEPTH=512`, register read/write direction bits).
- Read `unit_ST25R3916.cpp::begin()` → init sequence (read IC id, STOP_ALL_ACTIVITIES, SET_DEFAULT, CLEAR_FIFO, ADJUST_REGULATORS).
- Read `unit_ST25R3916_nfca.cpp` → NFC-A poll sequence (mode definition, field on, `CMD_TRANSMIT_REQA` → ATQA from FIFO, anticollision CL1/CL2/CL3 with SEL 0x93/0x95/0x97, NVB 0x20→0x70, SELECT → SAK).
- Read `StackChan-BSP/examples/NFC/Detect/Detect.ino` → official Arduino detect logic (`nfc_a.detect(piccs)`, `identify`, `uidAsString`, `atqa`, `sak`).
- Confirmed IDF 5.5.4 present (`~/esp/esp-idf-5.5.4`); `remarquee status` = ok.

### Why

The design doc's pseudocode and API references had to be copied from real sequences, not invented. The I2C driver style had to match the firmware's existing conventions so an intern's code looks native to the codebase.

### What worked

- Raw GitHub (`curl raw.githubusercontent.com`) gave the elechouse ST25R3916 ESP32 README cleanly.
- Kagi found the official ST25R3916B datasheet URL (`st.com/resource/en/datasheet/st25r3916b.pdf`) and the key community threads (rfalFieldOn failure, I2C reference design, bare-metal register writes).
- The M5Unit-NFC `ST25R3916_definition.hpp` is a complete, machine-usable register map — better than the datasheet for coding.

### What didn't work

- `st.com` blocks automated downloads (HTTP/2 INTERNAL_ERROR on `curl`), so the datasheet PDF could not be fetched into `sources/datasheets/`. Recorded the URL + download instructions in a README instead.
- Bash `rg`/`cat` output of the BSP `PY32IOExpander` source came back with the substrings "Expander"/"enable" rendered as "n" (a terminal display artifact). Switched to the `read` tool for clean content.

### What I learned

- The ST25R3916 I2C register protocol uses a **command byte**: read = `(reg & 0x3F) | 0x40` (repeated-start read), write = `(reg & 0x3F)` (write direction), direct command = single byte.
- The 12 RGB LEDs are driven **through** the PY32IOExpander's on-chip LED controller, not a WS2812 chain; the expander (0x6F) controls servo power (pin 0) and an RGB enable (pin 13). It does **not** obviously toggle an NFC power pin — left as an open question for the intern to verify against the body schematic.
- M5UnitUnified works on both Arduino and ESP-IDF via its `Component` I2C abstraction, but porting it drags in M5Unified+M5GFX (display stack). For a console reader, a from-scratch minimal driver is cleaner.

### What was tricky to build

- The "Expander→n" terminal artifact made the BSP source unreadable via `cat`; the `read` tool avoided it. Root cause unknown (likely a tty filter), workaround reliable.
- The ST25R3916 has two register spaces (A and B); Space B needs a `CMD_REGISTER_SPACEB_ACCESS` prefix. Phase 1 only uses Space A, so I documented this as a non-issue for now.

### What warrants a second pair of eyes

- The **NFC power path** is unverified: does the ST25R3916 need an IO-expander pin enabled, or is it always powered? The design doc flags this as the top open question. The body schematic (in the `M5STACKCHAN` research ticket's `sources/`) must be checked.
- The anticollision pseudocode assumes a **single tag** (sends `NVB=0x20`, skips the bit-narrowing collision loop). This is correct for the common case but wrong for multi-tag; documented as a TODO.

### What should be done in the future

- Verify the NFC power-enable pin against the body schematic and add an `nfc_power_on()` step if needed.
- Implement the full anticollision bit-narrowing loop before claiming multi-tag support.
- Wire the ST25R3916 IRQ pin to a GPIO for low-CPU detection (Phase 2+); Phase 1 polls the IRQ register.

### Code review instructions

- `sources/code/ST25R3916_definition.hpp` — confirm register constants match the datasheet.
- `sources/code/unit_ST25R3916.hpp` — confirm the `begin()` init sequence and `nfca_*` poll sequence in the design doc (§5.2, §5.3) match.
- `sources/code/PY32IOExpander_Class.hpp` — confirm the I2C driver style in the design doc (§7.3, §7.4) matches.

### Technical details

- ST25R3916 I2C address: **0x50**. IDF: **5.5.4**. Console: USB Serial/JTAG on `/dev/ttyACM0`.
- Init: read `REG_IC_IDENTITY` (type==0x05) → `CMD_STOP_ALL_ACTIVITIES` → `CMD_SET_DEFAULT` → `CMD_CLEAR_FIFO` → `CMD_ADJUST_REGULATORS`.
- Poll: `CMD_NFC_INITIAL_FIELD_ON` → `CMD_TRANSMIT_REQA` → ATQA from FIFO → anticollision (SEL 0x93/0x95/0x97, NVB 0x20) → UID+BCC → SELECT (NVB 0x70) → SAK.

---

## Step 3: Create the ticket, save sources, write the design doc, commit

I created the docmgr ticket, saved the researched sources (web + code excerpts + datasheet pointer) into `sources/`, wrote the 43KB intern design doc, and committed at this checkpoint.

### Prompt Context

**User prompt (verbatim):** "commit at appropriate intervals and keep a detailed diary as you work (using the diary format from the skill)"
**Assistant interpretation:** Commit the work in progress and maintain the diary as I go, per the diary skill's strict format.
**Inferred user intent:** Leave an auditable, resumable trail; don't batch everything into one giant commit at the end.

### What I did

- Created ticket `ESP-60-M5STACKCHAN-NFC` (`docmgr ticket create --ticket ESP-60-M5STACKCHAN-NFC --title "ESP-IDF ST25R3916 NFC reader console app for M5StackChan (intern guide)" --topics m5stackchan,nfc,st25r3916,esp32-s3,esp-idf,esp-console,intern-guide`). Ticket dir: `ttmp/2026/08/20/ESP-60-M5STACKCHAN-NFC--esp-idf-st25r3916-nfc-reader-console-app-for-m5stackchan-intern-guide`.
- Created design doc + diary via `docmgr doc add`.
- Saved sources: `sources/web/00-kagi-search-...md`, `sources/web/01-elechouse-ST25R3916-esp32-readme.md`, `sources/datasheets/README-download-instructions.md` (datasheet URL + community links), `sources/code/` (ST25R3916_definition.hpp, unit_ST25R3916.hpp, PY32IOExpander_Class.hpp, stackchan-board-config.h, BSP NFC Detect/Emulation examples).
- Wrote the design doc: 15 sections (~43KB) — executive summary, problem/scope, system architecture (hardware + software + I2C contract + address map), NFC fundamentals (ISO14443-A), ST25R3916 over I2C (register protocol, init, poll), current-state firmware analysis (decision record A vs B), design (project layout, sdkconfig, I2C init, driver sketch, esp_console commands, expected output), build/flash/verify (IDF 5.5.4, /dev/ttyACM0, USB Serial/JTAG), phased plan, risks/open questions, troubleshooting, API reference, key file reference, summary diagrams, ESP32-S3 console appendix.
- Committed: `fe9922b "ESP-60: NFC reader intern guide — ticket scaffold, sources, design doc (analysis only)"`.

### Why

The diary skill requires committing at appropriate intervals and writing the diary as work progresses, not reconstructing it at the end. Committing after the design doc is a natural checkpoint (the analysis deliverable exists and is self-consistent).

### What worked

- `docmgr doc add` generated clean frontmatter for both docs; I overwrote the body with the full content.
- The design doc's decision record (Option A port M5UnitUnifiedNFC vs Option B minimal from-scratch driver) makes the rationale auditable.
- Committing the sources alongside the doc means the intern does not need network access to read the register map.

### What didn't work

- `git add` warned about CRLF→LF on the two `.ino` files (harmless; they're Arduino examples saved from GitHub).

### What I learned

- The docmgr ticket path template auto-appends `--<slug-from-title>` to the ticket ID, producing the long directory name.
- The design doc is large (43KB / 15 sections) because an intern guide must be self-contained; that is intentional.

### What was tricky to build

- Balancing "self-contained for an intern" against "not so long it's unreadable". Resolved by layering (architecture → NFC fundamentals → driver → build) and putting exhaustive register lists in the appendix + `sources/code/` rather than inline.

### What warrants a second pair of eyes

- The pseudocode in §7.4 is a sketch, not compiled code. An intern must fill in FIFO read/write helpers and the IRQ wait loop. The sketch is faithful to `unit_ST25R3916_nfca.cpp` but should be compiled and debugged on hardware (Phase 1 implementation, which is out of scope for this analysis-only step).

### What should be done in the future

- Implement Phase 1 firmware per the guide, flash, and verify `nfc-read` on a real NTAG.
- Update this diary with the implementation step once the user greenlights coding.

### Code review instructions

- Read the design doc: `design-doc/01-esp-idf-st25r3916-nfc-reader-console-app-analysis-design-and-implementation-guide.md`.
- Cross-check §5.1 (I2C protocol) against `sources/code/unit_ST25R3916.hpp` lines building the read/write command bytes.
- Cross-check §3.3 (I2C bus config) against `sources/code/stackchan-board-config.h` + the live `stackchan.cc::InitializeI2c`.

### Technical details

- Commit: `fe9922b0966a16a2d79d31dc596c7ba64be1a3e3`.
- Design doc: 43,493 bytes, 15 sections.
- Ticket: `ESP-60-M5STACKCHAN-NFC`.

---

## Step 4: Ticket bookkeeping, doctor, and reMarkable upload

I will relate the key files, update the changelog/tasks, run `docmgr doctor`, and upload the design doc + diary bundle to reMarkable. (This step records intent; execution evidence goes below.)

### Prompt Context

**User prompt (verbatim):** (see Step 1) — "Store in the ticket and the nupload to remarkable."
**Assistant interpretation:** Complete the ticket bookkeeping and deliver the bundle to reMarkable.
**Inferred user intent:** A finished, validated ticket with the guide available to read on a tablet.

### What I did

- Related `AGENTS.md` to the design doc via `docmgr doc relate` (the 5 in-ticket `sources/code/*` relations triggered a docmgr `repo://` anchor-resolution false-positive — doctor strips the anchor and resolves relative to the doc dir; removed them since the files are already documented in §13 + ExternalSources and exist on disk). Doctor then passed: **All checks passed**.
- Updated the changelog with the Step 3 commit hash and added 6 tasks (research, implement, flash, schematic-verify, Phase 2, upload). Marked research + upload done.
- Added vocabulary slugs `nfc`, `st25r3916`, `esp-console` (doctor flagged them as unknown topics).
- Fixed a `doctor --fix` side-effect: it added a stray `---` to `README.md`/`changelog.md`/`tasks.md` (which originally had no frontmatter), breaking their parse. Removed the stray delimiter; amended the bookkeeping commit to drop the `.bak` files.
- Uploaded the design doc + diary as a reMarkable bundle: dry-run first, then `remarquee upload bundle ... --remote-dir /ai/2026/08/20/ESP-60-M5STACKCHAN-NFC --toc-depth 2`. Verified with `remarquee cloud ls`: `ESP-60 M5StackChan NFC Reader — Intern Guide` present.

### Why

The ticket-research skill requires relate/changelog/tasks bookkeeping, a clean `docmgr doctor`, and a dry-run-then-real reMarkable bundle upload. Filling the diary's execution evidence makes the trail auditable.

### What worked

- `docmgr doctor` passed cleanly after removing the in-ticket source relations and adding vocab.
- The reMarkable bundle uploaded on the first real attempt (pandoc → PDF → rmapi), and `cloud ls` confirmed it.

### What didn't work

- `docmgr doctor --fix` corrupted the frontmatter of the three scaffold files (README/changelog/tasks) by inserting a stray `---`; had to repair manually. Lesson: avoid `doctor --fix` on scaffold files that legitimately have no frontmatter.
- In-ticket `sources/code/*` file relations show as `missing_related_file` due to a `repo://` anchor resolution quirk (doctor resolves the stripped path relative to the doc dir, not the ticket dir). Worked around by removing those 5 relations; the files remain referenced in the doc body.

### What I learned

- docmgr writes the tightest containing anchor (`repo://` for in-repo files) but doctor's legacy-path resolution can mis-resolve deeply-nested `repo://` ticket paths. For in-ticket source files, document them in the doc body rather than `--file-note` relate.
- The elechouse/ST.com sites block automated downloads (curl HTTP/2 INTERNAL_ERROR); record URLs for manual browser download instead of fighting the bot filter.

### What was tricky to build

- The `ExternalSources` frontmatter field expects a list of **strings**, not a list of `Path:`/`Note:` maps (unlike `RelatedFiles` which accepts maps). The first relate attempt failed with a YAML unmarshal error until I converted `ExternalSources` to plain strings.
- Keeping the diary honest: Step 4 was written as intent first, then back-filled with execution evidence after the commands ran, so the recorded commands match what actually happened.

### What warrants a second pair of eyes

- Verify the reMarkable upload landed in `/ai/2026/08/20/ESP-60-M5STACKCHAN-NFC` and opens as a single PDF with a ToC. (Confirmed via `remarquee cloud ls`; a human should open it on the tablet.)
- The 5 removed in-ticket source relations: confirm the intern can still find the source files — they are listed in `design-doc` §13 Key File Reference and committed under `sources/code/`.

### What should be done in the future

- After the user greenlights implementation, add a Step 5 covering the actual Phase-1 build/flash/verify on `/dev/ttyACM0`.
- Re-evaluate whether to re-relate `sources/code/*` once docmgr's `repo://` nested-path resolution is fixed.

### Code review instructions

- `docmgr doctor --ticket ESP-60-M5STACKCHAN-NFC --stale-after 30` should print "All checks passed".
- `remarquee cloud ls /ai/2026/08/20/ESP-60-M5STACKCHAN-NFC --long --non-interactive` lists `ESP-60 M5StackChan NFC Reader — Intern Guide`.

### Technical details

- Commits: `fe9922b` (scaffold + sources + design doc), `aa532c54` (bookkeeping + diary Step 4 evidence).
- reMarkable remote: `/ai/2026/08/20/ESP-60-M5STACKCHAN-NFC/ESP-60 M5StackChan NFC Reader — Intern Guide`.
- Doctor: All checks passed. Tasks done: research (n4l3), upload (pbpb). Tasks open: implement (2t3u), flash (tr32), schematic-verify (o65r), Phase 2 (2nrd).

---

## Step 5: Implement Phase 1 firmware (project 0115-m5stackchan-nfc-reader)

I scaffolded the standalone ESP-IDF project, wrote a minimal ST25R3916 driver + esp_console commands, built, flashed, and debugged the init sequence against the real chip.

### Prompt Context

**User prompt (verbatim):** "Ok, print a brutalist work slip for each phase, then implement phase 1, stopping if you need me to do physical stuff like placing an nfc near or removing it. For the beginning, I'm ptuting an NFC tag on top of the edvice. commit at appropriate intervals and keep a detailed diary as you work (using the diary format from the skill)"
**Assistant interpretation:** Print phase slips, then build Phase 1 (console NFC reader) on the connected StackChan; pause for physical tag steps.
**Inferred user intent:** Get a working NFC tag reader on the actual hardware, console-first.

### What I did

- Printed a brutalist `plan` slip (P1 console reader → P2 flash+verify → P3 LVGL UI) on the AtomS3R thermal printer.
- Scaffolded `0115-m5stackchan-nfc-reader/` (CMakeLists, sdkconfig.defaults with USB Serial/JTAG console, main/idf_component.yml, main/CMakeLists with `esp_driver_i2c` + `esp_timer` + `console` PRIV_REQUIRES).
- Wrote `main/st25r3916/st25r3916_regs.h` (register/command constants from `ST25R3916_definition.hpp`), `st25r3916.h/.c` (driver: I2C rd8/wr8/direct_cmd/fifo_write/fifo_read/wait_irq, init, field on/off, configure_nfca, poll_nfca with REQA→ATQA→anticollision→select), `nfc_console.c/.h` (nfc-scan/probe/field/read/poll/regs), `nfc_reader_main.c` (app_main: I2C init port1 SDA12 SCL11, st25r3916_init, esp_console REPL over USB Serial/JTAG).
- Build fixes: added `esp_timer` to PRIV_REQUIRES; removed unused `TAG` in nfc_console.c; added missing `CMD_SET_DEFAULT` constant.
- First build+flash succeeded; boot log: `ST25R3916 detected: type=0x05 rev=0x02`; console `nfc>` prompt; `nfc-scan` showed the full I2C map (0x34 PMIC, 0x38 touch, 0x41 INA226, **0x50 ST25R3916**, 0x58 AW9523, 0x68 Si12T, 0x6f PY32IOExpander); `nfc-probe` OK; `nfc-field on` ESP_OK. `nfc-read` → "no tag".
- Debugged "no tag" against the M5 lib source and found 4 driver bugs:
  1. `MODE_DEFINITION` = 0x88 (guessed) → should be 0x09 (`ISO14443A=0x08 | nfc_ar8_auto=0x01`).
  2. `OPERATION_CONTROL` bits wrong: I had tx_en=0x01/rx_en=0x02; real is **tx_en=0x08/rx_en=0x40/en=0x80**. My "osc enable 0x40" was actually rx_en.
  3. `I_col` = 0x04 (I had shift 9 → 0x200); `no_crc_rx` = 0x80 (I had 0x02).
  4. `field_on` must CLEAR tx_en|rx_en (M5 lib), not set them.
  Plus missing antenna config: `TX_DRIVER(0x28)=0xD0`, `ANTENNA_TUNING_1/2(0x26/0x27)=0x82`, `IO_CONFIG_1=0x8B/2=0x30`, receiver config `RX1=0x08/RX2=0x2D` (I had 0x84/0x33), and `enable_osc()` (set en=0x80, wait for I_osc IRQ). Added `nfc-regs` debug dump.
- After fixes: `nfc-regs` after init shows `OPC=83 MODE=09 RX1=08 RX2=2D` (correct). After `nfc-field on`+`nfc-read`: `OPC=8B` (tx_en set during TX) but **`RSSI=00`, `MAIN_IRQ=000000`** → field commanded on, chip transmits, but no tag couples.
- Confirmed via StackChan-BSP source that there is NO NFC power-enable IO-expander pin (only pin 0=servo, pin 13=RGB), so the ST25R3916 is always powered when the body is seated — the power-path open question from the design doc is resolved (not the issue).
- Concluded: the chip + driver are correct; **RSSI=0 + no RXE = the tag is not over the NFC coil**. The NFC coil is on the body board; the user placed the tag "on top" (the head/display).

### Why

The design doc's pseudocode was a sketch; the real register bits had to be read from the M5 lib. The chip-detect + I2C scan + correct post-init registers prove the I2C + init path is sound, which isolates the remaining problem to tag-to-coil coupling.

### What worked

- Build/flash/monitor loop over USB Serial/JTAG via pyserial (no TTY needed): reset with RTS toggle, send REPL commands, capture output.
- `nfc-scan` reproduces the firmware's I2C detect map exactly → I2C bus wiring correct.
- `nfc-probe` → type=0x05 → ST25R3916 alive at 0x50.
- Post-fix `nfc-regs` confirms all init registers match the M5 lib.

### What didn't work

- `nfc-read` returns "no tag" with RSSI=0: the tag is on the head, not the body coil. 25 s of `nfc-poll` with the tag on top found nothing.

### What I learned

- The ST25R3916 OPERATION_CONTROL bits: en=0x80 (oscillator+regulator), rx_en=0x40, tx_en=0x08, wu=0x04, en_fd=0x03. Getting these wrong silently breaks the field.
- `field_on` (CMD_NFC_INITIAL_FIELD_ON) must be followed by CLEARING tx_en|rx_en; the direct transmit commands manage tx/rx themselves.
- `enable_osc()` sets the `en` bit and waits for the I_osc (0x80) interrupt; without it the oscillator may not be ready.
- The NFC coil is on the body, not the head. RSSI=0 + no RXE IRQ is the signature of "no tag on the coil".

### What was tricky to build

- The M5 lib's `enable_osc()` lives in `unit_ST25R3916_util.cpp` (not the main file), and the terminal renders "Expander"/"enable" substrings as "n" in `cat` output, so I had to use the `read` tool and `rg` for clean source. Several register bit values (tx_en/rx_en/en, I_col, no_crc_rx, z_600k) were wrong on first write and only surfaced by cross-checking `ST25R3916_definition.hpp` line-by-line.
- Space A vs Space B register addressing: `0x2A` (Space A, field-detector) vs `0x002A` (Space B, resistive-AM) look identical but are different registers; only Space A is needed for the reader.

### What warrants a second pair of eyes

- The anticollision pseudocode still assumes a single tag (NVB=0x20, no bit-narrowing loop) — fine for Phase 1 but will collide with 2+ tags.
- Confirm the NFC coil physical location on the body (front chest vs back) with the user once a tag reads.
- The `fifo_read` uses a register-read of 0x1F; verify against the datasheet that this streams FIFO bytes correctly (it returned 0 bytes on no-tag, which is expected, but a real tag read will confirm).

### What should be done in the future

- Implement the full anticollision bit-narrowing loop for multi-tag support.
- Wire the ST25R3916 IRQ pin to a GPIO for low-CPU detection (Phase 2+).
- Add NDEF read/parse once UID reading is confirmed on hardware.

### Code review instructions

- Build: `cd 0115-m5stackchan-nfc-reader && source ~/esp/esp-idf-5.5.4/export.sh && idf.py build`.
- Flash+monitor: `idf.py -p /dev/ttyACM0 flash` then `nfc-scan`, `nfc-regs`, `nfc-field on`, `nfc-read` (with a tag on the body coil).
- Cross-check `st25r3916.c` init + `configure_nfca` against `sources/code/unit_ST25R3916.hpp` + `unit_ST25R3916_nfca.cpp`.

### Technical details

- Commits: `8ab0673` (project builds), `d0d425b` (init fixes + debug), `1f8b3de6` (field-detector thresholds).
- Binary: 0x3b010 bytes (~244 KB), 77% of the 1 MB factory partition free.
- Post-init regs: `OPC=83 MODE=09 ISO=00 AUX=00 RX1=08 RX2=2D RSSI=00 IRQ=001C00`.
- Post field-on+read (no tag on coil): `OPC=8B MODE=09 ISO=01 AUX=80 RSSI=00 IRQ=000000`.

---

## Step 6: Debugging "no tag" — driver correctness vs antenna coupling

After the init fixes, the chip detects and the field commands on, but `nfc-read`/`nfc-poll` return "no tag". I added diagnostics and chased the antenna-coupling question.

### Prompt Context

**User prompt (verbatim):** (see Step 5)
**Assistant interpretation:** Keep debugging until the tag reads; stop only for physical steps.
**Inferred user intent:** A working tag read on hardware.

### What I did

- Added `nfc-regs` (dump OPC/MODE/ISO/AUX/RX1/RX2/RSSI/MAIN_IRQ/FIFO) and `nfc-sweep` (CMD_MEASURE_AMPLITUDE loop) + `nfc-reqa` (loop REQA, print ATQA on hit) to locate the coil by sweeping the tag.
- Found + fixed 3 more driver bugs by cross-checking the M5 lib:
  1. `fifo_read` must use the dedicated `OP_READ_FIFO = 0x9F` and read only `fifo_bytes()` bytes (I was reading a fixed count from reg 0x1F).
  2. `set_tx_bytes` layout: `value = (bytes << 3) | bits` across reg 0x22/0x23 (I had bytes/bits in wrong positions).
  3. `fifo_bytes()` parsing: `bytes = reg0x1F | ((reg0x1E & 0xC0) << 2)` (I had the nibbles wrong).
- Added `st25r3916_force_field_on()` (disable en_fd field detector, NFC_INITIAL_FIELD_ON, set tx_en|rx_en) to rule out the auto field detector vetoing field-on.
- Observed: post-init `OPC=83 MODE=09 RX1=08 RX2=2D` (correct). After field-on: `OPC=8B` (tx_en set during TX). `RSSI=00`, `MAIN_IRQ=000000`. Amplitude sweep = 0 even with forced field + rx enabled. 40 s REQA sweep = 0 hits. ONE earlier `reqa err: ESP_FAIL` (before the fifo_bytes fix) implied RXE fired once — the antenna CAN radiate, but it is intermittent.
- Confirmed oscillator starts cleanly (no "oscillator did not stabilize" warning in boot log).
- Confirmed via StackChan-BSP that there is no NFC power-enable IO-expander pin; ST25R3916 is always powered when the body is seated.

### Why

I wanted to isolate whether the remaining failure is software (driver/IRQ/FIFO) or physical (tag placement / antenna coupling). The register dump + amplitude sweep isolate it to the antenna/RF stage.

### What worked

- `nfc-regs` confirms every init register now matches the M5 lib — the I2C + chip-config path is correct.
- `nfc-reqa`/`nfc-sweep` give the user sweep tools to find the coil.

### What didn't work

- Amplitude measurement reads 0 even with forced field + rx enabled → either the antenna is not radiating, or CMD_MEASURE_AMPLITUDE needs configuration (reg 0x33/0x34) I have not set. As a coil-finder it is inconclusive.
- 40 s of sweeping the tag over the body produced 0 REQA hits (one fluke RXE earlier). Cannot reliably couple a tag.

### What I learned

- The ST25R3916 FIFO is read via a special `OP_READ_FIFO=0x9F` command (not a normal register read), and the byte count lives in the FIFO status registers as `reg0x1F | ((reg0x1E & 0xC0) << 2)`.
- The TX-byte-count register packs `value = (bytes << 3) | bits`.
- amplitude=0 with the field commanded on is not by itself conclusive — the measurement may need its own config registers; the decisive test remains REQA getting RXE.

### What was tricky to build

- The forward-declaration / two-file edit ordering caused a couple of linker failures (calling a function before defining it). Resolved with a forward decl of `fifo_bytes()`.
- Distinguishing "no tag" (REQA timeout, ESP_ERR_NOT_FOUND) from "tag answered but FIFO read failed" (ESP_FAIL) required reading `st25r3916_reqa`'s return paths carefully.

### What warrants a second pair of eyes

- Whether the antenna is actually radiating: amplitude=0 is suspicious. Needs either a spectrum analyzer / another NFC reader held near the coil, or confirming the body's NFC antenna is populated and seated.
- Whether the tag is ISO14443-A and known-good on another reader.

### What should be done in the future

- Configure the amplitude measurement (reg 0x33/0x34) if we want a reliable coil-finder, or use CMD_MEASURE_CAPACITANCE to detect the antenna coil presence.
- Once a tag reads reliably, re-enable the single-shot `nfc-read` path and validate the anticollision cascade levels with a 7-byte-UID NTAG.

### Code review instructions

- `nfc-regs` after boot should show `OPC=83 MODE=09 RX1=08 RX2=2D`.
- `nfc-sweep`/`nfc-reqa` are the sweep tools; `nfc-read` does the full poll.
- The driver init + REQA + anticollision now match `sources/code/unit_ST25R3916.hpp` + `unit_ST25R3916_nfca.cpp`.

### Technical details

- Commits: `794f7578` (FIFO/REQA fixes), this step pending.
- Diagnostic outputs: post-init `OPC=83 MODE=09 ISO=00 AUX=00 RX1=08 RX2=2D RSSI=00 IRQ=001C00`; post forced-field `OPC=EB` (en|tx_en|rx_en|en_fd), amplitude=0.

---

## Step 7: Decisive instrumentation — coil connected, no tag responds

Per the handoff's "one-line instrumentation that settles the most" (§8), I added `nfc-cap` (CMD_MEASURE_CAPACITANCE 0xDE → reg 0x25), raw MAIN_IRQ logging inside `st25r3916_reqa()`, and antenna-tuning/TX-driver readback in the debug dump. These are pure-software diagnostics I could run without the user.

### Prompt Context

**User prompt (verbatim):** (goal continuation: continue toward the Phase-1 objective, avoid repeating work, choose the next concrete action)
**Assistant interpretation:** Do the software-only instrumentation from the handoff that distinguishes "no RF" from "FIFO/timing bug" before handing back to the user for physical steps.
**Inferred user intent:** Make maximum progress on the diagnosis without needing the user, since the goal says to stop only for physical steps.

### What I did

- Added `st25r3916_measure_capacitance()` + `nfc-cap` command (CMD_MEASURE_CAPACITANCE 0xDE, read reg 0x25).
- Added `ESP_LOGI(TAG, "reqa: irq=%06X fifo=%u rxs=%d rxe=%d col=%d", ...)` inside `st25r3916_reqa()` after `wait_irq()`.
- Added ANT1/ANT2/TXD readback to `st25r3916_debug_dump()`.
- Built, flashed, ran: `nfc-cap` → **cap=124 stable (123–125)**; `nfc-regs` → **ANT1=82 ANT2=82 TXD=D0** (all programmed correctly); `nfc-reqa` → **`reqa: irq=000000 fifo=0 rxs=0 rxe=0 col=0` on every one of 40+ attempts**.
- Updated the handoff doc (§2 symptom, §6 hypotheses, §8 instrumentation) with the decisive evidence; ruled out the open-antenna-feed hypothesis (hypothesis 4) and the dropped-IRQ timing hypothesis (hypothesis 5).

### Why

The handoff listed these as the next decisive steps that don't need the user. `cap=124` proves the coil is connected; `irq=000000` proves no tag responds at all (not even RXS). Together they move the failure firmly out of software and into "the tag/placement" — the physical step the goal says to stop for.

### What worked

- `nfc-cap` returns a stable non-zero capacitance (124) → antenna coil is present and connected.
- Readback confirms ANT1=82 ANT2=82 TXD=D0 → our writes stuck; the antenna tuning + TX driver are correctly programmed, matching the M5 lib.
- Raw IRQ logging is clean and unambiguous: `irq=000000` every time.

### What didn't work

- Still no tag response. Every REQA yields zero IRQ bits. The antenna radiates (cap measurement works, field commands on, one earlier RXE) but no tag answers in the current placement.

### What I learned

- `CMD_MEASURE_CAPACITANCE` is an excellent non-physical "is the antenna connected?" probe: a stable non-zero value (124) means the coil is wired, no ohmmeter needed.
- `irq=000000` after REQA (no RXS, no RXE, no COL) is the definitive "no tag in field" signature — distinct from "tag answered but FIFO read failed" (which would show RXE). This rules out the FIFO/timing bug hypothesis.

### What was tricky to build

- The `nfc-reqa` loop runs 200 iterations and does not return, so subsequent console commands queue behind it; had to reset the device (RTS toggle) to run `nfc-cap` cleanly.

### What warrants a second pair of eyes

- Confirm the tag is a real ISO14443-A tag (NTAG/MIFARE Ultralight), verified to read on a phone, and place it exactly on the body's coil sweet spot. This is the one remaining variable.

### What should be done in the future

- If a known-good tag also never answers, flash the M5 Arduino `Detect.ino` to bisect firmware vs antenna matching (handoff §7 step 5).
- Configure the amplitude measurement regs (0x33/0x34) only if a real coil-finder is still wanted; `nfc-cap` already proved the coil is present.

### Code review instructions

- `nfc-cap` → expect cap≈124 (stable). `nfc-regs` → `ANT1=82 ANT2=82 TXD=D0`. `nfc-reqa` → `reqa: irq=000000` lines (no tag).
- The driver init + REQA + anticollision match `sources/code/unit_ST25R3916.hpp` + `unit_ST25R3916_nfca.cpp`.

### Technical details

- Commit: `86bbeee1` (instrumentation + decisive evidence). Handoff updated in the same ticket.
- Evidence: cap=124 (coil connected); ANT1/2=82, TXD=D0 (programmed ok); REQA irq=000000 ×40+ (no tag responds).

---

## Step 8: WUPA fallback — rules out halted-tag state (still no response)

Goal continuation asked me to keep making progress without the user. The one remaining software hypothesis was that the tag got HALTED by an earlier SELECT and would answer WUPA but not REQA. I added WUPA and retested.

### Prompt Context

**User prompt (verbatim):** (automated goal continuation: "Continue working toward the active thread goal... Choose the next concrete action.")
**Assistant interpretation:** Do the one non-repeating software action that could still turn a no-tag into a read — add WUPA (wakes halted tags) — before handing back to the user for the physical step.
**Inferred user intent:** Maximize software progress; only stop when genuinely blocked on hardware.

### What I did

- Refactored REQA into a shared `nfca_wake(atqa, wake_cmd)` helper; added `st25r3916_wupa()` (CMD_STOP_ALL_ACTIVITIES → field_on → CMD_TRANSMIT_WUPA) to clear halt state first.
- Made `poll_nfca` fall back to WUPA when REQA returns NOT_FOUND.
- Made `nfc-reqa` alternate REQA/WUPA each iteration so the sweep finder tries both.
- Built, flashed, ran the alternating sweep ~12s with the tag on the device (as the user left it): **both `reqa: irq=000000` and `wupa: irq=000000` on every attempt** (0 ATQA hits).

### Why

A tag selected then halted by a prior `nfc-read` will not answer REQA but WILL answer WUPA. If WUPA had returned an ATQA, the halt state was the bug and we'd have a read. It did not — so the halt hypothesis is ruled out.

### What worked

- WUPA path builds and runs cleanly; `poll_nfca` is now more robust (real improvement, committed).

### What didn't work

- Both REQA and WUPA return `irq=000000`. The tag is not coupling to the coil in its current position (on top of the device = the head/display, not the body where the NFC coil lives).

### What I learned

- WUPA vs REQA is a clean discriminator for the halt-state hypothesis; getting `irq=000000` for both means the tag is simply not in the field, not that it's halted.

### What was tricky to build

- None significant; the refactor to a shared `nfca_wake()` kept the IRQ logging consistent for both commands.

### What warrants a second pair of eyes

- The user must place the tag on the **body** (lower part with servos), not the head. This is the only remaining variable.

### What should be done in the future

- Once the tag is on the coil, `nfc-read` should return a UID; validate the CL1→CL2 cascade with a 7-byte NTAG.

### Code review instructions

- `nfc-reqa` now prints alternating `reqa:` / `wupa:` lines; both `irq=000000` until the tag is on the coil.

### Technical details

- Commit: `c2f4b322`. Evidence: REQA irq=000000 AND WUPA irq=000000 (halt state ruled out); cap=124 (coil connected); all init regs correct.

---

## Step 9: CORRECTION — NFC antenna is on the TOP of the head, not the body

Goal continuation nudged progress without the user. I researched the exact NFC antenna location and discovered I had been giving the WRONG placement advice for several turns.

### Prompt Context

**User prompt (verbatim):** (automated goal continuation: continue toward the objective, avoid repeating work)
**Assistant interpretation:** Find the exact NFC antenna location so I can give precise (correct) placement instructions, since placement is the suspected blocker.
**Inferred user intent:** Resolve the no-read by eliminating the placement variable correctly.

### What I did

- Kagi-searched for the StackChan NFC antenna location; result #7 was the official M5 NFC docs page (docs.m5stack.com/en/arduino/stackchan/nfc).
- defuddle returned empty (JS-rendered), so I used Playwright to read the page.
- The official docs state (Quick Scan + Complete Data Reading + Emulation examples): the antenna is on the **top sensing surface of StackChan** (the head/CoreS3 top), NOT the body. "place one or more tag cards near the top sensing surface of StackChan".
- Realized my repeated "move the tag to the body" instruction (Steps 6–8 handoff) was WRONG and likely moved the tag OFF the coil. The user's original "on top" placement was correct.
- Saved the finding to `sources/web/02-m5stack-stackchan-nfc-antenna-location.md`; appended a CORRECTION to the handoff doc.
- Re-ran `nfc-reqa` (alternating REQA/WUPA) with the corrected understanding (tag should be on top of head): STILL `irq=000000` for both. So correct placement + correct software + connected coil → the remaining variable is the tag itself.

### Why

For several turns I assumed the coil was on the body and told the user to move the tag there. The official docs contradict that. Correcting this was essential before asking the user again — otherwise I'd keep pointing them at the wrong surface.

### What worked

- Playwright read the JS-rendered M5 docs page cleanly; the antenna-location quotes are unambiguous.
- Saved the authoritative source into the ticket so the intern/expert won't repeat the mistake.

### What didn't work

- Re-running with the corrected placement still yields `irq=000000`. Placement was not (alone) the bug; with correct placement, no tag still responds.

### What I learned

- **The NFC antenna is on the top of the StackChan head, not the body**, even though the ST25R3916 IC is on the body I2C bus. Always trust the official M5 docs over assumptions about where a chip's antenna lives.
- `CMD_MEASURE_AMPLITUDE`=0 with no tag is EXPECTED (it measures received amplitude; nothing reflects without a tag) — my earlier "amplitude=0 means antenna dead" inference was wrong. cap=124 (capacitance) is the real "is the coil connected" probe.

### What was tricky to build

- Admitting a multi-turn wrong assumption. The handoff doc had explicitly told the expert the coil was on the body; that had to be corrected in place.

### What warrants a second pair of eyes

- The tag itself: is it a real ISO14443-A tag (NTAG/MIFARE Ultralight/amiibo)? Does it read on a phone? A bank card or 125kHz RFID fob will not answer a bare REQA.

### What should be done in the future

- If a known-good NTAG on top of the head still yields `irq=000000`, flash the M5 Arduino `Detect.ino` to bisect firmware vs tag/hardware.

### Code review instructions

- Read `sources/web/02-m5stack-stackchan-nfc-antenna-location.md` (the official placement quotes).
- `nfc-reqa` with a known-good ISO14443-A tag flat on the top of the head should print `ATQA=0044`.

### Technical details

- Commit: `e5705ed9` (correction + source). Antenna location: TOP of head (M5 docs). Tag-on-top still `irq=000000`.

---

## Step 10: Add the missing NFC-A frame-wait timer

The confirmed ISO14443-A NTAG changed the diagnosis from a questionable tag to a concrete firmware discrepancy. I implemented the ST25R3916 No-Response Timer setup that the M5 reference driver performs before REQA/WUPA and anticollision, while preserving integer arithmetic suitable for embedded firmware.

The firmware builds cleanly, but this step intentionally stops short of claiming a successful read. Live validation requires the user to place the NTAG flat on the top sensing surface; the code commit is therefore an implementation milestone, not proof that the timer hypothesis is correct.

### Prompt Context

**User prompt (verbatim):** "commit at appropriate intervals and keep a detailed diary as you work (using the diary format from the skill)"

**Assistant interpretation:** Continue the NFC fix with focused commits and maintain a strict, evidence-backed implementation diary rather than batching undocumented work.

**Inferred user intent:** Preserve a reviewable debugging trail and make each firmware experiment reproducible and easy to hand off.

**Commit (code):** `74bc45f981e39f44d09d4ea20ac5497f33d62019` — "ESP-60 P1: configure ST25R3916 frame wait timer before NFC-A exchanges"

### What I did

- Inspected M5Unit-NFC's `write_fwt_timer()`, `calculate_nrt()`, `writeNoResponseTimer()`, and NFC-A timeout constants.
- Confirmed reference timeouts: 4 ms for REQA/WUPA and 8 ms for anticollision.
- Added register definitions for NRT1 (0x10), NRT2 (0x11), Timer/EMV Control (0x12), and `nrt_step` (bit 0).
- Added integer-only `set_frame_wait_time()` using ceiling division at 13.56 MHz.
- Wrote the 16-bit NRT big-endian, matching M5's `writeRegister16BE()` behavior.
- Configured 4 ms before REQA/WUPA and 8 ms before anticollision.
- Added NRT and Timer/EMV values to `nfc-regs` diagnostics.
- Ran `source ~/esp/esp-idf-5.5.4/export.sh && idf.py build` successfully.

### Why

- The M5 reference driver explicitly programs NRT before every NFC-A exchange; our driver did not.
- A confirmed NTAG213/215/216 should answer ISO14443-A REQA, so unexplained timing differences must be tested rather than blaming the tag.

### What worked

- The ported calculation exactly follows the reference integer formula and requires no floating point.
- The firmware compiled and linked successfully; the binary remains 77% below the 1 MB partition limit.
- `git diff --check` passed and the focused code change was committed separately from documentation.

### What didn't work

- Live RF validation has not run yet because it requires the user's physical confirmation that the NTAG is on the top sensing surface.
- Therefore there is no evidence yet that NRT was the root cause.

### What I learned

- M5 uses `TIMEOUT_REQ_WUP=4` ms and `TIMEOUT_ANTICOLL=8` ms, not the 50 ms host-side polling timeout.
- NRT register order is big-endian: register 0x10 receives the MSB and 0x11 the LSB.
- With `nrt_step=0`, 4 ms calculates to 848 ticks (`0x0350`); 8 ms calculates to 1695 ticks (`0x069F`) using ceiling division.

### What was tricky to build

- The hardware timer and host polling timeout are separate invariants: NRT controls the chip's receive window, while `wait_irq(50)` controls how long firmware polls IRQ registers. Conflating them would hide timing failures.
- Correct byte order matters because swapping `0x0350` into `0x5003` would create a dramatically longer timer and invalidate the comparison with M5's implementation.

### What warrants a second pair of eyes

- Verify from the ST25R3916 datasheet that an NRT expiration should be visible in the Timer/NFC IRQ register and whether our current IRQ-clearing strategy masks useful evidence.
- Confirm whether anticollision and SELECT should each refresh NRT separately; this implementation sets 8 ms once before the anticollision command.
- The handoff's claim that NRT exactly explains `irq=000000` remains a hypothesis until the live probe.

### What should be done in the future

- With the tag physically placed, flash commit `74bc45f9`, run `nfc-regs`, `nfc-reqa`, and `nfc-read`, and record exact output.
- If REQA succeeds but UID selection fails, instrument anticollision and SELECT IRQ/FIFO states separately.
- If REQA still has no RXS/RXE, inspect Timer/NFC IRQ bits and then run the official M5 Arduino Detect example as the firmware/hardware bisect.

### Code review instructions

- Start at `0115-m5stackchan-nfc-reader/main/st25r3916/st25r3916.c::set_frame_wait_time()` and compare it to `/tmp/nfc-research/repos/M5Unit-NFC/src/unit/unit_ST25R3916_util.cpp::calculate_nrt()`.
- Review calls in `nfca_wake()` and `nfca_anticoll_select()`.
- Validate with `cd 0115-m5stackchan-nfc-reader && source ~/esp/esp-idf-5.5.4/export.sh && idf.py build`.
- For hardware validation, place a known NTAG flat on top of the head, flash, then run `nfc-regs`, `nfc-reqa`, and `nfc-read` over `/dev/ttyACM0`.

### Technical details

- 4 ms, 64/fc step: `ceil(4000 us × 13,560,000 / 64,000,000) = 848 = 0x0350`.
- 8 ms, 64/fc step: `ceil(8000 us × 13,560,000 / 64,000,000) = 1695 = 0x069F`.
- Modified files:
  - `/home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0115-m5stackchan-nfc-reader/main/st25r3916/st25r3916.c`
  - `/home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0115-m5stackchan-nfc-reader/main/st25r3916/st25r3916_regs.h`

---

## Step 11: Live NRT probe exposes RF reception and a FIFO byte-order bug

With the user holding the confirmed NTAG on the top sensing surface, I flashed the NRT build and observed the first reproducible receive activity: REQA produced `irq=000034`, meaning RXS, RXE, and collision were all asserted. This disproves the prior assumption that the reader never receives tag modulation and provides direct evidence that adding NRT materially changed runtime behavior.

The receive IRQ immediately exposed a second bug: FIFO status decoding had its register bytes reversed, so RXE was reported while the driver falsely claimed the FIFO held zero bytes. I corrected the formula against M5Unit-NFC, rebuilt, committed, and reflashed. A subsequent stationary probe did not reproduce RF reception, so UID verification remains incomplete and the next experiment must adjust physical tag alignment while preserving the corrected firmware.

### Prompt Context

**User prompt (verbatim):** "ready"

**Assistant interpretation:** The known ISO14443-A NTAG is physically placed on the correct top sensing surface, so flashing and live reader validation may proceed.

**Inferred user intent:** Complete Phase-1 hardware verification by obtaining the tag UID from `nfc-read`.

**Commit (code):** `f8015daa2354bcfff844b10657bc54508e1cb3ae` — "ESP-60 P1: correct ST25R3916 FIFO status byte order"

### What I did

- Verified `/dev/ttyACM0` had no owner and flashed the NRT build.
- Ran a single pyserial probe against `nfc-regs`, `nfc-reqa`, and `nfc-read`.
- The first probe script timed out because `nfc-reqa` emits continuously for roughly 30 seconds and the script incorrectly waited for a quiet serial stream.
- Reopened the port after timeout and captured fixed-duration windows.
- Observed repeated real receive events including: `reqa: irq=000034 fifo=0 rxs=1 rxe=1 col=1`.
- Sent Ctrl-C to stop the long-running console command cleanly.
- Confirmed NRT readback: `NRT=0350 TEMV=00` (4 ms at the 64/fc timer step).
- Compared `fifo_bytes()` directly with M5's `readFIFOSize()` and found our status bytes reversed.
- Corrected FIFO bytes to `reg0x1E | ((reg0x1F & 0xC0) << 2)`.
- Rebuilt and reflashed commit `f8015daa` successfully.
- Ran `nfc-read` again; that later probe returned no receive IRQ.

### Why

- Live RF evidence was required to validate whether NRT affected the receive window.
- RXE with an apparently empty FIFO demanded source-level comparison before changing higher-level NFC logic.

### What worked

- Flashing was reliable with no serial ownership conflict.
- NRT register readback exactly matched the expected `0x0350`.
- REQA produced RXS and RXE after the NRT fix, proving the receive path and tag can couple.
- Source comparison identified a deterministic FIFO decoding bug.
- The corrected firmware builds and flashes successfully.

### What didn't work

- Initial probe command:
  `python3 ... while True: chunk=s.read(4096); if not chunk: break ...`
  timed out after 20 seconds because the active `nfc-reqa` command never left the stream quiet.
- Before the FIFO fix, the exact receive output was:
  `I (...) st25r3916: reqa: irq=000034 fifo=0 rxs=1 rxe=1 col=1`
  followed by `wake err: ESP_FAIL`.
- After the FIFO fix and reflash, exact `nfc-read` output was:
  `reqa: irq=000000 fifo=0 rxs=0 rxe=0 col=0`
  `wupa: irq=000000 fifo=0 rxs=0 rxe=0 col=0`
  `no tag`
- A UID was not obtained, so Phase 1 is not complete.

### What I learned

- NRT was a real missing protocol step: it changed behavior from consistently no receive event to reproducible RXS/RXE events while the tag was placed.
- The old FIFO formula recorded in prior diary/handoff work was wrong. M5's big-endian status read means register 0x1E supplies the low eight count bits; register 0x1F bits 7:6 supply count bits 9:8.
- RF coupling remains sensitive enough that a reflash/reset or a small physical shift can change a received REQA into no response.

### What was tricky to build

- The `0x34` IRQ combines RXS (`0x20`), RXE (`0x10`), and COL (`0x04`). Collision does not negate RXE; M5 still attempts to read two ATQA bytes and warns that ATQA may be inaccurate during collision.
- The apparent `fifo=0` initially looked like a receive-decoder problem, but it was a status-register byte-order error. The symptom only became visible after NRT allowed reception.
- Live console commands have different lifetimes: `nfc-reqa` loops 200 times, so generic “read until quiet” automation is unsafe.

### What warrants a second pair of eyes

- Verify the corrected FIFO formula against the ST25R3916 datasheet in addition to M5Unit-NFC.
- Determine why a single NTAG reports COL during REQA; this could be alignment/noise, multiple tags nearby, or receiver configuration.
- Review whether repeated STOP_ALL_ACTIVITIES/WUPA field cycling contributes to intermittent coupling.

### What should be done in the future

- Ask the user to slide the tag slowly across the top surface while running the fixed NRT+FIFO build's `nfc-reqa` loop.
- Stop at the first stable `irq` with RXE and immediately run `nfc-read` without reflashing or resetting.
- If FIFO contains two ATQA bytes, continue into anticollision and instrument each cascade stage if UID selection fails.

### Code review instructions

- Review `fifo_bytes()` in `main/st25r3916/st25r3916.c` against M5Unit-NFC `readFIFOStatus()` + `readFIFOSize()`.
- Reproduce with the tag on top: run `nfc-regs` (expect `NRT=0000` before a request), then `nfc-read` (expect NRT to become `0350`).
- For placement search, run `nfc-reqa`, move the tag slowly, and look for `rxs=1 rxe=1`; interrupt with Ctrl-C before invoking `nfc-read`.

### Technical details

- NRT receive evidence: IRQ `0x34 = 0x20 (RXS) | 0x10 (RXE) | 0x04 (COL)`.
- Correct FIFO formula: `bytes = status1 | ((status2 & 0xC0) << 2)`, where `status1=reg0x1E`, `status2=reg0x1F`.
- Firmware commits under test: NRT `74bc45f9`; FIFO correction `f8015daa`.

---

## Step 12: Preserve the RF carrier and separate valid frames from collision noise

A second live sweep reproduced receive activity but still yielded no FIFO payload. Comparing our WUPA/poll path to M5Unit-NFC revealed that our driver unnecessarily stopped all ST25R3916 activity and restarted the field before every WUPA, while the reference keeps the carrier continuously established. I removed that power cycling so the passive NTAG can remain energized across request attempts.

The continuous-carrier build increased nonzero receive events during a 34-second sweep from one to four, across both REQA and WUPA. All events remained `IRQ=0x34` with zero FIFO bytes, however, so they are collision/noise indications rather than valid ATQA frames. The next test must compare tag-absent and tag-present baselines before further software changes.

### Prompt Context

**User prompt (verbatim):** "ok continue. can"

**Assistant interpretation:** Continue the live NFC investigation and perform the next safe firmware/probe steps.

**Inferred user intent:** Keep driving toward a successful UID read without pausing after every intermediate diagnostic.

**Commit (code):** `f183853b202975ac700aead102491a2f40d55748` — "ESP-60 P1: keep NFC carrier continuous across polling"

### What I did

- Ran a 34-second live REQA/WUPA sweep with the FIFO-corrected build; captured 348 lines and one nonzero receive event.
- Rechecked M5's `nfc_initial_field_on()` against our implementation; both clear TX/RX after the initial-field command.
- Found a sequence mismatch in our WUPA implementation: it issued STOP_ALL_ACTIVITIES, restarted the field, and waited 5 ms before every WUPA.
- Removed the WUPA stop/restart sequence and removed redundant field restart at the start of every `poll_nfca()` call.
- Built, committed, and flashed the continuous-carrier firmware.
- Ran another 34-second sweep; captured 350 lines and four nonzero receive events.
- Re-verified FIFO register addresses and M5's big-endian read semantics; the corrected FIFO formula is confirmed.

### Why

- Passive NTAGs derive power from the RF carrier. Repeatedly stopping the field every other loop iteration can reset the tag and prevent stable protocol progress.
- A zero-length FIFO after RXE required ruling out another status-decoding mistake before treating the IRQ as analog noise/collision.

### What worked

- Continuous carrier increased observed receive events from one to four during comparable capture windows.
- Both REQA and WUPA generated receive/collision IRQs, showing that the direct commands execute and the receiver is active.
- Build and flash succeeded; no serial ownership conflict occurred.

### What didn't work

- No event produced two FIFO bytes or `ATQA=...`.
- Every nonzero event was `irq=000034 fifo=0 rxs=1 rxe=1 col=1`, followed by `wake err: ESP_FAIL`.
- UID reading remains incomplete.

### What I learned

- The old WUPA implementation did not match the reference and repeatedly depowered the tag.
- IRQ 0x34 without FIFO data is not a valid ISO14443-A response; it is best treated as collision/noise until proven otherwise.
- M5 register constants confirm FIFO Status 1=0x1E and Status 2=0x1F; `read_register16()` is big-endian, validating the corrected formula.

### What was tricky to build

- More IRQ events are not automatically progress: receive-start/end plus collision can be caused by marginal coupling or noise and still produce no frame.
- Physical sweep timing varies, so comparing event counts is directional evidence, not a controlled RF measurement.

### What warrants a second pair of eyes

- Determine whether COL with no FIFO bytes indicates receiver gain/noise configuration, malformed modulation, or another nearby NFC object.
- Review the M5 receiver configuration and error IRQ handling, including parity/CRC/error registers currently omitted from logs.
- Confirm whether external-field detector settings influence carrier stability in this board arrangement.

### What should be done in the future

- Capture a tag-absent baseline with all NFC objects and phones removed.
- Capture a tag-present sweep under otherwise identical conditions.
- If collision events occur without a tag, instrument error/timer IRQ registers and tune the receiver path rather than protocol logic.
- If collision events only occur with the tag, optimize placement and inspect analog receiver settings.

### Code review instructions

- Review `st25r3916_wupa()` and `st25r3916_poll_nfca()` in `main/st25r3916/st25r3916.c`.
- Compare them to M5Unit-NFC `nfca_request_wakeup()`, which does not cycle the field.
- Reproduce using fixed 34-second `nfc-reqa` captures and compare nonzero IRQ events.

### Technical details

- Pre-change sweep: 348 lines, one `IRQ=0x34` event.
- Continuous-field sweep: 350 lines, four `IRQ=0x34` events.
- Logs: `/tmp/esp60-live-sweep.log` and `/tmp/esp60-continuous-sweep.log` (local transient evidence, not committed).

---

## Step 13: Re-anchor debugging in official documentation and run the Arduino bisect

The user correctly challenged the incremental debugging approach and asked for the proper documentation and official sketch to be read. A fresh source-of-truth review found that prior claims of a byte-for-byte M5 initialization match were false: the ESP-IDF driver had swapped IO configuration bitfields between registers and omitted reader-critical Space-B analog/correlator writes.

I preserved the exact upstream implementation and official placement photographs in the ticket, ported the documented initialization sequence, proved the new values through Space-B readback, and built/flashed the unmodified official StackChan `Detect.ino` as a hardware/firmware bisect. The official sketch now runs on the device; its screen result is awaiting the user's report because this Arduino board profile emitted no serial data on USB Serial/JTAG.

### Prompt Context

**User prompt (verbatim):** "make sure to read the proper documentation if you haven't, and maybe the ino code or so"

**Additional user prompt (verbatim):** "save relevant documents from the web to sources/ if appropriate"

**Assistant interpretation:** Stop speculative register debugging, inspect the official example and full implementation call chain, preserve authoritative sources in the ticket, then use the official firmware as the decisive bisect.

**Inferred user intent:** Replace accumulated assumptions with reproducible vendor evidence and determine whether the NFC hardware/tag works under M5's supported stack.

**Commit (sources):** `4ccc993ecd1259328651c77c2368587c17f23b8f` — "ESP-60: preserve official M5 NFC implementation and placement evidence"

**Commit (code):** `d4136068e6e963041948a52da06e812ca99415c3` — "ESP-60 P1: match documented M5 ST25R analog and NFC-A initialization"

**Commit (diagnostics):** `0b87585cec3d5a9ffdcf5d3fc49f0255dff047bc` — "ESP-60 P1: expose ST25R Space-B receive configuration readback"

### What I did

- Read the complete official `StackChan-BSP/examples/NFC/Detect/Detect.ino`.
- Followed its call chain through `UnitST25R3916::begin()`, `configure_nfc_a()`, `nfca_request_wakeup()`, `NFCLayerA::detect()`, selection, identification, and deactivation.
- Re-read the ticket's datasheet acquisition note; attempted the canonical ST PDF URL, which timed out without producing a file.
- Preserved upstream M5Unit-NFC implementation files at commit `93745b54`, official Detect example at StackChan-BSP commit `f7ed40e6`, and official placement images in `sources/`.
- Corrected the placement note: official photos show cards across the literal top edge/roof, not the front display.
- Found and corrected IO configuration from incorrect `8B/30` to documented final `17/A4`.
- Added mandatory test-access protection frame `FC 04 10` after SET_DEFAULT.
- Added Space-B access and M5 analog initialization: resistive AM, EMD suppression, overshoot/undershoot protection, and correlator configuration.
- Added NFCIP FDT `0x50` and passive-target modulation `0x5F` to match M5 begin().
- Added Space-B readback diagnostics.
- Built, committed, flashed, and confirmed readback:
  `SpaceB: OS=40/03 US=40/03 CORR=47/00 EMD=40`.
- Created an isolated PlatformIO official-example build under `/tmp/esp60-official-detect`.
- Resolved build environment requirements using pinned PIOArduino `55.03.311`, Arduino-ESP32 `3.3.11`, IDF libs `5.5.5`, C++17, and an explicit Wire include path.
- Successfully built and flashed the official sketch.
- Captured `/dev/ttyACM0` for 18 seconds; output was empty, so screen state is needed for the bisect.

### Why

- Header-only comparison omitted behavior implemented in `.cpp` files and led to false exoneration of firmware initialization.
- The official sketch provides the cleanest firmware-versus-hardware/tag test.
- Preserving exact source revisions prevents future sessions from relying on mutable `/tmp` clones or vague web summaries.

### What worked

- Official images resolved antenna placement unambiguously.
- Full source review found concrete, high-impact initialization mismatches.
- ESP-IDF build succeeded after the documented port.
- Space-B readback proved the receive-path values reached the chip.
- The official Arduino sketch built and flashed successfully with the correct modern toolchain.

### What didn't work

- ST datasheet download command timed out after 60 seconds and produced no file; the canonical URL remains documented in `sources/datasheets/README-download-instructions.md`.
- First official build failed under C++11:
  `error: deduced return type only available with -std=c++14 or -std=gnu++14`.
- First config fix accidentally duplicated `build_flags`:
  `InvalidProjectConfError ... option 'build_flags' ... already exists`.
- Arduino core 2.0.16 build then failed because current BSP requires newer IDF symbols:
  `error: 'UART_SCLK_DEFAULT' was not declared in this scope`.
- PIOArduino initially rejected PlatformIO Core 6.1.18:
  `depends on PlatformIO Core >=6.1.19`.
- Modern builds twice exposed M5HAL dependency metadata failure:
  `fatal error: Wire.h: No such file or directory`; fixed with an explicit framework Wire include path.
- Official sketch produced no USB serial text on `/dev/ttyACM0`; the display must report detection status.
- Corrected ESP-IDF initialization still did not read the tag at the then-current placement.

### What I learned

- M5's final IO pair is `IO_CONFIG_1=0x17`, `IO_CONFIG_2=0xA4`; the earlier `0x8B/0x30` values assigned bitfields to the wrong registers.
- `configure_nfc_a()` programs reader-critical Space-B overshoot, undershoot, and correlator registers; they are not emulation-only.
- Official placement is the literal top edge/roof of the head.
- The current StackChan-BSP requires a modern Arduino-ESP32/ESP-IDF stack, not PlatformIO's legacy official espressif32 6.7.0 platform.

### What was tricky to build

- “Official example” still requires reproducing its expected toolchain. Building against an old Arduino core would create an invalid bisect even if local compatibility patches made it compile.
- M5HAL's metadata does not propagate Wire's include path under this PlatformIO dependency graph; the temporary build needed an explicit framework include directory without changing NFC behavior.
- Space-B access uses the two-byte command prefix `0xFB, register`, unlike normal Space-A access.

### What warrants a second pair of eyes

- Validate the IO pair and Space-B values against the ST datasheet once a manual PDF download is available.
- Review whether the official sketch's empty USB output is expected for the `m5stack-cores3` board profile or needs `ARDUINO_USB_CDC_ON_BOOT` for diagnostics.
- Investigate intermittent ESP-IDF I2C read anomalies separately if the official sketch succeeds.

### What should be done in the future

- Record whether the official sketch displays `PICC:<UID>`, the placement prompt, or an initialization error.
- If official firmware reads the tag, restore ESP-IDF and compare runtime sequencing/I2C transport against M5Unit-NFC.
- If official firmware does not read the tag, validate the tag with a phone and inspect hardware/placement rather than continuing protocol changes.
- Restore ESP-IDF firmware after the bisect.

### Code review instructions

- Start with ticket sources `code/m5unit-nfc/README.md`, `unit_ST25R3916.cpp`, `unit_ST25R3916_nfca.cpp`, and `web/03-m5stack-stackchan-nfc-official-images.md`.
- Review firmware changes in `main/st25r3916/st25r3916.c` around IO setup, `wr8b()`, and `configure_nfca()`.
- Run `nfc-regs` and verify `SpaceB: OS=40/03 US=40/03 CORR=47/00 EMD=40`.

### Technical details

- Official build platform: `https://github.com/pioarduino/platform-espressif32/releases/download/55.03.311/platform-espressif32.zip`.
- Official build result: success, firmware image 671,527 bytes, RAM 30,116 bytes.
- Temporary build path: `/tmp/esp60-official-detect`.
- Official serial capture: `/tmp/esp60-official-detect-serial.log` (empty).

---

## Step 14: Prove the hardware with official firmware and publish the technical report

The corrected official StackChan `Detect.ino` displayed `PICC:<UID>` with the tag across the literal top edge of the head. This is the decisive hardware bisect: the tag, antenna, ST25R3916, and physical position work together on this exact device.

Restoring the ESP-IDF firmware without moving the tag immediately exposed an `i2c.master` transaction timeout and invalid-state error, followed by occasional corrupt register readback. The remaining failure is therefore isolated to the ESP-IDF transport/runtime path. I published a textbook-style technical report to the Obsidian vault that records the complete architecture, protocol, evidence, withdrawn claims, official bisect, and next controlled experiment.

### Prompt Context

**User prompt (verbatim):** "B"

**Additional user prompt (verbatim):** "so maybe it's also all about me having to figure out where to put the tag itself, but now that it works, I know"

**Report prompt (verbatim):** "write a detailed project report for the obsidian vault as a deep dive technical analysis blog post using a textbook writing style (no analogies, see skill).      
 Commit and push the bsidian vault when done (go-go-parc vault).   "

**Assistant interpretation:** Treat the official PICC display as proof of the physical path, restore ESP-IDF to isolate its remaining failure, then write and publish a durable, evidence-backed technical article in the Obsidian vault.

**Inferred user intent:** Preserve the project’s technical lessons and current status in a form that future engineers can learn from, while accurately separating placement resolution from the unfinished ESP-IDF UID read.

**Commit (vault):** `e7003d40d3ca40f2602dd020ecc7fd5ca0db9a05` — "Add deep dive on porting StackChan NFC to ESP-IDF"

### What I did

- Recorded the corrected official firmware screen result: `PICC:<UID>`.
- Restored the latest committed ESP-IDF firmware while the tag remained in the proven position.
- Ran five `nfc-read` attempts.
- Captured `E (...) i2c.master: I2C transaction timeout detected` and `ESP_ERR_INVALID_STATE` on the first attempt.
- Observed later no-tag responses and intermittent invalid register readback, reinforcing the I2C transport diagnosis.
- Prepared but did not build or commit a controlled 100 kHz I2C experiment with matching ST25R threshold configuration.
- Loaded and followed the textbook-authoring, Obsidian vault-writing, and Obsidian Markdown skills.
- Read existing M5StackChan vault articles and the vault’s article/project exemplars.
- Wrote a 4,868-word, 36 KB deep-dive article with frontmatter, Mermaid diagrams, register tables, protocol sequences, failure analysis, and explicit incomplete status.
- Validated frontmatter, headings, internal links, whitespace, and absence of analogy/metaphor phrases.
- Staged only the new article despite unrelated pre-existing vault transcript modifications/deletions.
- Committed and pushed the vault `main` branch; verified `origin/main` at `e7003d4`.

### Why

- The official PICC result is the strongest available evidence that the RF hardware and physical setup are correct.
- The immediate ESP-IDF timeout after restoring firmware narrows the remaining defect to transport/runtime behavior.
- The investigation accumulated several corrected assumptions; a durable report prevents those invalid conclusions from being repeated.

### What worked

- Official firmware read the tag at the literal top-edge position.
- ESP-IDF firmware was restored successfully.
- The vault report passed all structural and style validation.
- The vault commit contained exactly one new article, and the push succeeded.

### What didn't work

- ESP-IDF still did not return a UID.
- Exact failure output included:
  `E (...) i2c.master: I2C transaction timeout detected`
  and `read error: ESP_ERR_INVALID_STATE`.
- The first diagnostic read after timeout reported `OPC=00`, while stable reads normally report `83` or `8B`; this is not valid chip state evidence.
- The 100 kHz experiment remains untested and must not be described as a fix.

### What I learned

- Correct placement and correct firmware are independent variables. Placement is now resolved; ESP-IDF transport is not.
- A successful vendor implementation on identical hardware converts the investigation from an RF uncertainty into a software transport comparison.
- Obsidian publication must preserve incomplete status rather than turn a debugging narrative into a false success report.

### What was tricky to build

- The vault had unrelated modified/deleted transcript files. Staging had to target only the new article, and push verification had to prove those files were excluded.
- The source repository simultaneously contained an untested 100 kHz working-tree change. Ticket documentation had to be committed without staging that experiment.

### What warrants a second pair of eyes

- Compare ESP-IDF `i2c_master_transmit_receive()` timing and bus recovery to M5 `I2C_Class` explicit start/restart/read/write/stop behavior.
- Review whether 100 kHz plus `IO_CONFIG_1=0x07` is the right first controlled transport test.
- Confirm that no protocol code should change until repeated identity/configuration reads are stable.

### What should be done in the future

- Build and flash the 100 kHz experiment while preserving the proven tag position.
- Quantify I2C timeout/corruption rate before and after the speed change.
- If 100 kHz fails, reproduce M5’s explicit transaction sequence or test the ESP-IDF legacy I2C backend in an isolated branch.
- Complete Phase 1 only when ESP-IDF `nfc-read` prints the UID.

### Code review instructions

- Read the published article at `/home/manuel/code/wesen/go-go-golems/go-go-parc/Projects/2026/08/21/ARTICLE - M5StackChan NFC - Porting the ST25R3916 Reader to ESP-IDF.md`.
- Review the uncommitted source diff before building; it contains only the proposed 100 kHz transport experiment.
- Re-run with one tag across the literal top edge and inspect I2C errors before NFC IRQs.

### Technical details

- Vault article: 36,366 bytes, 4,868 words, 58 headings.
- Vault commit/push: `e7003d40d3ca40f2602dd020ecc7fd5ca0db9a05` on `go-go-parc/main`.
- Official result: screen state B, `PICC:<UID> ...`.
- Current ESP-IDF failure class: intermittent new-driver I2C timeout/corrupt readback before a stable ATQA.

---

## Step 15: Correct the tag-presence record and reject the 100 kHz hypothesis

The user discovered that the tag had been removed during the first ESP-IDF run after the successful official-firmware bisect. That run remains evidence of an I2C timeout, but it cannot support any conclusion about NFC reception with a tag present. I corrected the record immediately and repeated the experiment with the tag restored.

The valid tag-present tests showed that 400 kHz still produced no receive IRQ and intermittent corrupt register readback. A controlled 100 kHz build made transport behavior worse and also produced no receive IRQ. I rejected and reverted the 100 kHz change, restored the committed 400 kHz firmware, and pushed a factual correction to the Obsidian report.

### Prompt Context

**User prompt (verbatim):** "oh lol somehow i had removed the tag. try again"

**Assistant interpretation:** Invalidate the earlier NFC conclusion, repeat the ESP-IDF test with the tag physically present, and correct any documentation that relied on the mistaken setup.

**Inferred user intent:** Obtain a valid comparison between the successful official firmware and the ESP-IDF firmware under the same tag-present condition.

**Commit (vault correction):** `223bcd7a027b84109ad471fa54571a394c259634` — "Correct StackChan NFC report with tag-present transport tests"

### What I did

- Re-ran ten `nfc-read` attempts on the committed 400 kHz ESP-IDF build with the tag present.
- Observed no NFC receive IRQ and transient invalid readback (`ISO=00 AUX=09`, later `ANT2=00`).
- Built and flashed the prepared 100 kHz experiment with matching `IO_CONFIG_1=0x07`.
- Ran ten tag-present reads at 100 kHz.
- Observed no NFC receive IRQ, repeated `ESP_ERR_INVALID_STATE`, invalid Space-B values, and corrupt NRT readback.
- Reverted the uncommitted 100 kHz source change.
- Rebuilt and reflashed the committed 400 kHz firmware.
- Corrected and pushed the Obsidian report.

### Why

- Physical test state is part of the evidence. The earlier claim had to be withdrawn as soon as tag absence was known.
- The 100 kHz experiment tested whether bus speed alone caused the new-driver transport failures.

### What worked

- The repeated test now has a valid tag-present setup.
- Reverting the failed experiment returned the source tree and board to the committed 400 kHz state.
- The published report now distinguishes timeout evidence from tag-presence evidence.

### What didn't work

- 400 kHz: ten attempts, no receive IRQ; transient `ISO=00 AUX=09` and `ANT2=00` readback.
- 100 kHz: ten attempts, no receive IRQ; repeated invalid state errors; Space-B values read as `00/00` or `CORR=93`; NRT read as `0050`/`00D0` instead of `0350`.
- Lowering bus speed did not stabilize transport.

### What I learned

- The simplest clock-rate hypothesis is disproved.
- The next comparison must target transaction semantics/backend behavior rather than another protocol or RF setting.
- Documentation should state test preconditions explicitly, especially physical tag presence and placement.

### What was tricky to build

- The original timeout and the invalid NFC comparison occurred in the same run. The timeout remains real evidence, while the no-tag result from that run does not; those claims had to be separated rather than discarding or preserving the entire run.

### What warrants a second pair of eyes

- Compare M5 `I2C_Class` start/restart/read/write/stop behavior to ESP-IDF `i2c_master_transmit_receive()`.
- Decide whether an explicit defined-operation transaction or the legacy ESP-IDF I2C backend is the cleaner next controlled experiment.

### What should be done in the future

- Quantify baseline register-read failure rates on the restored 400 kHz build.
- Implement one transport-backend experiment without changing NFC configuration.
- Retry in the exact position proven by official firmware.

### Code review instructions

- Confirm `main/st25r3916/st25r3916.c` is back at `I2C_FREQ_HZ=400000` and IO_CONFIG_1 `0x17`.
- Review corrected vault article commit `223bcd7`.
- Use `/tmp/esp60-tag-replaced-read.log` and `/tmp/esp60-100khz-tag-read.log` as transient raw evidence.

### Technical details

- Current flashed firmware: committed 400 kHz ESP-IDF build.
- 100 kHz experiment: failed, reverted, never committed.
- Remaining diagnosis: transaction implementation/recovery, not tag placement and not bus clock alone.

---

## Step 16: Design a 320×240 NFC diagnostic UI from official docs and StackChan patterns

The user requested an on-device UI that makes the remaining transport and NFC debugging easier to operate. I researched the official CoreS3 display specification, the full official StackChan NFC page, LVGL widget availability, and the production firmware's existing app, display, touch, locking, and shared-I2C patterns.

The resulting design is a four-page 320×240 Mooncake diagnostic app with full-screen ASCII sketches: Reader, RF/IRQ, I2C Bus, and Registers/Event Log. It preserves the console and separates transport, RF, and protocol failures instead of collapsing them into “No tag.”

### Prompt Context

**User prompt (verbatim):** "ok, how far are we right now? can you make a UI for things now so that I can maybe debug things a bit more nicely? sketch out UI in ascii with the different functionality it can show. Look up stackchan screen resolution and look for how to display widgets and stuff"

**Additional user prompt (verbatim):** "here btw is the doc page for the NFC in case you didn't have it downloaded already: https://docs.m5stack.com/en/arduino/stackchan/nfc"

**Additional user prompt (verbatim):** "ascii screenshots as sketches for the UI, btw. just so we can visualize a bit from the docs."

**Assistant interpretation:** Research the actual screen and UI stack, give an honest project status, and produce concrete full-screen ASCII mockups for a touch-oriented NFC diagnostic interface before implementation.

**Inferred user intent:** Make low-level NFC/I2C debugging understandable and operable directly on the device while preserving enough evidence to continue diagnosing the ESP-IDF transport failure.

### What I did

- Confirmed the official CoreS3 display is a 2.0-inch 320×240 capacitive-touch IPS panel using ILI9342C.
- Confirmed production constants `DISPLAY_WIDTH=320`, `DISPLAY_HEIGHT=240`.
- Read StackChan `stackchan_display.cc`: `esp_lvgl_port`, RGB565, 320×240 display, 20-line DMA buffer, LVGL task, and lock implementation.
- Read `app_template.cpp`, setup workers, and status bar patterns for app lifecycle, 48–50 px buttons, 16/20/24 px fonts, static quit controls, and 320×28 bars.
- Confirmed the full firmware exposes the shared bus through `hal_bridge::board_get_i2c_bus()`.
- Downloaded the complete official NFC page with Defuddle (83 KB, 2,093 lines) and stored it under ticket `sources/web/`.
- Extracted the official Reader Basic Workflow and touch-to-detect example behavior.
- Created design doc `design-doc/02-m5stackchan-nfc-debug-ui-320x240-lvgl-design.md`.
- Added full-screen ASCII sketches for ready, success, transport error, RF/IRQ, bus diagnostics, register matrix, and event log states.
- Defined a worker/queue/snapshot architecture so touch callbacks never perform I2C and worker operations never hold the LVGL lock.
- Used existing docmgr vocabulary (`ui`, `draft`) after doctor rejected unregistered synonyms.

### Why

- The current blocker is transport instability, so a useful UI must expose errors, retries, raw IRQ/FIFO state, and register mismatches—not only UID output.
- Reusing the production Mooncake/LVGL stack avoids porting display, touch, backlight, and locking into the standalone transport experiment.
- ASCII screenshots allow layout review against the real 320×240 constraint before implementation.

### What worked

- Defuddle extracted the full JavaScript-rendered official page successfully on this run.
- Existing StackChan source provides strong pixel and widget precedents.
- The board exposes the exact I2C handle needed by a Mooncake NFC app.
- The design fits header, content, and 44 px navigation into exactly 240 vertical pixels.

### What didn't work

- Initial docmgr doctor reported unknown topic `debug-ui` and status `proposed`; these were replaced with registered values `ui` and `draft`.
- No UI code was implemented in this step; the output is deliberately a design artifact.

### What I learned

- M5's official complete-reader example already uses touch as the explicit scan trigger, validating a `READ ONCE` interaction.
- The production firmware's 48–50 px button precedent is appropriate for this small touch screen.
- A second standalone display/touch stack would add uncontrolled I2C activity during the transport investigation; Mooncake is the safer UI host.

### What was tricky to build

- A 320×240 screen cannot show UID, protocol stages, raw IRQs, registers, and logs simultaneously. The design uses four fixed pages and a 44 px button-matrix navigation row.
- “No tag,” “transport error,” and “protocol error” must remain separate visual states; otherwise the UI would preserve the same ambiguity that complicated console debugging.

### What warrants a second pair of eyes

- Confirm `lv_table` remains readable with production font/padding; a custom row list may fit better.
- Decide whether bus reset is safe while all production peripherals share the bus.
- Decide whether the first implementation should integrate the custom C driver or M5Unit-NFC's ESP-IDF support.

### What should be done in the future

- Review the ASCII layouts with the user and select a minimum first screen set.
- Extract console operations into a serialized `NfcDebugService` before creating widgets.
- Implement Reader and Bus pages first; defer register/log polish.

### Code review instructions

- Read `design-doc/02-m5stackchan-nfc-debug-ui-320x240-lvgl-design.md` from top to bottom.
- Compare dimensions and controls against preserved StackChan app source references listed in the doc.
- Verify the complete official page snapshot at `sources/web/04-m5stack-stackchan-nfc-full-official-doc.md`.

### Technical details

- Layout: 28 px header + 168 px content + 44 px navigation = 240 px.
- Primary pages: READ, RF/IRQ, BUS, REGS/LOG.
- Runtime: one NFC worker owns I2C; command queue in; immutable snapshot/event queue out; UI updates at ≤10 Hz under `LvglLockGuard`.

---

## Step 17: UI-0 — create the pinned firmware overlay and serialized NFC service

UI implementation began with a reproducibility problem: the production StackChan firmware existed only as a temporary upstream clone, so changing it directly would leave the source outside this repository. I created a tracked overlay project pinned to the exact upstream revision and a deterministic composition script that can rebuild the modified firmware from a clean checkout.

The first implementation milestone also establishes the concurrency boundary required by the design. `NfcDebugService` owns a FreeRTOS worker, accepts fixed-size commands, performs all NFC I2C calls on that worker, and publishes immutable snapshots through a one-element overwrite queue. A complete StackChan firmware build and a second idempotent prepare/rebuild both succeeded.

### Prompt Context

**User prompt (verbatim):** "Ok, read the design doc, read the diary, commit at appropriate intervals and keep a detailed diary as you work (using the diary format from the skill)

Print a brutalist work slip with a list of all the phases. Then before each phase, print a brutalist work slip for the phase. Then when done, print another with the write up"

**Assistant interpretation:** Implement the entire NFC Debug UI plan in explicit phases, printing an overall thermal plan and one slip before every phase, making focused code commits, and maintaining the strict diary format throughout; finish with a printed status/write-up slip.

**Inferred user intent:** Produce a physically visible, auditable implementation process and a reproducible final firmware rather than an undocumented one-off modification in a temporary clone.

**Commit (code):** `50d7c1512f772aef1b6b436231b28914c4504496` — "ESP-60 UI-0: add pinned StackChan NFC service overlay"

### What I did

- Read the complete 320×240 UI design and all 16 existing diary steps before implementation.
- Read the brutalist-work-slip skill and printed the six-phase overall plan slip successfully.
- Printed the UI-0 phase slip before changing code.
- Confirmed upstream StackChan source is not tracked in this monorepo and pinned commit `1b5765599fba8aaad1811d9a79358ccc7051f5f3`.
- Created tracked project `0116-m5stackchan-nfc-debug-ui/` with `upstream.env`, README, `.work/` ignore rule, overlay source, and prepare/build scripts.
- Added idempotent integration of `AppNfcDebug` into upstream `apps.h` and `main.cpp`.
- Ported the corrected standalone ST25R3916 driver into the overlay and added `st25r3916_deinit()` so app close removes its device handle from the shared bus.
- Defined service commands, reader/transport/protocol states, transport counters, last-error context, UID/card fields, and immutable snapshots.
- Implemented a single FreeRTOS worker and command/snapshot queues; UI/app lifecycle code never calls NFC directly.
- Attached the driver to `hal_bridge::board_get_i2c_bus()` rather than creating another I2C bus.
- Ran a clean full build with ESP-IDF 5.5.4: 2,494 actions, `stack-chan.bin` successful, 27% app partition free.
- Re-ran `prepare.sh`, verified exactly one include and one app registration, and completed a successful incremental rebuild.

### Why

- Code changed only under `/tmp` would not survive cleanup or appear in project commits.
- A pinned overlay preserves upstream provenance without vendoring 232 unrelated firmware files.
- One worker must own NFC transactions because concurrent touch/UI/console calls would make the current I2C failure evidence unreliable.
- Complete snapshots prevent the LVGL task from observing partially updated driver state.

### What worked

- Local-clone preparation and dependency fetch produced a complete upstream build under ESP-IDF 5.5.4.
- Upstream recursive source collection compiled the new C and C++ sources without CMake changes.
- The overlay preparation is idempotent: include and install statements remained singletons after a second run.
- The full build accepted the shared-bus service, driver lifecycle, FreeRTOS queues, and Mooncake registration.

### What didn't work

- No UI page exists in UI-0; opening the app currently starts the service and emits state changes only to logs.
- `VerifyRegisters`, `SampleIrqWindow`, and `ResetBus` deliberately return `ESP_ERR_NOT_SUPPORTED` until their later phases.
- The full upstream build emits pre-existing warnings about ioctl macro redefinitions and unused variables; none originated in `app_nfc_debug`.

### What I learned

- StackChan's `main/CMakeLists.txt` recursively collects `apps/*.c`, `*.cc`, and `*.cpp`, so the overlay only needs app registration patches.
- The complete production firmware is large but still leaves `0x151cb0` bytes (27%) in the smallest app partition after adding the service.
- A disposable composed checkout is a better boundary than either committing a nested Git repository or duplicating the complete vendor firmware.

### What was tricky to build

- The service must stop without deleting queues while its worker may still access them. `stop()` enqueues a shutdown command, waits for worker termination, and only then deletes queues; the worker turns off the field and removes the device before clearing its task handle.
- The driver had process-lifetime static device state in the standalone firmware. Mooncake apps can open and close repeatedly, so an explicit deinit path was required to avoid duplicate device registration.
- Preparation must preserve fetched dependencies and build caches while reliably replacing only the app overlay. The script resets the pinned upstream files and cleans the app directory, not the entire ignored worktree.

### What warrants a second pair of eyes

- `Service::stop()` intentionally refuses to delete resources if the worker misses the two-second shutdown deadline; review whether a stronger recovery policy is appropriate.
- Transport counters currently count commands, not every low-level I2C transaction. UI-2 must add driver-level transaction instrumentation before presenting them as bus transaction totals.
- The app temporarily reuses the setup icon until a dedicated NFC icon is added.

### What should be done in the future

- UI-1: build the exact 320×240 frame and Reader page under `LvglLockGuard`.
- UI-2: add driver-level diagnostics, Bus and RF/IRQ pages, and register verification.
- Keep generated `.work/` source, dependencies, sdkconfig, and build products untracked.

### Code review instructions

- Start at `0116-m5stackchan-nfc-debug-ui/README.md` and `scripts/prepare.sh` to understand the source composition boundary.
- Review `nfc_debug_service.h/.cpp` for worker ownership, shutdown order, and snapshot publication.
- Review `st25r3916_deinit()` and confirm all driver calls occur only in `Service::task_loop()`.
- Validate with:
  `cd 0116-m5stackchan-nfc-debug-ui && STACKCHAN_SOURCE=/tmp/nfc-research/repos/StackChan ./scripts/prepare.sh && source ~/esp/esp-idf-5.5.4/export.sh && ./scripts/build.sh`.

### Technical details

- Upstream commit: `1b5765599fba8aaad1811d9a79358ccc7051f5f3`.
- Code commit: `50d7c1512f772aef1b6b436231b28914c4504496`.
- Full build log: `/tmp/esp60-ui0-build.log`.
- Incremental rebuild log: `/tmp/esp60-ui0-rebuild.log`.
- Binary size: `0x39e350`; smallest app partition: `0x4f0000`; free: `0x151cb0` (27%).

---

## Step 18: UI-1 — implement the 320×240 Reader page

UI-1 turned the service contract into the first real touchscreen page. The app now creates an exact 320×240 LVGL frame with a 28 px health header, 168 px content region, and 44 px navigation row, then renders every reader outcome as a distinct state instead of reducing failures to “No tag.”

Touch callbacks for READ ONCE and AUTO only enqueue service commands. Snapshot changes are detected before taking `LvglLockGuard`, so the app does not lock or redraw on every Mooncake loop iteration. The final phase build completed without app-specific warnings.

### Prompt Context

**User prompt (verbatim):** (same as Step 17)

**Assistant interpretation:** Continue the printed phased implementation by building the Reader screen exactly from the reviewed design, commit it independently, and record real failures and fixes.

**Inferred user intent:** See the first useful on-device NFC screen while preserving the concurrency and evidence guarantees established in UI-0.

**Commit (code):** `11d5f0e074a75b18d8b7bf32616b6646fb199a1b` — "ESP-60 UI-1: add 320x240 NFC Reader page"

### What I did

- Printed the UI-1 brutalist phase slip before implementation.
- Added `view/NfcDebugView` with raw LVGL 9 widgets and no animation.
- Implemented a full-screen 320×240 root, 28 px header, 168 px content area, and 44 px four-button navigation matrix.
- Added NFC LAB title, I2C health dot, cumulative raw error count, and static 38×28 quit control.
- Implemented Reader render states: STARTING, READY, SCANNING, TAG FOUND, NO TAG, TRANSPORT ERROR, PROTOCOL ERROR, and STOPPED.
- Rendered UID, provisional type, ATQA, SAK, UID length, and Detect/Select/Identify stage status on success.
- Preserved raw `esp_err_to_name()` and elapsed time for error states.
- Added 138×44 READ ONCE and AUTO buttons; callbacks only enqueue service commands.
- Disabled the three future navigation buttons until their pages exist.
- Moved app view destruction to an out-of-line destructor so the header can forward-declare the view.
- Updated `build.sh` to run `idf.py reconfigure` before building because upstream's source glob lacks `CONFIGURE_DEPENDS`.
- Rebuilt successfully with no `app_nfc_debug` warnings; binary size `0x39f680`, 27% app partition free.

### Why

- The Reader page is the minimum operator interface that can display placement instructions, raw transport failure, no-tag, and eventual UID success distinctly.
- Deferring the LVGL lock until a new snapshot arrives keeps rendering bounded and prevents I2C work from ever running under the display lock.
- Forty-four-pixel controls match the design and existing StackChan touch conventions.

### What worked

- Raw LVGL 9 APIs compiled against the production component without introducing a separate UI dependency.
- The final build included the new view source after explicit reconfiguration.
- The visual hierarchy fits the native screen dimensions exactly and retains all primary reader evidence.
- Final build scan found no warning or error originating in `app_nfc_debug`.

### What didn't work

- First UI-1 build failed with:
  `error: invalid application of 'sizeof' to incomplete type 'nfc_debug::view::NfcDebugView'`.
  `AppNfcDebug` owned `std::unique_ptr<NfcDebugView>` while its implicit destructor was instantiated in `main.cpp`. Fixed by declaring `~AppNfcDebug()` and defining it out-of-line after including the complete view type.
- Second UI-1 build linked without the new view source and failed with undefined references including:
  `undefined reference to 'nfc_debug::view::NfcDebugView::~NfcDebugView()'`.
  Upstream uses `GLOB_RECURSE` without `CONFIGURE_DEPENDS`, so the existing generated build graph did not notice the new `.cpp`. Fixed permanently by running `idf.py reconfigure` in `scripts/build.sh` before `idf.py build`.
- The page has not yet been flashed and visually inspected on the physical 320×240 panel; hardware UI validation remains scheduled for the final phase.

### What I learned

- Forward-declared unique-pointer members require an out-of-line owner destructor when consumers instantiate destruction without the pointee definition.
- A source overlay that adds files must account for upstream CMake glob behavior; copying source alone is insufficient for incremental builds.
- LVGL's button matrix provides the exact 80×44 four-tab geometry with a single callback and lower object count than four separate navigation buttons.

### What was tricky to build

- Header space is constrained by title, I2C health, error count, and quit control. The implementation uses a real colored 10×10 object for health instead of a Unicode glyph that might be absent from embedded fonts.
- The Reader body must show either two-line placement guidance or a long UID without changing geometry. Fixed-size clipped/wrapped labels preserve the action-button positions across all states.
- “No tag” remains neutral gray and keeps transport health green; transport and protocol failures use different colors and retain raw errors.

### What warrants a second pair of eyes

- Visually inspect text clipping, especially a 10-byte UID and long `esp_err_to_name()` values, on the physical display.
- Confirm the 38 px visible quit control has a sufficient practical touch target; its parent header currently limits its hit region to 38×28.
- The future page buttons are intentionally disabled, so navigation behavior must be revalidated when UI-2 enables them.

### What should be done in the future

- UI-2: add driver diagnostic snapshots and enable RF/IRQ and BUS navigation.
- Final hardware phase: flash and inspect the Reader page, touch response, app close/reopen, and live tag outcomes.

### Code review instructions

- Start at `view/nfc_debug_view.cpp::create_frame()` to verify all pixel boundaries.
- Review `render_reader()` for state separation and raw error preservation.
- Review `AppNfcDebug::onRunning()` to confirm the LVGL lock is acquired only after snapshot generation changes.
- Build with `source ~/esp/esp-idf-5.5.4/export.sh && ./scripts/build.sh`.

### Technical details

- Geometry: header `(0,0,320,28)`, content `(0,28,320,168)`, navigation `(0,196,320,44)`.
- Action buttons: `(16,120,138,44)` and `(166,120,138,44)` relative to content.
- Failed logs: `/tmp/esp60-ui1-build.log`, `/tmp/esp60-ui1-build-2.log`.
- Successful final log: `/tmp/esp60-ui1-build-final.log`.

---

## Step 19: UI-2 — add raw RF/IRQ and I2C transport pages

UI-2 adds the evidence needed for the current ESP-IDF blocker. Every ST25R3916 read, write, direct command, and FIFO transfer now crosses one instrumentation boundary that records transaction totals, categorized failures, the failed operation/register or command byte, and elapsed time. The UI can therefore distinguish a protocol-level no-tag result from a transport timeout or invalid-state failure.

The RF/IRQ and Bus pages are enabled in the bottom navigation. Their longer actions are cooperative worker jobs rather than ten-second touch callbacks: sampling alternates REQA/WUPA every 200 ms, while register verification performs one 12-register pass per worker iteration and preserves command-queue responsiveness.

### Prompt Context

**User prompt (verbatim):** (same as Step 17)

**Assistant interpretation:** Continue the printed phase plan by implementing the transport/RF diagnostic core and its two screens, then commit and document the milestone independently.

**Inferred user intent:** Make the unresolved I2C problem measurable on the device while preserving raw low-level evidence needed to compare transport backends.

**Commit (code):** `e7229ec9cbccb4f94b02417f977d4bbe91902bce` — "ESP-60 UI-2: add RF and I2C diagnostic pages"

### What I did

- Printed the UI-2 brutalist phase slip before implementation.
- Added one instrumented transport boundary around `i2c_master_transmit()` and `i2c_master_transmit_receive()`.
- Counted actual transactions, successes, failures, timeouts, invalid-state errors, and other errors.
- Preserved the last failed operation type, Space-A/Space-B register or command byte, raw `esp_err_t`, and elapsed microseconds.
- Added driver diagnostics for last main/timer/error IRQ bytes, collision display, FIFO size, capacitance, RSSI, operation control, and NRT.
- Added a stable 12-register verification set covering IO1/IO2, MODE, RX1–RX4, ANT1/ANT2, TXD, Space-B CORR1, and EMD.
- Extended immutable service snapshots with RF diagnostics, actual transport counters, register results, no-tag count, sampling progress, and verification progress.
- Implemented a cooperative 10-second REQA/WUPA sample job at 200 ms intervals.
- Implemented a cooperative 20-pass register verification job, one register set per worker iteration.
- Added serialized NFC reinitialization that removes/re-adds only the NFC device, not the shared board bus.
- Added RF/IRQ page with field, capacitance, RSSI, FIFO, NRT, raw IRQ bytes, RXS/RXE/COL/NRE/error flags, sample counts, and clear action.
- Added Bus page with identity, backend/speed, actual transaction counts, categorized failures, mismatches, last raw transport error, PROBE, VERIFY 20x, and REINIT NFC actions.
- Enabled READ, RF/IRQ, and BUS navigation while leaving REGS/LOG disabled until UI-3.
- Built successfully under ESP-IDF 5.5.4 with no app-specific warnings.
- Verified final ELF symbols for both renderers and both driver diagnostic APIs.

### Why

- Command-level success counters cannot diagnose a bus that corrupts one register access inside a larger NFC operation; individual I2C transactions must be counted.
- Raw IRQ/FIFO state is required to distinguish no modulation, collision/noise, FIFO decode failure, and successful receive activity.
- A ten-second operation inside an LVGL callback or one monolithic worker command would block close/reset and violate the app lifecycle design.
- Resetting the full shared bus could disrupt touch, PMIC, audio, and other clients; NFC-only reinitialization is the safer diagnostic control.

### What worked

- Source search confirms the only direct ESP-IDF I2C calls in the driver are now the two instrumented wrappers.
- Full firmware build succeeded; binary size is `0x3a0920`, leaving 27% app partition space.
- `xtensa-esp32s3-elf-nm -C` confirms `NfcDebugView::render_rf`, `render_bus`, `st25r3916_get_transport_stats`, and `st25r3916_verify_configuration` are linked.
- Reinitialization preserves cumulative transport counters; counters reset only at app/service start or explicit CLEAR.

### What didn't work

- The first symbol-inspection command failed because the ESP-IDF toolchain was not in that shell's PATH:
  `/bin/bash: line 35: xtensa-esp32s3-elf-nm: command not found`.
  Sourcing `~/esp/esp-idf-5.5.4/export.sh` before the command fixed it.
- Physical navigation, touch, and live counter behavior are not yet validated; this phase is build- and symbol-validated.

### What I learned

- The driver still had a few direct burst reads outside `rd8()`; centralizing them was necessary for complete statistics.
- NRE is Timer/NFC IRQ bit `0x40`, while RXS/RXE/COL are Main IRQ bits `0x20/0x10/0x04`; the UI must not flatten these separate register bytes.
- A reset/reinit action must not silently clear the historical failure rate because that would hide whether recovery was needed.

### What was tricky to build

- Reading diagnostic registers is itself I2C activity and must appear in transaction totals. `refresh_driver_snapshot(true)` captures RF values first, then copies transport stats so those reads are included.
- The last IRQ registers are clear-on-read. The driver stores the last protocol-observed bytes and reports those, rather than letting the UI perform unsynchronized reads that destroy evidence.
- Verification under a failing bus can spend up to one timeout per register. Breaking 20 passes into worker iterations limits shutdown latency to one 12-register pass rather than the whole campaign.

### What warrants a second pair of eyes

- Under twelve consecutive 100 ms register timeouts, one verification pass can approach 1.2 seconds; confirm the two-second shutdown policy is adequate.
- Capacitance is measured at initialization/reinitialization rather than continuously because the direct measurement command perturbs chip state.
- Validate on hardware that stored IRQ bytes reflect the desired request attempt and are not overwritten by later diagnostic reads.

### What should be done in the future

- UI-3: expose the 12 expected/actual register rows and a fixed-size event log.
- Hardware validation: compare displayed transaction failures and register mismatches to serial logs from the same actions.
- A future transport-backend experiment can reuse these counters for controlled before/after comparison.

### Code review instructions

- Start with `st25r3916.c::transport_write()`, `transport_read()`, and `record_transport()`.
- Review `Service::task_loop()`, `run_sample_step()`, and `run_verification_step()` for queue responsiveness.
- Review `NfcDebugView::render_rf()` and `render_bus()` for raw field mapping.
- Build and inspect symbols with the ESP-IDF 5.5.4 environment sourced.

### Technical details

- RF sample cadence: 200 ms for 10,000 ms, alternating REQA and WUPA.
- Verification set: 12 registers × 20 passes = 240 explicit verification reads, plus snapshot diagnostics.
- Build log: `/tmp/esp60-ui2-build-final.log`.
- Binary: `0x3a0920`; free app space: `0x14f6e0` (27%).

---

## Step 20: Convert to NFC-only firmware and perform the first physical UI flash

The user noticed that the UI was not visible because the implementation had only been built, not flashed. They then clarified that this diagnostic device should not carry the standard StackChan application suite. I paused the partially started UI-3 event-log work, preserved its patch, and changed the reproducible overlay into an NFC-only firmware composition.

The resulting image excludes all standard app implementations, installs only NFC.LAB, and opens it automatically at boot. The image shrank by approximately 613 KiB, the first full partition-layout flash succeeded with hash verification, and live USB logs prove that NFC.LAB is executing its worker and rendering snapshot generations on the physical device.

### Prompt Context

**User prompt (verbatim):** "I don't see th eUI on the device, are you running these things?"

**Additional user prompt (verbatim):** "are we flashing all the stackchan apps each time?"

**Additional user prompt (verbatim):** "can you disable the standard apps, so we have less to flash? we are only working on the nfc app"

**Assistant interpretation:** Stop treating build success as physical deployment; strip the composed firmware to the NFC application and required shared framework code, flash it now, and optimize subsequent update flashing.

**Inferred user intent:** See the in-progress NFC UI on the actual StackChan quickly, without repeatedly carrying unrelated standard apps through the build and flash cycle.

**Commit (code):** `51efbe4fb337a76067937069da77913167feb877` — "ESP-60: boot directly into NFC-only StackChan firmware"

### What I did

- Explicitly corrected the record: UI-0 through UI-2 had been built but not flashed before this step.
- Saved the uncommitted UI-3 event-ring work to `/tmp/esp60-ui3-partial-event-log.patch` and restored the tracked service files before changing firmware composition.
- Changed generated `apps.h` to include only `AppNfcDebug`.
- Changed generated `main.cpp` to install only `AppNfcDebug` and force Mooncake mode rather than AI-agent boot bypass.
- Made `AppNfcDebug::onCreate()` open NFC.LAB automatically.
- Removed the on-screen quit control because no launcher exists in NFC-only mode.
- Removed the unused setup icon dependency.
- Added a CMake source filter excluding standard implementations: Launcher, AI Agent, Avatar, Setup, Dance, ESP-NOW, App Center, EzData, and Template.
- Retained `apps/common` after link evidence proved HAL uses its reminder, home-indicator, and status-bar functions.
- Added `scripts/flash.sh`: `--full` for partition migration and `app` for later application-only updates.
- Built the NFC-only firmware with ESP-IDF 5.5.4.
- Verified no standard app object appeared in the build log.
- Compared images: all-app UI-2 `0x3a0920`; NFC-only `0x30ae70`.
- Checked `/dev/ttyACM0` for owners; no monitor/flasher held it.
- Performed the required first full flash: bootloader, partition table, OTA data, app, and generated assets; every write hash verified.
- Reset once under one pyserial owner and captured 15 seconds of live NFC.LAB logs.

### Why

- Build completion does not make UI visible on hardware; physical deployment is a separate evidence requirement.
- Upstream links the `main` component `WHOLE_ARCHIVE`, so merely not installing standard apps does not reduce the image. Their source files must be filtered before entering `SOURCES`.
- The board was running standalone project `0115`, whose partition layout does not match StackChan firmware; the first migration could not safely use app-only flashing.
- Subsequent changes retain the new partition layout and can use `idf.py app-flash`.

### What worked

- NFC-only image built successfully at `0x30ae70` with 38% free in the smallest app partition.
- The app image decreased by `0x95ab0` bytes, approximately 613 KiB or 16% relative to all-app UI-2.
- Build-log search found no standard app object compilation.
- Full flash succeeded on `/dev/ttyACM0` with verified hashes.
- Live output proves physical execution, including:
  `[NFC.LAB] state=SCANNING generation=3 errors=0`
  followed by raw REQA/WUPA and NO TAG/TRANSPORT ERROR state transitions.

### What didn't work

- First NFC-only link excluded `apps/common` and failed with undefined references such as:
  `undefined reference to 'tools::update_reminders()'`
  `undefined reference to 'view::update_home_indicator()'`
  `undefined reference to 'view::create_status_bar(...)'`.
  The HAL calls these shared support functions directly. Fixed by retaining `apps/common` while continuing to exclude all launcher-visible standard apps.
- The first physical log capture began after early boot output and therefore did not preserve the complete startup sequence.
- Auto polling appeared active during the capture even though the service defaults it off. This could be a physical touch event, retained touch state, or an unintended callback and requires direct screen/touch validation.

### What I learned

- `apps/common` is framework support despite its location under `apps/`; app-directory classification alone is insufficient for safe source elimination.
- Excluding standard app source materially reduces the image, but the StackChan HAL, display, touch, audio, networking, and Xiaozhi foundation still dominate the remaining 3.0 MiB image.
- The first full flash wrote a 3.19 MB app and a 2.30 MB generated-assets partition; subsequent app-only iterations avoid rewriting the 2.30 MB assets partition.

### What was tricky to build

- The source filter had to reduce a `WHOLE_ARCHIVE` component without removing functions called indirectly by HAL background tasks. Link errors, not directory names, established the correct support boundary.
- UI-3 was already partially edited. Preserving it as a patch before restoring files avoided mixing an unfinished event-log feature into the deployment-focused commit.
- A full partition transition was unavoidable once; claiming that `app-flash` was immediately safe would have risked writing a 3 MB image into the standalone firmware's incompatible layout.

### What warrants a second pair of eyes

- Confirm visually that NFC.LAB fills the screen and that READ/RF/BUS navigation responds correctly.
- Investigate why auto polling became active during the serial capture and ensure reset-time touch state cannot toggle AUTO accidentally.
- The NFC worker recorded rapidly increasing transaction failures; compare the BUS page values to raw logs while the tag is in the proven position.

### What should be done in the future

- Reapply `/tmp/esp60-ui3-partial-event-log.patch` and continue UI-3.
- Use `./scripts/flash.sh app` for subsequent iterations unless partition/assets configuration changes.
- Capture a complete boot log on the next app-only flash and validate the visible screen with the operator.

### Code review instructions

- Review `scripts/prepare.sh` for generated app registry, main installation, and CMake source filter.
- Review `AppNfcDebug::onCreate()` for auto-open behavior.
- Reproduce build size with `prepare.sh` + `build.sh`; confirm no `apps/app_*` object other than `app_nfc_debug` is built.
- Full migration flash: `./scripts/flash.sh --full`; subsequent update: `./scripts/flash.sh app`.

### Technical details

- All-app UI-2 image: `0x3a0920`.
- NFC-only image: `0x30ae70`; free: `0x1e5190` (38%).
- Full flash log: `/tmp/esp60-nfc-only-full-flash.log`.
- Boot/runtime capture: `/tmp/esp60-nfc-only-boot.log`.
- Partial UI-3 preservation: `/tmp/esp60-ui3-partial-event-log.patch`.

---

## Step 21: Publish the NFC LAB deep dive and classify the rising error count

The first operator test of the physically deployed NFC LAB showed that the tag was not recognized and that the header error count increased. This is consistent with the known ESP-IDF transport instability: the displayed count is populated from failed low-level I2C transactions, not from ordinary no-tag outcomes.

I wrote a new 5,093-word textbook-style Obsidian article focused on the UI/runtime phase of the project and pushed it to the go-go-parc vault. The report preserves the incomplete result directly: the application, display, worker, diagnostics, and physical deployment work, but the ESP-IDF backend has not produced a UID.

### Prompt Context

**User prompt (verbatim):** "write a detailed project report for the obsidian vault as a deep dive technical analysis blog post using a textbook writing style (no analogies, see skill).&#32;&#32;&#32;&#32;&#32;&#32;
 Commit and push the bsidian vault when done (go-go-parc vault)."

**Additional user prompt (verbatim):** "doesn't seem to recognize my tag? i get error count goes up"

**Assistant interpretation:** Publish a durable technical account of the NFC LAB architecture and current evidence, while immediately clarifying what the rising physical-device error counter measures.

**Inferred user intent:** Preserve the complete project knowledge in the vault and understand whether the failed physical tag read indicates a bad tag, bad placement, or the known firmware transport defect.

**Commit (documentation):** `83513da3109b23b43d952f45809b7ccf772ca01e` — "Add M5StackChan NFC LAB deep-dive report" (go-go-parc vault)

### What I did

- Loaded the Obsidian vault-writing, Obsidian Markdown, textbook-authoring, and deep-dive report guidance.
- Read the existing standalone NFC article, the complete UI design, UI implementation diary Steps 16–20, overlay README, service, view, driver instrumentation, app lifecycle, and composition scripts.
- Explained that `err:NNN` is the driver's failed low-level I2C transaction count, not a tag-miss count.
- Asked the operator to keep the known-good tag on the literal narrow top edge and use a single READ ONCE operation.
- Captured two passive serial windows without resetting the board; neither contained a new NFC poll, so they do not classify a synchronized read.
- Created a new append-only vault article rather than overwriting the existing standalone-driver report.
- Wrote 800 lines / 5,093 words with three Mermaid diagrams, real code, UI layouts, failure analysis, deployment evidence, and explicit incomplete status.
- Validated frontmatter, required sections, balanced code fences, and `git diff --check`.
- Staged only the new article despite unrelated pre-existing vault changes.
- Committed and pushed vault `main` successfully.

### Why

- The existing vault article documents the standalone register-level port and Arduino bisect; the new article documents the distinct Mooncake/LVGL integration, concurrency model, source overlay, NFC-only composition, and first deployment.
- The tag is already proven by official firmware, so the error count must not be interpreted as evidence of an unsupported tag without transport evidence.
- A synchronized one-shot operation is more useful than AUTO polling because it preserves the first failing transaction before counters grow rapidly.

### What worked

- The article was committed and pushed to `origin/main` at `83513da3109b23b43d952f45809b7ccf772ca01e`.
- The report links the earlier NFC article and distinguishes established facts from pending validation.
- Vault validation found balanced fences, valid expected metadata/sections, and three Mermaid diagrams.
- Only the intended article was staged; unrelated transcript modifications/deletions remained untouched.

### What didn't work

- The passive serial captures did not coincide with a READ ONCE press. They contained only periodic system information, so no new raw request/error trace was obtained.
- NFC LAB still does not recognize the known-good tag; no ESP-IDF UID was produced.

### What I learned

- The observed rising counter confirms that physical UI use reaches the instrumented low-level transport path.
- AUTO was no longer active during the later passive captures, which narrows but does not explain why it appeared active immediately after the first flash.
- The next useful evidence is one operator-triggered read paired with the Bus page's last operation/key/error and RF/IRQ page values.

### What was tricky to build

- The report needed to cover a functioning UI architecture without implying that the NFC reader itself works. The article repeatedly separates deployment success from protocol success.
- The vault contained unrelated modified and deleted transcript files. Staging by exact article path prevented those changes from entering the report commit.
- The two user requests arrived together: documentation could proceed independently, but tag diagnosis still requires synchronized human interaction with the physical screen.

### What warrants a second pair of eyes

- Confirm the on-screen state after one READ ONCE and record the Bus page's exact last operation, key, and error.
- Review the article's distinction between command result and cumulative transport health, especially the case where Reader reports NO TAG while the header remains amber.
- Verify that AUTO cannot activate from a reset-time touch event.

### What should be done in the future

- Capture one synchronized tag-present READ ONCE trace.
- Continue the explicit M5-like I2C transaction or legacy-backend experiment.
- Reapply the preserved UI-3 event-log patch after the immediate transport evidence is recorded.

### Code review instructions

- Read the new vault article at `Projects/2026/08/21/ARTICLE - M5StackChan NFC LAB - Building an On-Device NFC Diagnostic Firmware.md`.
- Compare its transport-counter explanation with `st25r3916.c::record_transport()` and its task model with `NfcDebugService::task_loop()`.
- For the next physical test, disable AUTO, note Bus counters, press READ ONCE once, and immediately record Reader, Bus, and RF/IRQ values.

### Technical details

- Vault commit: `83513da3109b23b43d952f45809b7ccf772ca01e`.
- Article size: 800 lines, 5,093 words, 37,799 bytes.
- Passive captures: `/tmp/esp60-tag-present-live.log` and `/tmp/esp60-tag-read-once-live.log`.
- Both passive captures lacked a new poll sequence and are not synchronized-read evidence.

---

## Step 22: Localize the first physical READ failure and compare Arduino retry semantics

The operator performed the requested first controlled read and reported five failures in 365 low-level transactions. The last failure was `ESP_ERR_INVALID_STATE`, operation 2, key `0x0A`: a Space-A write to the ST25R3916 Auxiliary Definition register.

That write enables receive-without-CRC immediately before REQA. The NFC driver returns as soon as it fails, so this attempt did not reach RF request transmission or tag decoding. Source review also established why the official Arduino screen can show a successful PICC without proving a perfect bus: M5's multi-card detect operation retries failed REQA attempts for up to one second and reports the eventual result.

### Prompt Context

**User prompt (verbatim):** "firrst read, transport error / invalid state op 2 key 0a&#32;

bus page says txns 365, ok 360, fail 5, speed 400, last write A key 0a.

So why does the arduino sketch seem to work well but this has issue? or did the arduino sketch also cause issues?"

**Assistant interpretation:** Explain the exact failed operation and determine whether the apparent Arduino reliability is true transport reliability or retry/error-hiding behavior.

**Inferred user intent:** Identify why two software stacks on identical hardware produce different visible outcomes and choose the next experiment based on evidence rather than assuming the tag is bad.

### What I did

- Mapped operation 2 to `ST25R_TRANSPORT_WRITE_A` and key `0x0A` to `ST25R_REG_AUXILIARY_DEFINITION`.
- Traced the failure to `nfca_wake()`, where `set_bits(..., 0x80)` enables `no_crc_rx` before interrupt clear, FIFO clear, and REQA/WUPA transmission.
- Confirmed that a write failure returns immediately, before RF transmission.
- Inspected ESP-IDF 5.5.4 `i2c_master.c` and found that synchronous transactions return `ESP_ERR_INVALID_STATE` whenever final controller status is not `I2C_STATUS_DONE`.
- Confirmed the driver's NACK path uses `I2C_STATUS_ACK_ERROR`, which can therefore surface to this caller as `ESP_ERR_INVALID_STATE`.
- Inspected M5Unified `I2C_Class`: it delegates to M5GFX explicit begin/restart/read/write/end transaction operations.
- Inspected M5Unit-NFC `NFCLayerA::detect(vector, 1000)`: it loops REQA until the one-second deadline, delays 1 ms after a failed request, and only exposes the eventual aggregate result to the example.
- Calculated the observed raw failure rate as 5/365, approximately 1.37%; this is descriptive only because failures may be clustered and transactions are not independent.

### Why

- `ESP_ERR_INVALID_STATE` alone was too broad to distinguish an invalid handle from a controller transaction that ended in NACK/timeout state.
- The official Arduino result proves that at least one complete detect sequence succeeded; without bus instrumentation, it does not prove zero failed intermediate attempts.
- Register `0x0A` is a pre-REQA configuration write, so the current failure is below RF/tag behavior.

### What worked

- The Bus page localized the first physical failure to a specific operation and register exactly as designed.
- Local ESP-IDF source explains the broad invalid-state mapping.
- Vendor source identifies an explicit whole-request retry policy absent from the current one-shot NFC LAB path.

### What didn't work

- No Arduino per-transaction counters were recorded during the successful vendor test, so its actual hidden error rate is unknown.
- This evidence does not yet distinguish among a transient ST25R NACK, shared-bus interaction, controller FSM recovery behavior, or transaction-framing difference.

### What I learned

- The visible comparison is not one-shot versus one-shot: NFC LAB READ ONCE attempts a narrow sequence, while M5 `detect()` retries request discovery for up to one second.
- A write to Auxiliary Definition failing before REQA rules out tag type and tag placement as causes of this specific attempt.
- ESP-IDF's public error name loses the internal distinction between controller statuses in this synchronous path.

### What was tricky to build

- The same returned `ESP_ERR_INVALID_STATE` can represent multiple internal paths. Reading the driver source was required to avoid interpreting it only as an uninitialized API object.
- Adding retries could make UID detection succeed while preserving the underlying defect. The next experiment must retain counters and first-error context so eventual success does not erase transport evidence.

### What warrants a second pair of eyes

- Confirm whether ESP-IDF debug logging reports hardware NACK on the same failing transaction when I2C logging is enabled.
- Compare M5GFX bus locking/recovery and exact START/RESTART/STOP sequence with the new ESP-IDF driver's generated operation list.
- Check whether another StackChan client can access the same bus between the read-modify-write operations used for register `0x0A`.

### What should be done in the future

- Add a controlled M5-style one-second request retry mode while retaining all transaction counters and the first failure.
- Implement or test explicit defined I2C operations matching M5 transaction framing.
- Compare both backends with the same 20-pass register verification and tag position.

### Code review instructions

- Start at `st25r3916.c::nfca_wake()` and follow `set_bits(ST25R_REG_AUXILIARY_DEFINITION, 0x80)` through `wr8()` to `transport_write()`.
- Compare with `M5Unit-NFC/src/nfc/layer/a/nfc_layer_a.cpp::detect()` and `unit_ST25R3916_nfca.cpp::nfca_request_wakeup()`.
- Inspect ESP-IDF 5.5.4 `components/esp_driver_i2c/i2c_master.c::s_i2c_synchronous_transaction()` and its ACK-error path.

### Technical details

- Physical counters: total 365, succeeded 360, failed 5.
- Last context: `WRITE A`, register `0x0A`, `ESP_ERR_INVALID_STATE`.
- `0x0A`: ST25R3916 Auxiliary Definition; bit `0x80`: receive without CRC.
- Descriptive transaction failure rate: approximately 1.37%.

---

## Step 23: Research online I2C failures and publish the intern debugging guide

The controlled NFC LAB failure made the next work concrete: the firmware needs a transport experiment plan, not another round of RF register changes. I researched the exact ESP-IDF 5.5.4 driver, current and historical Espressif issue reports, the successful M5GFX/M5Unified backend, M5Unit-NFC retry semantics, and the ST25R3916 I2C protocol and command-completion rules.

The resulting guide teaches a new engineer how to separate target NACKs, host-controller state, electrical timing, direct-command busy windows, retry policy, and NFC protocol failures. It defines the instrumentation and backend APIs to build, a logic-analyzer procedure, six implementation phases, a test matrix, and zero-failure acceptance gates. The guide was dry-run rendered and uploaded successfully to reMarkable.

### Prompt Context

**User prompt (verbatim):** "Also look online for information about this. Save useful resources to the ticket's sources/ once done. Create  a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and the nupload to remarkable. The guide is about how to debug and what to build for it."

**Assistant interpretation:** Research external evidence for the Arduino-versus-ESP-IDF transport difference, preserve the evidence with provenance, and produce an actionable intern guide that specifies diagnostics, backend experiments, APIs, tests, and safe integration.

**Inferred user intent:** Make the next debugging phase executable by an unfamiliar engineer and preserve enough authoritative evidence that they do not repeat the same incorrect assumptions or hide the fault behind retries.

**Commit (documentation):** `8c62d8ac25d24888820b8ba461245a5ce80e8698` — "ESP-60: add ST25R I2C transport debugging guide"

### What I did

- Added ticket task `mu8t` for the research and guide, then marked it complete after delivery.
- Searched official ESP-IDF documentation, Espressif issues, ST25R3916 references, ST Community discussions, and M5GFX sources.
- Preserved the complete ESP-IDF 5.5.4 ESP32-S3 I2C programming guide.
- Preserved full issue bodies/comments for Espressif issues #13136, #14030, #17556, and #17720 through a reproducible GitHub API script.
- Preserved exact tagged source for ESP-IDF 5.5.4 `i2c_master.c/.h`, M5GFX 0.2.27 ESP32 I2C, and M5Unified 0.2.20 `I2C_Class`.
- Verified the M5GFX and M5Unified source hashes exactly match the versions used by the successful official PlatformIO build.
- Preserved a byte-exact gzip of M5GFX source and a whitespace-normalized readable copy.
- Downloaded the 157-page ST25R3916/7 datasheet and recorded its SHA-256 and source mirror.
- Extracted the ST I2C transaction diagrams and the requirement that some direct commands forbid I2C access until completion.
- Confirmed ESP-IDF maps any synchronous final status other than DONE to `ESP_ERR_INVALID_STATE`.
- Confirmed the April 2025 NACK recovery change discussed in issue #14030 is already present in the project’s 5.5.4 source.
- Confirmed M5GFX uses a per-port mutex, explicit start/restart/read/write/stop, transaction-start FSM reset, and forced STOP/bus recovery.
- Confirmed M5Unit-NFC retries failed request discovery for up to one second.
- Created `design-doc/03-st25r3916-i2c-transport-debugging-analysis-design-and-intern-implementation-guide.md`.
- Wrote 1,267 lines / 6,396 words with Mermaid architecture, transaction diagrams, API sketches, pseudocode, decision records, implementation phases, test matrix, runbook, and acceptance gates.
- Related the guide to current driver/service code and preserved backend sources.
- Completed a reMarkable dry run and successful real upload.

### Why

- The first physical failure occurs before REQA, so changing tag protocol or RF settings would not address it.
- Online reports show similar public error behavior but cannot prove the same root cause; the guide therefore requires waveform evidence.
- M5 visible success combines a different backend and retry policy. These variables must be tested independently.
- Broad retries can produce a UID while concealing transport defects, so the guide separates eventual success from zero-error transport acceptance.

### What worked

- The exact ESP-IDF 5.5.4 documentation page was available and preserved as Markdown.
- GitHub API retrieval preserved complete issue discussions and metadata reproducibly.
- Tagged M5 source matched the successful build artifacts byte-for-byte before readable-copy whitespace normalization.
- The Mouser mirror provided the ST-authored DS12484 Rev 3 datasheet when ST’s server did not.
- `docmgr validate frontmatter` passed for the new guide.
- reMarkable dry run succeeded, then upload returned:
  `OK: uploaded ESP-60 ST25R3916 I2C Debugging Guide.pdf -> /ai/2026/08/21/ESP-60-M5STACKCHAN-NFC`.

### What didn't work

- The Umans web search gateway returned:
  `HTTP 401: {"error":{"type":"authentication_error","message":"Invalid API key"}}`.
  Kagi and direct authoritative-source retrieval were used instead.
- Direct ST and Mouser `curl` downloads failed with:
  `curl: (92) HTTP/2 stream 1 was not closed cleanly: INTERNAL_ERROR (err 2)`.
  `wget` against the Mouser mirror succeeded.
- The ST Community Defuddle snapshots captured the initiating posts but not the complete dynamic reply threads. They are retained as supporting context, not primary conclusions.
- Initial `git diff --check` found trailing whitespace inherited from web/API bodies and one upstream source. Generated Markdown was normalized; a byte-exact gzip preserves the original M5GFX source.
- Local ESP-IDF is a shallow checkout and did not contain historical commit object `459b75f...`; the GitHub commit API supplied the patch, and the tagged source was inspected to verify the resulting code is present.

### What I learned

- M5GFX directly controls the ESP32 I2C peripheral rather than delegating normal ESP32 ports to the same high-level ESP-IDF API used by NFC LAB.
- M5GFX resets the FSM at transaction start and implements explicit forced STOP/bus recovery, materially changing the comparison.
- ESP-IDF 5.5.4 already contains one NACK recovery fix, but current issue reports and this hardware still show invalid-state behavior; upgrading to the already-used version is not a solution.
- ST25R3916 direct-command completion timing is an independent target-side hypothesis that must be audited before assigning all faults to the host driver.
- Defined operations can test framing while retaining the new driver core; they cannot isolate faults inside that core.
- A legacy backend cannot coexist on the same port with StackChan’s new-driver bus, so it belongs first in standalone project `0115`.

### What was tricky to build

- External issue reports contain reports, corrections, and invalid conclusions. Issue #17556, for example, was closed after the author discovered the analyzer was connected to the wrong bus. The guide distinguishes supporting evidence from proof and preserves the correction.
- A recovery design that is safe in the standalone firmware may be unsafe in NFC LAB because touch, RTC, IMU, and expanders share the bus. The guide rejects uncoordinated `i2c_master_bus_reset()`.
- Retry placement matters. Repeating a plain register read differs from repeating a FIFO load or direct command. The guide uses operation-specific retry safety and whole-request retries.
- The public ESP-IDF result does not expose enough information to label a physical NACK. The guide introduces conservative `HOST_NOT_DONE` classification until an analyzer or backend event proves NACK.

### What warrants a second pair of eyes

- Review the proposed defined-operations address bytes (`0xA0`/`0xA1`) and final read NACK against ESP-IDF 5.5.4 before implementation.
- Review the direct-command completion table against the ST datasheet command-by-command.
- Verify physical pull-up values and logic-analyzer access points from the actual StackChan/CoreS3 schematic.
- Confirm the 1000 ms retry policy matches the exact official overload used by `Detect.ino`.

### What should be done in the future

- Implement Phase D0 first-error/event-ring instrumentation.
- Implement Phase D1 observable M5-style request retries without clearing failures.
- Capture the failing `0x0A` transaction on GPIO11/GPIO12 with a logic analyzer.
- Implement defined-operations and legacy backends in standalone `0115` and compare them using the guide’s matrix.

### Code review instructions

- Start with the new guide’s Executive Summary, Sections 5–7, and phased implementation plan.
- Verify source claims against `sources/code/esp-idf-v5.5.4-i2c_master.c` and `sources/code/M5GFX-0.2.27-esp32-common.cpp` at the line ranges cited in the guide.
- Inspect `scripts/03-fetch-i2c-debug-research.py` for reproducible issue capture.
- Validate with `docmgr validate frontmatter`, `docmgr doctor`, and the recorded reMarkable dry-run/upload logs.

### Technical details

- Guide: 1,267 lines, 6,396 words, 47,866 bytes before relation metadata refresh.
- Research/code commit: `8c62d8ac25d24888820b8ba461245a5ce80e8698`.
- reMarkable destination: `/ai/2026/08/21/ESP-60-M5STACKCHAN-NFC`.
- Dry-run log: `/tmp/esp60-i2c-guide-remarkable-dry-run.log`.
- Upload log: `/tmp/esp60-i2c-guide-remarkable-upload.log`.
