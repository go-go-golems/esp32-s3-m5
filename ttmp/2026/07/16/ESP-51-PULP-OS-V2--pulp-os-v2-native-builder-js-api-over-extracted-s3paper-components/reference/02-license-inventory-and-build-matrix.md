---
Title: License Inventory and Build Matrix
Ticket: ESP-51-PULP-OS-V2
Status: active
Topics:
    - papers3
    - eink
    - esp32s3
    - microquickjs
    - architecture
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-07-16T16:50:03.013621512-04:00
WhatFor: ""
WhenToUse: ""
---

# License Inventory and Build Matrix

## License inventory (both firmwares)

| Component | Origin | License | Notes |
|---|---|---|---|
| MicroQuickJS engine (`0112/components/mquickjs`, `0114/components/mquickjs`) | Bellard/Gordon, vendored via imports/esp32-mqjs-repl (upstream bellard/mquickjs @ 84d793e0) | MIT | Per-firmware copies by design (atom tables are stdlib-specific); provenance READMEs in each copy |
| stb_truetype v1.26 (vendored in s3paper_core) | Sean Barrett | Public domain / MIT dual | Trusted firmware-embedded fonts ONLY (not hardened for hostile files — ESP-50 design-doc/04) |
| PT Serif subset (`components/s3paper_core/fonts/PTSerifUkr.ttf`) | ParaType | OFL 1.1 | Latin+Ukrainian subset via pyftsubset (ESP-50 scripts/53) |
| Liberation Sans Bold subset (`LibSansBoldUkr.ttf`) | Red Hat | OFL 1.1 (Liberation license) | Subset via ESP-50 scripts/54; LiberationSans-LICENSE.txt alongside |
| M5Unified ==0.2.18 / M5GFX ==0.2.25 | M5Stack | MIT | Registry-pinned in s3paper_m5; dependencies.lock committed |
| ESP-IDF 5.3.4 | Espressif | Apache-2.0 | Pinned toolchain |
| s3paper_* components, firmwares, PULP apps | this repository | repository license | Original work (ESP-50/51/52) |
| Shevchenko seed book text (`0114/main/app_book_seed.h`) | Taras Shevchenko, 1845-1847 | Public domain | Ukrainian Cyrillic font-path proof |

## Build matrix (verified 2026-07-16)

| Target | Toolchain | Command | Result |
|---|---|---|---|
| s3paper_core host suite | g++ (ASan/UBSan) | `cd components/s3paper_core/tests/host && make run` | PASS 38,174 checks |
| 0112-papers3-reader-primitives | ESP-IDF 5.3.4 | `idf.py build` | OK, 0xfb8b0 bytes (75% partition free) |
| 0114-papers3-pulp-os | ESP-IDF 5.3.4 | `idf.py build` | OK, 0xf5670 bytes (76% free) |
| pulp stdlib generator + pulpjsc + bytecode | host gcc (needs -m32 multilib) | `tools/js/gen_pulp_stdlib.sh && tools/js/build_bytecode_apps.sh` | OK, pulp image 29,372 B |

Rebuild order after a stdlib change: gen_pulp_stdlib.sh -> build_bytecode_apps.sh -> idf.py build (atoms couple all three).
