#!/usr/bin/env python3
"""
Track C helper (0018-QEMU-UART-ECHO):

Run ESP32-S3 QEMU with `-serial mon:stdio` under a pseudo-terminal (PTY),
send a few payloads as if typing, and write a transcript.

This is a compromise between:
- "manual typing" (hard to automate in CI-like runs), and
- simple stdin piping (which can behave differently than a TTY).

Assumptions:
- You have built the 0018 firmware so these exist:
  - build/qemu_flash.bin
  - build/qemu_efuse.bin
- `qemu-system-xtensa` is on PATH (usually after `source ~/esp/esp-idf-5.4.1/export.sh`)

Usage:
  cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0018-qemu-uart-echo-firmware
  source ~/esp/esp-idf-5.4.1/export.sh
  python ../ttmp/2025/12/29/0018-QEMU-UART-ECHO--track-c-minimal-uart-echo-firmware-to-isolate-qemu-uart-rx/scripts/02-run-qemu-mon-stdio-pty.py \
    --out ../ttmp/2025/12/29/0018-QEMU-UART-ECHO--track-c-minimal-uart-echo-firmware-to-isolate-qemu-uart-rx/sources/track-c-qemu-mon-stdio-pty-$(date +%Y%m%d-%H%M%S).txt
"""

from __future__ import annotations

import argparse
import os
import pty
import shutil
import signal
import subprocess
import time
from pathlib import Path


def _write(f, s: str) -> None:
    f.write(s)
    f.flush()


def _build_qemu_args(project_dir: Path) -> list[str]:
    flash = project_dir / "build" / "qemu_flash.bin"
    efuse = project_dir / "build" / "qemu_efuse.bin"
    return [
        "qemu-system-xtensa",
        "-M",
        "esp32s3",
        "-drive",
        f"file={flash},if=mtd,format=raw",
        "-drive",
        f"file={efuse},if=none,format=raw,id=efuse",
        "-global",
        "driver=nvram.esp32c3.efuse,property=drive,value=efuse",
        "-global",
        "driver=timer.esp32s3.timg,property=wdt_disable,value=true",
        "-nic",
        "user,model=open_eth",
        "-nographic",
        "-serial",
        "mon:stdio",
    ]


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--project-dir", default=os.getcwd(), help="ESP-IDF project dir (default: cwd).")
    ap.add_argument("--out", required=True, help="Transcript output path.")
    ap.add_argument("--runtime-s", type=float, default=12.0, help="How long to run before terminating QEMU.")
    args = ap.parse_args()

    project_dir = Path(args.project_dir).resolve()
    out_path = Path(args.out).resolve()

    if shutil.which("qemu-system-xtensa") is None:
        raise SystemExit(
            "qemu-system-xtensa not found on PATH. "
            "Did you `source ~/esp/esp-idf-5.4.1/export.sh` (or equivalent) first?"
        )

    flash = project_dir / "build" / "qemu_flash.bin"
    efuse = project_dir / "build" / "qemu_efuse.bin"
    if not flash.exists() or not efuse.exists():
        raise SystemExit(f"Missing {flash} or {efuse}. Build/generate them first.")

    out_path.parent.mkdir(parents=True, exist_ok=True)

    qemu_args = _build_qemu_args(project_dir)

    master_fd, slave_fd = pty.openpty()
    start = time.time()

    with out_path.open("w", encoding="utf-8") as f:
        _write(f, "## Track C: QEMU UART via mon:stdio under PTY\n")
        _write(f, f"cwd: {project_dir}\n")
        _write(f, "qemu cmd:\n")
        _write(f, "  " + " ".join(qemu_args) + "\n\n")

        qemu = subprocess.Popen(
            qemu_args,
            cwd=str(project_dir),
            stdin=slave_fd,
            stdout=slave_fd,
            stderr=slave_fd,
            close_fds=True,
        )

        try:
            # Let boot output flow.
            time.sleep(2.5)

            # Send payloads like a human would type.
            payloads = [b"abc\r\n", b"1+2\r\n", b"xyz\r\n"]
            for p in payloads:
                os.write(master_fd, p)
                _write(f, f"\n[pty] send: {p!r}\n")
                time.sleep(1.0)

            # Drain PTY output until runtime expires.
            os.set_blocking(master_fd, False)
            while time.time() - start < args.runtime_s:
                try:
                    data = os.read(master_fd, 4096)
                except BlockingIOError:
                    time.sleep(0.05)
                    continue
                if not data:
                    time.sleep(0.05)
                    continue
                _write(f, data.decode("utf-8", errors="replace"))
        finally:
            qemu.send_signal(signal.SIGTERM)
            try:
                qemu.wait(timeout=3)
            except subprocess.TimeoutExpired:
                qemu.kill()
                qemu.wait(timeout=3)
            _write(f, f"\n[qemu] exit code: {qemu.returncode}\n")

    try:
        os.close(master_fd)
    except OSError:
        pass
    try:
        os.close(slave_fd)
    except OSError:
        pass

    return 0


if __name__ == "__main__":
    raise SystemExit(main())


