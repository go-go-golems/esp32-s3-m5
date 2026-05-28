#!/usr/bin/env python3
"""Generate host-side screenshots for 3D renderer buffer configurations.

This compares logical render resolutions and Z-buffer depth choices using the
host prototype from ticket 0097. It writes one PNG per configuration plus a CSV
of memory/performance counters. The goal is to choose a firmware starting point
from evidence rather than only from estimates.
"""

from __future__ import annotations

import argparse
import csv
import importlib.util
import sys
import time
from pathlib import Path
from types import SimpleNamespace

CONFIGS = [
    # logical, zbits, lat, lon
    (40, 8, 18, 28),
    (40, 16, 18, 28),
    (60, 8, 18, 28),
    (60, 16, 18, 28),
    (80, 8, 18, 28),
    (80, 16, 18, 28),
    (120, 8, 18, 28),
    (120, 16, 18, 28),
    (240, 8, 18, 28),
    (240, 16, 18, 28),
]

MESH_CONFIGS = [
    # logical, zbits, lat, lon
    (80, 16, 10, 16),
    (80, 16, 14, 22),
    (80, 16, 18, 28),
    (80, 16, 24, 36),
    (120, 16, 14, 22),
    (120, 16, 18, 28),
    (120, 16, 24, 36),
]


def load_prototype():
    prototype_path = Path(__file__).with_name("01-host-planet-renderer-prototype.py")
    spec = importlib.util.spec_from_file_location("planet_proto", prototype_path)
    if spec is None or spec.loader is None:
        raise SystemExit(f"could not load {prototype_path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def run_one(proto, out_dir: Path, logical: int, zbits: int, lat: int, lon: int, prefix: str) -> dict[str, object]:
    name = f"{prefix}-L{logical}-Z{zbits}-lat{lat}-lon{lon}"
    out = out_dir / f"{name}.png"
    args = SimpleNamespace(
        out=out,
        logical=logical,
        zbits=zbits,
        lat=lat,
        lon=lon,
        contrast=1.4,
        camera_angle=0.0,
        planet_angle=0.65,
        ring_angle=0.0,
    )
    start = time.perf_counter()
    rgb, stats = proto.render(args)
    render_ms = (time.perf_counter() - start) * 1000.0
    proto.write_png(out, 240, 240, rgb)

    row: dict[str, object] = {
        "name": name,
        "file": out.name,
        "logical": logical,
        "zbits": zbits,
        "lat": lat,
        "lon": lon,
        "host_render_ms": f"{render_ms:.3f}",
        **stats,
    }
    return row


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out-dir", type=Path, default=Path("artifacts/buffer-config-comparison"))
    args = ap.parse_args()

    proto = load_prototype()
    args.out_dir.mkdir(parents=True, exist_ok=True)

    rows: list[dict[str, object]] = []
    for logical, zbits, lat, lon in CONFIGS:
        rows.append(run_one(proto, args.out_dir, logical, zbits, lat, lon, "resolution"))
    for logical, zbits, lat, lon in MESH_CONFIGS:
        rows.append(run_one(proto, args.out_dir, logical, zbits, lat, lon, "mesh"))

    csv_path = args.out_dir / "stats.csv"
    fieldnames = list(rows[0].keys())
    with csv_path.open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)

    print(f"wrote {len(rows)} screenshots to {args.out_dir}")
    print(f"wrote {csv_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
