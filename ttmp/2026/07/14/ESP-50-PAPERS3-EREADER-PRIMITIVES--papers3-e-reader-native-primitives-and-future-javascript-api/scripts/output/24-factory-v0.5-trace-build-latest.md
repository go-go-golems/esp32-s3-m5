---
Title: FactoryTest V0.5 Stock-Source Trace Build Evidence
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
Summary: "Exact-IDF clean, trace-off, and trace-on FactoryTest V0.5 source-lineage builds without hardware access."
LastUpdated: 2026-07-14T22:46:24Z
WhatFor: "Establish F1/F2 source controls before replaying or instrumenting the physical factory sequence."
WhenToUse: "Review before any 0109 flash or interpretation of FactoryTest-derived runtime events."
---

# FactoryTest V0.5 stock-source trace build evidence

- Factory source: `V0.5` / `5e275ad4b70abb85f7193fda137844730e64c4db`
- ESP-IDF: `v5.3.3` / `6db3dc25df7325c1c81b7cd7d4e42babff7a818e`
- M5GFX: `0.2.15` / `c6f92dc03226cdc04d67c705a2020f62ad21ad01`
- M5Unified: `0.2.10` / `5580ff6923a868cc71d5b30c962186bde2c85b67`
- Trace patch SHA-256: `1873cd3b7485aa3f576448c28499f29aec5aff1dd18f6880e690c5109369c254`
- FreeRTOS tick: `100 Hz`
- Hardware modified: **no**

| Variant | M5GFX source | Trace | Application bytes | Application SHA-256 | ELF SHA-256 |
|---|---|---|---:|---|---|
| clean | unpatched | off | 1362704 | `ad858733ab2ddd5c664f33ab593a3ea7775b26dbe35c3e575a3fe47c235d753f` | `c869ed4d8e9bf487d131138a791007a0b6cee0f03fa018b512eecd6b27cb5283` |
| F1/off | patched | off | 1362704 | `3d9bf37a5c5faa120fa1dccf357e8d0676a77495359754d062a5fa654dd2d2b3` | `760178407d604729320c72086bb72aa347432863fca7a6df3992631ce3657334` |
| F2/trace | patched | 1024 × 48-byte ring | 1364144 | `95334c261762205ab95d3f578a5d3d0a0eac4fe7fffdfd1ada0e836ba8a2d755` | `ff7f1f7b522f4649c31e551f0d9448439c31eecff46a652e9f1a4c4d1ae1d48d` |

All three builds completed from clean build directories. Each preserves the same 13 upstream FactoryTest/IDF 5.3.3 warnings, normalized warning-set SHA-256 `94b79ce766cf8493c539b639126954cb25de6c67e3b64f76a547068e8e709e3c`; tracing introduced no new warning. Build commands contain no flash or monitor action.
