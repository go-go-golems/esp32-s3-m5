# Tasks

## Tracking conventions

- The 14 unprefixed `Phase N` tasks are milestone/exit-gate tasks. Check one only after all `[PN.x]` subtasks pass and the diary/changelog/review instructions are current.
- `[PN.x]` is the stable human-readable work-breakdown key; the `t:xxxx` comment is the immutable docmgr task ID used by `docmgr task check`.
- Work phases in order unless the diary records an explicit dependency-driven exception.
- Commit focused implementation changes at coherent subtask boundaries. Record code commit hashes in the diary, then commit the diary/task/changelog update separately when practical.
- Hardware-only tasks remain open until real-board evidence is saved under `scripts/output/` or a phase-specific evidence directory.

| Phase | Detailed tasks | Milestone task |
|---|---:|---|
| 0 | 12 | `la6t` |
| 1 | 10 | `q22x` |
| 2 | 11 | `t1yc` |
| 3 | 10 | `hdvv` |
| 4 | 9 | `a7sc` |
| 5 | 10 | `kkmo` |
| 6 | 11 | `mcac` |
| 7 | 11 | `0dxd` |
| 8 | 11 | `ylnh` |
| 9 | 11 | `3ysy` |
| 10 | 9 | `gnr0` |
| 11 | 10 | `r4zd` |
| 12 | 11 | `pj4p` |
| 13 | 10 | `9quv` |

**Total:** 14 milestone tasks plus 146 detailed tasks. At the start of a future session, run `docmgr task list --ticket ESP-50-PAPERS3-EREADER-PRIMITIVES`, find the first unchecked `[PN.x]`, and read the latest diary step before changing code.

## TODO

- [ ] Phase 0 - Freeze and document a reproducible PaperS3 hardware/toolchain/component qualification matrix before application work <!-- t:la6t -->
- [x] Phase 1 - Scaffold the new native reader-primitives firmware with one UI owner task, USB Serial/JTAG console, and message-based commands <!-- t:q22x -->
- [ ] Phase 2 - Implement defensive geometry, status/result types, display transactions, clipping, draw operations, and a deterministic fake display backend <!-- t:t1yc -->
- [ ] Phase 3 - Implement and hardware-qualify the EPD refresh planner, waveform selection, dirty-region alignment/merging, ghosting accounting, and visual test corpus <!-- t:hdvv -->
- [ ] Phase 4 - Implement normalized touch events, hit testing, gestures, input-idle tracking, timers, and quiet/deferred region scheduling <!-- t:a7sc -->
- [ ] Phase 5 - Implement font loading, UTF-8 decoding, glyph metrics, text measurement, line breaking, and deterministic page-layout fixtures <!-- t:kkmo -->
- [ ] Phase 6 - Implement SD-first content sources, a versioned library catalog, stable book identities, settings, and atomic position persistence <!-- t:mcac -->
- [ ] Phase 7 - Implement locator-based streaming pagination, page offset caches keyed by layout settings, next/previous traversal, and invalidation rules <!-- t:0dxd -->
- [ ] Phase 8 - Ship a native vertical slice with library, reading view, page turns, progress, bookmarks, and resume without any JavaScript runtime <!-- t:ylnh -->
- [ ] Phase 9 - Generalize the proven vertical slice into retained widget trees, layout, flat draw-op output, regions, dependency invalidation, and routable pages <!-- t:3ysy -->
- [ ] Phase 10 - Implement coordinated power-off/deep-sleep, wake sources, final persistence flush, display quiescence, and resume contracts <!-- t:gnr0 -->
- [ ] Phase 11 - Run a bounded MicroQuickJS feasibility spike covering ESP32-S3 integration, memory limits, C API rooting, syntax compatibility, cancellation, and trusted bytecode <!-- t:r4zd -->
- [ ] Phase 12 - Bind the stable primitive ABI into the fluent s3paper JavaScript layer and port the hello, status, library, and reader acceptance scripts <!-- t:pj4p -->
- [ ] Phase 13 - Harden with long-run ghosting, malformed-content, power-loss, heap, concurrency, latency, and battery tests; then publish intern and operator guides <!-- t:9quv -->
- [x] [P0.1] Create a minimal standalone PaperS3 EPD qualification firmware with no reader/application dependencies <!-- t:mxag -->
- [ ] [P0.2] Record exact board revision, flash/PSRAM configuration, USB port identity, and reset/attach behavior <!-- t:er7u -->
- [x] [P0.3] Create reproducible build configurations for matrix cells A-D with exact ESP-IDF, M5GFX, and M5Unified pins <!-- t:4s33 -->
- [x] [P0.4] Add boot diagnostics for display count, logical/physical size, rotation, free heap, largest DMA block, and PSRAM <!-- t:lat8 -->
- [x] [P0.5] Implement full white, full black, full white, grayscale bars, checkerboard, and text quality test scenes <!-- t:onog -->
- [x] [P0.6] Implement Issue 181 boundary tests for 1-16 pixel widths, all corners, edges, and portrait/landscape full ranges <!-- t:3w54 -->
- [x] [P0.7] Implement a mixed partial/full refresh soak command with heap integrity checks and timing counters <!-- t:krae -->
- [ ] [P0.8] Implement display idle, sleep/power transition, and wake/reinitialization qualification commands <!-- t:y655 -->
- [ ] [P0.9] Run the identical visual/logging corpus for matrix cells A-D on real hardware and preserve photos/logs <!-- t:io1c -->
- [ ] [P0.10] Decide and document the accepted toolchain/component pin and any narrowly required local M5GFX patch <!-- t:5cww -->
- [ ] [P0.11] Add Phase 0 build/flash/probe scripts and component-SHA capture under the ticket scripts directory <!-- t:lied -->
- [ ] [P0.12] Update the guide, README, diary, and changelog with measured Phase 0 results and review instructions <!-- t:6sog -->
- [x] [P1.1] Create the next-numbered PaperS3 reader-primitives firmware directory and minimal ESP-IDF project files <!-- t:p0mo -->
- [x] [P1.2] Add sdkconfig.defaults for ESP32-S3, octal PSRAM, 16MB flash, custom partitions, and USB Serial/JTAG console <!-- t:l97g -->
- [x] [P1.3] Pin the accepted Phase 0 M5GFX/M5Unified revisions reproducibly and commit dependencies.lock where applicable <!-- t:ambe -->
- [x] [P1.4] Define bounded AppEvent, AppCommand, and AppReply message types with explicit payload ownership <!-- t:x5vy -->
- [x] [P1.5] Create the single UI/application owner task and prohibit display/model mutation from producer tasks <!-- t:pt40 -->
- [x] [P1.6] Route console commands through the command queue and return results through bounded reply queues <!-- t:z15o -->
- [x] [P1.7] Add status, heap, display, event-queue, and task diagnostics to the USB console <!-- t:kvf8 -->
- [x] [P1.8] Add explicit queue-full, reply-timeout, invalid-command, and shutdown behavior <!-- t:x8lf -->
- [x] [P1.9] Stress console and synthetic input producers concurrently and verify deterministic owner-task ordering <!-- t:uc4q -->
- [x] [P1.10] Document build, flash, monitor, architecture ownership, and validation commands in the firmware README <!-- t:cw1v -->
- [x] [P2.1] Define StatusCode, Status, and Result contracts without exceptions or silent boolean failures <!-- t:3s70 -->
- [x] [P2.2] Implement half-open Point, Size, Insets, and Rect types using overflow-safe intermediate arithmetic <!-- t:7jda -->
- [x] [P2.3] Implement rectangle contains, intersection, union, clamp, empty, area, and rotation transforms <!-- t:84f7 -->
- [ ] [P2.4] Implement and document EPD damage alignment based on Phase 0 measurements and driver constraints <!-- t:lvjt -->
- [x] [P2.5] Define bounded POD DrawOp variants and stable frame-arena references for text and bitmap payloads <!-- t:5fc6 -->
- [x] [P2.6] Implement frame arena capacity accounting, lifetime rules, reset, and explicit overflow errors <!-- t:229c -->
- [x] [P2.7] Implement clip-stack validation and draw-op clipping independent of M5GFX <!-- t:vl4t -->
- [x] [P2.8] Implement a deterministic fake display backend that records normalized draw/present traces <!-- t:1f68 -->
- [x] [P2.9] Implement the M5 display backend transaction shell with wait, startWrite, endWrite, timeout, and recovery <!-- t:99ls -->
- [x] [P2.10] Add host tests for geometry overflow, clipping, capacity limits, operation order, and fake-backend traces <!-- t:u2yz -->
- [ ] [P2.11] Render the same primitive fixture through fake and M5 backends and preserve expected traces/screenshots <!-- t:tb0m -->
- [x] [P3.1] Define PresentIntent, EpdWaveform, RefreshReason, RefreshContext, RefreshPlan, and PresentResult contracts <!-- t:1n6u -->
- [x] [P3.2] Implement damage collection, bounds clamp, EPD alignment, overlap/nearby merge, and capacity fallback <!-- t:t14u -->
- [ ] [P3.3] Map semantic present intents to the waveform modes qualified in Phase 0 <!-- t:1ow9 -->
- [ ] [P3.4] Instrument queue wait, render time, panel busy time, aligned area, mode, and cleanup reason for every present <!-- t:k7og -->
- [x] [P3.5] Track turns, partial area, high-contrast area, elapsed time, screen changes, and wake state since full refresh <!-- t:91ka -->
- [x] [P3.6] Implement clean-full triggers for first render, route changes, wake, explicit request, and configurable budget <!-- t:o3n6 -->
- [x] [P3.7] Add refresh-policy console inspection and deterministic synthetic-history host tests <!-- t:6w1j -->
- [ ] [P3.8] Build committed visual fixtures for checkerboards, gray bars, inverse text, folios, page pairs, corners, and edges <!-- t:cmmr -->
- [x] [P3.9] Run and capture a 10,000-update mixed refresh soak with heap integrity and timing summaries <!-- t:g16p -->
- [ ] [P3.10] Review photographs/logs, approve a baseline policy, and document known ghosting limits <!-- t:71dg -->
- [ ] [P4.1] Implement physical-to-logical touch coordinate transforms for every supported rotation <!-- t:omr2 -->
- [ ] [P4.2] Define normalized PointerEvent down/move/up/cancel records with pointer ID and monotonic timestamps <!-- t:prii -->
- [ ] [P4.3] Implement a pointer state machine that prevents duplicate taps and cancels stale/incomplete sequences <!-- t:91av -->
- [ ] [P4.4] Implement configurable tap, long-press, and cardinal-swipe recognizers <!-- t:nju8 -->
- [ ] [P4.5] Emit immutable hit regions from layout output and perform deepest/topmost deterministic hit testing <!-- t:dfp0 -->
- [ ] [P4.6] Implement one monotonic scheduler for region deadlines, persistence deadlines, and inactivity deadlines <!-- t:mo55 -->
- [ ] [P4.7] Track last-input time and implement quiet/deferred region scheduling without direct timer drawing <!-- t:cox7 -->
- [ ] [P4.8] Add recorded touch-trace fixtures and host replay tests for zones, gestures, cancellation, and rotation <!-- t:ox8h -->
- [ ] [P4.9] Route normalized input/timer events through AppEvent and validate owner-task integration on hardware <!-- t:utsz -->
- [ ] [P5.1] Define reader typography requirements and compare candidate font formats for size, quality, metrics, and speed <!-- t:3r0u -->
- [ ] [P5.2] Select, license, package, and document the initial regular reader font and diagnostic fallback font <!-- t:19jk -->
- [ ] [P5.3] Implement incremental UTF-8 decoding with replacement behavior and byte/codepoint position tracking <!-- t:pav8 -->
- [ ] [P5.4] Define FontId, GlyphId, GlyphMetrics, GlyphBitmap, FontMetrics, and font-fallback contracts <!-- t:lt14 -->
- [ ] [P5.5] Implement glyph measurement and loading so layout and rendering use the same metrics <!-- t:yqtu -->
- [ ] [P5.6] Implement paragraph segmentation, whitespace normalization policy, and explicit paragraph spacing/indent rules <!-- t:bhb3 -->
- [ ] [P5.7] Implement measured line breaking with long-word handling and no split inside UTF-8 sequences <!-- t:vwfp -->
- [ ] [P5.8] Emit stable GlyphRun draw operations with frame-arena-owned text/glyph data <!-- t:t0p5 -->
- [ ] [P5.9] Add host fixtures for lowercase, punctuation, malformed UTF-8, long words, Latin accents, and fallback glyphs <!-- t:xip7 -->
- [ ] [P5.10] Compare host golden line breaks with PaperS3 output and approve body-text quality/refresh behavior <!-- t:zfpj -->
- [ ] [P6.1] Implement non-destructive microSD initialization using the qualified PaperS3 pin/bus configuration <!-- t:11x4 -->
- [ ] [P6.2] Implement ContentSource with Size, ReadAt, and Hash operations plus embedded-fixture and SD-text adapters <!-- t:ryu3 -->
- [ ] [P6.3] Define stable BookId and ContentHash derivation independent of transient library list ordering <!-- t:7ji5 -->
- [ ] [P6.4] Define a versioned, length-checked, checksummed BookRecord and catalog serialization format <!-- t:i78k -->
- [ ] [P6.5] Implement bounded SD library scanning, metadata defaults, duplicate handling, and deterministic sorting <!-- t:ll6s -->
- [ ] [P6.6] Define settings and structured TextLocator persistence records with schema versions <!-- t:1y51 -->
- [ ] [P6.7] Implement atomic temp/write/flush/rename/backup updates and recovery after interrupted writes <!-- t:lyfu -->
- [ ] [P6.8] Keep disposable derived caches separate from catalog and critical resume state <!-- t:atj4 -->
- [ ] [P6.9] Handle absent, removed, corrupt, and reinserted cards as recoverable application states <!-- t:gky6 -->
- [ ] [P6.10] Add console commands to list, verify, rescan, inspect, and recover catalog/state without formatting media <!-- t:jxg5 -->
- [ ] [P6.11] Add host and hardware tests for corrupt records, interrupted writes, card removal, and remount <!-- t:5zpj -->
- [ ] [P7.1] Define TextLocator, PageLayout, LineLayout, LayoutKey, and PageCountEstimate contracts <!-- t:1lxo -->
- [ ] [P7.2] Include content hash, font, size, line height, margins, viewport, hyphenation, alignment, and engine version in LayoutKey <!-- t:yqb2 -->
- [ ] [P7.3] Implement forward page composition from a locator using the shared measured text layout pipeline <!-- t:tgse -->
- [ ] [P7.4] Implement next-page traversal with guaranteed forward progress and explicit end-of-content status <!-- t:vvwh -->
- [ ] [P7.5] Implement previous-page traversal using sparse checkpoints and bounded backward reconstruction <!-- t:abca -->
- [ ] [P7.6] Implement in-memory page/checkpoint caches with explicit capacity and eviction behavior <!-- t:hfge -->
- [ ] [P7.7] Implement non-blocking total-progress estimation without scanning the whole book before first display <!-- t:kt5h -->
- [ ] [P7.8] Persist disposable pagination checkpoints keyed by LayoutKey and validate them before reuse <!-- t:07pv -->
- [ ] [P7.9] Invalidate/recover cached pages and resume locators after typography, viewport, engine, or content changes <!-- t:g964 -->
- [ ] [P7.10] Test empty, one-page, huge-paragraph, malformed, and multi-megabyte books for bounded memory and latency <!-- t:vh8v -->
- [ ] [P7.11] Verify next/previous round trips preserve locator ranges across cache eviction and reboot <!-- t:4otq -->
- [ ] [P8.1] Define native reader application states for boot, card missing, library, opening, reading, error, and sleeping <!-- t:djij -->
- [ ] [P8.2] Implement the library controller with stable BookId selection, title/author metadata, and reading progress <!-- t:p2kg -->
- [ ] [P8.3] Implement the native library screen with empty, missing-card, corrupt-book, and selected-book states <!-- t:j8d5 -->
- [ ] [P8.4] Implement the reader controller with current book, PageLayout, locator, progress, and bookmark state <!-- t:ajl0 -->
- [ ] [P8.5] Implement the reading screen with measured body text, title, folio, progress, library action, and bookmark action <!-- t:7qyr -->
- [ ] [P8.6] Wire left/right touch zones, library selection, back navigation, and bookmark gestures through AppEvent <!-- t:3c5c -->
- [ ] [P8.7] Integrate refresh intents so page turns, route changes, errors, and overlays use the qualified planner <!-- t:sayr -->
- [ ] [P8.8] Restore the last valid book/locator on boot and coalesce position writes during reading <!-- t:sda9 -->
- [ ] [P8.9] Route list/open/goto/info/refresh/bookmark console commands through the owner task <!-- t:25f9 -->
- [ ] [P8.10] Execute the end-to-end TXT acceptance flow on hardware, including power cycle and resume <!-- t:9gye -->
- [ ] [P8.11] Record native vertical-slice screenshots, latency, heap, known limitations, and intern review instructions <!-- t:r3wg -->
- [ ] [P9.1] Define typed widget variants for text, row, column, spacer, divider, progress, list, book, and region <!-- t:fopv -->
- [ ] [P9.2] Implement a bounded widget arena with generation-safe handles and explicit stale/capacity errors <!-- t:h6i3 -->
- [ ] [P9.3] Implement native builder helpers without embedding callbacks or transient borrowed pointers in nodes <!-- t:ybmi -->
- [ ] [P9.4] Implement measured row/column layout, padding, gap, fixed/flexible sizing, alignment, and overflow rules <!-- t:d5dy -->
- [ ] [P9.5] Compile laid-out widgets into flat frame-arena DrawOps and immutable hit regions <!-- t:xdgp -->
- [ ] [P9.6] Implement previous/current render-state comparison and dependency-based damage invalidation <!-- t:aofv -->
- [ ] [P9.7] Implement named pages, header/content/footer/overlay slots, route push/back, and full-refresh route policy <!-- t:anwd -->
- [ ] [P9.8] Implement RegionSpec dependencies, intervals, quiet behavior, and scheduler integration <!-- t:vzbo -->
- [ ] [P9.9] Implement native hello, status, library, and reader fixtures matching the studio's intended semantics <!-- t:2awq -->
- [ ] [P9.10] Migrate the Phase 8 reader onto the generic widget/page system without behavior regression <!-- t:zxxd -->
- [ ] [P9.11] Add golden layout/draw-op/refresh-plan traces and hardware screenshots for all native fixtures <!-- t:n7h2 -->
- [ ] [P10.1] Define inactivity, explicit sleep, low-battery shutdown, and user-cancel power policies <!-- t:9i89 -->
- [ ] [P10.2] Force pending locator/settings/catalog persistence before any power transition <!-- t:teie -->
- [ ] [P10.3] Wait for display idle with timeout handling and render the selected retained sleep image <!-- t:078s -->
- [ ] [P10.4] Quiesce timers, app events, storage activity, SD, and future script execution in a documented order <!-- t:542x -->
- [ ] [P10.5] Verify and configure supported RTC/button wake sources for the actual PaperS3 board revision <!-- t:go0n -->
- [ ] [P10.6] Implement wake/reinitialize flow for display, touch, SD, catalog, reader state, and refresh history <!-- t:kark -->
- [ ] [P10.7] Implement low-battery behavior using qualified battery/USB detection without corrupting state <!-- t:tv0p -->
- [ ] [P10.8] Run repeated sleep/wake, shutdown-during-write, missing-card-on-wake, and low-battery simulations <!-- t:1k7u -->
- [ ] [P10.9] Measure idle/standby behavior and document wake limitations, reset behavior, and operator recovery <!-- t:5fwn -->
- [ ] [P11.1] Pin an exact MicroQuickJS commit and record source, license, build flags, and local integration strategy <!-- t:zyxp -->
- [ ] [P11.2] Cross-compile a minimal MicroQuickJS context for ESP32-S3 without linking it into the production reader path <!-- t:atze -->
- [ ] [P11.3] Measure context startup and failure behavior at several fixed memory-arena sizes <!-- t:868e -->
- [ ] [P11.4] Bind one diagnostic C function and validate argument conversion, exceptions, logging, and stack checks <!-- t:durp -->
- [ ] [P11.5] Bind one generation-safe opaque widget handle and validate finalization and stale-handle errors <!-- t:dygk -->
- [ ] [P11.6] Exercise compacting GC and audit every native JSValue lifetime with JSGCRef rooting discipline <!-- t:vq48 -->
- [ ] [P11.7] Evaluate source execution and trusted relocated 32-bit bytecode; document compatibility/security constraints <!-- t:m1w2 -->
- [ ] [P11.8] Compile syntax probes for fluent chains, closures, var/let/const, arrows, modules, spread, and candidate transpiled output <!-- t:0fdb -->
- [ ] [P11.9] Establish and test execution budget, cancellation/watchdog behavior, exception recovery, and runaway-script handling <!-- t:dzfz -->
- [ ] [P11.10] Publish memory, latency, syntax, safety results and make an explicit proceed/postpone decision <!-- t:s384 -->
- [ ] [P12.1] Define and version the s3paper native ABI plus capability-query and compatibility behavior <!-- t:qoou -->
- [ ] [P12.2] Generate/register a minimal MicroQuickJS standard library containing only required s3paper and diagnostic APIs <!-- t:wiie -->
- [ ] [P12.3] Implement ES5-compatible fluent wrappers for paper, page, text, row, col, spacer, divider, progress, list, book, and region <!-- t:5toh -->
- [ ] [P12.4] Implement generation-safe JS wrappers and a rooted callback registry with deterministic teardown <!-- t:xmua -->
- [ ] [P12.5] Map dynamic values to CallbackId, DependencyId, and RegionId without storing JS closures in native layout nodes <!-- t:0yat -->
- [ ] [P12.6] Dispatch gesture, timer, route, and selection events to JS outside display transactions <!-- t:pqfs -->
- [ ] [P12.7] Validate JS-produced patches/descriptors for type, bounds, capacity, ownership, and stale handles before applying them <!-- t:muo1 -->
- [ ] [P12.8] Implement the host authoring/transpile/compile/relocate/embed pipeline pinned to the runtime commit <!-- t:ibe5 -->
- [ ] [P12.9] Port the hello, status, library, and reader studio programs to the supported on-device dialect <!-- t:wipy -->
- [ ] [P12.10] Compare native and JS normalized layout, DrawOp, hit-region, and refresh-plan traces <!-- t:17nn -->
- [ ] [P12.11] Test script exceptions, OOM, callback teardown, invalid descriptors, and app fallback to a native error screen <!-- t:rs5w -->
- [ ] [P13.1] Consolidate host unit, golden trace, serialization, pagination, gesture, refresh, and JS binding tests into repeatable commands <!-- t:4u67 -->
- [ ] [P13.2] Add malformed UTF-8, catalog, state, cache, TXT, and future EPUB fuzz/regression corpora <!-- t:666s -->
- [ ] [P13.3] Run long-duration mixed refresh, navigation, storage, and heap-integrity soak tests on hardware <!-- t:ge9q -->
- [ ] [P13.4] Run power-loss injection across catalog writes, position writes, page-cache writes, and sleep transitions <!-- t:jcv3 -->
- [ ] [P13.5] Saturate event, command, reply, draw-op, widget, callback, and scheduler capacities and verify explicit recovery <!-- t:srtu -->
- [ ] [P13.6] Run script OOM, exception storm, timeout, stale-handle, and repeated context create/destroy tests <!-- t:2lez -->
- [ ] [P13.7] Measure first-page latency, page-turn latency, panel busy time, heap high-water marks, and battery/idle behavior <!-- t:jyhq -->
- [ ] [P13.8] Verify clean builds from the documented component pins and confirm build artifacts remain ignored <!-- t:8t9i -->
- [ ] [P13.9] Complete the intern guide, architecture/API references, hardware operator playbook, and troubleshooting guide <!-- t:lrvm -->
- [ ] [P13.10] Run final docmgr validation, code review checklist, firmware build, hardware acceptance, and reMarkable delivery <!-- t:y5v5 -->
- [x] Publish the complete PaperS3 EPD investigation as a textbook-style Obsidian research article <!-- t:dv3y -->
- [x] [P0.13] Audit the pinned EPD_Painter PaperS3 waveform, scan timing, power sequencing, memory ownership, and cleanup/DC-balance behavior before hardware use <!-- t:i55j -->
- [x] [P0.14] Write the independent-driver experiment design with safety gates, controlled transitions, area matrix, optical capture, and explicit claims each result can support <!-- t:jetk -->
- [x] [P0.15] Create and reproducibly build a minimal pinned PaperS3 independent-driver control firmware without application, network, storage, or touch dependencies <!-- t:4v4e -->
- [x] [P0.16] Add bounded serial commands for hard cleanup, HIGH white/black, area fractions, reader page, status, and final DC-balanced cleanup <!-- t:v19b -->
- [ ] [P0.17] Flash and run the minimal independent-driver smoke sequence on hardware, preserving exact build metadata, transcript, timing, heap, and operator observations <!-- t:j92a -->
- [ ] [P0.18] Run controlled origin-target and area-load optical fixtures; decide whether waveform behavior or analog rail/VCOM measurement is the next branch <!-- t:6b2c -->
- [x] [P0.17a] Decode and hash M5GFX 0.2.15/0.2.25 PaperS3 LUTs, bus configuration, and power ordering without hardware access <!-- t:qg59 -->
- [x] [P0.17b] Design compile-time-gated nonblocking M5GFX runtime trace events and fixed-ring schema <!-- t:ekmz -->
- [x] [P0.17c] Implement trace-timing hooks for enqueue/dequeue, eraser/target arming, power, frame, and idle events without scan-loop printing <!-- t:k0uh -->
- [x] [P0.17d] Build and audit trace-disabled and trace-timing M5GFX controls; quantify binary/IRAM and observer-effect differences before flash <!-- t:v5zs -->
- [x] [P0.17e] Create immutable per-run experiment directories with preregistration, manifest JSON, JSONL events, optical evidence, hashes, and separate automatic/optical dispositions <!-- t:onob -->
- [ ] [P0.17f] Create 0109 FactoryTest V0.5 lineage control under exact IDF 5.3.3: replay/video exact merged F0, then build/audit stock-source trace-off F1 and trace-on F2 preserving the black-to-white-to-grayscale boot sequence; capture ring only from F2 and keep all runs separately labeled <!-- t:6nak -->
- [ ] [P0.17g] Design reviewed external logic/rail/VCOM capture for the unmodified factory binary <!-- t:31al -->
- [x] [P0.17f.1] Install and verify exact ESP-IDF v5.3.3, then build matrix Cells A/B without flashing <!-- t:cv8d -->
- [x] [P0.19] Preregister and implement a native EPD_Painter fixed-aperture white-black-white density step-response firmware <!-- t:jtk1 -->
- [x] [P0.19.1] Build/audit exact direct-driver artifact and preserve binary identity <!-- t:bflw -->
- [x] [P0.19.2] Run one fixed-head synchronized density/firmware-marker capture with safe serial ownership <!-- t:ca9i -->
- [ ] [P0.19.3] Analyze fixed-point step response and record operator/optical disposition <!-- t:utzh -->
- [ ] [P0.20] Run a native direct-driver 2-bit gray-code ladder (00→55→AA→FF→00) with synchronized fixed-aperture density evidence <!-- t:1qly -->
