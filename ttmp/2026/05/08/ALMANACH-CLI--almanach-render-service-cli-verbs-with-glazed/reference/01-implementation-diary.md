---
Title: Implementation Diary
Ticket: ALMANACH-CLI
Status: active
Topics:
    - almanach
    - go
    - console
    - rendering
    - thermal-printer
    - tooling
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Chronological notes for the ALMANACH-CLI documentation and future implementation work."
LastUpdated: 2026-05-08T06:25:00-04:00
WhatFor: "Use this diary to resume the CLI-verb implementation without rediscovering the analysis context."
WhenToUse: "Read before implementing or reviewing ALMANACH-CLI changes."
---

# Implementation Diary

## 2026-05-08 — Ticket and design guide creation

Created ticket `ALMANACH-CLI` for adding first-class CLI verbs to `stoms3r/cmd/almanach-render-service` using the Glazed command framework.

The user specifically requested:

- CLI verbs so rendering and printing do not require manually driving the local HTTP API.
- Glazed command framework usage.
- `ObjectFromFile` layout input so JSON and YAML layout files are both accepted.
- A detailed intern-facing analysis/design/implementation guide.
- Storage under docmgr and upload to reMarkable.

Reviewed the existing render-service structure and the Glazed authoring skill. Important findings recorded in the design guide:

- The current binary is server-only; `main.go` immediately starts HTTP service mode.
- Rendering is currently coupled to `Server.renderWithChrome` and hardcodes `http://localhost:<port>/almanach`.
- Chrome needs an HTTP URL because the SPA host page references `/almanach/bundle.js`; therefore the CLI should use an internal ephemeral `127.0.0.1:0` server rather than trying to use `file://`.
- The current capture selector is `.paper-shell`; the print-oriented CLI should default to `.paper-body` while keeping `--selector` configurable.
- The current capture CSS hides rails/topbar but leaves clipping risks from `.almanach-app`, `.workspace`, and `.canvas`.
- The Go default layout structs are not fully aligned with the frontend schema in `almanach-studio.jsx`.

Created eight implementation tasks in `tasks.md`:

1. Refactor `main.go` into Glazed root with `serve` command preserving current behavior.
2. Add `render` CLI verb with `TypeObjectFromFile` layout input, YAML/JSON support, PNG/bitmap outputs, and debug artifacts.
3. Add `print` CLI verb that renders once and posts bitmap to ESP32 printer with configurable feed lines.
4. Add `inspect` CLI verb for DOM metrics, selector validation, and cutoff debugging.
5. Refactor renderer to accept base URL, render options, selector, viewport, threshold, and capture CSS.
6. Add ephemeral localhost static server for one-shot CLI rendering.
7. Align default Go layout generation with frontend Almanach Studio schema.
8. Update devctl plugin and README to use the new CLI verbs.

Wrote the main design guide at:

`ttmp/2026/05/08/ALMANACH-CLI--almanach-render-service-cli-verbs-with-glazed/design-doc/01-cli-verbs-with-glazed-analysis-design-and-implementation-guide.md`

Next implementation step should be a small first patch that aligns layout schema and introduces `RenderOptions`, before adding Glazed dependencies. That minimizes review risk and makes renderer behavior testable before CLI parsing is involved.

## Step 1: Detailed phased task breakdown

The first implementation action was to turn the coarse ticket tasks into a phased work queue. This makes the work reviewable in small commits: schema fixes first, renderer refactor second, one-shot infrastructure third, Glazed verbs fourth, and docs/devctl validation last.

This step did not change product code. It changed the ticket plan so that future commits can close focused task IDs and keep the diary/changelog aligned with the implementation sequence.

### Prompt Context

**User prompt (verbatim):** "Ok, add detailed tasks (in phases) to the ticket, then work on them one by one, committing at aappropriate intervals, keeping a diary."

**Assistant interpretation:** Add a phased implementation checklist to `ALMANACH-CLI`, then start executing tasks in order while committing code/docs and updating the diary.

**Inferred user intent:** The user wants a disciplined implementation workflow, not a single large unreviewable change.

**Commit (code):** N/A — documentation/task planning only at this point.

### What I did

- Added detailed `Phase 0` through `Phase 5` tasks to `ttmp/.../tasks.md`.
- Kept the existing coarse tasks as summary milestones.
- Recorded this diary step before beginning code changes.

### Why

- The ticket needed enough task granularity to support incremental commits and review.
- The phases isolate risk: schema alignment can be tested without Glazed, renderer capture can be tested before CLI parsing, and CLI verbs can reuse already-proven internals.

### What worked

- `docmgr task add` appended the phased task list cleanly.
- The resulting task list now has enough structure for step-by-step implementation.

### What didn't work

- N/A.

### What I learned

- The existing ticket had good top-level milestones but not enough atomic implementation tasks for the requested workflow.

### What was tricky to build

- The main trick was preserving the original milestones while adding actionable subtasks rather than replacing the ticket's prior structure.

### What warrants a second pair of eyes

- Review whether the phase ordering is correct: schema alignment and renderer refactor are intentionally before Glazed command wiring.

### What should be done in the future

- Check off tasks only after matching code, tests, diary, and changelog entries are complete.

### Code review instructions

- Start with `ttmp/2026/05/08/ALMANACH-CLI--almanach-render-service-cli-verbs-with-glazed/tasks.md`.
- Confirm that phases cover schema, renderer, ephemeral server, Glazed verbs, docs, devctl, and smoke testing.

### Technical details

- The detailed tasks start at Phase 0.1 and continue through Phase 5.3.
