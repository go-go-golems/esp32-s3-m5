#!/usr/bin/env python3
import argparse
import difflib
from pathlib import Path
import sys

def read_lines(path: Path) -> list[str]:
    try:
        return path.read_text(encoding="utf-8", errors="replace").splitlines(keepends=True)
    except FileNotFoundError:
        raise SystemExit(f"error: file not found: {path}")

def main() -> int:
    parser = argparse.ArgumentParser(description="Unified diff for ESP-IDF sdkconfig files.")
    parser.add_argument("left", type=Path)
    parser.add_argument("right", type=Path)
    parser.add_argument("--out", type=Path, default=None, help="Write diff to file instead of stdout")
    args = parser.parse_args()

    left_lines = read_lines(args.left)
    right_lines = read_lines(args.right)

    diff = difflib.unified_diff(
        left_lines,
        right_lines,
        fromfile=str(args.left),
        tofile=str(args.right),
    )

    if args.out:
        args.out.parent.mkdir(parents=True, exist_ok=True)
        args.out.write_text("".join(diff), encoding="utf-8")
    else:
        sys.stdout.writelines(diff)
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
