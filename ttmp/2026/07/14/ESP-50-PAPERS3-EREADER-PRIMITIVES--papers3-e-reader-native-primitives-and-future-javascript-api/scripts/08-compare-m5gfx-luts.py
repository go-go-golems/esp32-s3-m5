#!/usr/bin/env python3
"""Download M5GFX Panel_EPD sources and compare built-in waveform LUTs."""

from __future__ import annotations

import hashlib
import pathlib
import re
import urllib.request

ROOT = pathlib.Path(__file__).resolve().parents[1]
DEST = ROOT / "sources" / "code" / "m5gfx-lut-comparison"
TAGS = ("0.2.15", "0.2.25")
LUTS = ("lut_quality", "lut_text", "lut_fast", "lut_fastest", "lut_eraser")
URL = "https://raw.githubusercontent.com/m5stack/M5GFX/{tag}/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp"


def extract(source: str, name: str) -> str:
    match = re.search(
        rf"(?:static\s+)?(?:constexpr\s+)?[^;\n]*\b{name}\b.*?\{{(.*?)\}};",
        source,
        re.S,
    )
    if not match:
        raise RuntimeError(f"could not find {name}")
    return re.sub(r"\s+", "", match.group(1))


def main() -> None:
    DEST.mkdir(parents=True, exist_ok=True)
    sources: dict[str, str] = {}
    lines = ["M5GFX built-in EPD LUT comparison", ""]
    for tag in TAGS:
        url = URL.format(tag=tag)
        source = urllib.request.urlopen(url, timeout=30).read().decode("utf-8")
        sources[tag] = source
        path = DEST / f"Panel_EPD-{tag}.cpp"
        normalized_source = "\n".join(line.rstrip() for line in source.splitlines()) + "\n"
        path.write_text(normalized_source, encoding="utf-8")
        lines.extend(
            [
                f"tag={tag}",
                f"url={url}",
                f"source_sha256={hashlib.sha256(source.encode()).hexdigest()}",
            ]
        )
        for name in LUTS:
            value = extract(source, name)
            lines.append(f"{name}_sha256={hashlib.sha256(value.encode()).hexdigest()}")
        lines.append("")

    lines.append("pairwise_results:")
    for name in LUTS:
        old = extract(sources[TAGS[0]], name)
        new = extract(sources[TAGS[1]], name)
        lines.append(f"{name}={'IDENTICAL' if old == new else 'DIFFERENT'}")

    output = "\n".join(lines) + "\n"
    (DEST / "comparison.txt").write_text(output, encoding="utf-8")
    print(output, end="")


if __name__ == "__main__":
    main()
