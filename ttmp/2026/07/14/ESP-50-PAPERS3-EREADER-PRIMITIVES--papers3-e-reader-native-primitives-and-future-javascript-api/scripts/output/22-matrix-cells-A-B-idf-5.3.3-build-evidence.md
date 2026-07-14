---
Title: Matrix Cells A and B ESP-IDF 5.3.3 Build Evidence
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
Summary: "Exact ESP-IDF 5.3.3 warning-free build evidence for legacy and current M5 component matrix controls."
LastUpdated: 2026-07-14T22:27:00Z
WhatFor: "Close the missing build prerequisite for adjacent IDF and M5 library comparisons."
WhenToUse: "Use before flashing matrix Cell A or B, or building the FactoryTest V0.5 stock-source trace control."
---

# Matrix Cells A and B ESP-IDF 5.3.3 build evidence

Exact ESP-IDF tag `v5.3.3`, commit `6db3dc25df7325c1c81b7cd7d4e42babff7a818e`, is installed at `/home/manuel/esp/esp-idf-5.3.3`. Both previously blocked cells now build from clean exact-tag M5 component checkouts without warnings. Neither build command flashed or monitored hardware.

| Cell | IDF | M5GFX | M5Unified | Application bytes | Application SHA-256 | ELF SHA-256 |
|---|---|---|---|---:|---|---|
| A | 5.3.3 | 0.2.15 (`c6f92dc0...`) | 0.2.10 (`5580ff69...`) | 480752 | `cfb03a6f2570949e9e868776253add0816ac8b332694b5b3b2d192d61ed5fc12` | `3d01a57bccef1ce2626e1fd973891a26fa7ffc7e854185a0f99485c172fd3a33` |
| B | 5.3.3 | 0.2.25 (`ad9b8142...`) | 0.2.18 (`b1ffcc67...`) | 545120 | `3efe2423337820fd65f7e478d796b85d688f910260db7f6aa847faf0cdcde269` | `201796ca77eb9148bc25714ccb932a4064308d904cec3c361199f3e76447e022` |

Both use sdkconfig SHA-256 `ef8996d43ca27b84a0572cabbeeb7f660f9bda19d37f5ab5ff74d33b9dd9b4fa`. Final warning count is zero for both.

## Comparisons unlocked

- **A versus B:** same IDF 5.3.3, legacy versus current M5 stack.
- **B versus C:** same current M5 stack, IDF 5.3.3 versus 5.3.4.
- **A versus FactoryTest source lineage:** intended IDF and M5 library versions are now locally buildable.

These are build controls only. Optical equivalence still requires separately authorized physical runs with the established serial-ownership and observation protocol.

Hardware modified: **no**
