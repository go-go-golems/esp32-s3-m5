---
Title: "ESP-10-TUI-DOCS: tasks"
Ticket: ESP-10-TUI-DOCS
Status: active
Topics:
  - docs
  - tui
  - ux
  - testing
  - remarkable
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
Summary: "Keep Esper TUI documentation and review workflow sharp: compare doc, capture playbooks, scripts, and reMarkable export/upload."
LastUpdated: 2026-01-25T16:55:00-05:00
---

# Tasks

## TODO (check off in order)

Compare + screenshots:
- [ ] Ensure `ESP-02` compare doc has a section for every wireframe screen/overlay (and points to the current capture sets)
- [ ] Add missing screenshot sections (Help overlay; Edit Device dialog; any remaining spec screens)
- [ ] Keep “Screens still missing” list accurate (functional vs visual vs not yet captured)

Playbooks:
- [ ] Update/create a single “UX iteration loop” playbook that covers: build, run, tmux capture, compare, and firmware triggers
- [ ] Document exact commands for hardware runs (USB Serial/JTAG path selection, flash/test firmware)

PDF + reMarkable:
- [ ] Add script to render compare doc(s) to PDF (pandoc or equivalent)
- [ ] Add script to upload rendered PDF(s) to reMarkable and verify destination folder structure

Hygiene:
- [ ] Ensure all capture scripts keep state in temp dirs (no telemetry/XDG pollution)
- [ ] Document “how to clean up / re-run captures” safely

Closeout:
- [ ] Update changelog and close ticket when complete
