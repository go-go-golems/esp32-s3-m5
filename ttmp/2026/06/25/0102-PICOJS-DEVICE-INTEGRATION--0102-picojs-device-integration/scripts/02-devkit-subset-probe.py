#!/usr/bin/env python3
"""Probe the picoOS devkit subset apps now built into the 0102 firmware."""

from __future__ import annotations

import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[6]
CONSOLE = ROOT / "0102-esp32-p4-visual-quickjs-repl" / "tools" / "picocalc_console.py"


def main() -> int:
    commands = [
        str(CONSOLE),
        "--quiet-wake",
        "--expect", "picojs load home: ESP_OK ok=1",
        "--expect", "picojs home loaded",
        "--expect", "launcher",
        "--expect", "picojs load sysmon: ESP_OK ok=1",
        "--expect", "picojs sysmon loaded",
        "--expect", "kernel",
        "--expect", "load _",
        "--expect", "picojs load snake: ESP_OK ok=1",
        "--expect", "picojs snake loaded",
        "--expect", "score",
        "--expect", "demo grid/layers",
        "js reset",
        "picojs load home",
        "picojs dump",
        "js reset",
        "picojs load sysmon",
        "picojs run 1 1000",
        "picojs dump",
        "js reset",
        "picojs load snake",
        "picojs run 2 250",
        "picojs key left",
        "picojs dump",
    ]
    return subprocess.run(commands).returncode


if __name__ == "__main__":
    raise SystemExit(main())
