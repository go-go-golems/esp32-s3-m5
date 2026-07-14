#!/usr/bin/env python3
"""
tile_to_cpp.py — Convert a 135×240 PNG tile to a C++ RGB565 PROGMEM array.

Usage:
    python3 tile_to_cpp.py assets/tiles/sheet3_01.png --output src/tiles/sheet3_01.h
    python3 tile_to_cpp.py assets/tiles/ --output src/tiles/ --all

The output is a C header with:
  - A PROGMEM uint16_t array containing RGB565 pixel data
  - Width/height constants
  - Include guard
"""

import argparse
import os
import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    print("Pillow required: pip install Pillow")
    sys.exit(1)


def rgb888_to_rgb565(r, g, b):
    """Convert 8-bit RGB to 16-bit RGB565."""
    r5 = (r >> 3) & 0x1F
    g6 = (g >> 2) & 0x3F
    b5 = (b >> 3) & 0x1F
    return (r5 << 11) | (g6 << 5) | b5


def tile_to_cpp(png_path, output_path=None, var_name=None):
    """Convert a single PNG tile to a C++ RGB565 header."""
    img = Image.open(png_path).convert('RGB')
    w, h = img.size
    pixels = img.load()

    if var_name is None:
        var_name = Path(png_path).stem.replace('-', '_')

    guard = f"TILE_{var_name.upper()}_H"

    lines = []
    lines.append(f"// Auto-generated from {Path(png_path).name}")
    lines.append(f"// Size: {w}×{h}, Format: RGB565")
    lines.append(f"#ifndef {guard}")
    lines.append(f"#define {guard}")
    lines.append(f"")
    lines.append(f"#include <stdint.h>")
    lines.append(f"#include <pgmspace.h>")
    lines.append(f"")
    lines.append(f"static constexpr uint16_t TILE_{var_name.upper()}_W = {w};")
    lines.append(f"static constexpr uint16_t TILE_{var_name.upper()}_H = {h};")
    lines.append(f"")

    # Generate pixel data — 8 values per line for readability
    lines.append(f"static const uint16_t {var_name}_data[] PROGMEM = {{")
    pixel_values = []
    for y in range(h):
        for x in range(w):
            r, g, b = pixels[x, y]
            rgb565 = rgb888_to_rgb565(r, g, b)
            pixel_values.append(f"0x{rgb565:04X}")

    # Format: 8 values per line
    for i in range(0, len(pixel_values), 8):
        chunk = pixel_values[i:i+8]
        comma = "," if i + 8 < len(pixel_values) else ""
        lines.append(f"    {', '.join(chunk)}{comma}")

    lines.append(f"}};")
    lines.append(f"")
    lines.append(f"#endif // {guard}")
    lines.append(f"")

    result = '\n'.join(lines)

    if output_path:
        Path(output_path).parent.mkdir(parents=True, exist_ok=True)
        with open(output_path, 'w') as f:
            f.write(result)
        print(f"  {Path(png_path).name} → {output_path} ({w}×{h}, {len(pixel_values)} pixels)")
    else:
        print(result)

    return result


def main():
    parser = argparse.ArgumentParser(description='Convert PNG tiles to C++ RGB565 headers')
    parser.add_argument('input', help='Input PNG file or directory')
    parser.add_argument('--output', '-o', help='Output .h file or directory')
    parser.add_argument('--all', action='store_true', help='Process all PNGs in directory')
    args = parser.parse_args()

    input_path = Path(args.input)

    if input_path.is_dir() or args.all:
        if not input_path.is_dir():
            print(f"Error: {input_path} is not a directory")
            sys.exit(1)

        pngs = sorted(input_path.glob("*.png"))
        if not pngs:
            print(f"No PNG files found in {input_path}")
            sys.exit(1)

        out_dir = Path(args.output) if args.output else input_path / "cpp"
        out_dir.mkdir(parents=True, exist_ok=True)

        print(f"Converting {len(pngs)} tiles...")
        for png in pngs:
            out_file = out_dir / f"{png.stem}.h"
            tile_to_cpp(str(png), str(out_file))

        # Generate an index header that includes all tiles
        index_path = out_dir / "tiles_index.h"
        with open(index_path, 'w') as f:
            f.write("#ifndef TILES_INDEX_H\n#define TILES_INDEX_H\n\n")
            for png in pngs:
                f.write(f'#include "{png.stem}.h"\n')
            f.write(f"\n// Total: {len(pngs)} tiles\n")
            f.write(f"static constexpr uint16_t TILE_COUNT = {len(pngs)};\n\n")
            f.write("#endif // TILES_INDEX_H\n")
        print(f"\n  Index: {index_path}")
    else:
        tile_to_cpp(str(input_path), args.output)


if __name__ == '__main__':
    main()
