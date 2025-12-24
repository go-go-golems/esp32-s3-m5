---
Title: Diary
Ticket: 003-CARDPUTER-KEYBOARD-SERIAL
Status: active
Topics:
    - esp32s3
    - esp-idf
    - cardputer
    - keyboard
    - display
    - m5gfx
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-22T19:48:42.266146873-05:00
WhatFor: ""
WhenToUse: ""
---

# Diary — 003-CARDPUTER-KEYBOARD-SERIAL (scope pivot: Cardputer typewriter)

## Goal

Track implementation of a **Cardputer on-screen typewriter** demo (keyboard input rendered to the Cardputer LCD), including dead ends, validation steps, and commit hashes.

## Context

This ticket originally started as “keyboard → serial echo”, but the goal shifted to “keyboard → on-screen typewriter”. The existing `esp32-s3-m5/0007-cardputer-keyboard-serial` tutorial remains a useful known-good keyboard scanner.

We will follow the vendor firmware patterns from `M5Cardputer-UserDemo` for:

- Display bring-up via `M5GFX` autodetect (`board_M5Cardputer`)
- Text input UX (see `main/apps/app_texteditor/app_texteditor.cpp` and `main/apps/app_chat/app_chat.cpp`)

## Quick Reference

N/A (this diary is narrative; see the design doc for the structured plan).

## Step 1: “Nothing happens” was actually “echo is inline”; add unmistakable real-time feedback + autodetect

When testing `0007-cardputer-keyboard-serial`, it initially looked like the keyboard wasn’t working because the terminal prompt `> ` stays on the same line and **typed characters echo inline** (no newline). Only when pressing **Enter** does the program print a log line with the captured buffer, which made it feel like “Enter is required for any output”.

We later confirmed an additional source of confusion: some serial monitor UIs (or log capture layers) are effectively **line-buffered on the host side**, meaning they don’t display partial lines until they see a newline. That produces exactly this symptom:

- you type on the Cardputer → device outputs characters without `\\n`
- nothing shows up in the host UI
- you press Enter → device outputs `\\r\\n`
- the host UI finally flushes and you suddenly see the whole line at once

This is **not** the ESP32 buffering (we set `stdout` unbuffered and `fflush()` after each write); it’s the host viewer deciding to only show complete lines.

To reduce this confusion (and make bring-up more robust across Cardputer revisions), we added:

- A clearer startup log explaining inline echo semantics.
- `fflush(stdout)` after each `fputc/fputs` to avoid any host-side buffering surprises.
- Optional “newline-per-key” logging (`CONFIG_TUTORIAL_0007_LOG_KEY_EVENTS`) so every keypress produces an `ESP_LOGI` line.
- Optional keyboard IN0/IN1 **autodetect** between `13/15` and `1/2` (vendor HAL hints at an alternate pin list).
- A small output-settle delay knob (`CONFIG_TUTORIAL_0007_SCAN_SETTLE_US`) after changing scan outputs.

**Commit (esp32-s3-m5):** c0b5124c4915db6961b1999e2770d7aab52b4675 — "0007: improve keyboard echo UX + debug autodetect"

## Usage Examples

N/A.

## Related

- Design doc: `../design-doc/01-cardputer-keyboard-serial-echo-esp-idf.md`
