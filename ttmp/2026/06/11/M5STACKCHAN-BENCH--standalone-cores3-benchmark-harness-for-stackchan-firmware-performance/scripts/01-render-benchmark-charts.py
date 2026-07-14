#!/usr/bin/env python3
"""Render M5StackChan standalone benchmark charts for the Obsidian article.

The script reads BENCH_SUMMARY lines from a monitor log when available and
falls back to the first successful hardware run captured during the benchmark
investigation. It writes PNG charts to the Obsidian vault image directory.
"""
from __future__ import annotations

import argparse
import math
import re
from pathlib import Path
from typing import Dict, Iterable, List

import matplotlib.pyplot as plt
import numpy as np

DEFAULT_LOG = Path("/tmp/stackchan-bench-monitor4.log")
DEFAULT_OUT = Path(
    "/home/manuel/code/wesen/go-go-golems/go-go-parc/Projects/2026/06/11/images-m5stackchan-draw-performance"
)

FALLBACK_SUMMARIES = [
    "BENCH_SUMMARY mode=delay_1_tick duration_ms=10009 loop_count=974 loop_hz=97 heap_internal_free=209727 heap_internal_min=209015 psram_free=8059436 lvgl_wait_count=100 lvgl_wait_min_us=27 lvgl_wait_avg_us=1917 lvgl_wait_p95_us=17729 lvgl_wait_max_us=29682 lvgl_hold_count=100 lvgl_hold_min_us=753 lvgl_hold_avg_us=814 lvgl_hold_p95_us=827 lvgl_hold_max_us=1097 rgb_count=51 rgb_min_us=6170 rgb_avg_us=6912 rgb_p95_us=6749 rgb_max_us=15988 asset_count=11 asset_min_us=50 asset_avg_us=12660 asset_p95_us=138756 asset_max_us=138756",
    "BENCH_SUMMARY mode=target_60_fps duration_ms=10009 loop_count=495 loop_hz=49 heap_internal_free=209515 heap_internal_min=209015 psram_free=8059436 lvgl_wait_count=100 lvgl_wait_min_us=25 lvgl_wait_avg_us=30 lvgl_wait_p95_us=33 lvgl_wait_max_us=42 lvgl_hold_count=100 lvgl_hold_min_us=755 lvgl_hold_avg_us=810 lvgl_hold_p95_us=830 lvgl_hold_max_us=945 rgb_count=50 rgb_min_us=6096 rgb_avg_us=6460 rgb_p95_us=6513 rgb_max_us=6530 asset_count=10 asset_min_us=53 asset_avg_us=58 asset_p95_us=82 asset_max_us=82",
    "BENCH_SUMMARY mode=target_30_fps duration_ms=10029 loop_count=251 loop_hz=25 heap_internal_free=209727 heap_internal_min=209015 psram_free=8059436 lvgl_wait_count=84 lvgl_wait_min_us=25 lvgl_wait_avg_us=279 lvgl_wait_p95_us=31 lvgl_wait_max_us=7114 lvgl_hold_count=84 lvgl_hold_min_us=767 lvgl_hold_avg_us=805 lvgl_hold_p95_us=831 lvgl_hold_max_us=884 rgb_count=50 rgb_min_us=6048 rgb_avg_us=6162 rgb_p95_us=6525 rgb_max_us=6525 asset_count=10 asset_min_us=53 asset_avg_us=62 asset_p95_us=66 asset_max_us=66",
    "BENCH_SUMMARY mode=yield duration_ms=10025 loop_count=1304294 loop_hz=130104 heap_internal_free=209727 heap_internal_min=209015 psram_free=8059436 lvgl_wait_count=101 lvgl_wait_min_us=26 lvgl_wait_avg_us=28 lvgl_wait_p95_us=30 lvgl_wait_max_us=34 lvgl_hold_count=101 lvgl_hold_min_us=749 lvgl_hold_avg_us=813 lvgl_hold_p95_us=834 lvgl_hold_max_us=839 rgb_count=51 rgb_min_us=6092 rgb_avg_us=6429 rgb_p95_us=6492 rgb_max_us=6495 asset_count=11 asset_min_us=52 asset_avg_us=56 asset_p95_us=103 asset_max_us=103",
]

MODE_LABELS = {
    "delay_1_tick": "Delay\n1 tick",
    "target_60_fps": "Target\n60 FPS",
    "target_30_fps": "Target\n30 FPS",
    "yield": "Yield\nonly",
}
MODE_ORDER = ["delay_1_tick", "target_60_fps", "target_30_fps", "yield"]


def parse_summary_line(line: str) -> Dict[str, int | str]:
    fields: Dict[str, int | str] = {}
    for key, value in re.findall(r"([A-Za-z0-9_]+)=([^\s]+)", line):
        if key == "mode":
            fields[key] = value
        else:
            try:
                fields[key] = int(value)
            except ValueError:
                fields[key] = value
    return fields


def load_rows(log_path: Path) -> List[Dict[str, int | str]]:
    lines: Iterable[str]
    if log_path.exists():
        found = [line.strip() for line in log_path.read_text(errors="ignore").splitlines() if "BENCH_SUMMARY" in line]
        lines = found if found else FALLBACK_SUMMARIES
    else:
        lines = FALLBACK_SUMMARIES

    rows = [parse_summary_line(line) for line in lines]
    by_mode = {row["mode"]: row for row in rows}
    return [by_mode[mode] for mode in MODE_ORDER if mode in by_mode]


def setup_style() -> None:
    plt.rcParams.update(
        {
            "figure.facecolor": "#fbfbfb",
            "axes.facecolor": "#ffffff",
            "axes.edgecolor": "#c8c8c8",
            "axes.labelcolor": "#222222",
            "text.color": "#222222",
            "xtick.color": "#333333",
            "ytick.color": "#333333",
            "grid.color": "#e6e6e6",
            "font.size": 11,
            "axes.titleweight": "bold",
            "axes.titlesize": 15,
            "figure.titlesize": 18,
        }
    )


def labels(rows: List[Dict[str, int | str]]) -> List[str]:
    return [MODE_LABELS[str(row["mode"])] for row in rows]


def save(fig: plt.Figure, out_dir: Path, filename: str) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)
    fig.tight_layout()
    fig.savefig(out_dir / filename, dpi=180, bbox_inches="tight")
    plt.close(fig)


def annotate_bars(ax: plt.Axes, bars, fmt: str = "{:.0f}", yscale: str = "linear") -> None:
    for bar in bars:
        height = bar.get_height()
        if height <= 0:
            continue
        offset = height * 0.04 if yscale == "linear" else math.pow(10, math.log10(height) + 0.04) - height
        ax.text(
            bar.get_x() + bar.get_width() / 2,
            height + offset,
            fmt.format(height),
            ha="center",
            va="bottom",
            fontsize=9,
        )


def chart_loop_rates(rows: List[Dict[str, int | str]], out_dir: Path) -> None:
    xs = np.arange(len(rows))
    vals = [int(row["loop_hz"]) for row in rows]
    fig, ax = plt.subplots(figsize=(9, 5.2))
    bars = ax.bar(xs, vals, color=["#5470c6", "#91cc75", "#fac858", "#ee6666"])
    ax.set_yscale("log")
    ax.set_xticks(xs, labels(rows))
    ax.set_ylabel("Loop iterations per second (log scale)")
    ax.set_title("Standalone benchmark loop rate by pacing mode")
    ax.set_ylim(10, max(vals) * 2.5)
    ax.grid(True, axis="y", which="both", linestyle="--", alpha=0.7)
    annotate_bars(ax, bars, "{:.0f}", yscale="log")
    ax.text(
        0.02,
        0.95,
        "Loop Hz is not display FPS; expensive work is throttled in the benchmark.",
        transform=ax.transAxes,
        fontsize=10,
        va="top",
        bbox=dict(boxstyle="round,pad=0.35", facecolor="#fff6dd", edgecolor="#e0b84a"),
    )
    save(fig, out_dir, "benchmark-loop-rate-by-mode.png")


def chart_lvgl_lock(rows: List[Dict[str, int | str]], out_dir: Path) -> None:
    xs = np.arange(len(rows))
    width = 0.2
    series = [
        ("wait avg", "lvgl_wait_avg_us", "#5470c6"),
        ("wait p95", "lvgl_wait_p95_us", "#73a6ff"),
        ("hold avg", "lvgl_hold_avg_us", "#91cc75"),
        ("hold p95", "lvgl_hold_p95_us", "#3ba272"),
    ]
    fig, ax = plt.subplots(figsize=(10, 5.6))
    for i, (name, key, color) in enumerate(series):
        ax.bar(xs + (i - 1.5) * width, [int(row[key]) for row in rows], width, label=name, color=color)
    ax.set_xticks(xs, labels(rows))
    ax.set_yscale("log")
    ax.set_ylabel("Microseconds (log scale)")
    ax.set_title("LVGL lock wait vs lock hold for small UI updates")
    ax.grid(True, axis="y", which="both", linestyle="--", alpha=0.7)
    ax.legend(ncol=4, loc="upper center", bbox_to_anchor=(0.5, -0.12), frameon=False)
    ax.text(
        0.02,
        0.95,
        "Hold time is stable around 0.8 ms; delay_1_tick wait has cold-start / scheduling outliers.",
        transform=ax.transAxes,
        fontsize=10,
        va="top",
        bbox=dict(boxstyle="round,pad=0.35", facecolor="#eef7ff", edgecolor="#8ab6e6"),
    )
    save(fig, out_dir, "benchmark-lvgl-lock-wait-hold.png")


def chart_peripheral_costs(rows: List[Dict[str, int | str]], out_dir: Path) -> None:
    xs = np.arange(len(rows))
    width = 0.22
    fig, ax = plt.subplots(figsize=(10, 5.8))
    rgb_avg = [int(row["rgb_avg_us"]) for row in rows]
    rgb_p95 = [int(row["rgb_p95_us"]) for row in rows]
    asset_avg = [int(row["asset_avg_us"]) for row in rows]
    asset_p95 = [int(row["asset_p95_us"]) for row in rows]
    ax.bar(xs - 1.5 * width, rgb_avg, width, label="RGB avg", color="#ee6666")
    ax.bar(xs - 0.5 * width, rgb_p95, width, label="RGB p95", color="#f6a6a6")
    ax.bar(xs + 0.5 * width, asset_avg, width, label="asset avg", color="#fac858")
    ax.bar(xs + 1.5 * width, asset_p95, width, label="asset p95", color="#d6a019")
    ax.set_yscale("log")
    ax.set_xticks(xs, labels(rows))
    ax.set_ylabel("Microseconds (log scale)")
    ax.set_title("Peripheral and asset costs measured outside LVGL lock timing")
    ax.grid(True, axis="y", which="both", linestyle="--", alpha=0.7)
    ax.legend(ncol=4, loc="upper center", bbox_to_anchor=(0.5, -0.12), frameon=False)
    ax.text(
        0.02,
        0.95,
        "RGB refresh is consistently ~6–7 ms; first asset lookup includes cold-start initialization.",
        transform=ax.transAxes,
        fontsize=10,
        va="top",
        bbox=dict(boxstyle="round,pad=0.35", facecolor="#fff6dd", edgecolor="#e0b84a"),
    )
    save(fig, out_dir, "benchmark-rgb-asset-costs.png")


def chart_transfer_ceiling(out_dir: Path) -> None:
    regions = [
        ("32×32", 32 * 32),
        ("64×64", 64 * 64),
        ("160×120", 160 * 120),
        ("320×120", 320 * 120),
        ("320×240\nfull screen", 320 * 240),
    ]
    bytes_per_sec = 40_000_000 / 8
    fps = [bytes_per_sec / (pixels * 2) for _, pixels in regions]
    xs = np.arange(len(regions))
    fig, ax = plt.subplots(figsize=(9.5, 5.4))
    bars = ax.bar(xs, fps, color=["#73c0de", "#5470c6", "#91cc75", "#fac858", "#ee6666"])
    ax.set_yscale("log")
    ax.set_xticks(xs, [name for name, _ in regions])
    ax.set_ylabel("Theoretical SPI payload FPS (log scale)")
    ax.set_title("40 MHz SPI transfer ceiling by invalidated region size")
    ax.grid(True, axis="y", which="both", linestyle="--", alpha=0.7)
    annotate_bars(ax, bars, "{:.1f}", yscale="log")
    ax.text(
        0.02,
        0.95,
        "Ceiling assumes RGB565 payload only: no commands, no DMA gaps, no LVGL render time.",
        transform=ax.transAxes,
        fontsize=10,
        va="top",
        bbox=dict(boxstyle="round,pad=0.35", facecolor="#eef7ff", edgecolor="#8ab6e6"),
    )
    save(fig, out_dir, "spi-transfer-ceiling-by-region.png")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--log", type=Path, default=DEFAULT_LOG, help="monitor log containing BENCH_SUMMARY lines")
    parser.add_argument("--out-dir", type=Path, default=DEFAULT_OUT, help="output directory for PNG charts")
    args = parser.parse_args()

    setup_style()
    rows = load_rows(args.log)
    chart_loop_rates(rows, args.out_dir)
    chart_lvgl_lock(rows, args.out_dir)
    chart_peripheral_costs(rows, args.out_dir)
    chart_transfer_ceiling(args.out_dir)

    print(f"Rendered {4} charts to {args.out_dir}")


if __name__ == "__main__":
    main()
