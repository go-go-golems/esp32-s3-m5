---
Title: Investigation diary
Ticket: ESP-55-PULP-APP-LOADER
Status: active
Topics:
    - papers3
    - esp32s3
    - microquickjs
    - architecture
    - eink
    - javascript
    - storage
    - webserver
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-08-17T11:02:44.989239565-04:00
WhatFor: ""
WhenToUse: ""
---

# Investigation diary — ESP-55 PULP app loader

## Goal

Chronological record of the ESP-55 investigation: how the PULP OS v2 JS
runtime, build pipeline and native modules were surveyed, which host
experiments were run to size a dynamic-app design, what was blocked, and
how the intern guide was written. Each step is continuation-friendly:
commands and errors are verbatim.

## Step 1: Ticket creation, skills, and the first evidence sweep

The user asked for a new docmgr ticket that plans the modularisation of
`tools/js/apps/pulp.js` (one 1,125-line file holding the launcher and ten
apps) into per-app modules, plus dynamic loading of apps from the SD card
and over HTTP behind a launcher, delivered as an intern-level design and
implementation guide. I created `ESP-55-PULP-APP-LOADER`, seeded the task
list, and started two parallel read-only surveys (prior tickets' design
docs and diaries; the native `main/*` sources) while reading the JS host
core (`app_js.cpp`), the stdlib definition and the engine's bytecode API
myself.

### Prompt Context

**User prompt (verbatim):** "Let's create a new docmgr ticket to cleanup / modularize the @0114-papers3-pulp-os/tools/js/apps/pulp.js itself (by splitting out the apps) and allow apps to be loaded dynamically (over HTTP and stored on SD card), with a launcher.

Create  a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and the nupload to remarkable.

The device is attached over serial / USB.

commit at appropriate intervals and keep a detailed diary as you work (using the diary format from the skill). Store all scripts you write in the scripts/ folder of the ticket."

Mid-turn addition: "here's some docs /home/manuel/code/wesen/go-go-golems/go-go-parc/Projects/2026/07/28/PROJ - PULP OS Image Gallery - mDNS Browser Upload and the Bitmap Blit.md"

**Assistant interpretation:** Produce a research/design ticket (not the implementation) whose main deliverable is an exhaustive, evidence-anchored intern guide covering: what pulp.js is today, the JS runtime + build pipeline it depends on, a design for splitting it into app modules, a loader that pulls apps from SD or HTTP, a launcher, and a phased implementation plan; keep a diary; store scripts in the ticket; upload to reMarkable.

**Inferred user intent:** Make PULP OS extensible without reflashing (apps as files), shrink the monolith, and have a document a newcomer can implement from.

### What I did
- Loaded skills `ticket-research-docmgr-remarkable` and `diary`.
- Read `0114-papers3-pulp-os/tools/js/apps/pulp.js` (1,125 lines, 36,901 bytes; 15 banner-delimited sections), `main/app_js.cpp` (JS host core), `tools/js/pulp_stdlib.c` + `mqjs_stdlib_pulp.c` (stdlib table), `tools/js/pulpjsc.c`, `tools/js/build_bytecode_apps.sh`, `gen_pulp_stdlib.sh`, `components/mquickjs/mquickjs.h` (bytecode API) and `mquickjs.c:12800-12990` (`JS_RelocateBytecode`, `JS_LoadBytecode`).
- Read the reMarkable/Obsidian project note the user pointed at (ESP-54 gallery write-up) for the current-state narrative and the "device never decodes images" working rule.
- `docmgr ticket create-ticket --ticket ESP-55-PULP-APP-LOADER ...`, `docmgr doc add` (design-doc + diary), `docmgr task add` ×10.
- Launched two Explore agents (prior-ticket doc facts; native-side map). The native map is saved at `sources/01-native-side-map.md`.

### Why
- The design hinges on engine constraints (how bytecode is loaded, whether source can be evaluated at run time), so the engine and host were read first-hand rather than summarised from memory.

### What worked
- The docmgr scaffolding and the direct reads; the engine constraint was found quickly: `JS_LoadBytecode` refuses to run once any RAM atom exists (`mquickjs.c:12947-12948`) and `N_ROM_ATOM_TABLES_MAX = 2` (`mquickjs.c:182`), i.e. bytecode images are boot-time only and there is exactly one slot for the app image.
- `eval(str)` is present in the generated stdlib (`main/js_stdlib.h:4323`) — the parser is on the device, so source-eval loading needs no engine change.

### What didn't work
- The device is not reachable: `lsusb` shows `303a:1001 Espressif USB JTAG/serial debug unit` on bus 3 dev 33, but no `/dev/ttyACM*` / `/dev/serial/by-id/` exists because the `cdc_acm` kernel module is not loaded (`ls /sys/bus/usb/drivers/` has no `cdc_acm`; interface `3-3:1.0` has no driver). `sudo -n modprobe cdc_acm` → `sudo: a password is required`. On-device measurements are therefore deferred (task "Device experiment" left open) and the user needs to run `sudo modprobe cdc_acm` (or re-plug after loading it).
- zsh gotchas while scripting: `echo =====` fails (`no matches found: =====`, zsh `=cmd` expansion) and an unquoted `$F` holding a space-separated list is not word-split — use arrays.

### What I learned
- pulp.js source is 36,901 B but its bytecode image is 45,332 B (host `pulpjsc` count) and is copied into *internal* SRAM at boot (`app_js.cpp:111`, `MALLOC_CAP_INTERNAL`) — modularising the apps out of the image is also an internal-RAM win.
- The current app-switch boundary (`enter()` → `resetTree()`) is already the natural unload point for a dynamically loaded app.

### What was tricky to build
- Nothing built yet; the tricky part was establishing which loading path is *possible*: bytecode (boot-only, 1 slot) vs. source eval (available, but arena cost unknown → next step measures it).

### What warrants a second pair of eyes
- The claim that a second bytecode image cannot be loaded at run time — verified in the engine source, and re-verified empirically on the host in Step 2.

### What should be done in the future
- Re-run the on-device measurements once the console port is back.

### Code review instructions
- Start with `sources/01-native-side-map.md`, then `0114-papers3-pulp-os/main/app_js.cpp:100-130` and `components/mquickjs/mquickjs.c:12940-12962`.

### Technical details
- Bytecode facts: `JS_LoadBytecode` checks `unique_strings_len == 0`, `n_rom_atom_tables < 2`, magic/version/word size, and that the buffer was relocated in place; the image buffer must outlive the context.
