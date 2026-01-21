---
Title: Diary
Ticket: 0059-WS281X-COMPONENT
Status: active
Topics:
    - esp32c6
    - esp-idf
    - leds
    - rmt
    - component
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0049-xiao-esp32c6-mled-node/CMakeLists.txt
      Note: EXTRA_COMPONENT_DIRS wiring for ws281x + mled_node
    - Path: 0049-xiao-esp32c6-mled-node/main/mled_effect_led.c
      Note: Protocol PatternConfig -> ws281x component adapter
    - Path: components/ws281x/CMakeLists.txt
      Note: Component wiring and public REQUIRES
    - Path: components/ws281x/src/led_task.c
      Note: WS281x owner task + queue
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-21T16:46:39.053351492-05:00
WhatFor: ""
WhenToUse: ""
---


# Diary

## Goal

Capture the end-to-end work log for ticket `0059-WS281X-COMPONENT`: extracting the WS281x driver + pattern engine (currently embedded in `0049` and originally developed in `0044`) into a reusable ESP-IDF component under `esp32-s3-m5/components/`, then validating behavior via the `0049` MLED node host tools.

## Context

- `0049-xiao-esp32c6-mled-node` now drives WS281x LEDs by embedding copies of:
  - `led_ws281x.*`, `ws281x_encoder.*` (RMT TX driver + encoder)
  - `led_patterns.*` (pattern renderer)
  - `led_task.*` (single-owner task + queue)
  - `mled_effect_led.*` (adapter: MLED PatternConfig → led_task)
- We want to reuse that engine in other firmwares without copy/paste drift (same motivation as `components/mled_node`).

## Step 1: Create ticket workspace + define extraction tasks

Created the docmgr workspace and wrote a concrete checklist: create a component skeleton, move the WS281x sources into it, update `0049` to depend on it, and validate via build + flash + host pattern smoke tests.

### What I did
- Created ticket `0059-WS281X-COMPONENT`.
- Added initial docs (design doc, diary, validation playbook).
- Replaced `tasks.md` with an explicit extraction checklist.

### Why
- Keeping the steps mechanical reduces the chance of subtly breaking timing-sensitive WS281x behavior.

### What should be done next
- Implement the `components/ws281x` skeleton and move the engine sources.

## Step 2: Extract WS281x engine into `components/ws281x` and update 0049 to consume it

This step turns the WS281x driver/pattern engine into a proper ESP-IDF component so that other firmwares can reuse it without copy/paste drift. The `0049-xiao-esp32c6-mled-node` firmware is updated to depend on the component, leaving the protocol→LED mapping adapter (`mled_effect_led.c`) in the firmware.

**Commit (code):** 17b83eb — "ws281x: extract engine into reusable component"

### What I did
- Created `components/ws281x/`:
  - `CMakeLists.txt` registers the sources and public include dir.
  - Public headers under `components/ws281x/include/`:
    - `led_task.h`, `led_patterns.h`, `led_ws281x.h`, `ws281x_encoder.h`
  - Sources under `components/ws281x/src/`:
    - `led_task.c`, `led_patterns.c`, `led_ws281x.c`, `ws281x_encoder.c`
- Moved the WS281x engine files out of `0049-xiao-esp32c6-mled-node/main/` into the component using `git mv` (no behavioral edits intended).
- Updated `0049-xiao-esp32c6-mled-node` build wiring:
  - Added `${CMAKE_CURRENT_LIST_DIR}/../components/ws281x` to `EXTRA_COMPONENT_DIRS`.
  - Updated `main/CMakeLists.txt` to `PRIV_REQUIRES ws281x` and removed the now-moved sources from `SRCS`.
- Fixed a build-system gotcha: because consumer code includes component headers that themselves include `driver/rmt_tx.h`, the component must declare `esp_driver_rmt` as a public dependency (`REQUIRES`, not `PRIV_REQUIRES`) so that consumers inherit the include path.

### Why
- We want a single shared WS281x implementation used across firmwares (similar to `components/mled_node`), so fixes/features land once.

### What worked
- Build validation succeeded:
  - `source ~/esp/esp-idf-5.4.1/export.sh`
  - `idf.py -C 0049-xiao-esp32c6-mled-node build`

### What didn't work
- First build attempt failed with:
  - `fatal error: driver/rmt_tx.h: No such file or directory`
- Root cause: `esp_driver_rmt` was declared as a private requirement in the component and did not propagate include paths to consumers.
- Fix: make it a public `REQUIRES` in `components/ws281x/CMakeLists.txt`.

### What I learned
- If a component’s public headers include headers from other components, those dependencies must be declared under `REQUIRES` (public), not `PRIV_REQUIRES`.

### What was tricky to build
- Dependency propagation in ESP-IDF components: “private” requirements are not sufficient when downstream code includes the component’s public headers.

### What warrants a second pair of eyes
- Verify the component boundary is correct: we intentionally did not move `mled_effect_led.*` into the component because it is MLED-specific.

### What should be done in the future
- Optionally migrate `0044`/`0046` to use `components/ws281x` too (separate change, to keep this ticket focused).

### Code review instructions
- Start at `components/ws281x/CMakeLists.txt` and confirm `REQUIRES esp_driver_rmt`.
- Confirm `0049-xiao-esp32c6-mled-node` no longer contains the engine sources in `main/`, and builds cleanly.

## Step 3: Debug discovery regression (no PONGs) with instrumentation + host bind option

After the extraction, a “no PONGs received” symptom can happen even when the device is reachable by ICMP ping. The fastest way to separate “host multicast routing problem” from “node send/receive problem” is:
1) instrument the node to log the first few PINGs it receives (and log send errors), and
2) allow the host tools to force the multicast egress interface/IP to avoid multi-NIC routing surprises.

**Commit (code):** 8c5b566 — "mled_node: log first PINGs and pong send errors"  
**Commit (code):** 07edd6a — "tools: add --bind-ip for multicast routing"

### What I did
- Added limited PING receipt logging in `components/mled_node/src/mled_node.c`:
  - logs the first 5 PINGs with source IP:port
  - logs a warning if `send_pong` fails with `errno`
- Added `--bind-ip` to host tools:
  - `tools/mled_ping.py --bind-ip <host-ip>`
  - `tools/mled_smoke.py --bind-ip <host-ip>`
  - also sets `IP_MULTICAST_IF` so multicast egress is deterministic.

### Why
- On hosts with multiple interfaces (Wi‑Fi + Ethernet + VPN + Docker), multicast packets can leave on the “wrong” interface and nodes may reply to an unreachable source IP/port.

### What should be done next
- Flash the latest `0049` build and run:
  - `python3 tools/mled_ping.py --timeout 2 --repeat 2`
  - If no PONGs, retry with `--bind-ip <your-host-lan-ip>` and check device logs for `mled_node: PING ...` lines.

## Step 4: Fix stack protection fault in `mled_node` during cue runs

During host-driven cue runs (after the WS281x extraction), the device hit a `Guru Meditation Error: Stack protection fault` in the `mled_node` task shortly after `CUE_FIRE`. This is consistent with the node task stack being too small / too close to the guard, especially with a 2KB receive buffer allocated on the task stack and additional logging/handler call depth.

**Commit (code):** e576d5f — "mled_node: reduce stack use and bump task stack"

### What I did
- Reduced `mled_node` task stack pressure by moving the 2048-byte RX buffer off the task stack into a static buffer.
- Increased the node task stack size (and made it configurable via `CONFIG_MLED_NODE_TASK_STACK`, with a safe fallback default).

### Why
- Stack protection faults are often “Heisenbugs”: adding logs or slightly different timing can tip a marginal stack into overflow.
- Reducing stack usage at the source is more reliable than chasing individual call paths.

### What should be done next
- Re-flash and re-run the pattern smoke sequence; confirm no panics and that PING/PONG + ACK/APPLY continue to work.

## Quick Reference

N/A (see design doc + playbook).

## Usage Examples

N/A.

## Related

- Consumer firmware: `esp32-s3-m5/0049-xiao-esp32c6-mled-node/`
- Original engine work: `esp32-s3-m5/0044-xiao-esp32c6-ws281x-patterns-console/`
- Node protocol component: `esp32-s3-m5/components/mled_node/`
