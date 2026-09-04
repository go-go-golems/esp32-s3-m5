#!/usr/bin/env python3
"""Summarize structured M5 Arduino I2C trace captures as JSON."""

from __future__ import annotations

import argparse
import json
import re
import statistics
from collections import Counter
from pathlib import Path

PHASE_RE = re.compile(
    r"M5_PHASE phase=(\w+) ok=(\d+) elapsed_ms=(\d+) txns=(\d+) "
    r"succeeded=(\d+) failed=(\d+) buffered=(\d+) dropped=(\d+)"
)
EVENT_RE = re.compile(
    r"M5_I2C txn=(\d+) t_us=(\d+) elapsed_us=(\d+) kind=(\w+) "
    r"key=0x([0-9A-F]+) wlen=(\d+) rlen=(\d+) ok=(\d+) failure_stage=0x([0-9A-F]+)"
)
PICC_RE = re.compile(r"PICC:([0-9A-F]+)\s+(.+?)\s+([0-9A-F]{4})/([0-9A-F]{2})")
FAILED_IDENTIFY_RE = re.compile(r"Failed to identify ([0-9A-F]+) (.+?) ([0-9A-F]{4})/([0-9A-F]{2})")
ANSI_RE = re.compile(r"\x1b\[[0-?]*[ -/]*[@-~]")
IMPORTANT_KEYS = (0x02, 0x0A, 0x42, 0x4A, 0x80, 0x9F, 0xC6, 0xC7)


def percentile(values: list[int], fraction: float) -> int | None:
    if not values:
        return None
    return sorted(values)[int(fraction * (len(values) - 1))]


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("capture", type=Path)
    args = parser.parse_args()

    text = ANSI_RE.sub("", args.capture.read_bytes().decode("utf-8", errors="replace"))
    runs: list[dict] = []
    current: dict | None = None
    tags: list[dict] = []
    identify_failures: list[dict] = []

    for line in text.replace("\r\n", "\n").replace("\r", "\n").splitlines():
        match = PHASE_RE.search(line)
        if match:
            current = {
                "phase": match.group(1),
                "ok": bool(int(match.group(2))),
                "elapsed_ms": int(match.group(3)),
                "reported_transactions": int(match.group(4)),
                "reported_succeeded": int(match.group(5)),
                "reported_failed": int(match.group(6)),
                "reported_buffered": int(match.group(7)),
                "reported_dropped": int(match.group(8)),
                "events": [],
            }
            runs.append(current)
            continue
        match = EVENT_RE.search(line)
        if match and current is not None:
            current["events"].append(
                {
                    "sequence": int(match.group(1)),
                    "timestamp_us": int(match.group(2)),
                    "elapsed_us": int(match.group(3)),
                    "kind": match.group(4),
                    "key": int(match.group(5), 16),
                    "write_len": int(match.group(6)),
                    "read_len": int(match.group(7)),
                    "ok": bool(int(match.group(8))),
                    "failure_stage": int(match.group(9), 16),
                }
            )
            continue
        match = PICC_RE.search(line)
        if match:
            tags.append({"uid": match.group(1), "type": match.group(2), "atqa": match.group(3), "sak": match.group(4)})
        match = FAILED_IDENTIFY_RE.search(line)
        if match:
            identify_failures.append(
                {"uid": match.group(1), "provisional_type": match.group(2), "atqa": match.group(3), "sak": match.group(4)}
            )

    for run in runs:
        events = run.pop("events")
        elapsed = [event["elapsed_us"] for event in events]
        keys = Counter(event["key"] for event in events)
        run.update(
            {
                "captured_events": len(events),
                "captured_failed": sum(not event["ok"] for event in events),
                "median_transaction_us": statistics.median(elapsed) if elapsed else None,
                "p95_transaction_us": percentile(elapsed, 0.95),
                "max_transaction_us": max(elapsed) if elapsed else None,
                "important_key_counts": {f"0x{key:02X}": keys[key] for key in IMPORTANT_KEYS if keys[key]},
            }
        )

    print(json.dumps({"capture": str(args.capture), "runs": runs, "identified_tags": tags,
                      "failed_identifications": identify_failures}, indent=2))


if __name__ == "__main__":
    main()
