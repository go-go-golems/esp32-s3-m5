---
Title: 'Research: ESP-IDF host/Linux builds for fast iteration (typewriter)'
Ticket: 0011-HOST-DEBUG-COMPILE
Status: active
Topics:
    - esp-idf
    - linux
    - host
    - testing
    - ui
    - cardputer
    - keyboard
    - display
    - m5gfx
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../../../esp/esp-idf-5.4.1/examples/protocols/sockets/tcp_client/main/tcp_client_v6.c
      Note: 'Pattern for #if defined(CONFIG_IDF_TARGET_LINUX) host-specific code'
    - Path: ../../../../../../../../../../esp/esp-idf-5.4.1/examples/protocols/sockets/udp_client/CMakeLists.txt
      Note: Pattern for selecting COMPONENTS when IDF_TARGET=linux
    - Path: echo-base--openai-realtime-embedded-sdk/components/esp-protocols/components/esp_websocket_client/examples/linux/README.md
      Note: Example of ESP-IDF linux target build + run ELF directly
    - Path: echo-base--openai-realtime-embedded-sdk/components/esp-protocols/examples/mqtt/README.md
      Note: Example of ESP-IDF linux target build + run via idf.py monitor
    - Path: echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/22/006-CARDPUTER-TYPEWRITER--cardputer-typewriter-keyboard-on-screen-text/analysis/01-setup-architecture-cardputer-typewriter-keyboard-on-screen-text.md
      Note: Typewriter architecture baseline (device)
    - Path: esp32-s3-m5/0007-cardputer-keyboard-serial/main/hello_world_main.c
      Note: Keyboard scan logic to extract into shared core
    - Path: esp32-s3-m5/0011-cardputer-m5gfx-plasma-animation/main/hello_world_main.cpp
      Note: M5GFX autodetect + sprite present baseline
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-23T19:11:44.540672439-05:00
WhatFor: ""
WhenToUse: ""
---


## Executive summary

You can iterate on the Cardputer typewriter behavior without flashing by building a **host-runner** that reuses the same “typewriter core” logic as the ESP32 firmware.

ESP-IDF offers **two complementary host-side workflows**:

1) **Linux target (“host apps”, preview)**: `idf.py --preview set-target linux` builds an ELF that runs on your dev machine (no flash). This is useful if you want to keep the ESP-IDF build system and reuse select portable IDF components.

2) **Host tests**: compile and run pure logic on the host (unit tests / golden tests) without any device drivers. This is ideal for verifying typewriter semantics quickly.

For the typewriter specifically, the most productive structure is:

- Extract a **platform-agnostic typewriter core** (text buffer + cursor rules + key-to-token mapping) with **no ESP-IDF/M5GFX headers**.
- Build two thin “frontends”:
  - **Device frontend (ESP32-S3)**: keyboard scan → core → render via M5GFX canvas.
  - **Host frontend (Linux)**: stdin key events → core → render via a terminal UI (ncurses) or a simple stdout renderer.

This gives you “flashless” iteration on behavior while keeping hardware integration (keyboard GPIO scan + LCD present) as a separately validated layer.

## Problem statement

### Desired workflow

- Edit typewriter behavior (buffer rules, backspace/enter semantics, scrolling, cursor).
- Run instantly on host to validate behavior (preferably with repeatable tests).
- Only flash when validating hardware integration (keyboard scan GPIO and M5GFX present).

### Constraints

- M5GFX is hardware-focused; it’s not expected to compile or run “as-is” on Linux.
- Keyboard scan on device depends on GPIO; host must emulate key events.

## ESP-IDF-supported host workflows (what exists today)

### A) ESP-IDF Linux target (“host apps”)

ESP-IDF supports building some applications for a `linux` target. In multiple READMEs, this is called **preview** and uses:

```bash
idf.py --preview set-target linux
```

**How to run**

The examples show two common “run” approaches:

- Run the produced ELF directly (e.g. `./websocket.elf`)
- Or use `idf.py monitor` as a runner wrapper (no flash) for host apps

**Repo-local evidence**

- `esp_websocket_client` linux example:
  - `idf.py --preview set-target linux`
  - `idf.py build`
  - `./websocket.elf`
- `mqtt` linux example:
  - explicitly says you can run the project using `idf.py monitor` “no need to flash”

**Key technique: limit components for linux**

ESP-IDF examples sometimes select a different component set under Linux using the `IDF_TARGET` CMake variable. Example:

```text
if("${IDF_TARGET}" STREQUAL "linux")
    set(COMPONENTS main esp_netif lwip protocol_examples_tapif_io startup esp_hw_support esp_system nvs_flash)
endif()
```

(from the ESP-IDF `udp_client` example `CMakeLists.txt`).

This matters because many ESP-IDF components are MCU-specific; a host build often needs a smaller/different component set.

**Code-level conditionals**

ESP-IDF examples use:

- `#if defined(CONFIG_IDF_TARGET_LINUX)` for host-specific code paths

(see `tcp_client_v6.c`).

### B) Host tests

ESP-IDF’s examples include host-test markers (e.g. `@pytest.mark.host_test`), and ESP-IDF has guidance for Linux host testing. For our goal, host tests are the fastest loop for validating:

- key sequences → resulting buffer state
- backspace/enter edge cases
- scrolling/windowing logic

### C) QEMU (optional)

ESP-IDF also supports a QEMU workflow for some targets. This can be useful if you want a “more firmware-like” runtime on the host, but it’s usually heavier than a simple host runner for a text UI.

## Recommended design for “typewriter on host + device”

### 1) Split into core + adapters

**`typewriter_core` (portable library)**

- Inputs:
  - either raw key events (`KeyDown/KeyUp + modifiers`)
  - or higher-level tokens (`InsertChar('a')`, `Enter`, `Backspace`)
- State:
  - ring of lines + cursor position
- Outputs:
  - updated state or “dirty” signals (“needs redraw”)

**Device adapter**

- keyboard scan loop: reuse the robust scan/debounce logic from `esp32-s3-m5/0007-cardputer-keyboard-serial`
- render: reuse the full-screen `M5Canvas` pattern from `esp32-s3-m5/0011-cardputer-m5gfx-plasma-animation`

**Host adapter**

- input: stdin raw mode / ncurses
- render: terminal grid renderer (fast, deterministic)

### 2) Don’t try to make M5GFX run on Linux

Instead of porting M5GFX, keep the host runner’s rendering minimal. The shared part is the **behavior**, not the pixels.

### 3) Testing strategy

- **Golden tests**: feed a sequence of tokens, assert final lines/cursor.
- **Invariants**:
  - cursor stays within bounds
  - backspace at (0,0) is no-op
  - enter always produces a new line

## Two viable build layouts for this repo

### Option 1 (fastest): plain host build + ESP-IDF device build

- Put `typewriter_core` in a normal CMake library (no ESP headers).
- Host runner is a normal Linux executable.
- ESP32 runner is an ESP-IDF project that links `typewriter_core`.

This gives the fastest iteration loop and avoids linux-target component limitations.

### Option 2 (uniform tooling): ESP-IDF linux target host build

- Create a host-runner ESP-IDF project that sets target `linux`.
- Only include portable components (often just `main` + your own portable code).
- Use `CONFIG_IDF_TARGET_LINUX` where needed.

This keeps `idf.py` as the single entrypoint but requires stricter component hygiene.

## Practical next steps (for the next session)

- Choose Option 1 vs Option 2.
- Implement `typewriter_core` first + host tests.
- Add host runner UI (ncurses).
- Integrate device runner (Cardputer) only after behavior is stable.

## External sources to consult

- ESP-IDF “host apps” guide (Linux target; preview)
- ESP-IDF “Linux host testing / unit testing” guide

