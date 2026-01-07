---
Title: Cardputer-ADV fan control (GPIO3 relay)
Ticket: 001-FAN-CONTROL
Status: active
Topics:
    - esp-idf
    - esp32s3
    - cardputer
    - console
    - usb-serial-jtag
    - gpio
    - animation
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0037-cardputer-adv-fan-control-console
      Note: "Firmware project: esp_console over USB Serial/JTAG + GPIO3 relay control"
    - Path: 0037-cardputer-adv-fan-control-console/main/fan_console.c
      Note: "Command parsing (`fan ...`) + animation verbs/task"
    - Path: 0037-cardputer-adv-fan-control-console/main/fan_relay.c
      Note: "GPIO3 relay helper: init + on/off functions + polarity handling"
    - Path: 0037-cardputer-adv-fan-control-console/sdkconfig.defaults
      Note: "USB Serial/JTAG console defaults (avoid UART conflicts)"
    - Path: ttmp/2026/01/07/001-FAN-CONTROL--cardputer-adv-fan-control-gpio3-relay/reference/01-diary.md
      Note: "Implementation diary (step-by-step notes)"
    - Path: 0036-cardputer-adv-led-matrix-console/main/matrix_console.c
      Note: "Reference pattern: esp_console USB Serial/JTAG + animation loop structure"
ExternalSources: []
Summary: "Interactive esp_console firmware (USB Serial/JTAG) to control a fan relay on GPIO3, with named on/off animations."
LastUpdated: 2026-01-07T18:05:27.923495359-05:00
WhatFor: "Bring-up and interactive control of a fan relay on Cardputer-ADV using GPIO3."
WhenToUse: "Use when you want manual and scripted relay toggling patterns from esp_console (no UART console)."
---

# Cardputer-ADV fan control (GPIO3 relay)

## Overview

Goal: provide a small Cardputer-ADV firmware that exposes a `fan` REPL command to turn a relay-driven fan on/off and run simple on/off “animation” patterns (blink, strobe, tick, beat, burst) for validation and demos.

## Key Links

- Firmware project: `0037-cardputer-adv-fan-control-console`
- Reference pattern for `esp_console` + animations: `0036-cardputer-adv-led-matrix-console/main/matrix_console.c`
- Diary: `ttmp/2026/01/07/001-FAN-CONTROL--cardputer-adv-fan-control-gpio3-relay/reference/01-diary.md`

## Status

Current status: **active**

## Topics

- esp-idf
- esp32s3
- cardputer
- console
- usb-serial-jtag
- gpio
- animation

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
