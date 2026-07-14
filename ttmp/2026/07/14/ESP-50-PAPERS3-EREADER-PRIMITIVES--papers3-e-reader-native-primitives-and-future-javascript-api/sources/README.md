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

Defuddle only extracted the opening text of Issue 152. The full issue comments and current GitHub state are captured by `../scripts/03-query-upstream-state.sh` in `../scripts/output/03-upstream-state.txt`.

## Provenance and refresh

- Hashes and sizes: `../scripts/output/02-source-manifest.txt`.
- Re-fetch script: `../scripts/02-import-and-fetch-sources.sh`.
- Research chronology and known extraction failure: `../scripts/00-research-log.md`.

Web content and release state are date-sensitive. The current snapshot was gathered on 2026-07-14.
