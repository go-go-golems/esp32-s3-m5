---
Title: Extract Reusable Native ESP-IDF NFC Component
Ticket: ESP-61-REUSABLE-NFC-COMPONENT
Status: active
Topics:
    - nfc
    - esp-idf
    - st25r3916
    - m5stackchan
    - component-architecture
    - intern-guide
DocType: index
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: Design and implementation workspace for extracting ESP-60 reader, NDEF, MIFARE Classic, emulation, trace, and service behavior into a reusable native ESP-IDF component.
LastUpdated: 2026-08-22T19:30:00-04:00
WhatFor: Coordinate evidence, architecture, implementation, testing, and delivery of the reusable NFC component.
WhenToUse: Use when implementing, reviewing, or integrating the extracted NFC component into a standalone application or NFC LAB.
---

# Extract Reusable Native ESP-IDF NFC Component

## Overview

ESP-61 turns the successful NFC work from projects `0115`, `0116`, and `0117` into an implementation-ready component design. The ticket separates the mature upstream protocol library from repository-specific lifecycle, ownership, safety, error, event, and adapter concerns.

The target is a board-independent C++17 ESP-IDF component that accepts an application-owned I²C bus, returns structured results, supports a single-owner worker for multi-task applications, and keeps console, NVS, reboot, UI, and demonstration policy outside the protocol engine.

## Primary documents

- [Reusable Native ESP-IDF NFC Component Analysis Design and Intern Implementation Guide](design-doc/01-reusable-native-esp-idf-nfc-component-analysis-design-and-intern-implementation-guide.md)
- [Investigation Diary](reference/01-investigation-diary.md)
- [Tasks](tasks.md)
- [Changelog](changelog.md)

## Current findings

- There is no finished first-party reusable component yet; `0117/main` is an ESP-IDF application component.
- M5Unit-NFC is the reusable upstream protocol dependency and already accepts a native ESP-IDF bus handle.
- `st25r_trace` is nearly ready for standalone extraction.
- `0115` remains the minimal C transport/RF/UID regression harness.
- `0116` supplies the accepted single-worker/shared-bus application pattern.
- `0117` supplies proven broad reader, NDEF, Classic, and target behavior but requires separation from presentation and policy.

## Proposed implementation

The guide proposes:

1. `gogolem_nfc` synchronous Engine;
2. optional single-owner Service;
3. structured domain types and typed errors;
4. UID-bound mutation permits and detailed restoration reports;
5. caller-provided NDEF, Classic credentials, and emulation profiles;
6. optional console, NVS, and low-level trace adapters;
7. standalone and NFC LAB consumers;
8. phased host, hardware, integration, and endurance tests.

## Status

The research/design package is complete. Component implementation has not started. Open questions around upstream teardown, public allocation policy, asynchronous NDEF payload ownership, and physical sacrificial-tag validation are recorded in the primary guide.

## Topics

- nfc
- esp-idf
- st25r3916
- m5stackchan
- component-architecture
- intern-guide
