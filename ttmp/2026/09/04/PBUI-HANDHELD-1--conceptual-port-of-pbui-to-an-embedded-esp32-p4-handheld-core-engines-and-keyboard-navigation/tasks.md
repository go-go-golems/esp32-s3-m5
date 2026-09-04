# Tasks

The active baseline is design-doc/03, the fresh first-principles native review. Earlier QuickJS/native task lists are preserved in `archive/01-tasks-before-native-review.md`; they are not parallel commitments. Firmware implementation has not started.

## Review and delivery

- [x] Move the complete ticket into esp32-s3-m5, verify hashes, repair structured source references, and commit both repositories.
- [x] Independently inspect semantic engines, compiler/registries/tests, full handheld prototype, product/deck examples, and actual firmware driver/config implementations.
- [x] Run existing PBUI baseline tests and retain actual-source probes and a sanitized C++ selection experiment.
- [x] Write a replacement intern guide explaining mathematical foundations, design patterns, compatibility changes, native APIs, LCD/keyboard architecture, budgets, failures, and phased implementation.
- [x] Update index, supersession banners, file relationships, changelog, and detailed diary; validate with docmgr doctor.
- [ ] Dry-run and upload the new guide to reMarkable; record successful upload evidence and delivery commit.

## Active native implementation backlog (not started)

- [ ] Phase A: host CMake harness, strong identities, references/liveness, checked buffers, and compatibility fixture matrix.
- [ ] Phase B: graph, selector, ordered conditions, fixed-rule resolver, statuses, provenance, capacity failures, and fresh action checks.
- [ ] Phase C: direct relations, complete catalog planning, command schema, receiver policies, slot filters and grammar.
- [ ] Phase D: hierarchical interaction/acquisition state, stable deck/view/occurrence identity, return points, correlated choices, tray/history, hints/search/peek, and safe input-loss handling.
- [ ] Phase E: logical rows, viewports, lowercase font, shared interaction frame, cells/raster, exact dirty comparison, semantic/text goldens, and failed-paint tests.
- [ ] Phase F: 0104 ESP-IDF 5.4.2 scaffold, explicit shared components, keyboard/application/console ownership, 40 MHz synchronous LCD adapter, replay acknowledgments, and measured memory/latency/queue evidence.
- [ ] Phase G: full prototype demo and all six semantic tutorials, independent-card state, temporal liveness, documented behavior corrections, and host/device layout-specific goldens.
- [ ] Phase H: second real product; only then justify deferred families, compositions, smaller font, display concurrency, persistence, or split layouts.

## Required hardware and design checks

- [ ] Confirm physical flash size and use a real partitions.csv with copied custom-partition defaults.
- [ ] Measure per-task internal stack peaks, largest free blocks, arena peaks, and queue pressure on the exact board.
- [ ] Verify lowercase r/R and p/P, glyph/color legibility, input release handling, and safe recovery after I2C failure.
- [ ] Qualify any future 80 MHz change independently; do not assume old bench timings apply to current wiring.
