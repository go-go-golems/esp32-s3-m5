#!/usr/bin/env python3
"""Extract individual animal separator banners from a two-column sheet.

The script intentionally uses ImageMagick for image I/O so it does not require
Pillow. It thresholds the sheet, asks ImageMagick for connected components,
keeps the wide black components that correspond to full separator banners,
crops each banner with padding, normalizes it to a fixed thermal-layout width,
and writes PNG assets plus a manifest.
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
from pathlib import Path


def read_pgm(path: Path) -> tuple[int, int, bytes]:
    width_s, height_s = subprocess.check_output([
        "identify",
        "-format",
        "%w %h",
        str(path),
    ]).decode().split()
    width, height = int(width_s), int(height_s)
    pixels = subprocess.check_output([
        "convert",
        str(path),
        "-colorspace",
        "Gray",
        "-depth",
        "8",
        "gray:-",
    ])
    if len(pixels) != width * height:
        raise ValueError(f"unexpected pixel length {len(pixels)} for {width}x{height}")
    return width, height, pixels


def row_ink_counts(pixels: bytes, width: int, x0: int, x1: int, threshold: int) -> list[int]:
    counts: list[int] = []
    for y in range(len(pixels) // width):
        row = pixels[y * width + x0 : y * width + x1]
        counts.append(sum(1 for px in row if px < threshold))
    return counts


def find_runs(counts: list[int], min_count: int, merge_gap: int, min_height: int) -> list[tuple[int, int]]:
    raw: list[tuple[int, int]] = []
    start: int | None = None
    for y, count in enumerate(counts):
        if count >= min_count and start is None:
            start = y
        elif count < min_count and start is not None:
            raw.append((start, y - 1))
            start = None
    if start is not None:
        raw.append((start, len(counts) - 1))

    merged: list[tuple[int, int]] = []
    for run in raw:
        if not merged or run[0] - merged[-1][1] > merge_gap:
            merged.append(run)
        else:
            merged[-1] = (merged[-1][0], run[1])
    return [(a, b) for a, b in merged if b - a + 1 >= min_height]


def ink_bounds(pixels: bytes, width: int, x0: int, x1: int, y0: int, y1: int, threshold: int) -> tuple[int, int, int, int]:
    min_x, min_y = x1, y1
    max_x, max_y = x0, y0
    for y in range(y0, y1 + 1):
        row = pixels[y * width : (y + 1) * width]
        for x in range(x0, x1):
            if row[x] < threshold:
                if x < min_x:
                    min_x = x
                if x > max_x:
                    max_x = x
                if y < min_y:
                    min_y = y
                if y > max_y:
                    max_y = y
    if max_x < min_x or max_y < min_y:
        return (x0, y0, x1 - 1, y1)
    return (min_x, min_y, max_x, max_y)


def crop_with_convert(src: Path, dst: Path, bounds: tuple[int, int, int, int], target_width: int) -> None:
    x0, y0, x1, y1 = bounds
    w = x1 - x0 + 1
    h = y1 - y0 + 1
    subprocess.check_call([
        "convert",
        str(src),
        "-crop",
        f"{w}x{h}+{x0}+{y0}",
        "+repage",
        "-colorspace",
        "Gray",
        "-resize",
        f"{target_width}x",
        "-background",
        "white",
        "-gravity",
        "center",
        "-extent",
        f"{target_width}x{round(h * target_width / w)}",
        str(dst),
    ])


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--source", default="/home/manuel/Downloads/animal.png")
    ap.add_argument("--out-dir", required=True)
    ap.add_argument("--threshold-percent", type=str, default="88%")
    ap.add_argument("--min-component-width", type=int, default=300)
    ap.add_argument("--min-component-area", type=int, default=3000)
    ap.add_argument("--pad-x", type=int, default=14)
    ap.add_argument("--pad-y", type=int, default=10)
    ap.add_argument("--target-width", type=int, default=330)
    args = ap.parse_args()

    src = Path(args.source).expanduser().resolve()
    out_dir = Path(args.out_dir).expanduser().resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    width_s, height_s = subprocess.check_output([
        "identify",
        "-format",
        "%w %h",
        str(src),
    ]).decode().split()
    width, height = int(width_s), int(height_s)
    verbose = subprocess.check_output([
        "convert",
        str(src),
        "-colorspace",
        "gray",
        "-threshold",
        args.threshold_percent,
        "-define",
        "connected-components:verbose=true",
        "-connected-components",
        "8",
        "null:",
    ], stderr=subprocess.STDOUT).decode()

    component_re = re.compile(r"^\s*\d+:\s+(\d+)x(\d+)\+(\d+)\+(\d+)\s+[^\s]+\s+(\d+)\s+gray\(0\)", re.M)
    components = []
    for m in component_re.finditer(verbose):
        w, h, x, y, area = map(int, m.groups())
        if w >= args.min_component_width and area >= args.min_component_area:
            components.append({"x": x, "y": y, "width": w, "height": h, "area": area})
    components.sort(key=lambda c: (c["x"] >= width // 2, c["y"]))

    manifest = {
        "source": str(src),
        "width": width,
        "height": height,
        "threshold_percent": args.threshold_percent,
        "banners": [],
    }

    by_column = {1: [], 2: []}
    for c in components:
        by_column[1 if c["x"] < width // 2 else 2].append(c)

    for col, comps in by_column.items():
        comps.sort(key=lambda c: c["y"])
        for row, c in enumerate(comps, start=1):
            bx0 = max(0, c["x"] - args.pad_x)
            bx1 = min(width - 1, c["x"] + c["width"] - 1 + args.pad_x)
            by0 = max(0, c["y"] - args.pad_y)
            by1 = min(height - 1, c["y"] + c["height"] - 1 + args.pad_y)
            name = f"animal-banner-c{col:02d}-r{row:02d}.png"
            dst = out_dir / name
            crop_with_convert(src, dst, (bx0, by0, bx1, by1), args.target_width)
            manifest["banners"].append({
                "name": name,
                "column": col,
                "row": row,
                "source_bounds": {"x": bx0, "y": by0, "width": bx1 - bx0 + 1, "height": by1 - by0 + 1},
                "component_area": c["area"],
                "path": str(dst),
            })

    manifest["count"] = len(manifest["banners"])
    (out_dir / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps({"out_dir": str(out_dir), "count": manifest["count"]}, indent=2))


if __name__ == "__main__":
    main()
