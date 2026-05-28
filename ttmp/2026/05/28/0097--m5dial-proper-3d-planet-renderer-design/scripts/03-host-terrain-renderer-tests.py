#!/usr/bin/env python3
"""Host-side terrain renderer tests and analysis harness.

This script mirrors the firmware terrain3d algorithm closely enough to debug it
without reflashing the M5Dial after every camera/extent/color change.

It intentionally uses only Python's standard library. It can:

  * run algorithm sanity tests,
  * render one terrain configuration,
  * sweep terrain/camera configurations into PNGs + CSV,
  * generate a simple montage for visual comparison.

Examples:

  python3 03-host-terrain-renderer-tests.py test

  python3 03-host-terrain-renderer-tests.py render \
    --out artifacts/terrain-host-analysis/current.png \
    --extent 14 --grid 32 --distance 11 --height 3.2 --target-y 1.5

  python3 03-host-terrain-renderer-tests.py sweep \
    --out-dir artifacts/terrain-host-analysis
"""

from __future__ import annotations

import argparse
import binascii
import csv
import math
import struct
import sys
import unittest
import zlib
from dataclasses import dataclass
from pathlib import Path

R3D_W = 80
R3D_H = 80
SCALE = 3
FB_W = 240
FB_H = 240
ZMAX = 0xFFFF
NEAR = 3.0
FAR = 16.0

BAYER4 = (
    (0, 8, 2, 10),
    (12, 4, 14, 6),
    (3, 11, 1, 9),
    (15, 7, 13, 5),
)

PALETTE = (
    (0, 0, 0),        # black
    (255, 41, 64),    # warm red
    (48, 80, 255),    # cool blue
    (255, 255, 255),  # high white
)

COLOR_BLACK = 0
COLOR_WARM = 1
COLOR_COOL = 2
COLOR_HIGH = 3


@dataclass
class Vertex:
    x: float
    y: float
    z: float
    r: float
    g: float
    b: float


@dataclass
class Projected:
    x: float
    y: float
    z: float
    r: float
    g: float
    b: float
    visible: bool = True


@dataclass
class TerrainConfig:
    logical: int = 80
    grid: int = 32
    extent: float = 14.0
    distance: float = 11.0
    height: float = 3.2
    target_y: float = 1.5
    camera_angle: float = 0.0
    contrast: float = 1.4
    cull_backfaces: bool = False
    rotate_x_degrees: float = 0.0
    fit_mode: str = "firmware"


def png_chunk(kind: bytes, payload: bytes) -> bytes:
    return struct.pack(">I", len(payload)) + kind + payload + struct.pack(">I", binascii.crc32(kind + payload) & 0xFFFFFFFF)


def write_png(path: Path, width: int, height: int, rgb: bytes) -> None:
    raw = bytearray()
    stride = width * 3
    for y in range(height):
        raw.append(0)
        raw.extend(rgb[y * stride : (y + 1) * stride])
    ihdr = struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)
    png = b"\x89PNG\r\n\x1a\n" + png_chunk(b"IHDR", ihdr) + png_chunk(b"IDAT", zlib.compress(bytes(raw), 9)) + png_chunk(b"IEND", b"")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(png)


def read_png_dimensions_and_rgb(path: Path) -> tuple[int, int, bytes]:
    # Minimal decoder for PNGs written by write_png: non-interlaced RGB, filter 0.
    data = path.read_bytes()
    if not data.startswith(b"\x89PNG\r\n\x1a\n"):
        raise ValueError(f"not a PNG: {path}")
    pos = 8
    width = height = None
    idat = bytearray()
    while pos < len(data):
        size = struct.unpack(">I", data[pos : pos + 4])[0]
        kind = data[pos + 4 : pos + 8]
        payload = data[pos + 8 : pos + 8 + size]
        pos += 12 + size
        if kind == b"IHDR":
            width, height, bit_depth, color_type, *_ = struct.unpack(">IIBBBBB", payload)
            if bit_depth != 8 or color_type != 2:
                raise ValueError("unsupported PNG format")
        elif kind == b"IDAT":
            idat.extend(payload)
        elif kind == b"IEND":
            break
    if width is None or height is None:
        raise ValueError("missing IHDR")
    raw = zlib.decompress(bytes(idat))
    stride = width * 3
    rgb = bytearray()
    off = 0
    for _ in range(height):
        filt = raw[off]
        off += 1
        if filt != 0:
            raise ValueError("unsupported PNG filter")
        rgb.extend(raw[off : off + stride])
        off += stride
    return width, height, bytes(rgb)


def write_montage(image_paths: list[Path], out: Path, cols: int = 4, label_height: int = 0) -> None:
    imgs = [read_png_dimensions_and_rgb(p) for p in image_paths]
    if not imgs:
        return
    w, h = imgs[0][0], imgs[0][1]
    rows = (len(imgs) + cols - 1) // cols
    canvas = bytearray([255] * (cols * w * rows * h * 3))
    canvas_w = cols * w
    for idx, (_, _, rgb) in enumerate(imgs):
        ox = (idx % cols) * w
        oy = (idx // cols) * h
        for y in range(h):
            dst = ((oy + y) * canvas_w + ox) * 3
            src = y * w * 3
            canvas[dst : dst + w * 3] = rgb[src : src + w * 3]
    write_png(out, canvas_w, rows * h, bytes(canvas))


def clamp(v: float, lo: float, hi: float) -> float:
    return lo if v < lo else hi if v > hi else v


def terrain_noise2d(x: float, y: float) -> float:
    return (
        math.sin(x * 0.45 + 1.2) * math.cos(y * 0.31 + 0.4) * 0.55
        + math.sin(x * 1.1 + 0.5) * math.cos(y * 0.78 + 1.3) * 0.30
        + math.sin(x * 2.3 + 2.1) * math.cos(y * 1.7 + 0.2) * 0.15
    )


def build_terrain(cfg: TerrainConfig) -> tuple[list[Vertex], list[tuple[int, int, int]]]:
    verts: list[Vertex] = []
    step = cfg.extent / (cfg.grid - 1)
    rx = math.radians(cfg.rotate_x_degrees)
    crx, srx = math.cos(rx), math.sin(rx)

    for gy in range(cfg.grid):
        for gx in range(cfg.grid):
            x = -cfg.extent * 0.5 + gx * step
            z = -cfg.extent * 0.5 + gy * step
            h = terrain_noise2d(x * 0.4, z * 0.4) * 4.5
            dist_c = math.sqrt(x * x + z * z)
            h += (1.0 - min(1.0, dist_c / 8.0)) * -1.0

            # Optional test hook: apply the JSX rotateX(-PI/2)-style orientation
            # to see whether the firmware coordinate assumptions are wrong.
            y2 = h * crx - z * srx
            z2 = h * srx + z * crx

            t = clamp((h + 2.0) / 6.0, 0.0, 1.0)
            r = 0.04 + t * 0.12
            g = 0.06 + t * 0.16
            b = 0.38 + t * 0.54
            verts.append(Vertex(x, y2, z2, r, g, b))

    tris: list[tuple[int, int, int]] = []
    for gy in range(cfg.grid - 1):
        for gx in range(cfg.grid - 1):
            tl = gy * cfg.grid + gx
            tr = tl + 1
            bl = tl + cfg.grid
            br = bl + 1
            tris.append((tl, bl, tr))
            tris.append((tr, bl, br))
    return verts, tris


def camera_basis(cfg: TerrainConfig):
    eye = (
        math.sin(cfg.camera_angle) * cfg.distance,
        cfg.target_y + cfg.height,
        math.cos(cfg.camera_angle) * cfg.distance,
    )
    target = (0.0, cfg.target_y, 0.0)
    fx, fy, fz = target[0] - eye[0], target[1] - eye[1], target[2] - eye[2]
    fl = math.sqrt(fx * fx + fy * fy + fz * fz) or 1.0
    fx, fy, fz = fx / fl, fy / fl, fz / fl
    rx, ry, rz = fz, 0.0, -fx
    rl = math.sqrt(rx * rx + rz * rz) or 1.0
    rx, rz = rx / rl, rz / rl
    ux = fy * rz - fz * ry
    uy = fz * rx - fx * rz
    uz = fx * ry - fy * rx
    return eye, (rx, ry, rz), (ux, uy, uz), (fx, fy, fz)


def project(v: Vertex, basis, cfg: TerrainConfig) -> Projected:
    eye, right, up, fwd = basis
    dx, dy, dz = v.x - eye[0], v.y - eye[1], v.z - eye[2]
    vx = dx * right[0] + dy * right[1] + dz * right[2]
    vy = dx * up[0] + dy * up[1] + dz * up[2]
    vz = dx * fwd[0] + dy * fwd[1] + dz * fwd[2]
    if vz <= 0.1:
        return Projected(0, 0, 0, v.r, v.g, v.b, False)
    focal = 2.145
    lw = cfg.logical
    sx = (vx * focal / vz) * (lw * 0.5) + lw * 0.5
    sy = -(vy * focal / vz) * (lw * 0.5) + lw * 0.5
    return Projected(sx, sy, vz, v.r, v.g, v.b, True)


def quantize(r: float, g: float, b: float, x: int, y: int, contrast: float) -> int:
    r = clamp((r - 0.5) * contrast + 0.5, 0.0, 1.0)
    g = clamp((g - 0.5) * contrast + 0.5, 0.0, 1.0)
    b = clamp((b - 0.5) * contrast + 0.5, 0.0, 1.0)
    t = BAYER4[y & 3][x & 3] / 16.0
    lum = (r + g + b) / 3.0
    if r > 0.55 and g > 0.55 and b > 0.55 and lum > t:
        return COLOR_HIGH
    if r > b + 0.05:
        return COLOR_WARM if r > t else COLOR_BLACK
    if b > r + 0.05:
        return COLOR_COOL if b > t else COLOR_BLACK
    if lum > 0.25 and lum > t:
        return COLOR_COOL
    return COLOR_BLACK


def edge(a: Projected, b: Projected, px: float, py: float) -> float:
    return (b.x - a.x) * (py - a.y) - (b.y - a.y) * (px - a.x)


def rasterize(tris, projected, cfg: TerrainConfig) -> tuple[list[int], dict[str, int]]:
    lw = cfg.logical
    colorbuf = [COLOR_BLACK] * (lw * lw)
    zbuf = [ZMAX] * (lw * lw)
    stats = {
        "triangles_submitted": 0,
        "triangles_drawn": 0,
        "terrain_pixels": 0,
        "visible_vertices": sum(1 for p in projected if p.visible),
        "projected_min_x": 9999,
        "projected_max_x": -9999,
        "projected_min_y": 9999,
        "projected_max_y": -9999,
    }
    for p in projected:
        if p.visible:
            stats["projected_min_x"] = min(stats["projected_min_x"], int(math.floor(p.x)))
            stats["projected_max_x"] = max(stats["projected_max_x"], int(math.ceil(p.x)))
            stats["projected_min_y"] = min(stats["projected_min_y"], int(math.floor(p.y)))
            stats["projected_max_y"] = max(stats["projected_max_y"], int(math.ceil(p.y)))

    for i0, i1, i2 in tris:
        stats["triangles_submitted"] += 1
        a, b, c = projected[i0], projected[i1], projected[i2]
        if not (a.visible and b.visible and c.visible):
            continue
        area = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x)
        if abs(area) < 0.01:
            continue
        if cfg.cull_backfaces and area >= -0.01:
            continue
        inv_area = 1.0 / area
        minx = max(0, int(math.floor(min(a.x, b.x, c.x))))
        maxx = min(lw - 1, int(math.ceil(max(a.x, b.x, c.x))))
        miny = max(0, int(math.floor(min(a.y, b.y, c.y))))
        maxy = min(lw - 1, int(math.ceil(max(a.y, b.y, c.y))))
        drew = False
        for y in range(miny, maxy + 1):
            py = y + 0.5
            for x in range(minx, maxx + 1):
                px = x + 0.5
                w0 = edge(b, c, px, py) * inv_area
                w1 = edge(c, a, px, py) * inv_area
                w2 = edge(a, b, px, py) * inv_area
                if area < 0:
                    if w0 < 0 or w1 < 0 or w2 < 0:
                        continue
                else:
                    if w0 > 0 or w1 > 0 or w2 > 0:
                        continue
                    w0, w1, w2 = -w0, -w1, -w2
                z = w0 * a.z + w1 * b.z + w2 * c.z
                zi = int(clamp((z - NEAR) / (FAR - NEAR), 0.0, 1.0) * ZMAX + 0.5)
                idx = y * lw + x
                if zi >= zbuf[idx]:
                    continue
                zbuf[idx] = zi
                rr = w0 * a.r + w1 * b.r + w2 * c.r
                gg = w0 * a.g + w1 * b.g + w2 * c.g
                bb = w0 * a.b + w1 * b.b + w2 * c.b
                colorbuf[idx] = quantize(rr, gg, bb, x, y, cfg.contrast)
                stats["terrain_pixels"] += 1
                drew = True
        if drew:
            stats["triangles_drawn"] += 1
    return colorbuf, stats


def draw_sun(colorbuf: list[int], cfg: TerrainConfig, basis) -> int:
    sun = Vertex(0.0, 4.5, -8.0, 1.0, 0.12, 0.18)
    p = project(sun, basis, cfg)
    if not p.visible:
        return 0
    lw = cfg.logical
    halo_r = lw * 0.065
    core_r = lw * 0.045
    drawn = 0
    for y in range(max(0, int(p.y - halo_r - 1)), min(lw, int(p.y + halo_r + 2))):
        for x in range(max(0, int(p.x - halo_r - 1)), min(lw, int(p.x + halo_r + 2))):
            dx, dy = x + 0.5 - p.x, y + 0.5 - p.y
            d2 = dx * dx + dy * dy
            if d2 <= core_r * core_r or (d2 <= halo_r * halo_r and 7 > BAYER4[y & 3][x & 3]):
                colorbuf[y * lw + x] = COLOR_WARM
                drawn += 1
    return drawn


def expand(colorbuf: list[int], cfg: TerrainConfig) -> bytes:
    lw = cfg.logical
    scale = FB_W // lw
    rgb = bytearray(FB_W * FB_H * 3)
    mask_radius = 116
    for y in range(lw):
        for x in range(lw):
            c = colorbuf[y * lw + x]
            if c == COLOR_BLACK:
                continue
            col = PALETTE[c]
            for yy in range(scale):
                py = y * scale + yy
                for xx in range(scale):
                    px = x * scale + xx
                    dx, dy = px - 120, py - 120
                    if dx * dx + dy * dy <= mask_radius * mask_radius:
                        off = (py * FB_W + px) * 3
                        rgb[off : off + 3] = bytes(col)
    return bytes(rgb)


def render_terrain(cfg: TerrainConfig) -> tuple[bytes, dict[str, int | float | str]]:
    verts, tris = build_terrain(cfg)
    basis = camera_basis(cfg)
    projected = [project(v, basis, cfg) for v in verts]
    colorbuf, stats = rasterize(tris, projected, cfg)
    sun_pixels = draw_sun(colorbuf, cfg, basis)
    rgb = expand(colorbuf, cfg)
    stats.update(
        {
            "grid": cfg.grid,
            "extent": cfg.extent,
            "distance": cfg.distance,
            "height": cfg.height,
            "target_y": cfg.target_y,
            "angle": cfg.camera_angle,
            "rotate_x_degrees": cfg.rotate_x_degrees,
            "cull_backfaces": str(cfg.cull_backfaces),
            "vertices": len(verts),
            "triangles": len(tris),
            "sun_pixels": sun_pixels,
            "zbuffer_bytes": cfg.logical * cfg.logical * 2,
            "colorbuffer_bytes": cfg.logical * cfg.logical,
        }
    )
    return rgb, stats


class TerrainAlgorithmTests(unittest.TestCase):
    def test_noise_is_deterministic(self):
        self.assertAlmostEqual(terrain_noise2d(1.25, -3.5), terrain_noise2d(1.25, -3.5), places=7)

    def test_mesh_counts(self):
        cfg = TerrainConfig(grid=32)
        verts, tris = build_terrain(cfg)
        self.assertEqual(len(verts), 32 * 32)
        self.assertEqual(len(tris), 2 * 31 * 31)

    def test_fitted_extent_renders_more_than_edge_fragments(self):
        cfg = TerrainConfig(extent=14, grid=32, distance=11, height=3.2, target_y=1.5)
        _rgb, stats = render_terrain(cfg)
        self.assertGreater(stats["terrain_pixels"], 100)
        self.assertGreater(stats["visible_vertices"], 500)

    def test_large_jsx_extent_projects_far_outside_first_camera(self):
        small = TerrainConfig(extent=14, grid=32, distance=11, height=3.2, target_y=1.5)
        large = TerrainConfig(extent=40, grid=32, distance=11, height=3.2, target_y=1.5)
        _rgb_s, stats_s = render_terrain(small)
        _rgb_l, stats_l = render_terrain(large)
        self.assertGreater(stats_s["triangles_drawn"], stats_l["triangles_drawn"])
        self.assertLess(stats_s["projected_max_x"] - stats_s["projected_min_x"],
                        stats_l["projected_max_x"] - stats_l["projected_min_x"])


def cmd_test(_args) -> int:
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(TerrainAlgorithmTests)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    return 0 if result.wasSuccessful() else 1


def config_from_args(args) -> TerrainConfig:
    return TerrainConfig(
        logical=args.logical,
        grid=args.grid,
        extent=args.extent,
        distance=args.distance,
        height=args.height,
        target_y=args.target_y,
        camera_angle=args.angle,
        contrast=args.contrast,
        cull_backfaces=args.cull_backfaces,
        rotate_x_degrees=args.rotate_x_degrees,
    )


def cmd_render(args) -> int:
    cfg = config_from_args(args)
    rgb, stats = render_terrain(cfg)
    write_png(args.out, FB_W, FB_H, rgb)
    for k in sorted(stats):
        print(f"{k}: {stats[k]}")
    print(f"wrote: {args.out}")
    return 0


def cmd_sweep(args) -> int:
    out_dir: Path = args.out_dir
    out_dir.mkdir(parents=True, exist_ok=True)
    configs: list[tuple[str, TerrainConfig]] = []

    # Keep this intentionally small and diagnostic. These are the parameters most
    # likely to explain a too-sparse hardware capture.
    for extent in (10.0, 14.0, 20.0, 40.0):
        configs.append((f"extent-{extent:g}", TerrainConfig(extent=extent, grid=32, distance=11, height=3.2, target_y=1.5)))
    for distance in (7.0, 9.0, 11.0, 14.0):
        configs.append((f"distance-{distance:g}", TerrainConfig(extent=14, grid=32, distance=distance, height=3.2, target_y=1.5)))
    for height in (1.4, 2.2, 3.2, 4.5):
        configs.append((f"height-{height:g}", TerrainConfig(extent=14, grid=32, distance=11, height=height, target_y=1.5)))
    for target_y in (-1.0, 0.0, 1.5, 3.0):
        configs.append((f"target-y-{target_y:g}", TerrainConfig(extent=14, grid=32, distance=11, height=3.2, target_y=target_y)))
    for angle in (0.0, 0.4, 0.8, 1.2):
        configs.append((f"angle-{angle:g}", TerrainConfig(extent=14, grid=32, distance=11, height=3.2, target_y=1.5, camera_angle=angle)))

    rows: list[dict[str, object]] = []
    image_paths: list[Path] = []
    for name, cfg in configs:
        rgb, stats = render_terrain(cfg)
        path = out_dir / f"{name}.png"
        write_png(path, FB_W, FB_H, rgb)
        image_paths.append(path)
        rows.append({"name": name, "file": path.name, **stats})

    csv_path = out_dir / "terrain-sweep-stats.csv"
    with csv_path.open("w", newline="") as f:
        fieldnames = list(rows[0].keys())
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)

    montage = out_dir / "terrain-sweep-montage.png"
    write_montage(image_paths, montage, cols=4)
    print(f"wrote {len(rows)} terrain screenshots to {out_dir}")
    print(f"wrote {csv_path}")
    print(f"wrote {montage}")
    return 0


def add_render_args(p: argparse.ArgumentParser) -> None:
    p.add_argument("--out", type=Path, default=Path("terrain-host.png"))
    p.add_argument("--logical", type=int, default=80)
    p.add_argument("--grid", type=int, default=32)
    p.add_argument("--extent", type=float, default=14.0)
    p.add_argument("--distance", type=float, default=11.0)
    p.add_argument("--height", type=float, default=3.2)
    p.add_argument("--target-y", type=float, default=1.5)
    p.add_argument("--angle", type=float, default=0.0)
    p.add_argument("--contrast", type=float, default=1.4)
    p.add_argument("--cull-backfaces", action="store_true")
    p.add_argument("--rotate-x-degrees", type=float, default=0.0)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    sub = ap.add_subparsers(dest="cmd", required=True)

    p_test = sub.add_parser("test", help="run terrain algorithm unit tests")
    p_test.set_defaults(func=cmd_test)

    p_render = sub.add_parser("render", help="render one terrain configuration")
    add_render_args(p_render)
    p_render.set_defaults(func=cmd_render)

    p_sweep = sub.add_parser("sweep", help="render a diagnostic terrain parameter sweep")
    p_sweep.add_argument("--out-dir", type=Path, default=Path("artifacts/terrain-host-analysis"))
    p_sweep.set_defaults(func=cmd_sweep)

    args = ap.parse_args()
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
