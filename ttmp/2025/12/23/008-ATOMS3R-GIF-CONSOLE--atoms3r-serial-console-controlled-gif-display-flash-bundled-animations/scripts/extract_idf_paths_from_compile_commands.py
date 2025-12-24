#!/usr/bin/env python3
"""
Extract ESP-IDF paths from an ESP-IDF project's compile_commands.json.

Why this exists (for ticket 008):
- We often need the *exact* ESP-IDF checkout used by a build (e.g. to read
  `components/console/*` sources like `esp_console_repl_chip.c`).
- Guessing paths is error-prone; compile_commands.json is ground truth.

Usage:
  python3 extract_idf_paths_from_compile_commands.py \
    --compile-commands /abs/path/to/build/compile_commands.json

Optional:
  --contains components/console
  --show-includes
"""

from __future__ import annotations

import argparse
import json
import shlex
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


@dataclass(frozen=True)
class Entry:
    file: str
    command: str


def _iter_entries(data: list[dict]) -> Iterable[Entry]:
    for ent in data:
        cmd = ent.get("command")
        if not cmd:
            args = ent.get("arguments") or []
            cmd = " ".join(args)
        yield Entry(file=ent.get("file", ""), command=cmd)


def _extract_include_dirs(command: str) -> list[str]:
    parts = shlex.split(command)
    incs: list[str] = []
    i = 0
    while i < len(parts):
        p = parts[i]
        if p.startswith("-I") and len(p) > 2:
            incs.append(p[2:])
        elif p == "-I" and i + 1 < len(parts):
            incs.append(parts[i + 1])
            i += 1
        i += 1
    return incs


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--compile-commands", required=True, type=Path)
    ap.add_argument(
        "--contains",
        default="components/console",
        help="Only report include dirs containing this substring (default: components/console)",
    )
    ap.add_argument(
        "--show-includes",
        action="store_true",
        help="Print matching include directories (not just IDF root candidates).",
    )
    args = ap.parse_args()

    p: Path = args.compile_commands
    if not p.exists():
        raise SystemExit(f"compile_commands.json not found: {p}")

    data = json.loads(p.read_text(encoding="utf-8"))

    match = args.contains
    include_dirs: set[str] = set()
    roots: set[str] = set()
    entry_count = 0

    for ent in _iter_entries(data):
        entry_count += 1
        for inc in _extract_include_dirs(ent.command):
            if match in inc:
                include_dirs.add(inc)
            # Heuristic: anything with "/components/" can give us an IDF root candidate.
            if "/components/" in inc:
                roots.add(inc.split("/components/")[0])

    print(f"compile_commands_entries: {entry_count}")
    if args.show_includes:
        print(f"include_dirs_containing({match}):")
        for x in sorted(include_dirs):
            print(f"  {x}")

    print("idf_root_candidates:")
    for x in sorted(roots):
        print(f"  {x}")

    if include_dirs:
        print("notes:")
        print("  - If you see something like '<idf_root>/components/console', you can read:")
        print("    - <idf_root>/components/console/esp_console_repl_chip.c")
        print("    - <idf_root>/components/console/esp_console_common.c")
        print("    - <idf_root>/components/console/commands.c")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())


