---
Title: 'Build and Flash the Blinky Custom App: Intern Implementation Guide'
Ticket: M5STACKCHAN-BLINKY
Status: active
Topics:
    - m5stackchan
    - firmware
    - mooncake
    - custom-app
    - esp32-s3
    - blinky
    - intern-guide
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: build/firmware/components/mooncake/src/ability/ability.h
      Note: AppAbility class
    - Path: build/firmware/main/apps/app_blinky/app_blinky.cpp
      Note: Implemented direct 12-LED RGB HAL control and simple quit-button UI
    - Path: build/firmware/main/apps/app_blinky/app_blinky.h
      Note: Blinky app state and LVGL object declarations
    - Path: docs/guides/firmware-build-developer-guide.md
      Note: Full firmware build guide — prerequisite reading for the intern
    - Path: ttmp/2026/06/11/M5STACKCHAN--kawaii-desktop-robot-full-documentation-research/sources/StackChan/firmware/main/apps/app_template/app_template.cpp
      Note: Official app template implementation — lifecycle pattern reference
    - Path: ttmp/2026/06/11/M5STACKCHAN--kawaii-desktop-robot-full-documentation-research/sources/StackChan/firmware/main/apps/app_template/app_template.h
      Note: Official app template header — structural reference for Blinky
    - Path: ttmp/2026/06/11/M5STACKCHAN--kawaii-desktop-robot-full-documentation-research/sources/StackChan/firmware/main/hal/hal_mcp.cpp
      Note: Lines 72-88 — correct LED API usage via GetStackChan().leftNeonLight().setColor()
    - Path: ttmp/2026/06/11/M5STACKCHAN--kawaii-desktop-robot-full-documentation-research/sources/StackChan/firmware/main/main.cpp
      Note: app_main entry point — where apps are installed
ExternalSources: []
Summary: ""
LastUpdated: 2026-06-11T19:00:24.648116175-04:00
WhatFor: ""
WhenToUse: ""
---








# Build and Flash the Blinky Custom App: Intern Implementation Guide

## Overview

<!-- Provide a brief overview of the ticket, its goals, and current status -->

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- m5stackchan
- firmware
- mooncake
- custom-app
- esp32-s3
- blinky
- intern-guide

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
