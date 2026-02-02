---
Title: Zigbee2MQTT log tailing + decode analysis
Ticket: 0067-zigbee-powerplug
Status: active
Topics:
    - zigbee
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ttmp/2026/01/31/0067-zigbee-powerplug--zigbee2mqtt-third-reality-gen-2-plug/analysis/01-zigbee2mqtt-log-tailing-decode-analysis.md
      Note: Primary analysis
ExternalSources: []
Summary: How to tail Zigbee2MQTT logs, decode log lines, correlate with zigctl actions, and optionally store logs in SQLite for later querying.
LastUpdated: 2026-02-02T21:10:00-05:00
WhatFor: Give a practical, repeatable way to observe Zigbee2MQTT runtime behavior and debug zigctl interactions.
WhenToUse: Use when diagnosing join issues, verifying zigctl behavior, or building a log evidence trail.
---


# Zigbee2MQTT log tailing + decode analysis

## Goal
Provide concrete ways to tail Zigbee2MQTT logs (tmux, docker, and file logs), explain how to decode typical log lines (like the ones in the screenshot), correlate them with zigctl requests, and evaluate storing logs in SQLite for later query.

## Where the logs live (and how to tail them)

### 1) Live logs from the container (quickest)

```bash
# Stream Zigbee2MQTT logs
Docker logs -f z2m
```

This mirrors what you see in the tmux pane if Zigbee2MQTT is running there.

### 2) tmux pane (if running via playbook)

```bash
tmux attach -t z2m-test
```

Then look at the right pane (Zigbee2MQTT). This is useful for immediate visual inspection.

### 3) File logs inside the data folder (most durable)

The Zigbee2MQTT config in this repo uses a log directory under the container data volume:

```
.../scripts/zigbee2mqtt-test/data/log/<timestamp>/log.log
```

Tail it like this:

```bash
tail -f \
  /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/
  ttmp/2026/01/31/0067-zigbee-powerplug--zigbee2mqtt-third-reality-gen-2-plug/
  scripts/zigbee2mqtt-test/data/log/*/log.log
```

### 4) MQTT log topic (optional)

Zigbee2MQTT can publish logs to MQTT. If enabled, you can watch them with zigctl:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/zigctl

go run ./ listen raw \
  --broker mqtt://localhost:1884 \
  --topic 'zigbee2mqtt/bridge/logging' \
  --output json
```

This is useful when you want logs in the same stream as zigctl outputs.

## Decoding typical log lines (as seen in the screenshot)

The screenshot shows debug lines that look like:

```
[2026-02-01 21:11:26] debug: z2m: Received Zigbee message from ...
[2026-02-01 21:11:26] debug: zh:ember: [FRAME] ...
[2026-02-01 21:11:26] debug: zcl: ...
```

How to interpret:

- **Prefix**
  - `z2m:` indicates Zigbee2MQTT itself (MQTT publish, bridge events, parsing).
  - `zh:` indicates zigbee-herdsman low-level radio traffic (frames, network layer).
  - `zcl:` indicates Zigbee Cluster Library parsing (attributes, on/off reports).

- **What a normal join looks like**
  - You will see a `device_joined` event on `zigbee2mqtt/bridge/event`.
  - Z2M will log a `MQTT publish` to `zigbee2mqtt/<device>` soon after.

- **What a normal command looks like**
  - zigctl publishes to `zigbee2mqtt/<device>/set`.
  - Z2M logs something like `MQTT publish` or `Zigbee message from <device>`.
  - A follow-up attribute report usually appears (`zcl` or `attributeReport`).

From the screenshot, the log lines show normal attribute reports and ZCL parsing for a device. There is no obvious error or zigctl malfunction visible in those lines.

## Correlating zigctl to Zigbee2MQTT logs

When you run a zigctl command:

1. zigctl publishes to a request topic (e.g., `zigbee2mqtt/bridge/request/info`).
2. Z2M logs a request and publishes a response.
3. zigctl receives the response and prints it.

To correlate:

- **Filter logs by time** (run zigctl, then immediately tail logs).
- **Look for matching topics** in the logs: `bridge/request/...` or `.../set`.
- **Watch `bridge/event`** for join events when using `permit-join --watch`.

## Recommended workflow for debugging zigctl

1. Start a tail session for Z2M logs:
   ```bash
   docker logs -f z2m
   ```
2. Run your zigctl command in another terminal.
3. Confirm the Z2M log shows:
   - the expected request topic, and
   - a response or attribute report.
4. If the response is missing, check the MQTT broker logs (mosquitto) and the device list.

## Can we store and decode logs into SQLite?

Yes, and it is practical if you want long-term searchability. There are two workable approaches:

### Option A: Enable JSON logs, then ingest to SQLite

If logs are JSON, ingestion is easy.

- Set Zigbee2MQTT to JSON console logs (if acceptable in your environment):
  - `advanced.log_console_json: true` in `configuration.yaml`.

Then run a pipeline:

```bash
# Example pipeline (conceptual)
# tail -F log.log | python ingest_json_logs.py
```

Suggested SQLite schema:

```sql
CREATE TABLE IF NOT EXISTS z2m_logs (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  ts TEXT,
  level TEXT,
  scope TEXT,
  message TEXT,
  data_json TEXT,
  raw TEXT
);
CREATE INDEX IF NOT EXISTS idx_z2m_logs_ts ON z2m_logs(ts);
CREATE INDEX IF NOT EXISTS idx_z2m_logs_level ON z2m_logs(level);
CREATE INDEX IF NOT EXISTS idx_z2m_logs_scope ON z2m_logs(scope);
```

### Option B: Parse plaintext logs into SQLite

If logs remain plaintext, a small parser can split:

- timestamp
- log level
- namespace (`z2m`, `zh`, `zcl`)
- message text

Then insert into the same schema. This is slightly less robust but still workable.

### Example query

```sql
-- Find join events
SELECT ts, message
FROM z2m_logs
WHERE message LIKE '%device_joined%'
ORDER BY ts DESC;
```

### Is it worth it?

- **Yes** if you want to audit, correlate, or search long-term issues.
- **No** if you only need quick, ephemeral debugging (tail -f is enough).

## Summary

- **Best live tail:** `docker logs -f z2m` or tmux pane.
- **Best durable tail:** `tail -f data/log/*/log.log`.
- **Decode clues:** `z2m` = app logic, `zh` = radio, `zcl` = attribute parsing.
- **Correlation:** match topics and timestamps between zigctl commands and Z2M log lines.
- **SQLite storage:** feasible with either JSON or plaintext parsing; offers long-term query ability.
