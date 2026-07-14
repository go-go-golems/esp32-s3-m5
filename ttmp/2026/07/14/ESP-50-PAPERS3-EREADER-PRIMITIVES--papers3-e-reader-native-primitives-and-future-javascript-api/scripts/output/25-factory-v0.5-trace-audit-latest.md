---
Title: FactoryTest V0.5 Trace Control Audit
Ticket: ESP-50-PAPERS3-EREADER-PRIMITIVES
Status: active
Topics:
    - papers3
    - eink
    - esp-idf
    - hardware-qualification
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Observer and provenance audit for exact-IDF FactoryTest clean, F1 trace-off, and F2 trace-timing controls."
LastUpdated: 2026-07-14T22:48:05Z
WhatFor: "Gate stock-source-derived runtime tracing before physical F0/F1/F2 comparisons."
WhenToUse: "Review before flashing any 0109 variant or attributing ring events to FactoryTest V0.5."
---

# FactoryTest V0.5 trace control audit

Gate: **PASS**
Checks: **19 / 19 passed**
Hardware modified: **no**

## Checks

- [x] **Exact ESP-IDF 5.3.3 checkout is active** — v5.3.3 commit=6db3dc25df7325c1c81b7cd7d4e42babff7a818e
- [x] **Factory source lineage is exact V0.5** — FactoryTest V0.5 commit=5e275ad4b70abb85f7193fda137844730e64c4db
- [x] **Built-in black-white-grayscale function is byte-identical to V0.5 source** — boot_display_test_sha256=9b5bca6ed92fd1b07929c2d2ee07450737d22ff3eb49e1201b305195a403b02f
- [x] **F2 dump is ordered after factory sequence and before dashboard app installation** — sequence -> waitDisplay/dump -> install apps
- [x] **Legacy trace patch is exactly and reversibly applied** — patch_sha256=1873cd3b7485aa3f576448c28499f29aec5aff1dd18f6880e690c5109369c254
- [x] **Legacy trace patch preserves canonical LUTs** — canonical_lut_sha256=d24b2df188e4261d5891a0884e2510567ea45c38bcaebeb66ade1d4f4b979af3
- [x] **Clean and F1 compile trace completely off** — clean=off
- [x] **F2 enables 1024 x 48-byte ring** — capacity=1024
- [x] **All variants preserve 100 Hz tick and USB Serial/JTAG console** — tick=100Hz; console=USB Serial/JTAG
- [x] **Tracing introduces no new build warning** — count=13; normalized_sha256=6e43218d6428aa292278ebbfe0c1cd900e4bdb5b6236f67e26e3876e6de2db92
- [x] **F1 critical machine code is byte-identical to clean source control** — panel-task=bfd33e8b590ed377cadf604be2ee5bcbd38f2ded59d681d5e59059a07b3e7611 same=true; power-control=0688a43e27ad3af9b410419477f0eda234f4e464f5007fc3ccc77a0833e884d4 same=true; app-main=47198cf5d1318d66ae144975d279bdc4b7eb39d7cd65f9fbf697c343fccc70d1 same=true
- [x] **F1 contains no trace hook or ring** — symbols=absent
- [x] **F2 links strong hook and exact ring BSS** — ring_bss=49152
- [x] **No F2 trace hook executes inside row loop** — frame hooks bracket 540-row loop
- [x] **Legacy M5GFX patch has no hot-path printing or allocation** — hook/counter additions only
- [x] **F2 has bounded hook call sites** — hook_call_sites=10
- [x] **F2 application growth is bounded** — off=1362704; trace=1364144; delta=1440
- [x] **Exact F0 merged release remains available** — release_sha256=d6733a0ca378f95335fa5fba4d4d992fb1dd97c17557b20e9aebfca08ba6d624
- [x] **Build workflow cannot flash or monitor hardware** — set-target/build/size only

## Disposition

F0 remains the exact released-binary optical control and has no ring. F1 is eligible as the stock-source trace-off proxy because its factory boot function is source-identical and its app/driver critical text is byte-identical to the clean build. F2 is eligible for later operator-gated use only after F0 and F1 observations; its ring dump occurs after the built-in sequence reaches display idle.

## Artifact identities

- Clean app SHA-256: `ad858733ab2ddd5c664f33ab593a3ea7775b26dbe35c3e575a3fe47c235d753f`
- F1 app SHA-256: `3d9bf37a5c5faa120fa1dccf357e8d0676a77495359754d062a5fa654dd2d2b3`
- F2 app SHA-256: `95334c261762205ab95d3f578a5d3d0a0eac4fe7fffdfd1ada0e836ba8a2d755`
- Trace patch SHA-256: `1873cd3b7485aa3f576448c28499f29aec5aff1dd18f6880e690c5109369c254`
- Canonical LUT SHA-256: `d24b2df188e4261d5891a0884e2510567ea45c38bcaebeb66ade1d4f4b979af3`
