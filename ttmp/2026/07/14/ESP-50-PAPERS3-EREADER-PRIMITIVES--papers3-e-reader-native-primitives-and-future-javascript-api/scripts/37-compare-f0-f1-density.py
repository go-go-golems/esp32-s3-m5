#!/usr/bin/env python3
"""Compare F0 and F1 fixed-point density traces after host-marker normalization."""

from __future__ import annotations

import argparse
import csv
import json
import math
import statistics
from dataclasses import dataclass
from pathlib import Path

TICKET = Path(__file__).resolve().parents[1]
F0 = TICKET / "scripts/experiments/EXP-20260715-008-factory-f0-dynamic-density"
F1 = TICKET / "scripts/experiments/EXP-20260715-010-factory-f1-density-only-control"


@dataclass(frozen=True)
class Point:
    time: float
    density: float


def load_csv(path: Path) -> list[Point]:
    rows = [
        Point(
            float(row["seconds_from_flash_runner_complete"]),
            float(row["density_estimate"]),
        )
        for row in csv.DictReader(path.open())
    ]
    if len(rows) < 3 or any(
        later.time <= earlier.time for earlier, later in zip(rows, rows[1:])
    ):
        raise ValueError(f"invalid/nonmonotonic timeline: {path}")
    return rows


def interpolate(points: list[Point], time: float) -> float | None:
    if time < points[0].time or time > points[-1].time:
        return None
    for left, right in zip(points, points[1:]):
        if right.time >= time:
            fraction = (time - left.time) / (right.time - left.time)
            return left.density + fraction * (right.density - left.density)
    return points[-1].density


def baseline(points: list[Point], seconds: float) -> float:
    values = [point.density for point in points if 0 <= point.time <= seconds]
    if not values:
        raise ValueError("baseline window has no samples")
    return statistics.mean(values)


def pearson(left: list[float], right: list[float]) -> float:
    if len(left) != len(right) or len(left) < 3:
        raise ValueError("invalid correlation inputs")
    left_mean, right_mean = statistics.mean(left), statistics.mean(right)
    numerator = sum((x - left_mean) * (y - right_mean) for x, y in zip(left, right))
    left_energy = sum((x - left_mean) ** 2 for x in left)
    right_energy = sum((y - right_mean) ** 2 for y in right)
    return numerator / math.sqrt(left_energy * right_energy)


def compare(
    f0: list[Point],
    f1: list[Point],
    baseline_seconds: float,
    start: float,
    end: float,
    step: float,
    lag: float,
) -> tuple[float, float, int]:
    base_f0, base_f1 = baseline(f0, baseline_seconds), baseline(f1, baseline_seconds)
    left, right = [], []
    n = int(round((end - start) / step)) + 1
    for index in range(n):
        time = start + index * step
        value_f0, value_f1 = interpolate(f0, time), interpolate(f1, time + lag)
        if value_f0 is not None and value_f1 is not None:
            left.append(value_f0 - base_f0)
            right.append(value_f1 - base_f1)
    rms = math.sqrt(statistics.mean((x - y) ** 2 for x, y in zip(left, right)))
    return pearson(left, right), rms, len(left)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--f0", type=Path, default=F0 / "density-timeline.csv")
    parser.add_argument("--f1", type=Path, default=F1 / "density-timeline.csv")
    parser.add_argument("--baseline-seconds", type=float, default=1.0)
    parser.add_argument("--start-seconds", type=float, default=0.0)
    parser.add_argument("--end-seconds", type=float, default=25.0)
    parser.add_argument("--step-seconds", type=float, default=0.1)
    parser.add_argument("--max-lag-seconds", type=float, default=3.0)
    parser.add_argument("--lag-step-seconds", type=float, default=0.1)
    parser.add_argument(
        "--output-json", type=Path, default=F1 / "f0-f1-comparison.json"
    )
    parser.add_argument(
        "--output-markdown", type=Path, default=F1 / "05-f0-comparison.md"
    )
    args = parser.parse_args()
    f0, f1 = load_csv(args.f0), load_csv(args.f1)
    lags = [
        round(index * args.lag_step_seconds, 9)
        for index in range(
            int(round(-args.max_lag_seconds / args.lag_step_seconds)),
            int(round(args.max_lag_seconds / args.lag_step_seconds)) + 1,
        )
    ]
    candidates = []
    for lag in lags:
        correlation, rms, count = compare(
            f0,
            f1,
            args.baseline_seconds,
            args.start_seconds,
            args.end_seconds,
            args.step_seconds,
            lag,
        )
        candidates.append(
            {
                "lag_seconds_f1_sampled_at_f0_plus_lag": lag,
                "pearson": correlation,
                "rms_delta_d": rms,
                "count": count,
            }
        )
    best = max(candidates, key=lambda item: item["pearson"])
    result = {
        "schema": "esp50.f0-f1-density-comparison.v1",
        "f0": str(args.f0),
        "f1": str(args.f1),
        "baseline_seconds": args.baseline_seconds,
        "comparison_window_seconds": [args.start_seconds, args.end_seconds],
        "resample_step_seconds": args.step_seconds,
        "f0_baseline_density": baseline(f0, args.baseline_seconds),
        "f1_baseline_density": baseline(f1, args.baseline_seconds),
        "best_alignment": best,
        "all_lags": candidates,
        "interpretation": "Pearson correlation compares baseline-subtracted point-density shape after host flash-runner completion. It does not prove spatial equivalence, absolute-density equivalence after movement, or semantic display-phase identity.",
    }
    args.output_json.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")
    args.output_markdown.write_text(
        f"""---
Title: "F0/F1 Fixed-Point Density Comparison"
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
Summary: "Deterministic host-marker-normalized F0/F1 point-density shape comparison."
LastUpdated: 2026-07-15T01:45:00Z
WhatFor: "Decide whether F1 trace-off is an adequate fixed-point source proxy before F2."
WhenToUse: "Review after F1 density-only capture and before F2 physical authorization."
---

# F0/F1 fixed-point density comparison

## Result

The best baseline-subtracted shape correlation over `{args.start_seconds:.1f}`–`{args.end_seconds:.1f}` seconds after flash-runner completion is **{best["pearson"]:.6f}** with F1 sampled at F0 time plus `{best["lag_seconds_f1_sampled_at_f0_plus_lag"]:+.1f}` seconds. Equivalently, shift the F1 timeline by `{(-best["lag_seconds_f1_sampled_at_f0_plus_lag"]):+.1f}` seconds to overlay it on F0. The normalized RMS difference is `{best["rms_delta_d"]:.6f} D` over `{best["count"]}` resampled points.

```text
F0 first-{args.baseline_seconds:.1f}s baseline: {result["f0_baseline_density"]:.9f} D
F1 first-{args.baseline_seconds:.1f}s baseline: {result["f1_baseline_density"]:.9f} D
best Pearson correlation: {best["pearson"]:.6f}
best F1 sample lag relative to F0: {best["lag_seconds_f1_sampled_at_f0_plus_lag"]:+.1f} s
normalized RMS difference: {best["rms_delta_d"]:.6f} D
```

## Limits

This compares only baseline-subtracted shape at the unchanged Printalyzer aperture, normalized by each run's host `flash_runner_complete` marker. It cannot prove spatial equivalence, absolute optical density equivalence after a movement, or that each candidate maps to the same semantic display phase. It is nevertheless the required trace-off observer/source control for deciding whether F2 is meaningful.
"""
    )
    print(f"best_pearson={best['pearson']:.9f}")
    print(f"best_lag={best['lag_seconds_f1_sampled_at_f0_plus_lag']:+.1f}")
    print(f"rms_delta_d={best['rms_delta_d']:.9f}")
    print(f"json={args.output_json}")
    print(f"markdown={args.output_markdown}")


if __name__ == "__main__":
    main()
