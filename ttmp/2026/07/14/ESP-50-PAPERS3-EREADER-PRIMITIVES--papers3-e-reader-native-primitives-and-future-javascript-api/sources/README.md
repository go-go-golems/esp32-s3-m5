# Source Inventory

## Local inputs

- `local/s3paper-api-design.md` — original fluent API proposal from `~/Downloads/s3paper-api-design.md`.
- `local/s3paper-studio.jsx` — executable browser prototype from `~/Downloads/s3paper-studio.jsx`.

The user prompt named `s3paper-api-design.md` twice but said “files.” `s3paper-studio.jsx` was the only other `s3paper*` file in Downloads and is the implementation corresponding to the design, so it was imported as the second input.

## Defuddle web captures

| File | Primary source | Why it matters |
|---|---|---|
| `web/01-m5stack-papers3-hardware.md` | M5Stack PaperS3 docs | Hardware, pin map, touch, storage, power |
| `web/02-m5papers3-userdemo.md` | Factory demo repository | Official build/component baseline |
| `web/03-m5gfx-papers3-driver.md` | M5GFX PaperS3 notes | Current in-tree driver versus historical EPDiy |
| `web/04-m5gfx-issue-181-panel-epd-heap-corruption.md` | M5GFX Issue 181 | Odd-width and rotation heap-corruption defects |
| `web/05-m5gfx-issue-152-waveform-and-ghosting.md` | M5GFX Issue 152 | Opening waveform/ghosting regression report |
| `web/06-mquickjs-readme.md` | MicroQuickJS README | Runtime, C API, GC, syntax, memory, bytecode |
| `web/07-diy-esp32-epub-reader.md` | atomic14 reader | Embedded EPUB pipeline and layout precedent |
| `web/08-diy-ebook-reader-article.md` | atomic14 article | Narrative implementation context |
| `web/09-crossink-architecture.md` | CrossInk architecture | Modern SD cache/activity/reader precedent |
| `web/10-m5stack-papers3-touch.md` | M5Stack touch docs | GT911 touch API examples |
| `web/11-m5gfx-releases.md` | M5GFX releases | Current release state, including 0.2.25 |
| `web/12-epdiy-waveform-timings.md` | EPDiy waveform timing wiki | Origin/target lookup and phase timing model |
| `web/13-epdiy-parallel-pixel-drive.md` | EPDiy direct-drive wiki | Gate/source signals, VCOM, ±15 V pixel actions |
| `web/14-epdiy-vendor-waveforms.md` | EPDiy waveform documentation | Vendor waveform provenance and temperature/mode advantages |
| `web/15-electrophoretic-waveform-dc-balance.md` | Frontiers waveform study | Particle activation, reference states, and DC balance |
| `web/16-electrophoretic-ghosting-low-power-waveform.md` | EPD ghosting study | Physics and measured ghosting context |
| `web/17-m5gfx-issue-119-full-thread.md` | M5GFX Issue 119 | PaperS3 progressive gray and M5GFX-versus-EPDiy evidence |
| `web/18-m5gfx-issue-152-full-thread.md` | M5GFX Issue 152 | Full maintainer discussion of overload, recovery, and unstable gradations |
| `web/19-m5gfx-issue-157-pushsprite-regression.md` | M5GFX Issue 157 | PaperS3 M5GFX 0.2.15 canvas regression and 0.2.16 fix |
| `web/20-m5gfx-issue-160-idf54-stripes.md` | M5GFX Issue 160 | IDF 5.4 open-drain GPIO stripe regression and root cause |
| `web/21-m5gfx-issue-166-panel-instability.md` | M5GFX Issue 166 | Panel, driver-overload, and board-circuit instability hypotheses |
| `web/22-reddit-epd-grayscale-matrix.md` | FastEPD author post | PaperS3-specific 25-action grayscale matrix, captured with Defuddle |
| `web/23-home-assistant-papers3-epdiy-idf55.md` | Home Assistant forum | PaperS3 EPDiy fork on ESP-IDF 5.5.1, captured with Defuddle |
| `web/24-lilygo-issue-93-rails-vcom-corruption.md` | LilyGo EPD47 Issue 93 | Same panel-class rail ripple, VCOM, and corruption evidence on different hardware |
| `web/25-fastepd-issue-29-4bpp-corruption.md` | FastEPD Issue 29 | Fixed 4-bpp backup-buffer corruption affecting 960×540 paths |

## Downloaded code references

- `code/m5gfx-lut-comparison/` — exact M5GFX 0.2.15 and 0.2.25 `Panel_EPD.cpp` sources plus normalized LUT hashes; all five built-in EPD LUTs are identical.
- `code/epd-painter-753c521.../` — complete build-relevant source plus selected documentation/examples from the independent PaperS3-specific EPD_Painter driver, including assembly, waveform presets, hard-clear/DC-balance logic, boot/power control, and calibrator.

## Hardware and live evidence

- `hardware/ED047TC1-datasheet.{pdf,txt}` — official panel electrical, VCOM, timing, temperature, and optical specification.
- `hardware/PaperS3-schematic-V1.0.{pdf,png,txt}` — official board schematic and render/extraction.
- `hardware/PaperS3-schematic-epd-{power,rails}.png` — enlarged crops used to read the analog EPD circuit directly.
- `hardware/epdiy_ED047TC1.h` — external ED047TC1-specific origin/target waveform reference.
- `hardware/2026-07-14-cell-{C,D}/` — exact build metadata, tmux transcripts, and operator observations.
- `hardware/factory-v0.5/` — official merged FactoryTest V0.5 binary, provenance, flash transcript, and operator disposition.

Defuddle only extracted the opening text of the original Issue 152 capture. Full GitHub issue threads are now generated reproducibly by `../scripts/06-download-epd-bug-reports.py`; standard forum/Reddit pages use Defuddle markdown captures.

## Provenance and refresh

- Hashes and sizes: `../scripts/output/02-source-manifest.txt`.
- Re-fetch script: `../scripts/02-import-and-fetch-sources.sh`.
- Research chronology and known extraction failure: `../scripts/00-research-log.md`.

Web content and release state are date-sensitive. The current snapshot was gathered on 2026-07-14.
