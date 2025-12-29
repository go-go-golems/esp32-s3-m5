---
Title: Diary
Ticket: 0016-SPIFFS-AUTOLOAD
Status: active
Topics:
    - esp32s3
    - esp-idf
    - qemu
    - spiffs
    - flash
    - filesystem
    - javascript
    - microquickjs
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-29T14:05:03.108671137-05:00
WhatFor: ""
WhenToUse: ""
---

# Diary

## Goal

Keep a detailed record of actions, observations, and hypotheses while investigating SPIFFS first-boot formatting behavior and MicroQuickJS autoload library parse errors.

## Step 1: Create a dedicated bug ticket and capture the observed symptom cluster

This step spins off the SPIFFS/autoload problem into its own ticket so we can debug it independently from the QEMU input issue. The goal is to preserve the exact observed boot sequence and point directly at the code responsible for SPIFFS init, first-boot library creation, and JS evaluation.

### What I did

- Created ticket `0016-SPIFFS-AUTOLOAD`.
- Created a dedicated bug report document and wrote an initial analysis framing the two sub-issues:
  - SPIFFS mount failure → formatting (first boot)
  - Autoload JS parse errors (`[mtag]: expecting ';'`)
- Captured the key boot snippet into `sources/qemu-spiffs-autoload-snippet.txt`.
- Related the bug report to the firmware code that implements SPIFFS + autoload loader.

### Why

The symptom cluster is tightly connected (SPIFFS formatting + library creation + immediate load attempt) and easy to lose in the bigger “port to Cardputer” thread. A focused ticket lets us converge faster.

### What worked

- We have a stable, repeatable log sequence to anchor debugging.
- We’ve pinpointed the exact functions involved in `main/main.c`.

### What didn’t work

N/A (this step is documentation/setup).

### What I learned

- The autoload JS content written by `create_example_libraries()` looks like basic ES5.1, yet MicroQuickJS rejects it consistently; we should not assume “looks valid” means “parses in this engine”.

### What should be done next

- Map `-10025` to a SPIFFS error constant in ESP-IDF and decide whether it’s expected “first boot” behavior.
- Read back and dump the first bytes of the created `.js` files to confirm actual on-disk contents match the string literals.
- Try a minimal file (`1+2;`) under `/spiffs/autoload/` to establish whether the JS loader works at all.

## Context

<!-- Provide background context needed to use this reference -->

## Quick Reference

<!-- Provide copy/paste-ready content, API contracts, or quick-look tables -->

## Usage Examples

<!-- Show how to use this reference in practice -->

## Related

<!-- Link to related documents or resources -->
