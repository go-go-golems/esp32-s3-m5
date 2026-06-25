#!/usr/bin/env python3
"""Probe PicoJS frame/timer callbacks through the reusable 0102 UART helper."""

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
        "--expect", "picojs run: ESP_OK count=3 dt_ms=1000",
        "--expect", "frames=3",
        "--expect", "errors=0",
        "--expect", "ticks: 3",
        "js reset",
        "picojs load interactive",
        "picojs run 3 1000",
        "picojs status",
        "picojs dump",
    ]
    first = subprocess.run(commands)
    if first.returncode != 0:
        return first.returncode

    error_source = (
        "js eval "
        "var app=OS.app('err');var n=0;"
        "var p=app.panel('main').frame('single').title(' err ');"
        "p.text(function(){return 'n='+n}).at('center',2);"
        "app.compute(function(){n++; if(n===2) throw new Error('compute-boom');});"
        "app.mount();"
    )
    commands = [
        str(CONSOLE),
        "--quiet-wake",
        "--expect", "[eval] ok=1",
        "--expect", "picojs run: ESP_OK count=2 dt_ms=100",
        "--expect", "frames=2",
        "--expect", "errors=1",
        "--expect", "n=2",
        "js reset",
        error_source,
        "picojs run 2 100",
        "picojs status",
        "picojs dump",
    ]
    return subprocess.run(commands).returncode


if __name__ == "__main__":
    raise SystemExit(main())
