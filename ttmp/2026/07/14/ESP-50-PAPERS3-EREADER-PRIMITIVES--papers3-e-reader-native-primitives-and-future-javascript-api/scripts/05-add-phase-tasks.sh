#!/usr/bin/env bash
set -euo pipefail

TICKET='ESP-50-PAPERS3-EREADER-PRIMITIVES'
existing="$(docmgr task list --ticket "$TICKET")"

while IFS= read -r task; do
  [[ -z "$task" ]] && continue
  if grep -Fq -- "$task" <<<"$existing"; then
    printf 'skip existing: %s\n' "$task"
    continue
  fi
  docmgr task add --ticket "$TICKET" --text "$task"
  existing+=$'\n'"$task"
done <<'TASKS'
[P0.1] Create a minimal standalone PaperS3 EPD qualification firmware with no reader/application dependencies
[P0.2] Record exact board revision, flash/PSRAM configuration, USB port identity, and reset/attach behavior
[P0.3] Create reproducible build configurations for matrix cells A-D with exact ESP-IDF, M5GFX, and M5Unified pins
[P0.4] Add boot diagnostics for display count, logical/physical size, rotation, free heap, largest DMA block, and PSRAM
[P0.5] Implement full white, full black, full white, grayscale bars, checkerboard, and text quality test scenes
[P0.6] Implement Issue 181 boundary tests for 1-16 pixel widths, all corners, edges, and portrait/landscape full ranges
[P0.7] Implement a mixed partial/full refresh soak command with heap integrity checks and timing counters
[P0.8] Implement display idle, sleep/power transition, and wake/reinitialization qualification commands
[P0.9] Run the identical visual/logging corpus for matrix cells A-D on real hardware and preserve photos/logs
[P0.10] Decide and document the accepted toolchain/component pin and any narrowly required local M5GFX patch
[P0.11] Add Phase 0 build/flash/probe scripts and component-SHA capture under the ticket scripts directory
[P0.12] Update the guide, README, diary, and changelog with measured Phase 0 results and review instructions
[P1.1] Create the next-numbered PaperS3 reader-primitives firmware directory and minimal ESP-IDF project files
[P1.2] Add sdkconfig.defaults for ESP32-S3, octal PSRAM, 16MB flash, custom partitions, and USB Serial/JTAG console
[P1.3] Pin the accepted Phase 0 M5GFX/M5Unified revisions reproducibly and commit dependencies.lock where applicable
[P1.4] Define bounded AppEvent, AppCommand, and AppReply message types with explicit payload ownership
[P1.5] Create the single UI/application owner task and prohibit display/model mutation from producer tasks
[P1.6] Route console commands through the command queue and return results through bounded reply queues
[P1.7] Add status, heap, display, event-queue, and task diagnostics to the USB console
[P1.8] Add explicit queue-full, reply-timeout, invalid-command, and shutdown behavior
[P1.9] Stress console and synthetic input producers concurrently and verify deterministic owner-task ordering
[P1.10] Document build, flash, monitor, architecture ownership, and validation commands in the firmware README
[P2.1] Define StatusCode, Status, and Result contracts without exceptions or silent boolean failures
[P2.2] Implement half-open Point, Size, Insets, and Rect types using overflow-safe intermediate arithmetic
[P2.3] Implement rectangle contains, intersection, union, clamp, empty, area, and rotation transforms
[P2.4] Implement and document EPD damage alignment based on Phase 0 measurements and driver constraints
[P2.5] Define bounded POD DrawOp variants and stable frame-arena references for text and bitmap payloads
[P2.6] Implement frame arena capacity accounting, lifetime rules, reset, and explicit overflow errors
[P2.7] Implement clip-stack validation and draw-op clipping independent of M5GFX
[P2.8] Implement a deterministic fake display backend that records normalized draw/present traces
[P2.9] Implement the M5 display backend transaction shell with wait, startWrite, endWrite, timeout, and recovery
[P2.10] Add host tests for geometry overflow, clipping, capacity limits, operation order, and fake-backend traces
[P2.11] Render the same primitive fixture through fake and M5 backends and preserve expected traces/screenshots
[P3.1] Define PresentIntent, EpdWaveform, RefreshReason, RefreshContext, RefreshPlan, and PresentResult contracts
[P3.2] Implement damage collection, bounds clamp, EPD alignment, overlap/nearby merge, and capacity fallback
[P3.3] Map semantic present intents to the waveform modes qualified in Phase 0
[P3.4] Instrument queue wait, render time, panel busy time, aligned area, mode, and cleanup reason for every present
[P3.5] Track turns, partial area, high-contrast area, elapsed time, screen changes, and wake state since full refresh
[P3.6] Implement clean-full triggers for first render, route changes, wake, explicit request, and configurable budget
[P3.7] Add refresh-policy console inspection and deterministic synthetic-history host tests
[P3.8] Build committed visual fixtures for checkerboards, gray bars, inverse text, folios, page pairs, corners, and edges
[P3.9] Run and capture a 10,000-update mixed refresh soak with heap integrity and timing summaries
[P3.10] Review photographs/logs, approve a baseline policy, and document known ghosting limits
[P4.1] Implement physical-to-logical touch coordinate transforms for every supported rotation
[P4.2] Define normalized PointerEvent down/move/up/cancel records with pointer ID and monotonic timestamps
[P4.3] Implement a pointer state machine that prevents duplicate taps and cancels stale/incomplete sequences
[P4.4] Implement configurable tap, long-press, and cardinal-swipe recognizers
[P4.5] Emit immutable hit regions from layout output and perform deepest/topmost deterministic hit testing
[P4.6] Implement one monotonic scheduler for region deadlines, persistence deadlines, and inactivity deadlines
[P4.7] Track last-input time and implement quiet/deferred region scheduling without direct timer drawing
[P4.8] Add recorded touch-trace fixtures and host replay tests for zones, gestures, cancellation, and rotation
[P4.9] Route normalized input/timer events through AppEvent and validate owner-task integration on hardware
[P5.1] Define reader typography requirements and compare candidate font formats for size, quality, metrics, and speed
[P5.2] Select, license, package, and document the initial regular reader font and diagnostic fallback font
[P5.3] Implement incremental UTF-8 decoding with replacement behavior and byte/codepoint position tracking
[P5.4] Define FontId, GlyphId, GlyphMetrics, GlyphBitmap, FontMetrics, and font-fallback contracts
[P5.5] Implement glyph measurement and loading so layout and rendering use the same metrics
[P5.6] Implement paragraph segmentation, whitespace normalization policy, and explicit paragraph spacing/indent rules
[P5.7] Implement measured line breaking with long-word handling and no split inside UTF-8 sequences
[P5.8] Emit stable GlyphRun draw operations with frame-arena-owned text/glyph data
[P5.9] Add host fixtures for lowercase, punctuation, malformed UTF-8, long words, Latin accents, and fallback glyphs
[P5.10] Compare host golden line breaks with PaperS3 output and approve body-text quality/refresh behavior
[P6.1] Implement non-destructive microSD initialization using the qualified PaperS3 pin/bus configuration
[P6.2] Implement ContentSource with Size, ReadAt, and Hash operations plus embedded-fixture and SD-text adapters
[P6.3] Define stable BookId and ContentHash derivation independent of transient library list ordering
[P6.4] Define a versioned, length-checked, checksummed BookRecord and catalog serialization format
[P6.5] Implement bounded SD library scanning, metadata defaults, duplicate handling, and deterministic sorting
[P6.6] Define settings and structured TextLocator persistence records with schema versions
[P6.7] Implement atomic temp/write/flush/rename/backup updates and recovery after interrupted writes
[P6.8] Keep disposable derived caches separate from catalog and critical resume state
[P6.9] Handle absent, removed, corrupt, and reinserted cards as recoverable application states
[P6.10] Add console commands to list, verify, rescan, inspect, and recover catalog/state without formatting media
[P6.11] Add host and hardware tests for corrupt records, interrupted writes, card removal, and remount
[P7.1] Define TextLocator, PageLayout, LineLayout, LayoutKey, and PageCountEstimate contracts
[P7.2] Include content hash, font, size, line height, margins, viewport, hyphenation, alignment, and engine version in LayoutKey
[P7.3] Implement forward page composition from a locator using the shared measured text layout pipeline
[P7.4] Implement next-page traversal with guaranteed forward progress and explicit end-of-content status
[P7.5] Implement previous-page traversal using sparse checkpoints and bounded backward reconstruction
[P7.6] Implement in-memory page/checkpoint caches with explicit capacity and eviction behavior
[P7.7] Implement non-blocking total-progress estimation without scanning the whole book before first display
[P7.8] Persist disposable pagination checkpoints keyed by LayoutKey and validate them before reuse
[P7.9] Invalidate/recover cached pages and resume locators after typography, viewport, engine, or content changes
[P7.10] Test empty, one-page, huge-paragraph, malformed, and multi-megabyte books for bounded memory and latency
[P7.11] Verify next/previous round trips preserve locator ranges across cache eviction and reboot
[P8.1] Define native reader application states for boot, card missing, library, opening, reading, error, and sleeping
[P8.2] Implement the library controller with stable BookId selection, title/author metadata, and reading progress
[P8.3] Implement the native library screen with empty, missing-card, corrupt-book, and selected-book states
[P8.4] Implement the reader controller with current book, PageLayout, locator, progress, and bookmark state
[P8.5] Implement the reading screen with measured body text, title, folio, progress, library action, and bookmark action
[P8.6] Wire left/right touch zones, library selection, back navigation, and bookmark gestures through AppEvent
[P8.7] Integrate refresh intents so page turns, route changes, errors, and overlays use the qualified planner
[P8.8] Restore the last valid book/locator on boot and coalesce position writes during reading
[P8.9] Route list/open/goto/info/refresh/bookmark console commands through the owner task
[P8.10] Execute the end-to-end TXT acceptance flow on hardware, including power cycle and resume
[P8.11] Record native vertical-slice screenshots, latency, heap, known limitations, and intern review instructions
[P9.1] Define typed widget variants for text, row, column, spacer, divider, progress, list, book, and region
[P9.2] Implement a bounded widget arena with generation-safe handles and explicit stale/capacity errors
[P9.3] Implement native builder helpers without embedding callbacks or transient borrowed pointers in nodes
[P9.4] Implement measured row/column layout, padding, gap, fixed/flexible sizing, alignment, and overflow rules
[P9.5] Compile laid-out widgets into flat frame-arena DrawOps and immutable hit regions
[P9.6] Implement previous/current render-state comparison and dependency-based damage invalidation
[P9.7] Implement named pages, header/content/footer/overlay slots, route push/back, and full-refresh route policy
[P9.8] Implement RegionSpec dependencies, intervals, quiet behavior, and scheduler integration
[P9.9] Implement native hello, status, library, and reader fixtures matching the studio's intended semantics
[P9.10] Migrate the Phase 8 reader onto the generic widget/page system without behavior regression
[P9.11] Add golden layout/draw-op/refresh-plan traces and hardware screenshots for all native fixtures
[P10.1] Define inactivity, explicit sleep, low-battery shutdown, and user-cancel power policies
[P10.2] Force pending locator/settings/catalog persistence before any power transition
[P10.3] Wait for display idle with timeout handling and render the selected retained sleep image
[P10.4] Quiesce timers, app events, storage activity, SD, and future script execution in a documented order
[P10.5] Verify and configure supported RTC/button wake sources for the actual PaperS3 board revision
[P10.6] Implement wake/reinitialize flow for display, touch, SD, catalog, reader state, and refresh history
[P10.7] Implement low-battery behavior using qualified battery/USB detection without corrupting state
[P10.8] Run repeated sleep/wake, shutdown-during-write, missing-card-on-wake, and low-battery simulations
[P10.9] Measure idle/standby behavior and document wake limitations, reset behavior, and operator recovery
[P11.1] Pin an exact MicroQuickJS commit and record source, license, build flags, and local integration strategy
[P11.2] Cross-compile a minimal MicroQuickJS context for ESP32-S3 without linking it into the production reader path
[P11.3] Measure context startup and failure behavior at several fixed memory-arena sizes
[P11.4] Bind one diagnostic C function and validate argument conversion, exceptions, logging, and stack checks
[P11.5] Bind one generation-safe opaque widget handle and validate finalization and stale-handle errors
[P11.6] Exercise compacting GC and audit every native JSValue lifetime with JSGCRef rooting discipline
[P11.7] Evaluate source execution and trusted relocated 32-bit bytecode; document compatibility/security constraints
[P11.8] Compile syntax probes for fluent chains, closures, var/let/const, arrows, modules, spread, and candidate transpiled output
[P11.9] Establish and test execution budget, cancellation/watchdog behavior, exception recovery, and runaway-script handling
[P11.10] Publish memory, latency, syntax, safety results and make an explicit proceed/postpone decision
[P12.1] Define and version the s3paper native ABI plus capability-query and compatibility behavior
[P12.2] Generate/register a minimal MicroQuickJS standard library containing only required s3paper and diagnostic APIs
[P12.3] Implement ES5-compatible fluent wrappers for paper, page, text, row, col, spacer, divider, progress, list, book, and region
[P12.4] Implement generation-safe JS wrappers and a rooted callback registry with deterministic teardown
[P12.5] Map dynamic values to CallbackId, DependencyId, and RegionId without storing JS closures in native layout nodes
[P12.6] Dispatch gesture, timer, route, and selection events to JS outside display transactions
[P12.7] Validate JS-produced patches/descriptors for type, bounds, capacity, ownership, and stale handles before applying them
[P12.8] Implement the host authoring/transpile/compile/relocate/embed pipeline pinned to the runtime commit
[P12.9] Port the hello, status, library, and reader studio programs to the supported on-device dialect
[P12.10] Compare native and JS normalized layout, DrawOp, hit-region, and refresh-plan traces
[P12.11] Test script exceptions, OOM, callback teardown, invalid descriptors, and app fallback to a native error screen
[P13.1] Consolidate host unit, golden trace, serialization, pagination, gesture, refresh, and JS binding tests into repeatable commands
[P13.2] Add malformed UTF-8, catalog, state, cache, TXT, and future EPUB fuzz/regression corpora
[P13.3] Run long-duration mixed refresh, navigation, storage, and heap-integrity soak tests on hardware
[P13.4] Run power-loss injection across catalog writes, position writes, page-cache writes, and sleep transitions
[P13.5] Saturate event, command, reply, draw-op, widget, callback, and scheduler capacities and verify explicit recovery
[P13.6] Run script OOM, exception storm, timeout, stale-handle, and repeated context create/destroy tests
[P13.7] Measure first-page latency, page-turn latency, panel busy time, heap high-water marks, and battery/idle behavior
[P13.8] Verify clean builds from the documented component pins and confirm build artifacts remain ignored
[P13.9] Complete the intern guide, architecture/API references, hardware operator playbook, and troubleshooting guide
[P13.10] Run final docmgr validation, code review checklist, firmware build, hardware acceptance, and reMarkable delivery
TASKS
