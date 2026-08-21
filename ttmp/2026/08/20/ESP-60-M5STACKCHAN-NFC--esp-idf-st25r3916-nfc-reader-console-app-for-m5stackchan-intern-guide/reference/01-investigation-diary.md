---
title: Investigation Diary
doc_type: reference
ticket: ESP-60-M5STACKCHAN-NFC
topics:
  - m5stackchan
  - nfc
  - st25r3916
  - esp32-s3
  - esp-idf
  - esp-console
  - intern-guide
status: active
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
