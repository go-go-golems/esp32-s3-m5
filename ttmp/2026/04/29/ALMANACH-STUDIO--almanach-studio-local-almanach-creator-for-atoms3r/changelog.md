# Changelog

## 2026-04-29

- Initial workspace created


## 2026-04-29

Created comprehensive design/implementation guide for Almanach Studio on AtomS3R. Analyzed JSX source, serve-claude-experiments reference project, and 0017-atoms3r-web-ui firmware pattern. Documented esbuild static-compile strategy, EMBED_TXTFILES embedding, URI handler registration, and font handling. 15-section guide with diagrams, pseudocode, API references.


## 2026-04-29

Uploaded bundled PDF to reMarkable at /ai/2026/04/29/ALMANACH-STUDIO


## 2026-04-29

Step 2: Implemented Almanach Studio hosting in stoms3r/ (correct firmware). esbuild IIFE bundle (210.9 KB) embedded via EMBED_TXTFILES. Added GET /almanach and GET /almanach/bundle.js handlers. Build passes: 1.01 MB binary, 74% free. Commit 7924e1b.


## 2026-04-29

Step 3: Monochrome themes (all #000/#fff, no gray), dithering selector (F-S/None/Auto), font scale slider (1.0-2.0×, default 1.3×), default width 384px. Commits 2494359, 101ffed, baee459.


## 2026-04-29

Step 4: Wrote textbook-style technical report (26 KB) covering full system architecture. Stored in Obsidian vault + copied to ticket. Has Mermaid diagrams, code snippets, tables, failure modes, working rules.


## 2026-04-29

Step 5: Added direct print button — renders paper, binarizes to 1-bit B/W, POSTs raw bitmap to /api/print/bitmap. No PNG intermediate. Commit 99e0016.

