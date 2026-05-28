#!/usr/bin/env python3
"""Host-side prototype for the M5Dial proper 3D planet renderer.

The script intentionally mirrors the proposed firmware architecture:

- render at a coarse logical resolution (80x80 by default)
- use a full logical Z-buffer
- rasterize opaque triangles
- quantize directly to four palette indices with Bayer 4x4 dithering
- upscale logical pixels to a 240x240 PNG
- draw solid, non-dithered UI text after the scene pass

It is not a production renderer. It is an executable design sketch for validating
memory budgets, data flow, depth precision, and visual composition before writing
ESP-IDF code.
"""

from __future__ import annotations

import argparse
import binascii
import math
import struct
import zlib
from dataclasses import dataclass
from pathlib import Path

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


def noise2(x: float, y: float) -> float:
    # Deterministic cheap pseudo-noise. Good enough for host visual validation.
    return math.sin(x * 1.37 + y * 2.11) * 0.55 + math.sin(x * 3.91 - y * 1.73) * 0.30 + math.cos(x * 2.47 + y * 0.83) * 0.15


def build_uv_sphere(radius: float, lat_steps: int, lon_steps: int) -> tuple[list[Vertex], list[tuple[int, int, int]]]:
    verts: list[Vertex] = []
    for iy in range(lat_steps + 1):
        v = iy / lat_steps
        theta = -math.pi / 2 + v * math.pi
        y = math.sin(theta)
        ring_r = math.cos(theta)
        for ix in range(lon_steps):
            u = ix / lon_steps
            phi = u * math.tau
            x = math.cos(phi) * ring_r
            z = math.sin(phi) * ring_r

            n = noise2(x * 2.0 + z * 0.7, y * 2.0) * 0.16 + noise2(z * 1.2, y * 1.5) * 0.07
            s = radius * (1.0 + n * 0.5)
            px, py, pz = x * s, y * s, z * s

            lat = py / radius
            heat = max(0.0, lat)
            cold = max(0.0, -lat)
            speckle = math.sin(px * 5.0) * math.cos(pz * 5.0) * 0.5 + 0.5
            rr = heat * (0.6 + speckle * 0.5) + max(0.0, n) * 0.5
            gg = 0.0
            bb = cold * (0.6 + speckle * 0.5) + max(0.0, -n) * 0.3
            verts.append(Vertex(px, py, pz, min(1.0, rr), gg, min(1.0, bb)))

    tris: list[tuple[int, int, int]] = []
    for iy in range(lat_steps):
        for ix in range(lon_steps):
            a = iy * lon_steps + ix
            b = iy * lon_steps + ((ix + 1) % lon_steps)
            c = (iy + 1) * lon_steps + ix
            d = (iy + 1) * lon_steps + ((ix + 1) % lon_steps)
            if iy != 0:
                tris.append((a, c, b))
            if iy != lat_steps - 1:
                tris.append((b, c, d))
    return verts, tris


def build_ring(inner: float = 3.55, outer: float = 3.82, segments: int = 96) -> tuple[list[Vertex], list[tuple[int, int, int]]]:
    verts: list[Vertex] = []
    tilt = math.pi / 2.4
    ct, st = math.cos(tilt), math.sin(tilt)
    for i in range(segments):
        a = i * math.tau / segments
        ca, sa = math.cos(a), math.sin(a)
        for rad in (inner, outer):
            x = ca * rad
            z0 = sa * rad
            y0 = 0.0
            # Rotate ring around X.
            y = y0 * ct - z0 * st
            z = y0 * st + z0 * ct
            verts.append(Vertex(x, y, z, 0.35, 0.45, 1.0))
    tris: list[tuple[int, int, int]] = []
    for i in range(segments):
        j = (i + 1) % segments
        a, b = i * 2, i * 2 + 1
        c, d = j * 2, j * 2 + 1
        tris.append((a, c, b))
        tris.append((b, c, d))
    return verts, tris


def camera_basis(angle: float, distance: float = 9.0, height: float = 0.5):
    eye = (math.sin(angle) * distance, height, math.cos(angle) * distance)
    target = (0.0, 0.0, 0.0)
    fx, fy, fz = target[0] - eye[0], target[1] - eye[1], target[2] - eye[2]
    fl = math.sqrt(fx * fx + fy * fy + fz * fz)
    fx, fy, fz = fx / fl, fy / fl, fz / fl
    rx, ry, rz = fz, 0.0, -fx
    rl = math.sqrt(rx * rx + rz * rz)
    rx, rz = rx / rl, rz / rl
    ux = fy * rz - fz * ry
    uy = fz * rx - fx * rz
    uz = fx * ry - fy * rx
    return eye, (rx, ry, rz), (ux, uy, uz), (fx, fy, fz)


def project(v: Vertex, basis, logical_w: int, logical_h: int, model_angle: float) -> Projected | None:
    cm, sm = math.cos(model_angle), math.sin(model_angle)
    # Model rotation around Y.
    wx = v.x * cm + v.z * sm
    wy = v.y
    wz = -v.x * sm + v.z * cm

    eye, right, up, fwd = basis
    dx, dy, dz = wx - eye[0], wy - eye[1], wz - eye[2]
    vx = dx * right[0] + dy * right[1] + dz * right[2]
    vy = dx * up[0] + dy * up[1] + dz * up[2]
    vz = dx * fwd[0] + dy * fwd[1] + dz * fwd[2]
    if vz <= 0.1:
        return None
    focal = 2.145
    sx = (vx * focal / vz) * (logical_w * 0.5) + logical_w * 0.5
    sy = -(vy * focal / vz) * (logical_h * 0.5) + logical_h * 0.5
    return Projected(sx, sy, vz, v.r, v.g, v.b)


def quantize(r: float, g: float, b: float, x: int, y: int, contrast: float) -> int:
    r = max(0.0, min(1.0, (r - 0.5) * contrast + 0.5))
    g = max(0.0, min(1.0, (g - 0.5) * contrast + 0.5))
    b = max(0.0, min(1.0, (b - 0.5) * contrast + 0.5))
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


def rasterize(tris, projected, colorbuf, zbuf, logical_w: int, logical_h: int, zbits: int, contrast: float, cull_backfaces: bool = True):
    zmax = (1 << zbits) - 1
    near, far = 3.0, 16.0
    drawn = 0
    for i0, i1, i2 in tris:
        a, b, c = projected[i0], projected[i1], projected[i2]
        if a is None or b is None or c is None:
            continue
        area = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x)
        # Backface culling. Sign depends on screen y direction; this sign is for the current projection.
        # Thin two-sided geometry such as the ring disables culling.
        if cull_backfaces and area >= -0.01:
            continue
        if abs(area) < 0.01:
            continue
        inv_area = 1.0 / area
        minx = max(0, int(math.floor(min(a.x, b.x, c.x))))
        maxx = min(logical_w - 1, int(math.ceil(max(a.x, b.x, c.x))))
        miny = max(0, int(math.floor(min(a.y, b.y, c.y))))
        maxy = min(logical_h - 1, int(math.ceil(max(a.y, b.y, c.y))))
        for y in range(miny, maxy + 1):
            py = y + 0.5
            for x in range(minx, maxx + 1):
                px = x + 0.5
                w0 = ((b.x - px) * (c.y - py) - (b.y - py) * (c.x - px)) * inv_area
                w1 = ((c.x - px) * (a.y - py) - (c.y - py) * (a.x - px)) * inv_area
                w2 = 1.0 - w0 - w1
                if area < 0:
                    inside = w0 >= 0 and w1 >= 0 and w2 >= 0
                else:
                    inside = w0 <= 0 and w1 <= 0 and w2 <= 0
                    w0, w1, w2 = -w0, -w1, -w2
                if not inside:
                    continue
                z = w0 * a.z + w1 * b.z + w2 * c.z
                zn = max(0.0, min(1.0, (z - near) / (far - near)))
                zi = int(zn * zmax + 0.5)
                idx = y * logical_w + x
                if zi >= zbuf[idx]:
                    continue
                zbuf[idx] = zi
                rr = w0 * a.r + w1 * b.r + w2 * c.r
                gg = w0 * a.g + w1 * b.g + w2 * c.g
                bb = w0 * a.b + w1 * b.b + w2 * c.b
                colorbuf[idx] = quantize(rr, gg, bb, x, y, contrast)
                drawn += 1
    return drawn


FONT = {
    "P": (0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10),
    "L": (0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F),
    "A": (0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11),
    "N": (0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11),
    "E": (0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F),
    "T": (0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04),
}


def draw_text(rgb: bytearray, width: int, x: int, y: int, text: str, scale: int, color: tuple[int, int, int]) -> None:
    cursor = x
    for ch in text:
        glyph = FONT.get(ch)
        if glyph is None:
            cursor += 4 * scale
            continue
        for gy, row in enumerate(glyph):
            for gx in range(5):
                if row & (1 << (4 - gx)):
                    for yy in range(scale):
                        for xx in range(scale):
                            px = cursor + gx * scale + xx
                            py = y + gy * scale + yy
                            if 0 <= px < width and 0 <= py < width:
                                off = (py * width + px) * 3
                                rgb[off : off + 3] = bytes(color)
        cursor += 6 * scale


def render(args) -> tuple[bytes, dict[str, int]]:
    lw = args.logical
    scale = 240 // lw
    assert lw * scale == 240, "logical size must divide 240"
    colorbuf = [COLOR_BLACK] * (lw * lw)
    zmax = (1 << args.zbits) - 1
    zbuf = [zmax] * (lw * lw)

    sphere_verts, sphere_tris = build_uv_sphere(2.6, args.lat, args.lon)
    ring_verts, ring_tris = build_ring(3.55, 3.82, 96)
    basis = camera_basis(args.camera_angle)
    planet_proj = [project(v, basis, lw, lw, args.planet_angle) for v in sphere_verts]
    ring_proj = [project(v, basis, lw, lw, args.ring_angle) for v in ring_verts]

    # Back ring, planet, front ring is left for firmware polish. For prototype,
    # z-buffered ring+planet verifies the memory/rasterization path.
    ring_pixels = rasterize(ring_tris, ring_proj, colorbuf, zbuf, lw, lw, args.zbits, args.contrast, cull_backfaces=False)
    planet_pixels = rasterize(sphere_tris, planet_proj, colorbuf, zbuf, lw, lw, args.zbits, args.contrast, cull_backfaces=True)

    rgb = bytearray(240 * 240 * 3)
    mask_radius = 116
    for y in range(lw):
        for x in range(lw):
            cidx = colorbuf[y * lw + x]
            rgb_color = PALETTE[cidx]
            for yy in range(scale):
                py = y * scale + yy
                for xx in range(scale):
                    px = x * scale + xx
                    dx, dy = px - 120, py - 120
                    off = (py * 240 + px) * 3
                    if dx * dx + dy * dy <= mask_radius * mask_radius:
                        rgb[off : off + 3] = bytes(rgb_color)

    # Solid UI overlay, not dithered.
    draw_text(rgb, 240, 84, 26, "PLANET", 2, PALETTE[COLOR_WARM])
    stats = {
        "logical_pixels": lw * lw,
        "zbuffer_bytes": lw * lw * (args.zbits // 8),
        "physical_framebuffer_bytes_2bpp": 240 * 240 // 4,
        "sphere_vertices": len(sphere_verts),
        "sphere_triangles": len(sphere_tris),
        "ring_vertices": len(ring_verts),
        "ring_triangles": len(ring_tris),
        "planet_pixels": planet_pixels,
        "ring_pixels": ring_pixels,
        "pixel_scale": scale,
    }
    return bytes(rgb), stats


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", type=Path, default=Path("planet-prototype.png"))
    ap.add_argument("--logical", type=int, default=80, choices=(40, 48, 60, 80, 120, 240))
    ap.add_argument("--zbits", type=int, default=16, choices=(8, 16))
    ap.add_argument("--lat", type=int, default=18)
    ap.add_argument("--lon", type=int, default=28)
    ap.add_argument("--contrast", type=float, default=1.4)
    ap.add_argument("--camera-angle", type=float, default=0.0)
    ap.add_argument("--planet-angle", type=float, default=0.65)
    ap.add_argument("--ring-angle", type=float, default=0.0)
    args = ap.parse_args()

    rgb, stats = render(args)
    write_png(args.out, 240, 240, rgb)
    for k, v in stats.items():
        print(f"{k}: {v}")
    print(f"wrote: {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
