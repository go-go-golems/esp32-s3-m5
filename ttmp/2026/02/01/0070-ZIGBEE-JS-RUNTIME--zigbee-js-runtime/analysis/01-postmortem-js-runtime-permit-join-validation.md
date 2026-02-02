---
Title: Postmortem: JS Runtime & Permit-Join Validation
Ticket: 0070-ZIGBEE-JS-RUNTIME
Status: active
Topics:
  - zigbee
  - javascript
  - goja
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
  - Path: ttmp/2026/02/01/0070-ZIGBEE-JS-RUNTIME--zigbee-js-runtime/scripts/04-permit-join-watch-yaml.js
    Note: Permit-join + watch script used for validation
  - Path: ttmp/2026/02/01/0070-ZIGBEE-JS-RUNTIME--zigbee-js-runtime/scripts/05-confirm-run-permit-join.sh
    Note: plz-confirm gated wrapper script
  - Path: zigctl/pkg/jsruntime/zigctlmod/watch.go
    Note: JS watch stream implementation
  - Path: zigctl/pkg/jsruntime/zigctlmod/client.go
    Note: JS client wrapper + logging
Summary: Postmortem for JS runtime validation and permit-join operator workflows, covering CLI/script issues and fixes.
LastUpdated: 2026-02-02T00:00:00-05:00
WhatFor: Capture issues, root causes, and improvements for JS runtime validation workflows.
WhenToUse: Use when repeating permit-join validation or extending JS runtime tooling.
---

# Postmortem: JS Runtime & Permit-Join Validation

## Executive Summary
We validated the zigctl JS runtime and permit-join flow, but hit several operator-workflow issues: a timeout window too short for watch-based scripts, positional-arg ambiguity that misrouted a timeout into a device field, and a plz-confirm JSON parsing bug caused by an incorrect stdin pipeline. We fixed each by adjusting timeouts, switching to key=value parsing, widening the watch topic to include `bridge/#`, and repairing JSON parsing in the confirmation script.

No device join events were observed during the final permit-join window, but the bridge stream showed steady traffic and confirmed permit-join responses and device state updates.

## Timeline (condensed)
- Initial JS runtime smoke test: watch loop exceeded default timeout; rerun with longer timeout.
- Permit-join JS script: timeout value interpreted as device name, returning `Device '60s' does not exist`.
- Added key=value parsing to the JS script to prevent argument shifting.
- Permit-join succeeded with `status: ok` when timeout was passed correctly; tmux logs confirmed request/response topics.
- Added plz-confirm wrapper script and discovered JSON parsing failure due to stdin being consumed by a here-doc.
- Switched to `python3 -c` to read stdin and parse plz-confirm output reliably.
- Expanded watch topic to `bridge/#` to emit bridge messages even without join events.
- Re-ran operator-gated script; permit-join response `status: ok`, bridge info/logging streamed, no join events.

## Issues, Root Causes, Fixes, and Prevention

### 1) Watch loop timed out before completing the join window
- **Symptom:** The JS smoke test timed out at 10s while the watch window needed ~60s.
- **Root cause:** Script duration exceeded default CLI timeout; the command was killed early.
- **Fix:** Reran with a longer timeout, allowing the watch window to complete.
- **Prevention:** Standardize timeouts in runbooks (e.g., `--timeout 60s` or command wrapper with timeout aligned to watch duration).

### 2) Positional args caused timeout to be interpreted as a device name
- **Symptom:** Error `Device '60s' does not exist` on permit-join.
- **Root cause:** Positional parsing misassigned the timeout argument to the `device` slot.
- **Fix:** Added key=value parsing (`timeout=60s`) with positional fallback.
- **Prevention:** Prefer key=value args for scripts with optional parameters; document this in runbooks.

### 3) plz-confirm JSON parsing failed (approval always false)
- **Symptom:** The confirmation script always returned `approved=false` even after approving.
- **Root cause:** `python3 - <<'PY'` used a here-doc, which consumed stdin and ignored the piped JSON from `plz-confirm`.
- **Fix:** Switched to `python3 -c` so stdin remained the JSON stream; added array/object parsing.
- **Prevention:** Avoid here-doc with piped JSON. Add a small test harness that exercises the confirm script and asserts `approved=true` for an approval.

### 4) No join events observed during the permit-join window
- **Symptom:** Permit-join succeeded, but no device join events appeared.
- **Root cause:** No device was placed into pairing mode during the window (or join events were absent).
- **Fix:** None needed for tooling; the permit-join flow itself worked.
- **Prevention:** Use an operator-gated prompt with a clear “pair now” window; optionally add a `plz-confirm` reminder step before permit-join starts.

### 5) Minimal bridge output when no joins occurred
- **Symptom:** With a watch limited to `bridge/event`, output was sparse during idle periods.
- **Root cause:** `bridge/event` only fires on specific events (joins, etc.).
- **Fix:** Default the watch topic to `bridge/#` so state, logging, and info messages appear.
- **Prevention:** For diagnostics, default to `bridge/#` and allow narrowing when noise is undesirable.

### 6) Docmgr command flag mismatch
- **Symptom:** `docmgr task list --format json` failed with “unknown flag”.
- **Root cause:** Incorrect flag; `docmgr task list` does not accept `--format`.
- **Fix:** Used standard list output and updated diary with the failure.
- **Prevention:** Use `docmgr task list --with-glaze-output --output csv --` for scriptable output.

## What We Did Better Over Time
- Added debug logging in the JS runtime client and watch stream, making it easier to see request, subscription, and response behavior.
- Introduced a plz-confirm-gated wrapper, reducing the chance of missing pairing windows.
- Improved usability by switching to key=value args for optional settings.
- Broadened watch output to provide immediate signal even when idle.

## Remaining Gaps / Recommendations
- **Join verification:** If device join validation is required, schedule a pairing window and run the operator-gated script while the plug is actively in pairing mode.
- **Output volume control:** Add a `--watch-topic` arg to the wrapper script so operators can choose `bridge/event` vs `bridge/#` without editing the JS.
- **Regression guard:** Add a small smoke test that confirms `plz-confirm confirm --output json` returns an array and that the parser returns `true` when approved.

## Appendix: Commands Run (representative)
```
# Run operator-gated permit-join watcher
ttmp/2026/02/01/0070-ZIGBEE-JS-RUNTIME--zigbee-js-runtime/scripts/05-confirm-run-permit-join.sh

# Direct JS run (key=value args)
go run ./ js run ttmp/2026/02/01/0070-ZIGBEE-JS-RUNTIME--zigbee-js-runtime/scripts/04-permit-join-watch-yaml.js \
  --arg broker=mqtt://localhost:1884 \
  --arg baseTopic=zigbee2mqtt \
  --arg seconds=120 \
  --arg timeout=60s \
  --arg watchTopic=bridge/#
```
