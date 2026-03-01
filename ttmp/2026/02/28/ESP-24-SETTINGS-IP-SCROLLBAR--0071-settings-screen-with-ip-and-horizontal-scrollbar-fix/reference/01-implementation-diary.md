---
Title: Implementation Diary
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
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0071-cardputer-adv-photo-timer/main/app_main.cpp
      Note: Implements settings view and scrollbar behavior updates
Summary: Diary for adding settings/IP visibility and removing horizontal scrollbar artifact in 0071.
LastUpdated: 2026-03-01T16:50:00Z
WhatFor: Track implementation and validation details for ESP-24.
WhenToUse: Use when reviewing or extending settings UI in 0071.
---

# Implementation Diary

## Goal

Document the implementation of a settings view with live IP display and the fix for the horizontal scrollbar artifact in the timer list UI.

## Step 1: Add settings view + IP status line

I introduced a view-state model in `app_main.cpp` so the same list container can render either timer controls or a settings screen. A new `Settings` item opens the settings view, and a `Back` action returns to timer controls.  

Settings uses `wifi_mgr_get_status()` and formats host-order IPv4 into `IPSTR` form for the on-device label (`Wi-Fi IP: 192.168.0.196`).

### Prompt Context

**User prompt (verbatim):** "we rebooted, try again, also add a settings entry to view the ip. also there is a horizontal scrollbar at the bottom? Maybe you can find in past tickets a way to get the screenshot of the UI and thus verify by yourself.


Create a new ticket for the settings / scrollbar. and another for the screenshot. write a detailed implementation plan and tasks in the ticket.

Then work on them."

**Assistant interpretation:** Add and validate a settings entry that displays device IP, and track work under dedicated tickets.

**Inferred user intent:** Improve on-device usability for connecting to the web UI and make the UI behavior verifiable.

### What I did

- Added `UiView` state and settings/timer transitions.
- Added list actions:
  - timer view: `Settings`
  - settings view: `Back`
- Implemented IP-line formatting via `wifi_mgr_status_t`.

### What worked

- Settings screen reliably showed connected IP from shared Wi-Fi manager state.

### What didn't work

- N/A for this step.

### What was tricky to build

- Keeping list rebuild/focus behavior stable while switching views required centralizing the view switch logic and rebuilding bindings immediately.

### What warrants a second pair of eyes

- Focus state transitions when switching rapidly between views with encoder input.

### What should be done in the future

- Expand settings with SSID/state summary if needed.

### Code review instructions

- Review `app_main.cpp` around:
  - `UiView`
  - `settings_ip_line()`
  - `rebuild_preset_list()`

### Technical details

- API used: `wifi_mgr_get_status(wifi_mgr_status_t*)`
- Display formatting uses `IPSTR` and `IP2STR`.

## Step 2: Eliminate horizontal scrollbar and verify with captures

I constrained the list to vertical scroll only and disabled root-screen scrollability to prevent the bottom horizontal scrollbar from appearing. I also forced label clipping and full-width buttons to avoid layout overflow side effects.  

Using the screenshot workflow, I captured both timer and settings views and confirmed: no bottom horizontal scrollbar in timer view, and IP visible in settings.

### Prompt Context

**User prompt (verbatim):** (same as Step 1)

**Assistant interpretation:** Validate scrollbar fix and settings IP visually using captured UI output.

**Inferred user intent:** Confirm fixes with concrete evidence, not assumptions.

### What I did

- Applied UI scroll/label constraints:
  - `lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE)`
  - `lv_obj_set_scroll_dir(s_list, LV_DIR_VER)`
  - `LV_LABEL_LONG_CLIP` and width constraints for labels/buttons
- Captured and reviewed:
  - `/tmp/0071-ui-timer.png`
  - `/tmp/0071-ui-settings.png`

### What worked

- Timer capture shows only right-side vertical scrollbar.
- Settings capture shows `Wi-Fi IP: 192.168.0.196`.

### What didn't work

- N/A after final validation.

### What was tricky to build

- Automated view switching for capture required adding a small console `ui timer|settings` command so screenshots are reproducible without manual encoder interaction.

### What warrants a second pair of eyes

- Check if list item text clipping (`-` line in timer screenshot where a longer row is clipped) needs additional UX polish.

### What should be done in the future

- Consider a slightly taller list region or different row typography to reduce clipping on long preset labels.

### Code review instructions

- Verify scroll-related calls in `app_main.cpp` `create_ui()`.
- Compare captured images:
  - `/tmp/0071-ui-timer.png`
  - `/tmp/0071-ui-settings.png`

### Technical details

- Validation captures were produced by:
  - `tools/capture_screenshot_qoi_from_console.py`
  - QOI decoded to BMP, then converted to PNG for inspection.

## Related

- `ttmp/2026/02/28/ESP-24-SETTINGS-IP-SCROLLBAR--0071-settings-screen-with-ip-and-horizontal-scrollbar-fix/design-doc/01-0071-settings-and-scrollbar-implementation-plan.md`
- `ttmp/2026/02/28/ESP-24-SETTINGS-IP-SCROLLBAR--0071-settings-screen-with-ip-and-horizontal-scrollbar-fix/tasks.md`
