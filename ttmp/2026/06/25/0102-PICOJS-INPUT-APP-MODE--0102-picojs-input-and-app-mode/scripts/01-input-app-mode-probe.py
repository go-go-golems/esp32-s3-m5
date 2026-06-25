#!/usr/bin/env python3
"""Probe PicoJS console key injection and app-mode state."""

from __future__ import annotations

import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[6]
CONSOLE = ROOT / "0102-esp32-p4-visual-quickjs-repl" / "tools" / "picocalc_console.py"


def main() -> int:
    commands = [
        str(CONSOLE),
        "--quiet-wake",
        "--expect", "picojs load interactive: ESP_OK ok=1",
        "--expect", "picojs mode: ESP_OK app_mode=1",
        "--expect", "app_mode=1",
        "--expect", "KEY left",
        "--expect", "picojs key: ESP_OK token=left",
        "--expect", "last key: left",
        "--expect", "KEY a",
        "--expect", "picojs key: ESP_OK token=a",
        "--expect", "last key: a",
        "--expect", "picojs mode: ESP_OK app_mode=0",
        "--expect", "app_mode=0",
        "js reset",
        "picojs load interactive",
        "picojs mode app",
        "picojs status",
        "picojs key left",
        "picojs dump",
        "picojs key a",
        "picojs dump",
        "picojs mode repl",
        "picojs status",
    ]
    return subprocess.run(commands).returncode


if __name__ == "__main__":
    raise SystemExit(main())
