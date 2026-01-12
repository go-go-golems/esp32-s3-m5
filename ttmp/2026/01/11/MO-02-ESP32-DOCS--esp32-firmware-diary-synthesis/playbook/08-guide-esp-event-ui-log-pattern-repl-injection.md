---
Title: 'Guide: esp_event UI log pattern + REPL injection'
Ticket: MO-02-ESP32-DOCS
Status: active
Topics:
    - esp32
    - firmware
    - documentation
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: 'Expanded research and writing guide for the esp_event UI log pattern doc.'
LastUpdated: 2026-01-11T19:44:01-05:00
WhatFor: 'Help a ghostwriter draft the esp_event UI log pattern document.'
WhenToUse: 'Use before writing the esp_event UI log guide.'
---

# Guide: esp_event UI log pattern + REPL injection

## Purpose

This guide enables a ghostwriter to produce a single-topic doc that explains how to build a UI log driven by `esp_event`, with events posted from keyboard edges and a REPL command handler. The end product should show a clear producer -> bus -> UI log flow.

## Assignment Brief

Write a developer guide that teaches the event bus pattern through a concrete UI log example. The doc should show how to register handlers, post events from multiple sources, and render a scrollable log. It should also show how the REPL can inject custom events into the same bus.

## Environment Assumptions

- ESP-IDF 5.4.x
- Cardputer hardware
- USB Serial/JTAG console access

## Source Material to Review

- `esp32-s3-m5/0028-cardputer-esp-event-demo/README.md`
- `esp32-s3-m5/0030-cardputer-console-eventbus/README.md`
- `esp32-s3-m5/ttmp/2026/01/04/0028-ESP-EVENT-DEMO--esp-idf-esp-event-demo-cardputer-keyboard-parallel-tasks/reference/01-diary.md`
- `esp32-s3-m5/ttmp/2026/01/05/0030-CARDPUTER-CONSOLE-EVENTBUS--cardputer-keyboard-esp-console-esp-event-on-screen-event-log/reference/01-diary.md`

## Technical Content Checklist

- Event loop setup (`esp_event_loop_create_default`, handler registration)
- Event producers: keyboard edges, periodic tasks, random tasks
- Event consumer: UI log task with scroll/pause controls
- REPL commands that post events into the bus (0030 `bus>` prompt)
- Keyboard bindings for scroll/pause/clear

## Pseudocode Sketch

Use a simple sketch to show event flow and rendering.

```c
// Pseudocode: event bus with UI log
init_event_loop()
register_handler(EVENT_KEY_EDGE, ui_log_handler)
register_handler(EVENT_HEARTBEAT, ui_log_handler)

keyboard_task:
  if key_edge:
    esp_event_post(EVENT_KEY_EDGE, payload)

repl_cmd("evt post <msg>"):
  esp_event_post(EVENT_CUSTOM, msg)

ui_task:
  while receive_event():
    append_to_log(event)
    render_log()
```

## Pitfalls to Call Out

- Event handler work must be short to avoid blocking UI updates.
- Multiple producers can overwhelm the UI without throttling.
- REPL input and UI rendering can contend for console output.

## Suggested Outline

1. Pattern overview: event bus + UI log
2. Core event loop setup
3. Producer tasks and event types
4. UI log consumer and keyboard bindings
5. REPL injection commands
6. Troubleshooting checklist

## Commands

```bash
cd esp32-s3-m5/0028-cardputer-esp-event-demo
idf.py set-target esp32s3
idf.py build flash monitor

cd ../0030-cardputer-console-eventbus
idf.py build flash monitor
```

## Exit Criteria

- Doc includes a diagram of producer -> event bus -> UI log.
- Doc documents the keyboard bindings and REPL commands.
- Doc lists at least two failure modes and mitigations.

## Notes

Keep the scope on the `esp_event` UI log pattern; avoid broader system architecture topics.
