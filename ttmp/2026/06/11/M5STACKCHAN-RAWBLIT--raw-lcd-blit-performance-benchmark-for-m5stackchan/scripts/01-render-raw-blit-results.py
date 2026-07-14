#!/usr/bin/env python3
"""Render M5StackChan raw LCD blit benchmark charts from serial logs."""
from __future__ import annotations

import argparse
import re
from pathlib import Path

import matplotlib.pyplot as plt

SUMMARY_RE = re.compile(r"RAWBLIT_SUMMARY\s+(.*)$")
ALLOC_RE = re.compile(r"RAWBLIT_ALLOC_FAIL\s+(.*)$")


def parse_kv(payload: str) -> dict[str, str]:
    out: dict[str, str] = {}
    for part in payload.strip().split():
        if "=" in part:
            k, v = part.split("=", 1)
            out[k] = v
    return out


def parse_logs(paths: list[Path]) -> tuple[list[dict[str, object]], list[dict[str, object]]]:
    rows: list[dict[str, object]] = []
    allocs: list[dict[str, object]] = []
    for path in paths:
        if not path.exists():
            continue
        for raw in path.read_text(errors="ignore").splitlines():
            clean = re.sub(r"\x1b\[[0-9;]*m", "", raw)
            m = SUMMARY_RE.search(clean)
            if m:
                d = parse_kv(m.group(1))
                row: dict[str, object] = dict(d)
                row["log"] = str(path)
                for key in [
                    "w", "h", "x", "y", "chunk_h", "bytes_per_frame", "frames", "elapsed_us",
                    "fps_x100", "mbps_x100", "payload_util_x100", "clock_hz", "fill_avg_us",
                    "complete_avg_us", "heap_internal_free", "heap_internal_min",
                ]:
                    if key in row:
                        row[key] = int(str(row[key]))
                row["fps"] = int(row["fps_x100"]) / 100.0
                row["mbps"] = int(row["mbps_x100"]) / 100.0
                row["clock_mhz"] = int(row["clock_hz"]) // 1_000_000
                rows.append(row)
                continue
            m = ALLOC_RE.search(clean)
            if m:
                d = parse_kv(m.group(1))
                allocs.append({**d, "log": str(path)})
    return rows, allocs


def setup_ax(ax, title: str, ylabel: str) -> None:
    ax.set_title(title, fontsize=12, weight="bold")
    ax.set_ylabel(ylabel)
    ax.grid(axis="y", alpha=0.28)
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)


def autolabel(ax, bars, fmt="{:.1f}") -> None:
    for b in bars:
        h = b.get_height()
        ax.text(b.get_x() + b.get_width() / 2, h, fmt.format(h), ha="center", va="bottom", fontsize=8)


def save_fullscreen_chart(rows: list[dict[str, object]], out: Path) -> None:
    cases = [
        "full_320x240_chunk120", "full_320x240_chunk80", "full_320x240_chunk80_solid",
        "full_320x240_chunk40", "full_320x240_chunk20",
    ]
    labels = ["120 gen", "80 gen", "80 solid", "40 gen", "20 gen"]
    clocks = [40, 60, 80]
    fig, ax = plt.subplots(figsize=(11, 5.4), constrained_layout=True)
    width = 0.24
    xs = range(len(cases))
    colors = {40: "#4C78A8", 60: "#F58518", 80: "#54A24B"}
    for i, clock in enumerate(clocks):
        vals = []
        for case in cases:
            match = next((r for r in rows if r["case"] == case and r["clock_mhz"] == clock), None)
            vals.append(float(match["fps"]) if match else 0.0)
        positions = [x + (i - 1) * width for x in xs]
        bars = ax.bar(positions, vals, width=width, label=f"{clock} MHz requested", color=colors[clock])
        autolabel(ax, bars)
    ax.set_xticks(list(xs), labels, rotation=20, ha="right")
    setup_ax(ax, "Raw full-screen blit throughput by chunk height", "Frames/s")
    ax.legend(frameon=False)
    ax.text(0.01, -0.28, "Note: 60 MHz requested behaves like 40 MHz; 240-line full-frame allocation failed.",
            transform=ax.transAxes, fontsize=9)
    fig.savefig(out, dpi=180)
    plt.close(fig)


def save_partial_chart(rows: list[dict[str, object]], out: Path) -> None:
    cases = ["half_320x120_chunk40", "quarter_160x120_chunk40", "tile_80x60_chunk60", "tile_32x32_chunk32"]
    labels = ["320×120", "160×120", "80×60", "32×32"]
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 4.8), constrained_layout=True)
    width = 0.34
    xs = range(len(cases))
    colors = {40: "#4C78A8", 80: "#54A24B"}
    for i, clock in enumerate([40, 80]):
        fps_vals = []
        mb_vals = []
        for case in cases:
            match = next((r for r in rows if r["case"] == case and r["clock_mhz"] == clock), None)
            fps_vals.append(float(match["fps"]) if match else 0.0)
            mb_vals.append(float(match["mbps"]) if match else 0.0)
        pos = [x + (i - 0.5) * width for x in xs]
        b1 = ax1.bar(pos, fps_vals, width=width, label=f"{clock} MHz", color=colors[clock])
        b2 = ax2.bar(pos, mb_vals, width=width, label=f"{clock} MHz", color=colors[clock])
        autolabel(ax1, b1)
        autolabel(ax2, b2)
    ax1.set_xticks(list(xs), labels, rotation=20, ha="right")
    ax2.set_xticks(list(xs), labels, rotation=20, ha="right")
    setup_ax(ax1, "Partial-region FPS", "Frames/s")
    setup_ax(ax2, "Partial-region effective throughput", "MB/s")
    ax1.legend(frameon=False)
    ax2.legend(frameon=False)
    fig.savefig(out, dpi=180)
    plt.close(fig)


def save_latency_chart(rows: list[dict[str, object]], out: Path) -> None:
    cases = ["full_320x240_chunk120", "full_320x240_chunk80", "full_320x240_chunk40", "full_320x240_chunk20"]
    labels = ["120", "80", "40", "20"]
    fig, ax = plt.subplots(figsize=(10, 5.2), constrained_layout=True)
    for clock, color, marker in [(40, "#4C78A8", "o"), (80, "#54A24B", "s")]:
        vals = []
        for case in cases:
            match = next((r for r in rows if r["case"] == case and r["clock_mhz"] == clock), None)
            vals.append(int(match["complete_avg_us"]) / 1000.0 if match else 0.0)
        ax.plot(labels, vals, marker=marker, linewidth=2.5, label=f"{clock} MHz requested", color=color)
        for x, y in zip(labels, vals):
            ax.text(x, y, f"{y:.1f}", ha="center", va="bottom", fontsize=8)
    setup_ax(ax, "Average transfer-completion latency per full-screen chunk", "Milliseconds per chunk")
    ax.set_xlabel("Chunk height in lines")
    ax.legend(frameon=False)
    fig.savefig(out, dpi=180)
    plt.close(fig)


def write_markdown(rows: list[dict[str, object]], allocs: list[dict[str, object]], out: Path, image_dir: Path) -> None:
    best = max((r for r in rows if str(r["case"]).startswith("full_320x240")), key=lambda r: float(r["fps"]))
    lines = [
        "# Raw Blit Benchmark Results",
        "",
        "Generated from serial monitor logs under `/tmp/stackchan-rawblit-*-monitor.log`.",
        "",
        f"Best full-screen measured case: `{best['case']}` at {best['clock_mhz']} MHz requested, {best['fps']:.2f} FPS, {best['mbps']:.2f} MB/s.",
        "",
        "## Charts",
        "",
        f"![Full-screen throughput]({image_dir.name}/raw-blit-fullscreen-throughput.png)",
        "",
        f"![Partial region throughput]({image_dir.name}/raw-blit-partial-throughput.png)",
        "",
        f"![Chunk completion latency]({image_dir.name}/raw-blit-chunk-completion-latency.png)",
        "",
        "## Summary table",
        "",
        "| Clock | Case | FPS | MB/s | Chunk | Pattern | Avg complete us |",
        "|---:|---|---:|---:|---:|---|---:|",
    ]
    for r in sorted(rows, key=lambda x: (int(x["clock_mhz"]), str(x["case"]))):
        lines.append(
            f"| {r['clock_mhz']} MHz | `{r['case']}` | {float(r['fps']):.2f} | {float(r['mbps']):.2f} | {r['chunk_h']} | {r.get('pattern','')} | {r.get('complete_avg_us','')} |"
        )
    if allocs:
        lines += ["", "## Allocation failures", "", "| Log | Case | Bytes per chunk | Internal heap free |", "|---|---|---:|---:|"]
        for a in allocs:
            lines.append(f"| `{Path(str(a['log'])).name}` | `{a.get('case','')}` | {a.get('bytes_per_chunk','')} | {a.get('heap_internal_free','')} |")
    out.write_text("\n".join(lines) + "\n")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("logs", nargs="*", type=Path)
    args = parser.parse_args()
    logs = args.logs or sorted(Path("/tmp").glob("stackchan-rawblit-*-monitor.log"))
    args.output_dir.mkdir(parents=True, exist_ok=True)
    rows, allocs = parse_logs(logs)
    if not rows:
        raise SystemExit("no RAWBLIT_SUMMARY rows found")
    save_fullscreen_chart(rows, args.output_dir / "raw-blit-fullscreen-throughput.png")
    save_partial_chart(rows, args.output_dir / "raw-blit-partial-throughput.png")
    save_latency_chart(rows, args.output_dir / "raw-blit-chunk-completion-latency.png")
    write_markdown(rows, allocs, args.report, args.output_dir)
    print(f"parsed {len(rows)} summary rows from {len(logs)} logs")
    print(f"wrote {args.output_dir} and {args.report}")


if __name__ == "__main__":
    main()
