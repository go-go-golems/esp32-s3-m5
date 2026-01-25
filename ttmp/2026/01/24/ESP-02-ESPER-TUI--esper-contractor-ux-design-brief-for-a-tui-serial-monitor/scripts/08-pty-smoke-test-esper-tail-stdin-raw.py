#!/usr/bin/env python3
import os
import pty
import select
import subprocess
import sys
import time

TICKET = "ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor"
DETECT = f"./{TICKET}/scripts/01-detect-port.sh"


def detect_port():
    env = os.environ.copy()
    try:
        out = subprocess.check_output([DETECT], env=env, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
        sys.stderr.write(e.output.decode(errors="replace"))
        raise
    return out.decode().strip()


def read_until(master_fd, deadline, max_bytes=20000):
    buf = bytearray()
    while time.time() < deadline and len(buf) < max_bytes:
        r, _, _ = select.select([master_fd], [], [], 0.2)
        if not r:
            continue
        try:
            chunk = os.read(master_fd, 4096)
        except OSError:
            break
        if not chunk:
            break
        buf.extend(chunk)
    return bytes(buf)


def main():
    espport = os.environ.get("ESPPORT") or detect_port()
    baud = os.environ.get("BAUD", "115200")

    cmd = [
        "bash",
        "-lc",
        f"cd esper && go run ./cmd/esper tail --port {espport!s} --baud {baud!s} --stdin-raw",
    ]

    master_fd, slave_fd = pty.openpty()
    p = subprocess.Popen(cmd, stdin=slave_fd, stdout=slave_fd, stderr=slave_fd, close_fds=True)
    os.close(slave_fd)

    try:
        # Give it time to compile/start and connect.
        time.sleep(2.0)

        # Send a command to the esp_console REPL (raw mode, so \r is typical).
        os.write(master_fd, b"help\r")
        time.sleep(1.0)

        # Read some output.
        out1 = read_until(master_fd, time.time() + 4.0)

        # Exit always with Ctrl-] (0x1d).
        os.write(master_fd, b"\x1d")

        try:
            p.wait(timeout=6.0)
        except subprocess.TimeoutExpired:
            p.kill()
            p.wait(timeout=3.0)

        out2 = read_until(master_fd, time.time() + 1.0)
        sys.stdout.buffer.write(out1 + out2)
        return 0
    finally:
        try:
            os.close(master_fd)
        except OSError:
            pass


if __name__ == "__main__":
    raise SystemExit(main())

