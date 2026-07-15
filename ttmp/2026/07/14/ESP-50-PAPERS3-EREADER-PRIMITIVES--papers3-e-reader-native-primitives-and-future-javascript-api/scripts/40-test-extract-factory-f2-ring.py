#!/usr/bin/env python3
"""Synthetic no-hardware test for F2 combined-capture ring extraction."""

from __future__ import annotations

import json
import subprocess
import tempfile
from pathlib import Path

SCRIPT = Path(__file__).with_name("38-extract-factory-f2-ring.py")


def event(sequence: int, line: str, host_ns: int) -> dict[str, object]:
    return {
        "schema": "esp50.synchronized-serial.v1",
        "sequence": sequence,
        "source": "firmware",
        "event": "rx_line",
        "host_monotonic_ns": host_ns,
        "host_utc_ns": 1_700_000_000_000_000_000 + host_ns,
        "host_utc": "2026-07-15T00:00:00.000000000Z",
        "line": line,
    }


def main() -> None:
    with tempfile.TemporaryDirectory(prefix="esp50-f2-ring-test-") as directory:
        root = Path(directory)
        capture = root / "capture.jsonl"
        ring = [
            {
                "schema": 1,
                "sequence": 4,
                "valid": True,
                "timestamp_us": 100,
                "name": "POWER_ON_BEGIN",
            },
            {
                "schema": 1,
                "sequence": 5,
                "valid": True,
                "timestamp_us": 200,
                "name": "FRAME_BEGIN",
            },
            {
                "schema": 1,
                "sequence": 6,
                "valid": True,
                "timestamp_us": 300,
                "name": "FRAME_END",
            },
            {
                "schema": 1,
                "sequence": 7,
                "valid": True,
                "timestamp_us": 400,
                "name": "POWER_OFF_END",
            },
            {
                "schema": 1,
                "sequence": 8,
                "valid": True,
                "timestamp_us": 500,
                "name": "DISPLAY_IDLE",
            },
        ]
        records = [
            event(0, "boot line", 1000),
            event(
                1,
                "FACTORY_TRACE_DUMP_BEGIN schema=esp50.factory-v05-runtime-trace.v1 begin=4 end=9 overwritten=0",
                2000,
            ),
            *[
                event(index + 2, json.dumps(item), 2100 + index * 100)
                for index, item in enumerate(ring)
            ],
            event(7, "FACTORY_TRACE_DUMP_END total=9", 3000),
        ]
        capture.write_text("".join(json.dumps(record) + "\n" for record in records))
        transcript, output_ring, aligned, alignment = (
            root / "firmware.log",
            root / "ring.jsonl",
            root / "aligned.jsonl",
            root / "alignment.json",
        )
        subprocess.run(
            [
                str(SCRIPT),
                "--capture",
                str(capture),
                "--transcript",
                str(transcript),
                "--ring",
                str(output_ring),
                "--aligned-ring",
                str(aligned),
                "--alignment",
                str(alignment),
            ],
            check=True,
        )
        extracted = [json.loads(line) for line in output_ring.read_text().splitlines()]
        mapped = [json.loads(line) for line in aligned.read_text().splitlines()]
        metadata = json.loads(alignment.read_text())
        assert [item["sequence"] for item in extracted] == [4, 5, 6, 7, 8]
        assert metadata["ring_records"] == 5
        assert metadata["idle_device_timestamp_us"] == 500
        assert mapped[0]["host_monotonic_ns_estimate"] == -398_000
        assert mapped[-1]["host_monotonic_ns_estimate"] == 2_000
        print("f2_ring_extract_test=PASS records=5")


if __name__ == "__main__":
    main()
