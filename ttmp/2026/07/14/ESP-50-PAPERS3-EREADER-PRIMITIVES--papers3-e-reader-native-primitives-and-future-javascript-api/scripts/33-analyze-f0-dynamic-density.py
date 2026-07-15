#!/usr/bin/env python3
"""Reproduce the exact-F0 dynamic density timeline, bins, change candidates, and SVG."""

from __future__ import annotations

import argparse
import csv
import html
import json
import statistics
from dataclasses import dataclass
from pathlib import Path
from typing import Any

TICKET = Path(__file__).resolve().parents[1]
EXP = TICKET / "scripts/experiments/EXP-20260715-008-factory-f0-dynamic-density"


@dataclass
class Sample:
    sequence: int
    monotonic_ns: int
    utc: str
    channel_0: int
    channel_1: int
    gain_index: int
    integration_index: int
    density: float
    saturated: bool
    valid: bool


def read_jsonl(path: Path) -> list[dict[str, Any]]:
    return [json.loads(line) for line in path.read_text().splitlines() if line.strip()]


def load_samples(path: Path) -> tuple[list[dict[str, Any]], list[Sample]]:
    records = read_jsonl(path)
    sequences = [record["sequence"] for record in records]
    if sequences != list(range(len(records))):
        raise ValueError("capture event sequence is not contiguous")
    samples = []
    for record in records:
        parsed = record.get("parsed", {})
        if parsed.get("kind") != "raw_sensor":
            continue
        derived = parsed["derived"]
        samples.append(
            Sample(
                sequence=record["sequence"],
                monotonic_ns=record["host_monotonic_ns"],
                utc=record["host_utc"],
                channel_0=parsed["channel_0"],
                channel_1=parsed["channel_1"],
                gain_index=parsed["gain_index"],
                integration_index=parsed["integration_index"],
                density=derived["density_estimate"],
                saturated=derived["saturated"],
                valid=derived["density_estimate_valid"],
            )
        )
    if not samples:
        raise ValueError("capture contains no raw samples")
    return records, samples


def make_bins(
    samples: list[Sample], start_ns: int, width_seconds: float
) -> list[dict[str, Any]]:
    bins: dict[int, list[Sample]] = {}
    for sample in samples:
        relative = (sample.monotonic_ns - start_ns) / 1e9
        bins.setdefault(int(relative / width_seconds), []).append(sample)
    result = []
    previous_mean: float | None = None
    for index in sorted(bins):
        values = [sample.density for sample in bins[index]]
        mean = statistics.mean(values)
        result.append(
            {
                "index": index,
                "start_seconds": index * width_seconds,
                "end_seconds": (index + 1) * width_seconds,
                "count": len(values),
                "mean_density": mean,
                "minimum_density": min(values),
                "maximum_density": max(values),
                "delta_from_previous_mean": None
                if previous_mean is None
                else mean - previous_mean,
            }
        )
        previous_mean = mean
    return result


def render_svg(
    samples: list[Sample],
    start_ns: int,
    markers: dict[str, int],
    candidates: list[dict[str, Any]],
    path: Path,
) -> None:
    width, height = 1400, 560
    left, right, top, bottom = 80, 30, 35, 65
    plot_w, plot_h = width - left - right, height - top - bottom
    times = [(sample.monotonic_ns - start_ns) / 1e9 for sample in samples]
    values = [sample.density for sample in samples]
    x_max = max(times)
    y_min = min(values) - 0.02
    y_max = max(values) + 0.02

    def x(value: float) -> float:
        return left + (value / x_max) * plot_w

    def y(value: float) -> float:
        return top + (y_max - value) / (y_max - y_min) * plot_h

    points = " ".join(f"{x(t):.2f},{y(value):.2f}" for t, value in zip(times, values))
    elements = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
        '<rect width="100%" height="100%" fill="white"/>',
        f'<rect x="{left}" y="{top}" width="{plot_w}" height="{plot_h}" fill="#fafafa" stroke="#999"/>',
    ]
    for seconds in range(0, int(x_max) + 1, 5):
        xpos = x(seconds)
        elements.append(
            f'<line x1="{xpos:.2f}" y1="{top}" x2="{xpos:.2f}" y2="{top + plot_h}" stroke="#ddd"/>'
        )
        elements.append(
            f'<text x="{xpos:.2f}" y="{height - 35}" text-anchor="middle" font-size="13">{seconds}</text>'
        )
    for step in range(6):
        value = y_min + (y_max - y_min) * step / 5
        ypos = y(value)
        elements.append(
            f'<line x1="{left}" y1="{ypos:.2f}" x2="{left + plot_w}" y2="{ypos:.2f}" stroke="#e5e5e5"/>'
        )
        elements.append(
            f'<text x="{left - 10}" y="{ypos + 4:.2f}" text-anchor="end" font-size="13">{value:.2f}</text>'
        )
    colors = {"flash_begin": "#8e44ad", "flash_runner_complete": "#c0392b"}
    for name in ("flash_begin", "flash_runner_complete"):
        relative = (markers[name] - start_ns) / 1e9
        xpos = x(relative)
        color = colors[name]
        elements.append(
            f'<line x1="{xpos:.2f}" y1="{top}" x2="{xpos:.2f}" y2="{top + plot_h}" stroke="{color}" stroke-width="2"/>'
        )
        elements.append(
            f'<text x="{xpos + 4:.2f}" y="{top + 16}" font-size="12" fill="{color}">{html.escape(name)}</text>'
        )
    for candidate in candidates:
        xpos = x(candidate["start_seconds"])
        elements.append(
            f'<line x1="{xpos:.2f}" y1="{top}" x2="{xpos:.2f}" y2="{top + plot_h}" stroke="#f39c12" stroke-opacity="0.35"/>'
        )
    elements.extend(
        [
            f'<polyline points="{points}" fill="none" stroke="#1769aa" stroke-width="1.6"/>',
            f'<text x="{width / 2}" y="22" text-anchor="middle" font-size="18">Exact F0 fixed-point Printalyzer density trace</text>',
            f'<text x="{width / 2}" y="{height - 10}" text-anchor="middle" font-size="14">Seconds from first raw sample</text>',
            f'<text transform="translate(18 {height / 2}) rotate(-90)" text-anchor="middle" font-size="14">Host-derived density (D)</text>',
            "</svg>",
        ]
    )
    path.write_text("\n".join(elements) + "\n")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--capture", type=Path, default=EXP / "raw-dynamic-f0.jsonl")
    parser.add_argument("--host-events", type=Path, default=EXP / "host-events.jsonl")
    parser.add_argument("--output-csv", type=Path, default=EXP / "density-timeline.csv")
    parser.add_argument(
        "--output-json", type=Path, default=EXP / "density-analysis.json"
    )
    parser.add_argument("--output-markdown", type=Path, default=EXP / "04-analysis.md")
    parser.add_argument("--output-svg", type=Path, default=EXP / "density-timeline.svg")
    parser.add_argument("--bin-seconds", type=float, default=0.5)
    parser.add_argument("--candidate-delta", type=float, default=0.01)
    args = parser.parse_args()

    records, samples = load_samples(args.capture)
    host = read_jsonl(args.host_events)
    markers = {record["event"]: record["host_monotonic_ns"] for record in host}
    required = {
        "raw_stream_confirmed",
        "flash_begin",
        "flash_runner_complete",
        "orchestrator_complete",
    }
    if missing := required - markers.keys():
        raise ValueError(f"missing host markers: {sorted(missing)}")
    start_ns = samples[0].monotonic_ns
    bins = make_bins(samples, start_ns, args.bin_seconds)
    candidates = [
        item
        for item in bins
        if item["delta_from_previous_mean"] is not None
        and abs(item["delta_from_previous_mean"]) >= args.candidate_delta
    ]
    cleanup = [
        record["line"] for record in records if record.get("event") == "tx_line"
    ][-3:]
    capture_end = next(
        record for record in reversed(records) if record.get("event") == "capture_end"
    )
    densities = [sample.density for sample in samples]
    result = {
        "schema": "esp50.f0-dynamic-density-analysis.v1",
        "capture": str(args.capture),
        "host_events": str(args.host_events),
        "sample_count": len(samples),
        "duration_seconds": (samples[-1].monotonic_ns - start_ns) / 1e9,
        "post_flash_runner_seconds": (
            samples[-1].monotonic_ns - markers["flash_runner_complete"]
        )
        / 1e9,
        "density": {
            "mean": statistics.mean(densities),
            "median": statistics.median(densities),
            "standard_deviation": statistics.stdev(densities),
            "minimum": min(densities),
            "maximum": max(densities),
            "range": max(densities) - min(densities),
        },
        "saturated_samples": sum(sample.saturated for sample in samples),
        "invalid_samples": sum(not sample.valid for sample in samples),
        "cleanup_commands": cleanup,
        "capture_result": capture_end["result"],
        "markers_relative_to_first_sample_seconds": {
            name: (value - start_ns) / 1e9 for name, value in markers.items()
        },
        "bin_seconds": args.bin_seconds,
        "candidate_delta": args.candidate_delta,
        "candidate_change_bins": candidates,
        "bins": bins,
    }
    args.output_json.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")

    with args.output_csv.open("w", newline="") as stream:
        writer = csv.writer(stream)
        writer.writerow(
            [
                "event_sequence",
                "utc",
                "seconds_from_first_sample",
                "seconds_from_flash_runner_complete",
                "channel_0",
                "channel_1",
                "gain_index",
                "integration_index",
                "density_estimate",
                "saturated",
                "valid",
            ]
        )
        for sample in samples:
            writer.writerow(
                [
                    sample.sequence,
                    sample.utc,
                    f"{(sample.monotonic_ns - start_ns) / 1e9:.9f}",
                    f"{(sample.monotonic_ns - markers['flash_runner_complete']) / 1e9:.9f}",
                    sample.channel_0,
                    sample.channel_1,
                    sample.gain_index,
                    sample.integration_index,
                    f"{sample.density:.12f}",
                    int(sample.saturated),
                    int(sample.valid),
                ]
            )

    marker_rows = "\n".join(
        f"| `{name}` | {(value - start_ns) / 1e9:.6f} s |"
        for name, value in markers.items()
    )
    candidate_rows = (
        "\n".join(
            f"| {item['start_seconds']:.1f} s | {item['mean_density']:.6f} D | "
            f"{item['delta_from_previous_mean']:+.6f} D |"
            for item in candidates
        )
        or "| — | — | — |"
    )
    args.output_markdown.write_text(
        f"""---
Title: "Analysis - EXP-20260715-008-factory-f0-dynamic-density"
Ticket: ESP-50-PAPERS3-EREADER-PRIMITIVES
Status: active
Topics:
    - papers3
    - eink
    - hardware-qualification
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Reproducible timeline and candidate change points for the exact-F0 fixed-point density trace."
LastUpdated: 2026-07-15T01:00:00Z
WhatFor: "Capture an objective fixed-point density trace of exact FactoryTest F0."
WhenToUse: "Establish F0 temporal black/white/grayscale behavior before source-derived F1/F2 runs."
---

# Exact F0 dynamic density analysis

## Automatic result

```text
samples: {len(samples)}
duration: {result["duration_seconds"]:.6f} seconds
post-flash-runner coverage: {result["post_flash_runner_seconds"]:.6f} seconds
saturated / invalid: {result["saturated_samples"]} / {result["invalid_samples"]}
density min / max: {min(densities):.6f} / {max(densities):.6f} D
cleanup: {" -> ".join(cleanup)}
capture result: {capture_end["result"]}
```

The fixed-point trace clearly detects multiple FactoryTest update blocks and the later periodic dashboard refresh. Exact title/black/white/grayscale endpoint labels remain provisional because F0 has no internal frame-boundary ring; F2 is required to join optical transitions to scheduler events without relying only on waveform shape.

## Host markers

| Marker | Relative to first sample |
|---|---:|
{marker_rows}

## Candidate 500 ms change bins

A candidate is a 500 ms bin whose mean differs from the previous bin by at least `{args.candidate_delta:.3f} D`. This is a deterministic activity detector, not a semantic phase classifier.

| Bin start | Mean density | Delta from previous bin |
|---:|---:|---:|
{candidate_rows}

## Files

- `density-timeline.csv` — every raw sample and both host-relative time axes.
- `density-analysis.json` — complete statistics, markers, bins, and candidates.
- `density-timeline.svg` — dependency-free visualization; orange lines mark candidate bins.
"""
    )
    render_svg(samples, start_ns, markers, candidates, args.output_svg)
    print(f"samples={len(samples)}")
    print(f"candidates={len(candidates)}")
    print(f"capture_result={capture_end['result']}")
    print(f"csv={args.output_csv}")
    print(f"json={args.output_json}")
    print(f"markdown={args.output_markdown}")
    print(f"svg={args.output_svg}")


if __name__ == "__main__":
    main()
