---
Title: M5GFX Runtime Trace Observer-Effect Audit
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
Summary: "Static/binary observer-effect audit for trace-off and fixed-ring M5GFX timing variants before hardware use."
LastUpdated: 2026-07-14T22:07:51Z
WhatFor: "Prove trace-off identity and bound trace-on perturbations before any physical comparison."
WhenToUse: "Review before authorizing an instrumented M5GFX flash or interpreting runtime timestamps."
---

# M5GFX runtime trace observer-effect audit

Gate: **PASS**
Checks: **18 / 18 passed**
Hardware modified: **no**

## Checks

- [x] **Patch is applied exactly and reversibly** — patch_sha256=6c21e45e0909accd2b5df5ae3178534192b10a51ec4a319ebc2309bfe983d89f
- [x] **Patched source preserves canonical LUTs** — canonical_lut_sha256=d24b2df188e4261d5891a0884e2510567ea45c38bcaebeb66ade1d4f4b979af3
- [x] **Off configuration compiles trace out** — sdkconfig_sha256=396f1c39263c0d2a66c66c8dbae5fad5a5cedca6c58044e90ea528553f08e295
- [x] **Timing configuration enables 512-record ring** — sdkconfig_sha256=3797d48d537e2455376ee0b5fc3229b881ffa6c5237e5cb9f369892a76e43047
- [x] **Both variants preserve 100 Hz tick** — CONFIG_FREERTOS_HZ=100
- [x] **Final builds are warning-free** — off=19-m5gfx-runtime-trace-off-20260714T220453Z.log; trace=19-m5gfx-runtime-trace-trace-20260714T220453Z.log
- [x] **Off ELF has no trace hook or ring** — symbols=absent
- [x] **Timing ELF links one strong hook** — lgfx_epd_trace_emit=1
- [x] **Timing ring is fixed at 512 x 48 bytes** — ring_bss_bytes=24576
- [x] **Trace-off critical driver code is byte-identical to clean Cell D** — panel-task: clean=634d10897d6fc00f0f31c5fdeeb9468ac131a6113ae42244a397d8707b6277b5 off=634d10897d6fc00f0f31c5fdeeb9468ac131a6113ae42244a397d8707b6277b5 same=true; power-control: clean=0688a43e27ad3af9b410419477f0eda234f4e464f5007fc3ccc77a0833e884d4 off=0688a43e27ad3af9b410419477f0eda234f4e464f5007fc3ccc77a0833e884d4 same=true
- [x] **No trace hook executes inside the row loop** — frame hooks bracket, rather than enter, the 540-row loop
- [x] **Patched M5GFX performs no hot-path printing or allocation** — added M5GFX lines contain fixed hook calls and counters only
- [x] **Firmware boot issues no display transaction** — M5.begin(clear_display=false), rotation, text defaults, console only
- [x] **Trace dumps are operator-requested after waitDisplay** — epd trace dump is outside M5GFX worker and guarded by display mutex
- [x] **Timing instrumentation growth is bounded to frame/power paths** — task_update=815->1024 (+209); powerControl=292->388 (+96)
- [x] **Linked timing image has bounded hook call sites** — hook_call_sites=10; none in row loop
- [x] **Application size delta is recorded and modest** — off=546064; trace=547648; delta=1584
- [x] **Build workflow contains no flash or monitor operation** — builds use set-target, build, and size only

## Interpretation

The trace-disabled critical M5GFX frame scheduler and power-control text sections are byte-identical to the previously built clean Cell D control. This is stronger than source inspection: trace arguments, queue queries, counters, and hooks compile completely out.

The timing variant is intentionally not byte-identical. It adds fixed-ring writes and one timestamp read per event at operation, queue, update-preparation, power, and frame boundaries. It does not count drive codes or log inside the 540-row loop, and it emits no serial output while rails are active. Static auditing bounds where perturbation can occur; only a later trace-off/trace-on physical timing comparison can measure its duration.

## Artifact identities

- Off application SHA-256: `609aba851db118ee26a3051d4f78ae96255229493f9783f60f43334355925e68`
- Trace application SHA-256: `a081daabe5a77d7405cde68e43955279ed5e5c0f954c2aee027b62d03fd9f6ea`
- Patch SHA-256: `6c21e45e0909accd2b5df5ae3178534192b10a51ec4a319ebc2309bfe983d89f`
- Canonical LUT SHA-256: `d24b2df188e4261d5891a0884e2510567ea45c38bcaebeb66ade1d4f4b979af3`
