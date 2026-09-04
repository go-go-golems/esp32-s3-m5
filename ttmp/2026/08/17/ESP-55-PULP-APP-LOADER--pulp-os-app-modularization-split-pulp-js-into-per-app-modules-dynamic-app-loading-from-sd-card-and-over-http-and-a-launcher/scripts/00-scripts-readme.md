---
Title: "ESP-55 helper scripts"
Ticket: ESP-55-PULP-APP-LOADER
Status: active
Topics:
    - papers3
    - microquickjs
    - tooling
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Experiment scripts for ESP-55: trial split bytecode sizes and the host eval harness."
LastUpdated: 2026-08-17T11:40:00-04:00
WhatFor: "Re-run the ESP-55 measurements."
WhenToUse: "When re-measuring app module costs on the host."
---

# ESP-55 scripts

- `01-trial-split-bytecode-sizes.py [--out DIR]` — cut `pulp.js` at its
  section banners, compile each section with the host `pulpjsc`, print a
  markdown table of source vs bytecode bytes (guide §3.2/§4.1). Requires
  `tools/js/host/pulpjsc` (run `tools/js/build_bytecode_apps.sh` first).
- `02-host-eval-harness.sh eval|bc <arena_kb> <file.js>...` — builds
  `02-host-eval-harness.c` against the firmware's vendored engine + host
  stdlib and measures arena/time for source eval (`eval`) or bytecode
  images (`bc`) (guide §4.2). Binary lands in the session scratchpad
  (`HARNESS_OUT` overrides).
