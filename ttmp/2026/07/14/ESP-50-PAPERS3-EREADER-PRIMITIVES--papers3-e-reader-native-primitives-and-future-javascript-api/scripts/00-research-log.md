---
Title: Research Trace
Ticket: ESP-50-PAPERS3-EREADER-PRIMITIVES
Status: active
Topics:
    - papers3
    - debugging
    - architecture
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Reproducible scripts, query sequence, snapshots, failures, and rerun instructions for the ESP-50 investigation."
LastUpdated: 2026-07-14T16:30:00-04:00
WhatFor: "Audit or refresh the research evidence behind the PaperS3 reader-primitives design."
WhenToUse: "Read before rerunning upstream queries, Defuddle captures, or local inventories."
---

# Research Trace

This directory makes the ESP-50 investigation reproducible. The extracted source material remains under `../sources/` because the original request explicitly asked for Defuddle downloads there. The scripts that imported, discovered, queried, and anchored that material live here, with timestamped/current-run outputs under `output/`.

## Sequence

1. `01-inventory-local-evidence.sh`
   - Inventories the PaperS3 firmwares (`0075` through `0082`), related ticket docs, local `~/Downloads` inputs, and the nested `M5PaperS3-UserDemo`/M5GFX/M5Unified Git state.
   - Snapshot: `output/01-local-inventory.txt`.
2. `02-import-and-fetch-sources.sh`
   - Imports `s3paper-api-design.md` and its only local companion, `s3paper-studio.jsx`.
   - Downloads the official PaperS3 page, M5Stack repositories/issues, MicroQuickJS README, and two external e-reader architecture references with `defuddle parse --md`.
   - Current hashes and sizes: `output/02-source-manifest.txt`.
3. `03-query-upstream-state.sh`
   - Uses GitHub's API through `gh api` to capture issue states, releases, relevant commit history, the Issue 181 fix, inclusion in M5GFX 0.2.25, ESP-IDF 5.4 support in M5GFX 0.2.17, current M5Unified, and MicroQuickJS HEAD.
   - Snapshot: `output/03-upstream-state.txt`.
4. `04-collect-line-anchors.sh`
   - Captures line anchors for the imported prototype, existing reader, factory HAL, and earlier queued canvas ABI.
   - Snapshot: `output/04-line-anchors.txt`.
5. `05-add-phase-tasks.sh`
   - Creates the original detailed Phase 0–13 implementation roadmap.
6. `06-download-epd-bug-reports.py`
   - Downloads complete GitHub issue bodies and comments for the PaperS3/M5GFX, LilyGo rail/VCOM, and FastEPD cases.
7. `07-download-epd-painter-reference.sh`
   - Downloads the complete build-relevant EPD_Painter 1.0.7 source at commit `753c521da8aef59756df07c1a4eb88f1c64c8227`, selected docs/examples, and a hash manifest.
8. `08-compare-m5gfx-luts.py`
   - Downloads M5GFX 0.2.15/0.2.25 panel sources plus the 0.2.25 board mapping and proves the five built-in LUTs are identical.
9. `09-replay-factory-v0.5-flash.sh`
   - Replays or checks the exact official merged factory binary. Default `--check` mode is non-destructive; `--execute` refuses a serial port with another owner.
   - Timestamped check/flash logs live under `output/`.
10. `10-audit-epd-painter.py`
    - Performs the independent-driver pre-hardware audit: pin equivalence, waveform action counts, power sequencing, buffer initialization/allocation, completion semantics, cleanup, and safety gate.
    - Snapshot: `output/10-epd-painter-pre-hardware-audit.md`.
11. `11-prepare-epd-painter-control.sh`
    - Reconstructs the `0107` driver component from the strict upstream manifest plus `patches/11-epd-painter-pure-idf-hardening.patch`.
    - Applies with zero fuzz and proves the PaperS3 preset/waveform source remains byte-identical.
12. `12-build-epd-painter-control.sh`
    - Fails closed unless exact ESP-IDF 5.4.2 is active, recreates sdkconfig/build state, performs a clean warning-free build, captures size and hashes, and never flashes.
    - All failed and successful build logs plus latest evidence live under `output/12-*`.
13. `13-audit-built-epd-control.py`
    - Audits sdkconfig, compile definitions, symbols, no-drive boot behavior, bounded command surface, waveform/reader-fixture identity, initialization hardening, build warnings, flash absence, and IRAM headroom.
    - Timestamped audits plus `output/13-built-control-audit-latest.md` preserve gate history.
14. `14-generate-epd-control-fixtures.py`
    - Generates the deterministic 960×540 binary reader page from a pinned DejaVu Serif font, packs it into EPD_Painter's 2-bpp format, and records source/asset/preview hashes.
    - Preview: `output/14-reader-page-preview.png`; firmware asset: `0107-papers3-epd-painter-control/main/fixtures/reader_page.bin`.

## Web discovery queries

These Kagi queries were run before downloading primary sources:

- `M5Stack PaperS3 ESP-IDF 5.3.3 waveform corruption ESP-IDF 5.4 M5GFX issue`
- `site:github.com/m5stack/M5PaperS3-UserDemo issues ESP-IDF PaperS3 display waveform`
- `site:github.com/m5stack/M5GFX PaperS3 IDF 5.3.3 EPD issue waveform corruption`
- `M5Stack PaperS3 official documentation specifications touch SD RTC battery ESP32-S3`
- `microquickjs GitHub embedded JavaScript C API documentation memory limitations`
- `embedded e-reader pagination UTF-8 font rendering hyphenation ESP32 EPUB architecture`
- `M5GFX issue 181 fix pull request Panel_EPD two PSRAM heap corruption bugs`
- `site:github.com/m5stack/M5GFX commits Panel_EPD PaperS3 2026 fix rotation range_mod`
- `site:github.com/bellard/mquickjs README C API modules bytecode native functions`

## Important failures and interpretation notes

- Reading the project-root `README.md` failed because this repository has no root README:

  ```text
  ENOENT: no such file or directory, access '/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/README.md'
  ```

  The investigation instead read each target firmware README plus the repository `AGENTS.md` supplied to the agent.

- The original prompt named `~/Downloads/s3paper-api-design.md` twice while saying “files.” The Downloads directory contained exactly two `s3paper*` files: `s3paper-api-design.md` and `s3paper-studio.jsx`. Both were imported; the Markdown file was not duplicated.

- Defuddle extracted useful GitHub release content but then returned this metadata error:

  ```text
  Failed to parse URL: TypeError: Invalid URL
  ...
  code: 'ERR_INVALID_URL',
  input: '/m5stack/M5GFX/releases'
  ```

  `02-import-and-fetch-sources.sh` now stages extraction in a temporary file and preserves substantial output even when this known metadata-stage failure produces a non-zero exit.

- Defuddle's Issue 152 extraction only contains the opening report. `03-query-upstream-state.sh` therefore captures the complete issue comments through the GitHub API; the design guide distinguishes Defuddle source text from API snapshot evidence.

## Re-running

From the repository root:

```bash
T=ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api
$T/scripts/01-inventory-local-evidence.sh > $T/scripts/output/01-local-inventory.txt
$T/scripts/02-import-and-fetch-sources.sh
$T/scripts/03-query-upstream-state.sh > $T/scripts/output/03-upstream-state.txt
$T/scripts/04-collect-line-anchors.sh > $T/scripts/output/04-line-anchors.txt
$T/scripts/06-download-epd-bug-reports.py
$T/scripts/07-download-epd-painter-reference.sh
$T/scripts/08-compare-m5gfx-luts.py
$T/scripts/09-replay-factory-v0.5-flash.sh --check
$T/scripts/10-audit-epd-painter.py
$T/scripts/11-prepare-epd-painter-control.sh
$T/scripts/12-build-epd-painter-control.sh
$T/scripts/13-audit-built-epd-control.py
$T/scripts/14-generate-epd-control-fixtures.py
```

Upstream output is date-sensitive; review diffs instead of assuming a later run will match the 2026-07-14 snapshot.
