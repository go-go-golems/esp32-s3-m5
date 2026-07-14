#!/usr/bin/env python3
"""Generate the deterministic packed reader-page fixture for firmware 0107."""

from __future__ import annotations

import hashlib
import shutil
import subprocess
from pathlib import Path

TICKET = Path(__file__).resolve().parents[1]
REPO = Path(subprocess.check_output(["git", "-C", str(TICKET), "rev-parse", "--show-toplevel"], text=True).strip())
PROJECT = REPO / "0107-papers3-epd-painter-control"
FIXTURES = PROJECT / "main/fixtures"
OUTPUT = TICKET / "scripts/output"
FONT = Path("/usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf")
EXPECTED_FONT_SHA = "8f2c103bfa3fd5de71f1b92b18f21906b5a26871fb7e19a9a4c9af539c3cc7ab"
WIDTH, HEIGHT = 960, 540

TITLE = "Electrophoretic Display Qualification"
CHAPTER = "Chapter 1 — Controlled Transitions"
BODY = [
    "A reader page is a structured optical workload, not a full-field test.",
    "Margins remain white while glyph stems, bowls, and punctuation switch",
    "small regions of the panel. The resulting area fraction is modest, but",
    "the transitions are spatially dense and repeated across many gate rows.",
    "",
    "This fixed page separates rendering from display drive. Its pixels are",
    "generated offline, packed to the driver's two-bit format, and identified",
    "by a cryptographic digest. Runtime code performs no font selection, line",
    "breaking, anti-aliasing, storage access, or application animation.",
    "",
    "The first comparison asks only whether text reaches a stable dark endpoint",
    "after a known white origin. A repeated page then tests whether a commanded",
    "no-op disturbs the image before the final white cleanup is applied.",
]


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> None:
    convert = shutil.which("convert")
    if not convert:
        raise SystemExit("ImageMagick 'convert' is required")
    if not FONT.is_file() or digest(FONT) != EXPECTED_FONT_SHA:
        raise SystemExit(f"font missing or hash mismatch: {FONT}")

    FIXTURES.mkdir(parents=True, exist_ok=True)
    OUTPUT.mkdir(parents=True, exist_ok=True)
    preview = OUTPUT / "14-reader-page-preview.png"
    packed_path = FIXTURES / "reader_page.bin"
    text_path = FIXTURES / "reader_page.txt"

    args = [convert, "-size", f"{WIDTH}x{HEIGHT}", "xc:white", "-font", str(FONT), "-fill", "black"]
    args += ["-pointsize", "34", "-annotate", "+70+62", TITLE]
    args += ["-pointsize", "22", "-annotate", "+70+108", CHAPTER]
    y = 158
    for line in BODY:
        if line:
            args += ["-pointsize", "18", "-annotate", f"+70+{y}", line]
        y += 25
    args += ["-stroke", "black", "-strokewidth", "1", "-draw", "line 70,485 890,485"]
    args += ["-stroke", "none", "-pointsize", "15", "-annotate", "+70+515", "ESP-50 • fixed reader fixture"]
    args += ["-pointsize", "15", "-annotate", "+855+515", "1"]
    args += ["-threshold", "55%", "-strip", "-define", "png:exclude-chunks=date,time", str(preview)]
    subprocess.run(args, check=True)

    raw = subprocess.check_output([convert, str(preview), "-colorspace", "Gray", "-depth", "8", "gray:-"])
    expected = WIDTH * HEIGHT
    if len(raw) != expected:
        raise SystemExit(f"unexpected raw fixture size: {len(raw)} != {expected}")

    packed = bytearray(expected // 4)
    black_pixels = 0
    for i in range(0, expected, 4):
        value = 0
        for offset in range(4):
            black = raw[i + offset] < 128
            if black:
                black_pixels += 1
            value |= (3 if black else 0) << ((3 - offset) * 2)
        packed[i // 4] = value
    packed_path.write_bytes(packed)
    text_path.write_text("\n".join([TITLE, CHAPTER, "", *BODY]) + "\n", encoding="utf-8")

    version = subprocess.check_output([convert, "-version"], text=True).splitlines()[0]
    fraction = black_pixels / expected
    manifest = FIXTURES / "MANIFEST.md"
    manifest.write_text(
        "# Reader fixture manifest\n\n"
        f"- Dimensions: `{WIDTH}x{HEIGHT}`\n"
        "- Encoding: `2 bpp; 0=white, 3=black; four pixels per byte, MSB first`\n"
        f"- Source font: `{FONT}`\n"
        f"- Source font SHA-256: `{EXPECTED_FONT_SHA}`\n"
        f"- Renderer: `{version}`\n"
        f"- Black pixels: `{black_pixels}` (`{fraction:.6%}`)\n"
        f"- Source text SHA-256: `{digest(text_path)}`\n"
        f"- Packed fixture bytes: `{len(packed)}`\n"
        f"- Packed fixture SHA-256: `{digest(packed_path)}`\n"
        f"- Preview SHA-256: `{digest(preview)}`\n",
        encoding="utf-8",
    )
    print(f"packed={packed_path}")
    print(f"preview={preview}")
    print(f"packed_sha256={digest(packed_path)} black_fraction={fraction:.6%}")


if __name__ == "__main__":
    main()
