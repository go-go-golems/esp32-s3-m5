#!/usr/bin/env python3
"""Decode M5GFX Panel_EPD LUTs into canonical, reviewable evidence.

This is deliberately static: it never opens a serial port or modifies hardware.
It preserves every LUT row, derives each grayscale target's drive schedule, and
compares the exact legacy/current source snapshots used by the qualification
matrix.
"""

from __future__ import annotations

import hashlib
import json
import re
import subprocess
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[6]
TICKET = Path(__file__).resolve().parents[1]
MATRIX = ROOT / "0106-papers3-epd-qualification" / ".component-matrix"
OUTPUT = TICKET / "scripts" / "output"
CODE_NAMES = {0: "end", 1: "toward-black", 2: "toward-white", 3: "no-op"}
ARRAY_RE = re.compile(
    r"static\s+constexpr\s+const\s+uint32_t\s+(lut_[a-z]+)\[\]\s*=\s*\{(.*?)\n\s*\};",
    re.S,
)
TOKEN_RE = re.compile(r"LUT_MAKE\(([^)]*)\)|~0u|0u")


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def git_head(path: Path) -> str:
    return subprocess.check_output(["git", "-C", str(path), "rev-parse", "HEAD"], text=True).strip()


def parse_luts(source: str) -> dict[str, list[list[int]]]:
    result: dict[str, list[list[int]]] = {}
    for name, body in ARRAY_RE.findall(source):
        rows: list[list[int]] = []
        for match in TOKEN_RE.finditer(body):
            token = match.group(0)
            if token == "~0u":
                rows.append([3] * 16)
            elif token == "0u":
                rows.append([0] * 16)
            else:
                values = [int(value.strip(), 16 if value.strip().lower().startswith("0x") else 10)
                          for value in match.group(1).split(",")]
                if len(values) != 16 or any(value not in CODE_NAMES for value in values):
                    raise ValueError(f"invalid {name} row: {values}")
                rows.append(values)
        if not rows or rows[-1] != [0] * 16:
            raise ValueError(f"{name} has no all-zero terminator")
        result[name] = rows
    required = {"lut_quality", "lut_text", "lut_fast", "lut_fastest", "lut_eraser"}
    if set(result) != required:
        raise ValueError(f"unexpected LUT set: {sorted(result)}")
    return result


def target_schedules(rows: list[list[int]]) -> list[dict[str, object]]:
    schedules = []
    for tone in range(16):
        codes: list[int] = []
        for row in rows:
            code = row[tone]
            if code == 0:
                break
            codes.append(code)
        counts = Counter(codes)
        schedules.append({
            "tone": tone,
            "nominal": "black" if tone == 0 else "white" if tone == 15 else f"gray-{tone}",
            "codes": codes,
            "symbols": "".join({1: "B", 2: "W", 3: "-"}[code] for code in codes),
            "toward_black": counts[1],
            "toward_white": counts[2],
            "no_op": counts[3],
            "net_white_minus_black": counts[2] - counts[1],
        })
    return schedules


def decode(label: str, checkout: Path) -> dict[str, object]:
    panel = checkout / "src/lgfx/v1/platforms/esp32/Panel_EPD.cpp"
    bus = checkout / "src/lgfx/v1/platforms/esp32/Bus_EPD.cpp"
    board = checkout / "src/M5GFX.cpp"
    panel_bytes = panel.read_bytes()
    luts = parse_luts(panel_bytes.decode())
    canonical_luts = json.dumps(luts, sort_keys=True, separators=(",", ":")).encode()
    bus_text = bus.read_text()
    board_text = board.read_text()
    speed_match = re.search(r"bus_cfg\.bus_speed\s*=\s*(\d+);", board_text)
    padding_match = re.search(r"cfg_detail\.line_padding\s*=\s*(\d+);", board_text)
    if not speed_match or not padding_match:
        raise ValueError(f"PaperS3 bus configuration not found in {board}")
    return {
        "label": label,
        "commit": git_head(checkout),
        "panel_source": str(panel.relative_to(ROOT)),
        "panel_source_sha256": sha256_bytes(panel_bytes),
        "bus_source": str(bus.relative_to(ROOT)),
        "bus_source_sha256": sha256_bytes(bus.read_bytes()),
        "board_source": str(board.relative_to(ROOT)),
        "board_source_sha256": sha256_bytes(board.read_bytes()),
        "canonical_lut_sha256": sha256_bytes(canonical_luts),
        "bus_speed_hz": int(speed_match.group(1)),
        "line_padding_bytes": int(padding_match.group(1)),
        "power_sequence": {
            "on": ["OE high", "delay 100 us", "PWR high", "delay 100 us", "SPV high", "delay 1 ms"],
            "off": ["delay 1 ms", "PWR low", "delay 10 us", "OE low", "delay 100 us", "SPV low"],
            "source_verified": all(fragment in bus_text for fragment in (
                "gpio_hi(_config.pin_oe)", "delayMicroseconds(100)",
                "gpio_hi(_config.pin_pwr)", "gpio_lo(_config.pin_pwr)",
            )),
        },
        "luts": {
            name: {
                "rows_including_terminator": len(rows),
                "raw_rows": rows,
                "raw_sha256": sha256_bytes(json.dumps(rows, separators=(",", ":")).encode()),
                "targets": target_schedules(rows),
            }
            for name, rows in sorted(luts.items())
        },
    }


def render_markdown(document: dict[str, object]) -> str:
    legacy, current = document["snapshots"]
    lines = [
        "---",
        "Title: M5GFX PaperS3 Waveform Static Decoding",
        "Ticket: ESP-50-PAPERS3-EREADER-PRIMITIVES",
        "Status: active",
        "Topics:",
        "    - papers3",
        "    - eink",
        "    - hardware-qualification",
        "DocType: reference",
        "Intent: long-term",
        "Owners: []",
        "RelatedFiles: []",
        "ExternalSources: []",
        'Summary: "Canonical static decoding of legacy and current M5GFX PaperS3 LUTs, schedules, bus timing configuration, and power ordering."',
        f"LastUpdated: {document['generated_utc']}",
        'WhatFor: "Separate source-level waveform identity from runtime timing and physical electrical evidence."',
        'WhenToUse: "Use when comparing factory, qualification, and independent-driver EPD experiments."',
        "---",
        "",
        "# M5GFX PaperS3 waveform static decoding",
        "",
        "This report is generated without opening a serial port. `B` means code 1 (toward black), `W` means code 2 (toward white), and `-` means code 3 (no operation), matching M5GFX's source comments. These names are software semantics, not independently probed source-driver voltages.",
        "",
        "## Snapshot identity",
        "",
        f"- Legacy/factory-family M5GFX commit: `{legacy['commit']}`",
        f"- Current qualification M5GFX commit: `{current['commit']}`",
        f"- Legacy canonical LUT SHA-256: `{legacy['canonical_lut_sha256']}`",
        f"- Current canonical LUT SHA-256: `{current['canonical_lut_sha256']}`",
        f"- Canonical LUTs identical: **{'yes' if document['comparison']['canonical_luts_identical'] else 'no'}**",
        f"- PaperS3 bus speed: `{current['bus_speed_hz']}` Hz",
        f"- Encoded scan-line padding: `{current['line_padding_bytes']}` bytes",
        "",
        "## Power ordering",
        "",
        "The M5GFX `Bus_EPD` source commands the following fixed GPIO ordering; it does not program a rail voltage or VCOM value:",
        "",
        "- on: OE high → 100 µs → PWR high → 100 µs → SPV high → 1 ms;",
        "- off: 1 ms → PWR low → 10 µs → OE low → 100 µs → SPV low.",
        "",
        "## Target schedules",
        "",
        "The table shows the black and white endpoint schedules through the first terminator. No-op padding remains visible because it affects runtime frame count even though it does not command particle motion.",
        "",
        "| LUT | rows | black target | white target | row hash |",
        "|---|---:|---|---|---|",
    ]
    for name, lut in current["luts"].items():
        black = lut["targets"][0]["symbols"]
        white = lut["targets"][15]["symbols"]
        lines.append(f"| `{name}` | {lut['rows_including_terminator']} | `{black}` | `{white}` | `{lut['raw_sha256']}` |")
    lines += [
        "",
        "## What static decoding proves",
        "",
        "- The legacy/factory-family and current qualification LUT bytes can be compared exactly.",
        "- Every nominal grayscale target's software drive-code sequence is explicit and hashable.",
        "- Bus clock configuration, line padding, and power GPIO ordering are source-backed.",
        "",
        "## What it does not prove",
        "",
        "- Actual frame duration, GPIO edge timing, rail voltage, VCOM, current, or temperature.",
        "- That software code 1 and code 2 produce the intended physical source polarity on this board.",
        "- Which target/history the official merged binary was executing at a given optical instant.",
        "- That adding runtime logging leaves timing unchanged.",
        "",
        "The JSON companion preserves every raw LUT row and all sixteen derived target schedules for machine comparison.",
        "",
    ]
    return "\n".join(lines)


def main() -> None:
    OUTPUT.mkdir(parents=True, exist_ok=True)
    snapshots = [
        decode("legacy-0.2.15", MATRIX / "legacy" / "M5GFX"),
        decode("current-0.2.25", MATRIX / "current" / "M5GFX"),
    ]
    document = {
        "schema": "esp50.m5gfx-waveform-static.v1",
        "generated_utc": datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z"),
        "code_meanings": {str(key): value for key, value in CODE_NAMES.items()},
        "snapshots": snapshots,
        "comparison": {
            "canonical_luts_identical": snapshots[0]["canonical_lut_sha256"] == snapshots[1]["canonical_lut_sha256"],
            "per_lut_identical": {
                name: snapshots[0]["luts"][name]["raw_sha256"] == snapshots[1]["luts"][name]["raw_sha256"]
                for name in snapshots[0]["luts"]
            },
        },
    }
    json_path = OUTPUT / "17-m5gfx-waveform-static-decoding.json"
    md_path = OUTPUT / "17-m5gfx-waveform-static-decoding.md"
    json_path.write_text(json.dumps(document, indent=2, sort_keys=True) + "\n")
    md_path.write_text(render_markdown(document))
    print(f"json={json_path}")
    print(f"report={md_path}")
    print(f"canonical_lut_sha256={snapshots[1]['canonical_lut_sha256']}")
    print(f"legacy_current_identical={str(document['comparison']['canonical_luts_identical']).lower()}")
    print("hardware_modified=no")


if __name__ == "__main__":
    main()
