#!/usr/bin/env python3
"""Extract banners from a regular 2x15 red-grid illustration sheet.

This is for sheets like pastoral.png, foo.png, and marine.png where the designer
works directly on a red guide grid. The script detects the red grid lines, crops
inside each detected cell so guide lines are not included, trims white border,
normalizes each banner to a fixed width, and writes a manifest.
"""

from __future__ import annotations

import argparse
import json
import subprocess
from pathlib import Path


def identify_size(path: Path) -> tuple[int, int]:
    w, h = subprocess.check_output(["identify", "-format", "%w %h", str(path)]).decode().split()
    return int(w), int(h)


def run(cmd: list[str]) -> None:
    subprocess.check_call(cmd)


def read_rgb(path: Path, width: int, height: int) -> bytes:
    pixels = subprocess.check_output(["convert", str(path), "-depth", "8", "rgb:-"])
    if len(pixels) != width * height * 3:
        raise ValueError("unexpected RGB pixel byte count")
    return pixels


def group_positions(positions: list[int], max_gap: int = 2) -> list[int]:
    if not positions:
        return []
    groups: list[list[int]] = [[positions[0], positions[0]]]
    for p in positions[1:]:
        if p - groups[-1][1] <= max_gap:
            groups[-1][1] = p
        else:
            groups.append([p, p])
    return [round((a + b) / 2) for a, b in groups]


def is_red(r: int, g: int, b: int) -> bool:
    return r > 120 and (r - g) > 35 and (r - b) > 35


def detect_grid_lines(src: Path, width: int, height: int) -> tuple[list[int], list[int]]:
    rgb = read_rgb(src, width, height)
    ys: list[int] = []
    for y in range(height):
        count = 0
        off = y * width * 3
        for x in range(width):
            i = off + x * 3
            if is_red(rgb[i], rgb[i + 1], rgb[i + 2]):
                count += 1
        if count >= width * 0.35:
            ys.append(y)

    xs: list[int] = []
    for x in range(width):
        count = 0
        for y in range(height):
            i = (y * width + x) * 3
            if is_red(rgb[i], rgb[i + 1], rgb[i + 2]):
                count += 1
        if count >= height * 0.35:
            xs.append(x)

    x_lines = group_positions(xs)
    y_lines = group_positions(ys)
    if len(x_lines) < 3 or len(y_lines) < 2:
        raise RuntimeError(f"could not detect grid lines: x={x_lines}, y={y_lines}")
    return x_lines, y_lines


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--source", required=True, help="Grid sheet image, e.g. ~/Downloads/pastoral.png")
    ap.add_argument("--out-dir", required=True)
    ap.add_argument("--inset", type=int, default=6, help="Pixels to crop inside detected grid lines")
    ap.add_argument("--target-width", type=int, default=330)
    args = ap.parse_args()

    src = Path(args.source).expanduser().resolve()
    out_dir = Path(args.out_dir).expanduser().resolve()
    out_dir.mkdir(parents=True, exist_ok=True)
    width, height = identify_size(src)

    x_lines, y_lines = detect_grid_lines(src, width, height)
    manifest = {
        "source": str(src),
        "width": width,
        "height": height,
        "cols": len(x_lines) - 1,
        "rows": len(y_lines) - 1,
        "x_lines": x_lines,
        "y_lines": y_lines,
        "inset": args.inset,
        "banners": [],
    }

    stem = src.stem.replace(" ", "-")
    for row in range(len(y_lines) - 1):
        y0 = y_lines[row] + args.inset
        y1 = y_lines[row + 1] - 1 - args.inset
        for col in range(len(x_lines) - 1):
            x0 = x_lines[col] + args.inset
            x1 = x_lines[col + 1] - 1 - args.inset
            crop_w = x1 - x0 + 1
            crop_h = y1 - y0 + 1
            name = f"{stem}-banner-r{row + 1:02d}-c{col + 1:02d}.png"
            dst = out_dir / name
            run([
                "convert",
                str(src),
                "-crop",
                f"{crop_w}x{crop_h}+{x0}+{y0}",
                "+repage",
                "-fuzz",
                "8%",
                "-trim",
                "+repage",
                "-colorspace",
                "Gray",
                "-resize",
                f"{args.target_width}x",
                str(dst),
            ])
            manifest["banners"].append({
                "name": name,
                "row": row + 1,
                "column": col + 1,
                "source_bounds": {"x": x0, "y": y0, "width": crop_w, "height": crop_h},
                "path": str(dst),
            })

    manifest["count"] = len(manifest["banners"])
    (out_dir / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps({"out_dir": str(out_dir), "count": manifest["count"]}, indent=2))


if __name__ == "__main__":
    main()
