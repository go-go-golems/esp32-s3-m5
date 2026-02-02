---
Title: zigctl debug logging + log correlation design
Ticket: 0067-zigbee-powerplug
Status: active
Topics:
    - zigbee
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ttmp/2026/01/31/0067-zigbee-powerplug--zigbee2mqtt-third-reality-gen-2-plug/design-doc/02-zigctl-debug-logging-log-correlation-design.md
      Note: Design document
ExternalSources: []
Summary: Design for structured debug logging in zigctl and correlation with Zigbee2MQTT logs, plus optional SQLite log ingestion.
LastUpdated: 2026-02-02T21:12:00-05:00
WhatFor: Define how zigctl should log, correlate requests with Z2M logs, and optionally persist logs to SQLite.
WhenToUse: Use when adding debug logging or building a log forensics workflow.
---


# zigctl debug logging + log correlation design

## Executive Summary
Introduce structured debug logging in zigctl (request/response timing, topics, payload hashes) and provide a simple way to correlate zigctl actions with Zigbee2MQTT logs. Optionally ingest logs into SQLite for later query. This improves troubleshooting, especially for joins and device command issues.

## Problem Statement
Today zigctl emits structured output for command results but provides no internal debug trace of its own requests. Troubleshooting requires manual cross-referencing with Zigbee2MQTT logs. We need a consistent logging scheme (with a correlation ID per command) and a reliable way to query or archive logs over time.

## Proposed Solution
1. **Structured debug logging in zigctl**
   - Add a `--debug` flag and route logs to stderr.
   - Emit a per-command correlation ID (`corr_id`).
   - Log publish events, response topics, timeouts, and durations.

2. **Correlation with Z2M logs**
   - Use timestamps and MQTT topics as the primary join key.
   - For request/response flows, log the request topic and response topic in zigctl.

3. **Optional SQLite ingestion**
   - Provide a small ingestion script to parse Z2M logs (JSON preferred) into SQLite.
   - Use a schema that supports simple filters (ts, level, scope, topic, payload).

## Design Decisions
- **Log format:** JSON lines or key=value style on stderr (consistent with Glazed logging conventions).
- **Correlation ID:** random short token per command execution to tie zigctl log lines together.
- **Where to log:** stderr by default; output rows remain on stdout.
- **SQLite ingestion:** keep optional (outside zigctl) to avoid adding heavy dependencies.

## Alternatives Considered
- **No debug logs; rely solely on Z2M logs:** too opaque for zigctl debugging.
- **Embed SQLite ingestion in zigctl:** adds dependencies and runtime overhead.
- **Publish zigctl logs to MQTT:** useful but may pollute broker topics in production.

## Implementation Plan
1. Add a logging layer to zigctl root (optional debug flag).
2. Extend MQTT helper functions to accept a logger and correlation ID.
3. Log request/response timing and payload sizes for bridge commands.
4. Provide a small script (ticket scripts/) to ingest logs into SQLite.
5. Document workflows in analysis/playbook docs.

## Open Questions
- Do we want zigctl debug logs in JSON or human-readable format by default?
- Should zigctl expose a `--log-json` flag to force JSON output?

## Appendix: SQLite Schema Draft

```sql
CREATE TABLE IF NOT EXISTS z2m_logs (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  ts TEXT,
  level TEXT,
  scope TEXT,
  topic TEXT,
  message TEXT,
  payload TEXT,
  raw TEXT
);
```

## Appendix: Example correlation flow

- zigctl logs: `corr_id=abc123 request_topic=zigbee2mqtt/bridge/request/info response_topic=zigbee2mqtt/bridge/info`.
- Z2M logs: look for MQTT publish to `bridge/info` within the same second.
