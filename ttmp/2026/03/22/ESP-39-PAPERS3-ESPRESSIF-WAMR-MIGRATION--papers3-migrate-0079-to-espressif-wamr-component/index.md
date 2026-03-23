---
Title: PaperS3 migrate 0079 to Espressif WAMR component
Ticket: ESP-39-PAPERS3-ESPRESSIF-WAMR-MIGRATION
Status: active
Topics:
    - papers3
    - wasm
    - firmware
    - esp-idf
    - debugging
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0079-papers3-wamr-assemblyscript-console/main/idf_component.yml
      Note: Current upstream WAMR dependency declaration that will be migrated to the official Espressif component
    - Path: 0079-papers3-wamr-assemblyscript-console/main/CMakeLists.txt
      Note: Main component dependency list that currently references the upstream component alias
    - Path: 0079-papers3-wamr-assemblyscript-console/dependencies.lock
      Note: Resolved component lockfile that will show whether the migration landed on the expected package and version
    - Path: 0079-papers3-wamr-assemblyscript-console/sdkconfig.defaults
      Note: WAMR-related build configuration that must remain compatible after the dependency swap
    - Path: 0079-papers3-wamr-assemblyscript-console/main/wasm_runtime_service.cpp
      Note: Runtime bring-up code that provides the quickest compile-time and runtime signal after migration
ExternalSources:
    - https://components.espressif.com/components/espressif/wasm-micro-runtime/versions/2.4.0~1
    - https://components.espressif.com/components/espressif/wasm-micro-runtime/versions/2.4.0/dependencies?language=en
    - https://components.espressif.com/components/espressif/wasm-micro-runtime/versions/2.4.0/examples/esp-idf
    - local:ESP-39 external report.md
Summary: Migrate `0079` from the upstream Bytecode Alliance WAMR git dependency to Espressif's official Component Registry package and verify that the firmware still builds.
LastUpdated: 2026-03-22T16:14:19.112410541-04:00
WhatFor: Plan, document, and execute a bounded dependency migration from the upstream WAMR component to Espressif's official component package.
WhenToUse: Read this before changing the WAMR dependency in `0079`, validating the migration, or comparing behavior between the upstream and Espressif component packages.
---


# PaperS3 migrate 0079 to Espressif WAMR component

## Overview

This ticket performs a narrowly scoped migration in `0079-papers3-wamr-assemblyscript-console`: stop pulling WAMR from the upstream `bytecodealliance/wasm-micro-runtime` git dependency and instead consume Espressif's official `espressif/wasm-micro-runtime` component package.

The purpose is not cosmetic package cleanup. It is a controlled A/B experiment motivated by the `ESP-38` replay-isolation findings. We already established that plain WAMR execution is sufficient to destabilize later PaperS3 replay on `ESP-IDF 5.3.4`. The next high-value question is whether the official Espressif packaging and its ESP-IDF integration differ in a way that changes build behavior or runtime stability.

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field
- **Implementation Guide**: See `design/01-espressif-wamr-migration-guide.md`
- **Web Research Brief**: See `design/02-web-research-brief-for-espressif-wamr-on-papers3.md`
- **Panel EPD Analysis**: See `design/03-panel-epd-analysis-for-papers3-wamr-interference.md`
- **Diary**: See `reference/01-diary.md`

## Status

Current status: **active**

Immediate milestone: switch the dependency manifest and main component alias in `0079`, regenerate the resolved dependency set, and get a clean firmware build against the Espressif package.

## Topics

- papers3
- wasm
- firmware
- esp-idf
- debugging

## Tasks

See [tasks.md](./tasks.md) for the current task list.

## Changelog

See [changelog.md](./changelog.md) for recent changes and decisions.

## Structure

- design/ - Architecture and design documents
- reference/ - Prompt packs, API contracts, context summaries
- playbooks/ - Command sequences and test procedures
- scripts/ - Temporary code and tooling
- various/ - Working notes and research
- archive/ - Deprecated or reference-only artifacts
