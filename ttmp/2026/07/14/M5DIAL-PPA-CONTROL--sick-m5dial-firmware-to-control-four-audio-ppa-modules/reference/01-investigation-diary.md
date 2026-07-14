---
Title: Investigation diary
Ticket: M5DIAL-PPA-CONTROL
Status: active
Topics:
    - m5dial
    - esp32-s3
    - firmware
    - udp
    - lvgl
    - wifi
    - audio
    - esp-idf
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: repo://ttmp/2026/07/14/M5DIAL-PPA-CONTROL--sick-m5dial-firmware-to-control-four-audio-ppa-modules/design-doc/01-m5dial-ppa-scene-controller-design-and-implementation-guide.md
      Note: Primary deliverable authored in Step 2
    - Path: repo://ttmp/2026/07/14/M5DIAL-PPA-CONTROL--sick-m5dial-firmware-to-control-four-audio-ppa-modules/sources/m5dial-ppa-prototype/src/main.cpp
      Note: Prototype analyzed in Step 1; protocol reconstruction source
    - Path: repo://ttmp/vocabulary.yaml
      Note: Added udp and audio topic slugs for this ticket
ExternalSources: []
Summary: 'Chronological diary of the M5DIAL-PPA-CONTROL design work: prototype analysis, repo exploration, design decisions, doc authoring, and reMarkable delivery.'
LastUpdated: 2026-07-14T11:09:17.476782828-04:00
WhatFor: Continuation-friendly record of how the PPA Dial design doc was produced and what remains open.
WhenToUse: Read when picking up implementation of 0103-m5dial-ppa-dial or when re-verifying design assumptions.
---


# Diary

## Goal

Capture the investigation and design work for the M5DIAL-PPA-CONTROL ticket: analyzing the Arduino prototype at `/tmp/m5dial-ppa`, mapping reusable ESP-IDF/LVGL patterns in this repo, and producing the design doc for the production-quality M5Dial PPA scene controller firmware.

## Step 1: Prototype analysis, repo exploration, and ticket creation

The ticket started from a working but rough Arduino/PlatformIO prototype ("PPA Dial") that switches scenes on Four Audio PPA amplifier modules via a reverse-engineered UDP protocol. The first step was to read the prototype completely, sweep the repo for conventions and reusable firmware components, and set up the ticket workspace with the vocabulary it needs.

### Prompt Context

**User prompt (verbatim):** "Create a new docmgr ticket to create a sick m5dial firmware to control PPA software, see @/tmp/m5dial-ppa/ . Create a detailed design doc (see skills) and upload to remarkable."

**Assistant interpretation:** Create a docmgr ticket, write a comprehensive design document for a production-quality M5Dial firmware based on the prototype in /tmp/m5dial-ppa, and deliver the docs to the reMarkable device. No firmware implementation in this task.

**Inferred user intent:** Turn the ad-hoc prototype into a properly planned, repo-conventional ESP-IDF project with a design doc good enough to implement from (and to read/annotate on the reMarkable).

### What I did

- Read `/tmp/m5dial-ppa/src/main.cpp` (406 lines), `platformio.ini`, and `ANLEITUNG.md` in full.
- Ran an Explore agent over the repo for: docmgr ticket conventions (`ttmp/YYYY/MM/DD/TICKET--slug/`), firmware project conventions (top-level numbered ESP-IDF dirs), prior PPA/M5Dial/UDP work, and reusable M5Dial/LVGL components.
- Asked the user two scoping questions; answers: target **ESP-IDF + LVGL** (not PlatformIO), scope emphasis on **polished LVGL UI** (OTA/diagnostics/captive-portal deferred).
- Created the ticket and docs:
  - `docmgr vocab add --category topics --slug udp ...` and `--slug audio ...` (`m5dial`, `lvgl` already existed)
  - `docmgr ticket create-ticket --ticket M5DIAL-PPA-CONTROL --title "Sick M5Dial firmware to control Four Audio PPA modules" --topics m5dial,esp32-s3,firmware,udp,lvgl,wifi,audio,esp-idf`
  - `docmgr doc add --doc-type design-doc --title "M5Dial PPA scene controller — design and implementation guide"` and `--doc-type reference --title "Investigation diary"`
- Preserved the prototype (excluding `.pio/`, `.DS_Store`) into `sources/m5dial-ppa-prototype/` with rsync, since `/tmp` is volatile.
- Read reuse-candidate headers to anchor the design: `0072-m5dial-timer-demo/main/{m5dial_board.h,lvgl_port_m5dial.h,input_events.h,idf_component.yml}` and `0095-m5dial-wifi-bench/main/wifi_app.h`.

### Why

- The design doc must be evidence-based: every protocol claim is line-anchored into the preserved prototype, and every reuse claim into an existing repo file.
- Vocabulary slugs had to exist before `docmgr doctor` would pass with the chosen topics.

### What worked

- Repo already contains proven building blocks for this exact hardware: 0072 board bring-up + LVGL port, 0073 MVC screen pattern, 0095 native esp_wifi STA/AP manager. The design reuses all three nearly verbatim.
- The prototype is small and self-contained enough that the full UDP protocol (ping 0x00 / recall 0x04, status kinds 0x01/0x09/0x41, busy sub-code 0x03) could be reconstructed into a proper wire-format reference.

### What didn't work

- `docmgr vocab add --slug m5dial` failed with `Error: slug 'm5dial' already exists in category 'topics'` — the exploration report claimed the slug was missing. Checked `ttmp/vocabulary.yaml` directly with grep; only `udp` and `audio` were actually missing. Lesson: verify vocab against the YAML before batch-adding.
- A `sed -n '...p' file; echo ===; ...` compound command failed in zsh (`(eval):1: == not found`) — `===` was glob-interpreted; used a quoted `'---SPLIT---'` separator instead.

### What I learned

- No firmware in this repo uses UDP yet — the `ppa_client` socket task will be the first; nearest patterns are lwIP usage in `0095-m5dial-wifi-bench/main/wifi_app.c` and the host-side UDP tool `0049-xiao-esp32c6-mled-node/tools/mled_ping.py`.
- M5Dial projects use ESP-IDF 5.4.1 (repo `.envrc`, 0072 pins `idf >=5.4.0`); the ESP-IDF 5.3.4 rule in memory is PaperS3-specific.
- The prototype's presets.json schema (`scenes.<name>.{uid_<hex>|ip_<addr>}.{id,sub,name}`) is a hard compatibility contract with the Mac app "PPA Group Control".

### What was tricky to build

- Deciding the discovery-reply semantics: the prototype accepts both status kinds 0x01 and 0x09 as "module present" (main.cpp:109) even though 0x09 is otherwise an error kind. This looks like a capture-derived quirk (modules answer pings with 0x09 in some states), so the design preserves it verbatim rather than "cleaning it up" — cleaning it up would break discovery for those states.

### What warrants a second pair of eyes

- The protocol reference (design doc §4) — it is inferred from one prototype, itself inferred from packet captures. A review against a fresh capture of the Mac app, or against a real module, would de-risk the unknown header bytes (1, 10, 11) and the uid byte order.
- The decision to recall scene actions concurrently (design doc §5.3) — the prototype sends serially; concurrent sends to *different* modules should be safe but is unverified on real hardware.

### What should be done in the future

- Implementation per the phased plan in the design doc (Phases 1–6), starting with the `0103-m5dial-ppa-dial` scaffold.
- Decide the end-user flashing story (prebuilt binary + esptool vs. ESP Web Tools) — the ESP-IDF rewrite breaks the ANLEITUNG "just click Upload in PlatformIO" flow.
- Capture a status-query exchange from the Mac app if "active scene" read-back is ever wanted.

### Code review instructions

- Start with the design doc: `design-doc/01-m5dial-ppa-scene-controller-design-and-implementation-guide.md`, sections 4 (protocol) and 5 (architecture).
- Cross-check protocol claims against `sources/m5dial-ppa-prototype/src/main.cpp` — the doc cites line numbers into that file (e.g. `buildHeader` :73–80, `pumpUdp` :96–123, `recallAction` :140–156).
- Validate ticket health: `docmgr doctor --ticket M5DIAL-PPA-CONTROL --stale-after 30`.

### Technical details

- PPA wire format quick recap (full tables in design doc §4): UDP :5001, 12-byte LE header `type(1) ver(1) status(2) uid(4) seq(2) comp(1) res(1)`; ping = type 0x00 / status 0x0006 / comp 0xFE, 16 bytes; recall = type 0x04 / status 0x0102 / comp 0xFF, 18 bytes with `presetId`@13, `presetSub`@14; reply kinds 0x01 ok, 0x09 error (payload[12]==0x03 → busy), 0x41 wait. Timings: 2.5 s ack deadline, 5 attempts, 500 ms busy back-off, 20 s discovery refresh.

## Step 2: Design doc authoring, bookkeeping, and reMarkable upload

With the evidence gathered, the design doc was written in one pass: prototype current-state with line anchors, the reconstructed UDP protocol reference, gap analysis (8 gaps G1–G8), the ESP-IDF component/task architecture with the `ppa_client` API sketch, six decision records, a six-phase implementation plan, and a testing strategy centered on a host-side Python module simulator.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Complete the deliverable: finish docs, wire up docmgr relations/changelog/tasks, validate with doctor, and upload the bundle to reMarkable.

**Inferred user intent:** A finished, validated ticket whose docs can be read as one PDF on the reMarkable.

### What I did

- Wrote `design-doc/01-m5dial-ppa-scene-controller-design-and-implementation-guide.md` (sections 1–10 per the ticket-research skill checklist).
- Wrote this diary; related key files to both docs with `docmgr doc relate --file-note` (absolute paths); updated `tasks.md` and the ticket changelog.
- Ran `docmgr doctor --ticket M5DIAL-PPA-CONTROL --stale-after 30` until clean.
- Uploaded via `remarquee upload bundle` (dry-run first) to `/ai/2026/07/14/M5DIAL-PPA-CONTROL` and verified with `remarquee cloud ls`.

### Why

- Bundle upload (index + design doc + diary as one PDF with ToC) is the skill-preferred delivery for reMarkable reading.

### What worked

- See the changelog for the exact upload listing and doctor output summary.

### What didn't work

- (recorded in changelog/final report if the upload needed retries; none at authoring time)

### What I learned

- N/A (bookkeeping step).

### What was tricky to build

- Keeping the design doc navigable at ~10 sections: the protocol reference was pulled into its own top-level section (§4) because it is the only content not reconstructible from the repo — it must survive even if the rest of the design is superseded.

### What warrants a second pair of eyes

- Design decision D3 (NVS instead of LittleFS) silently drops prototype-config migration; confirm that a one-time re-provisioning is acceptable for existing device(s).

### What should be done in the future

- N/A (see Step 1 follow-ups).

### Code review instructions

- `docmgr doctor --ticket M5DIAL-PPA-CONTROL --stale-after 30` must pass; check `changelog.md` for the upload verification listing.

### Technical details

- Upload command shape: `remarquee upload bundle <index.md> <design-doc.md> <diary.md> --name "M5DIAL-PPA-CONTROL — PPA Dial design" --remote-dir /ai/2026/07/14/M5DIAL-PPA-CONTROL --toc-depth 2` (dry-run first).
