#!/usr/bin/env python3
"""Reproduce statistics and pairwise deltas for Printalyzer raw JSONL captures."""

from __future__ import annotations

import argparse
import json
import statistics
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Any

TICKET = Path(__file__).resolve().parents[1]
DEFAULT_CAPTURES = [
    (
        "EXP-004-initial",
        TICKET
        / "scripts/experiments/EXP-20260714-004-printalyzer-static-white-raw/raw-static-white.jsonl",
    ),
    (
        "EXP-005-repositioned",
        TICKET
        / "scripts/experiments/EXP-20260715-005-printalyzer-repositioned-white-raw/raw-repositioned-white.jsonl",
    ),
    (
        "EXP-006-fixed-repeat",
        TICKET
        / "scripts/experiments/EXP-20260715-006-printalyzer-fixed-position-repeat/raw-fixed-repeat.jsonl",
    ),
    (
        "EXP-007-hand-wave",
        TICKET
        / "scripts/experiments/EXP-20260715-007-printalyzer-hand-wave-perturbation/raw-hand-wave.jsonl",
    ),
]


@dataclass
class Metric:
    count: int
    mean: float
    median: float
    standard_deviation: float
    minimum: float
    maximum: float
    value_range: float
    maximum_absolute_excursion_from_median: float


@dataclass
class CaptureSummary:
    label: str
    path: str
    total_records: int
    raw_samples: int
    excluded_settling_samples: int
    post_settling_samples: int
    channel_0: Metric
    channel_1: Metric
    density: Metric
    saturated_samples: int
    invalid_density_estimates: int
    cleanup_commands: list[str]
    capture_result: str | None


def metric(values: list[float]) -> Metric:
    if not values:
        raise ValueError("cannot summarize an empty value list")
    median = statistics.median(values)
    return Metric(
        count=len(values),
        mean=statistics.mean(values),
        median=median,
        standard_deviation=statistics.stdev(values) if len(values) > 1 else 0.0,
        minimum=min(values),
        maximum=max(values),
        value_range=max(values) - min(values),
        maximum_absolute_excursion_from_median=max(
            abs(value - median) for value in values
        ),
    )


def load_records(path: Path) -> list[dict[str, Any]]:
    records = [
        json.loads(line) for line in path.read_text().splitlines() if line.strip()
    ]
    sequences = [record["sequence"] for record in records]
    if sequences != list(range(len(records))):
        raise ValueError(f"noncontiguous event sequence in {path}")
    return records


def summarize(label: str, path: Path, exclude_first: int) -> CaptureSummary:
    records = load_records(path)
    raw = [
        record
        for record in records
        if record.get("parsed", {}).get("kind") == "raw_sensor"
    ]
    if len(raw) <= exclude_first:
        raise ValueError(f"too few raw records in {path}")
    post = raw[exclude_first:]
    valid_density = [
        record["parsed"]["derived"]["density_estimate"]
        for record in post
        if record["parsed"]["derived"]["density_estimate_valid"]
    ]
    tx = [record["line"] for record in records if record.get("event") == "tx_line"]
    capture_end = next(
        (
            record
            for record in reversed(records)
            if record.get("event") == "capture_end"
        ),
        None,
    )
    return CaptureSummary(
        label=label,
        path=str(path),
        total_records=len(records),
        raw_samples=len(raw),
        excluded_settling_samples=exclude_first,
        post_settling_samples=len(post),
        channel_0=metric([record["parsed"]["channel_0"] for record in post]),
        channel_1=metric([record["parsed"]["channel_1"] for record in post]),
        density=metric(valid_density),
        saturated_samples=sum(
            record["parsed"]["derived"]["saturated"] for record in post
        ),
        invalid_density_estimates=sum(
            not record["parsed"]["derived"]["density_estimate_valid"] for record in post
        ),
        cleanup_commands=tx[-3:],
        capture_result=capture_end.get("result") if capture_end else None,
    )


def parse_capture(value: str) -> tuple[str, Path]:
    if "=" not in value:
        raise argparse.ArgumentTypeError("capture must use LABEL=PATH")
    label, path = value.split("=", 1)
    return label, Path(path)


def render_markdown(summaries: list[CaptureSummary], exclude_first: int) -> str:
    lines = [
        "---",
        "Title: Printalyzer Static Capture Calculations",
        "Ticket: ESP-50-PAPERS3-EREADER-PRIMITIVES",
        "Status: active",
        "Topics:",
        "    - papers3",
        "    - eink",
        "    - hardware-qualification",
        "DocType: reference",
        "Intent: long-term",
        "Owners: []",
        "RelatedFiles: []",
        "ExternalSources: []",
        'Summary: "Deterministic statistics and pairwise deltas for static Printalyzer raw captures."',
        "LastUpdated: 2026-07-15T01:15:00Z",
        'WhatFor: "Reproduce placement and repeatability calculations from immutable raw JSONL captures."',
        'WhenToUse: "Review static point-density stability, placement sensitivity, or capture validity."',
        "---",
        "",
        "# Printalyzer static capture calculations",
        "",
        f"Settling samples excluded from every capture: **{exclude_first}**.",
        "",
        "| Capture | post n | CH0 mean | Density mean | Density SD | Density range | Saturated | Invalid |",
        "|---|---:|---:|---:|---:|---:|---:|---:|",
    ]
    for summary in summaries:
        lines.append(
            f"| {summary.label} | {summary.post_settling_samples} | "
            f"{summary.channel_0.mean:.6f} | {summary.density.mean:.9f} D | "
            f"{summary.density.standard_deviation:.9f} D | "
            f"{summary.density.value_range:.9f} D | {summary.saturated_samples} | "
            f"{summary.invalid_density_estimates} |"
        )
    lines.extend(
        [
            "",
            "## Adjacent mean deltas",
            "",
            "| Earlier | Later | Signed delta | Absolute delta |",
            "|---|---|---:|---:|",
        ]
    )
    for earlier, later in zip(summaries, summaries[1:]):
        delta = later.density.mean - earlier.density.mean
        lines.append(
            f"| {earlier.label} | {later.label} | {delta:.9f} D | {abs(delta):.9f} D |"
        )
    lines.extend(["", "## Cleanup", ""])
    for summary in summaries:
        lines.append(
            f"- **{summary.label}:** result={summary.capture_result}; "
            f"commands=`{' -> '.join(summary.cleanup_commands)}`"
        )
    return "\n".join(lines) + "\n"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--capture", action="append", type=parse_capture, default=[])
    parser.add_argument("--exclude-first", type=int, default=2)
    parser.add_argument("--output-json", type=Path)
    parser.add_argument("--output-markdown", type=Path)
    args = parser.parse_args()
    captures = args.capture or DEFAULT_CAPTURES
    summaries = [summarize(label, path, args.exclude_first) for label, path in captures]
    payload = {
        "schema": "esp50.printalyzer-static-analysis.v1",
        "exclude_first": args.exclude_first,
        "captures": [asdict(summary) for summary in summaries],
        "adjacent_density_mean_deltas": [
            {
                "earlier": earlier.label,
                "later": later.label,
                "signed_delta": later.density.mean - earlier.density.mean,
                "absolute_delta": abs(later.density.mean - earlier.density.mean),
            }
            for earlier, later in zip(summaries, summaries[1:])
        ],
    }
    json_text = json.dumps(payload, indent=2, sort_keys=True) + "\n"
    markdown = render_markdown(summaries, args.exclude_first)
    if args.output_json:
        args.output_json.write_text(json_text)
    else:
        print(json_text, end="")
    if args.output_markdown:
        args.output_markdown.write_text(markdown)


if __name__ == "__main__":
    main()
