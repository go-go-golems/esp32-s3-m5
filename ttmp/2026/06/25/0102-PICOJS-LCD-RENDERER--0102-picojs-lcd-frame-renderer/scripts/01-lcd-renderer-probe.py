#!/usr/bin/env python3
"""Probe PicoJS LCD render path and screen-dump parity."""

from __future__ import annotations

import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[6]
CONSOLE = ROOT / "0102-esp32-p4-visual-quickjs-repl" / "tools" / "picocalc_console.py"


def main() -> int:
    commands = [
        str(CONSOLE),
        "--quiet-wake",
        "--expect", "actual_khz=40000",
        "--expect", "picojs load dashboard: ESP_OK ok=1",
        "--expect", "picojs render after load: ESP_OK",
        "--expect", "picojs render after frame: ESP_OK",
        "--expect", "picojs render: ESP_OK",
        "--expect", "PicoJS Dashboard",
        "--expect", "dashboard native picojs",
        "--expect", "screen dump: ESP_OK",
        "status",
        "js reset",
        "picojs load dashboard",
        "picojs frame 1000",
        "picojs render",
        "picojs dump",
        "screen dump",
        "status",
    ]
    return subprocess.run(commands).returncode


if __name__ == "__main__":
    raise SystemExit(main())
