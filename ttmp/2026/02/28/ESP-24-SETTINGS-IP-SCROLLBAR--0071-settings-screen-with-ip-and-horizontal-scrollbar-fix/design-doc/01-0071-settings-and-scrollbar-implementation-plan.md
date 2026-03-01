---
Title: 0071 Settings and Scrollbar Implementation Plan
Ticket: ESP-24-SETTINGS-IP-SCROLLBAR
Status: active
Topics:
    - cardputer
    - cardputer-adv
    - esp32-s3
    - firmware
    - ui
    - wifi
    - timer
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0071-cardputer-adv-photo-timer/main/app_main.cpp
      Note: Settings view state, Wi-Fi IP rendering, and list/scrollbar behavior
    - Path: components/wifi_mgr/include/wifi_mgr.h
      Note: `wifi_mgr_get_status()` API used for IP display
Summary: Add a settings entry/view to 0071 for live Wi-Fi IP visibility and remove horizontal scrollbar artifact from the main list UI.
LastUpdated: 2026-03-01T16:45:00Z
WhatFor: Define implementation and validation strategy for settings + scrollbar fixes.
WhenToUse: Use when reviewing 0071 UI navigation and list rendering behavior.
---

# 0071 Settings and Scrollbar Implementation Plan

## Executive Summary

Add a `Settings` entry to the encoder-driven LVGL list so users can open a dedicated settings view and inspect current Wi-Fi IP directly on-device.  

At the same time, constrain list and label behavior to eliminate the bottom horizontal scrollbar artifact.

## Problem Statement

1. Users need a direct on-device way to read the current IP address for web access.
2. Main UI showed an unwanted horizontal scrollbar at the bottom.

## Proposed Solution

1. Add UI view state (`Timer`, `Settings`) to `app_main.cpp`.
2. Add `Settings` list item in timer view and `Back` item in settings view.
3. In settings view, render:
   - connected: `Wi-Fi IP: <addr>`
   - disconnected: `Wi-Fi IP: disconnected`
4. Remove horizontal scroll behavior by:
   - disabling root screen scrolling,
   - forcing list scroll direction to vertical,
   - clipping long labels and forcing button/label widths to 100%.

## Design Decisions

1. **Use `wifi_mgr_get_status()` for IP**
   - Reuses shared component state instead of duplicating Wi-Fi internals.
2. **Keep navigation encoder-only**
   - Settings entry/back are regular list buttons to avoid separate input paths.
3. **Fix scroll behavior at object-level**
   - Avoid global LVGL style hacks; set list/screen flags directly in UI setup.

## Alternatives Considered

1. Show IP only in footer of timer view:
   - Rejected; footer already carries step timing and would reduce clarity.
2. Add separate settings screen file/module:
   - Rejected for current scope; simple enough to keep in `app_main.cpp`.

## Implementation Plan

1. Add `UiView` state and switch helper.
2. Update `rebuild_preset_list()` to render different content per view.
3. Add Wi-Fi IP formatting helper using `wifi_mgr_status_t`.
4. Adjust LVGL list/screen scroll flags and label long-mode clipping.
5. Build, flash, and validate with screenshot captures of timer/settings views.

## Open Questions

1. Should settings eventually include SSID and connection state details, not only IP?

## References

1. `0071-cardputer-adv-photo-timer/main/app_main.cpp`
2. `components/wifi_mgr/include/wifi_mgr.h`
