#!/usr/bin/env python3
"""Extract and approximately host-align an F2 ring from combined serial capture."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any

TICKET = Path(__file__).resolve().parents[1]
DEFAULT_EXP = TICKET / "scripts/experiments/EXP-20260715-011-factory-f2-ring-density"


def write_jsonl(path: Path, records: list[dict[str, Any]]) -> None:
    path.write_text(
        "".join(json.dumps(record, sort_keys=True) + "\n" for record in records)
    )


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--capture", type=Path, default=DEFAULT_EXP / "raw-dynamic-f2.jsonl"
    )
    parser.add_argument(
        "--transcript", type=Path, default=DEFAULT_EXP / "firmware-transcript.log"
    )
    parser.add_argument("--ring", type=Path, default=DEFAULT_EXP / "ring.jsonl")
    parser.add_argument(
        "--aligned-ring", type=Path, default=DEFAULT_EXP / "ring-host-aligned.jsonl"
    )
    parser.add_argument(
        "--alignment", type=Path, default=DEFAULT_EXP / "ring-alignment.json"
    )
    args = parser.parse_args()
    for output in (args.transcript, args.ring, args.aligned_ring, args.alignment):
        if output.exists():
            raise SystemExit(f"refusing to replace output: {output}")

    records = [
        json.loads(line)
        for line in args.capture.read_text().splitlines()
        if line.strip()
    ]
    firmware = [
        record
        for record in records
        if record.get("source") == "firmware" and record.get("event") == "rx_line"
    ]
    lines = [record["line"] for record in firmware]
    args.transcript.write_text("\n".join(lines) + "\n")
    begin = next(
        (record for record in firmware if "FACTORY_TRACE_DUMP_BEGIN" in record["line"]),
        None,
    )
    end = next(
        (
            record
            for record in reversed(firmware)
            if "FACTORY_TRACE_DUMP_END" in record["line"]
        ),
        None,
    )
    if begin is None or end is None:
        raise SystemExit("F2 ring dump markers absent")

    ring: list[dict[str, Any]] = []
    for record in firmware:
        line = record["line"]
        if not line.startswith("{"):
            continue
        try:
            item = json.loads(line)
        except json.JSONDecodeError:
            continue
        if item.get("schema") == 1:
            ring.append(item)
    if not ring or any(item.get("valid") is False for item in ring):
        raise SystemExit("ring empty or contains invalid records")
    sequences = [int(item["sequence"]) for item in ring]
    if sequences != list(range(sequences[0], sequences[0] + len(sequences))):
        raise SystemExit("ring sequence is not contiguous")
    idle = [item for item in ring if item.get("name") == "DISPLAY_IDLE"]
    if not idle:
        raise SystemExit("DISPLAY_IDLE absent from ring")
    required = {
        "POWER_ON_BEGIN",
        "FRAME_BEGIN",
        "FRAME_END",
        "POWER_OFF_END",
        "DISPLAY_IDLE",
    }
    names = {item.get("name") for item in ring}
    if missing := required - names:
        raise SystemExit(f"required ring event(s) absent: {sorted(missing)}")

    # Dump printing begins only after FactoryTraceDumpAfterDisplayIdle has called
    # waitDisplay(). Align the device's final idle event to the host receipt of
    # DUMP_BEGIN. This gives a useful relative device→host map but not a bounded
    # physical-time calibration: USB/printf delay before DUMP_BEGIN is unknown.
    anchor_device_us = max(int(item["timestamp_us"]) for item in idle)
    anchor_host_ns = int(begin["host_monotonic_ns"])
    aligned = []
    for item in ring:
        copy = dict(item)
        copy["host_monotonic_ns_estimate"] = (
            anchor_host_ns + (int(item["timestamp_us"]) - anchor_device_us) * 1000
        )
        copy["host_alignment"] = (
            "approximate: final DISPLAY_IDLE aligned to host receipt of FACTORY_TRACE_DUMP_BEGIN"
        )
        aligned.append(copy)
    write_jsonl(args.ring, ring)
    write_jsonl(args.aligned_ring, aligned)
    alignment = {
        "schema": "esp50.factory-f2-ring-host-alignment.v1",
        "capture": str(args.capture),
        "ring_records": len(ring),
        "sequence_begin": sequences[0],
        "sequence_end": sequences[-1],
        "dump_begin_host_monotonic_ns": anchor_host_ns,
        "dump_end_host_monotonic_ns": int(end["host_monotonic_ns"]),
        "idle_device_timestamp_us": anchor_device_us,
        "method": "map ring device timestamps linearly to host monotonic ns by aligning final DISPLAY_IDLE to host receipt of DUMP_BEGIN",
        "physical_precision_limit": "not bounded: printf and USB latency between idle and host DUMP_BEGIN receipt are not independently measured; use for relative sequence alignment, not sub-millisecond physical claims",
        "required_events": sorted(required),
    }
    args.alignment.write_text(json.dumps(alignment, indent=2, sort_keys=True) + "\n")
    print(f"ring_records={len(ring)}")
    print(f"ring={args.ring}")
    print(f"aligned_ring={args.aligned_ring}")
    print(f"alignment={args.alignment}")


if __name__ == "__main__":
    main()
