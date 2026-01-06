#!/usr/bin/env python3
"""
Track C helper (0018-QEMU-UART-ECHO):

Run ESP32-S3 QEMU with UART0 attached to a TCP serial server (default :5555),
connect as a client, send a few payloads, and write a transcript.

Why this exists:
- `idf.py qemu monitor` couples QEMU to idf_monitor, which makes "raw TCP" testing harder.
- We want a scriptable, repeatable way to prove whether UART RX bytes reach ESP-IDF.

Assumptions:
- You have built the 0018 firmware so these exist:
  - build/qemu_flash.bin
  - build/qemu_efuse.bin
- `qemu-system-xtensa` is on PATH (usually after `source ~/esp/esp-idf-5.4.1/export.sh`)

Usage (recommended):
  cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0018-qemu-uart-echo-firmware
  source ~/esp/esp-idf-5.4.1/export.sh
  python ../ttmp/2025/12/29/0018-QEMU-UART-ECHO--track-c-minimal-uart-echo-firmware-to-isolate-qemu-uart-rx/scripts/01-run-qemu-tcp-serial-5555.py \
    --out ../ttmp/2025/12/29/0018-QEMU-UART-ECHO--track-c-minimal-uart-echo-firmware-to-isolate-qemu-uart-rx/sources/track-c-qemu-serial-tcp-5555-$(date +%Y%m%d-%H%M%S).txt
"""

from __future__ import annotations

import argparse
import os
import shutil
import signal
import socket
import subprocess
import time
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class RunConfig:
    project_dir: Path
    out_path: Path
    host: str
    port: int
    connect_timeout_s: float
    run_timeout_s: float
    quiet_qemu_stdout: bool
    payloads: list[bytes]


def _write(f, s: str) -> None:
    f.write(s)
    f.flush()


def _wait_for_tcp(host: str, port: int, timeout_s: float) -> None:
    deadline = time.time() + timeout_s
    while time.time() < deadline:
        try:
            with socket.create_connection((host, port), timeout=0.5):
                return
        except OSError:
            time.sleep(0.1)
    raise TimeoutError(f"timed out waiting for {host}:{port} to accept connections")


def _drain_socket(f, sock: socket.socket, seconds: float) -> None:
    sock.settimeout(0.2)
    end = time.time() + seconds
    while time.time() < end:
        try:
            data = sock.recv(4096)
        except socket.timeout:
            continue
        if not data:
            return
        _write(f, data.decode("utf-8", errors="replace"))


def _build_qemu_args(project_dir: Path, port: int) -> list[str]:
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
        f"tcp::{port},server,nowait",
    ]


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument(
        "--project-dir",
        default=os.getcwd(),
        help="Path to the ESP-IDF project directory (default: cwd).",
    )
    ap.add_argument(
        "--out",
        required=True,
        help="Where to write the transcript (text file).",
    )
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=5555)
    ap.add_argument("--connect-timeout-s", type=float, default=10.0)
    ap.add_argument("--run-timeout-s", type=float, default=20.0)
    ap.add_argument(
        "--quiet-qemu-stdout",
        action="store_true",
        help="If set, do not prefix/record any QEMU process stdout into the transcript.",
    )
    args = ap.parse_args()

    cfg = RunConfig(
        project_dir=Path(args.project_dir).resolve(),
        out_path=Path(args.out).resolve(),
        host=args.host,
        port=args.port,
        connect_timeout_s=args.connect_timeout_s,
        run_timeout_s=args.run_timeout_s,
        quiet_qemu_stdout=bool(args.quiet_qemu_stdout),
        payloads=[b"abc\r\n", b"1+2\r\n", b"xyz\r\n"],
    )

    if shutil.which("qemu-system-xtensa") is None:
        raise SystemExit(
            "qemu-system-xtensa not found on PATH. "
            "Did you `source ~/esp/esp-idf-5.4.1/export.sh` (or equivalent) first?"
        )

    flash = cfg.project_dir / "build" / "qemu_flash.bin"
    efuse = cfg.project_dir / "build" / "qemu_efuse.bin"
    if not flash.exists() or not efuse.exists():
        raise SystemExit(
            f"Missing QEMU artifacts.\n"
            f"- {flash} (exists={flash.exists()})\n"
            f"- {efuse} (exists={efuse.exists()})\n"
            f"Build and generate them first, e.g.:\n"
            f"  cd {cfg.project_dir} && ./build.sh build && ./build.sh qemu (once)\n"
            f"(or generate qemu_flash.bin/efuse via the ESP-IDF qemu action)."
        )

    cfg.out_path.parent.mkdir(parents=True, exist_ok=True)

    qemu_args = _build_qemu_args(cfg.project_dir, cfg.port)

    start = time.time()
    with cfg.out_path.open("w", encoding="utf-8") as f:
        _write(f, "## Track C: QEMU UART over TCP serial\n")
        _write(f, f"cwd: {cfg.project_dir}\n")
        _write(f, f"tcp: {cfg.host}:{cfg.port}\n")
        _write(f, "qemu cmd:\n")
        _write(f, "  " + " ".join(qemu_args) + "\n\n")

        qemu = subprocess.Popen(
            qemu_args,
            cwd=str(cfg.project_dir),
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            stdin=subprocess.DEVNULL,
            text=True,
        )

        try:
            # Give QEMU a moment to emit warnings to stdout, then (optionally) record a bit of it.
            if qemu.stdout is not None and not cfg.quiet_qemu_stdout:
                stdout_start = time.time()
                while time.time() - stdout_start < 1.0:
                    line = qemu.stdout.readline()
                    if not line:
                        break
                    _write(f, "[qemu] " + line)

            _wait_for_tcp(cfg.host, cfg.port, cfg.connect_timeout_s)

            sock = socket.create_connection((cfg.host, cfg.port), timeout=2.0)
            _write(f, "\n[client] connected\n")

            # Let the banner flow for a bit.
            _drain_socket(f, sock, 3.0)

            for p in cfg.payloads:
                _write(f, f"\n[client] send: {p!r}\n")
                sock.sendall(p)
                _drain_socket(f, sock, 2.0)

            # Let it run a little longer (e.g., to see heartbeat/no-RX logs).
            remaining = cfg.run_timeout_s - (time.time() - start)
            if remaining > 0:
                _drain_socket(f, sock, min(5.0, remaining))

            _write(f, "\n[client] done; closing socket\n")
            sock.close()
        finally:
            # Stop QEMU.
            qemu.send_signal(signal.SIGTERM)
            try:
                qemu.wait(timeout=3)
            except subprocess.TimeoutExpired:
                qemu.kill()
                qemu.wait(timeout=3)

            _write(f, f"\n[qemu] exit code: {qemu.returncode}\n")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())



