#!/usr/bin/env python3
"""Generate focused terrain camera-combination sweeps.

This script preserves the ad-hoc sweep that identified better TERRAIN camera
candidates after the first sparse firmware capture. It imports the terrain
algorithm harness from `03-host-terrain-renderer-tests.py`, renders combinations
of camera angle, camera height, and target Y, then writes PNGs, a CSV, and a
montage into the ticket artifacts directory.

Examples:

  python3 04-terrain-combo-sweep.py

  python3 04-terrain-combo-sweep.py \
    --out-dir ../artifacts/terrain-host-analysis-v2 \
    --cols 6
"""

from __future__ import annotations

import argparse
import csv
import importlib.util
import sys
from pathlib import Path


def load_terrain_harness():
    path = Path(__file__).with_name("03-host-terrain-renderer-tests.py")
    spec = importlib.util.spec_from_file_location("terrain_renderer_tests", path)
    if spec is None or spec.loader is None:
        raise SystemExit(f"could not load terrain harness: {path}")
    module = importlib.util.module_from_spec(spec)
    # Required for dataclasses on Python 3.13 when loading by file path.
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def parse_float_list(text: str) -> list[float]:
    return [float(part.strip()) for part in text.split(",") if part.strip()]


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    default_out = Path(__file__).resolve().parents[1] / "artifacts" / "terrain-host-analysis-v2"
    ap.add_argument("--out-dir", type=Path, default=default_out)
    ap.add_argument("--angles", default="0,0.4,0.8,1.2", help="comma-separated camera angles in radians")
    ap.add_argument("--heights", default="1.4,2.2,3.2", help="comma-separated camera heights")
    ap.add_argument("--target-ys", default="-1,0,1.5", help="comma-separated target Y values")
    ap.add_argument("--extent", type=float, default=14.0)
    ap.add_argument("--grid", type=int, default=32)
    ap.add_argument("--distance", type=float, default=11.0)
    ap.add_argument("--cols", type=int, default=6)
    ap.add_argument("--top", type=int, default=12, help="print top N rows by terrain pixel count")
    args = ap.parse_args()

    terrain = load_terrain_harness()
    out_dir = args.out_dir
    out_dir.mkdir(parents=True, exist_ok=True)

    rows: list[dict[str, object]] = []
    images: list[Path] = []
    for angle in parse_float_list(args.angles):
        for height in parse_float_list(args.heights):
            for target_y in parse_float_list(args.target_ys):
                name = f"a{angle:g}-h{height:g}-ty{target_y:g}"
                cfg = terrain.TerrainConfig(
                    extent=args.extent,
                    grid=args.grid,
                    distance=args.distance,
                    height=height,
                    target_y=target_y,
                    camera_angle=angle,
                )
                rgb, stats = terrain.render_terrain(cfg)
                path = out_dir / f"{name}.png"
                terrain.write_png(path, 240, 240, rgb)
                images.append(path)
                rows.append({"name": name, "file": path.name, **stats})

    csv_path = out_dir / "combo-stats.csv"
    with csv_path.open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)

    montage_path = out_dir / "combo-montage.png"
    terrain.write_montage(images, montage_path, cols=args.cols)

    print(f"wrote {len(rows)} terrain combination screenshots to {out_dir}")
    print(f"wrote {csv_path}")
    print(f"wrote {montage_path}")
    print("\nTop candidates by terrain_pixels:")
    for row in sorted(rows, key=lambda r: int(r["terrain_pixels"]), reverse=True)[: args.top]:
        print(
            row["name"],
            "terrain_pixels", row["terrain_pixels"],
            "triangles_drawn", row["triangles_drawn"],
            "bounds",
            row["projected_min_x"], row["projected_max_x"],
            row["projected_min_y"], row["projected_max_y"],
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
