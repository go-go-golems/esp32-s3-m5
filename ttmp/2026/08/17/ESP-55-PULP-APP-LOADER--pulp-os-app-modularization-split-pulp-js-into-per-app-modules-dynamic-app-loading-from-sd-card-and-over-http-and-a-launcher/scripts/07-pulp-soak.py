#!/usr/bin/env python3
"""ESP-55 P7 soak: cycle the launcher through every app via synthetic
taps for N minutes while sampling js status. Companion to an HTTP poller
hitting /status + /apps/list. Uses the hold-open port discipline of
script 04 (no reset).

usage: 07-pulp-soak.py [--minutes 22] [--port P] [--output F]
Summary printed at the end: cycles, screens seen, exceptions, arena trend.
"""
import argparse, fcntl, os, re, select, sys, termios, time
from pathlib import Path

DEFAULT_PORT = Path(
    "/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_"
    "D0:CF:13:16:17:DC-if00")
# Launcher rows (y centers); 349 = 2048 traps swipe-down -> js pulp after.
ROWS = [178, 235, 292, 349, 406, 463, 520, 577, 634, 691, 748]

def drain(fd, dur, sink):
    end = time.monotonic() + dur
    out = []
    while time.monotonic() < end:
        r, _, _ = select.select([fd], [], [], 0.05)
        if r:
            try:
                c = os.read(fd, 4096)
            except OSError:
                break
            if c:
                t = c.decode("utf-8", "replace")
                out.append(t)
                sink.write(t)
    return "".join(out)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--minutes", type=float, default=22.0)
    ap.add_argument("--port", type=Path, default=DEFAULT_PORT)
    ap.add_argument("--output", type=Path, required=True)
    a = ap.parse_args()
    fd = os.open(a.port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
    at = termios.tcgetattr(fd)
    at[0] = at[1] = at[3] = 0
    at[6][termios.VMIN] = at[6][termios.VTIME] = 0
    termios.tcsetattr(fd, termios.TCSANOW, at)
    sink = a.output.open("w")
    def cmd(c, settle=2.0):
        sink.write(f"\n# >>> {c}\n")
        os.write(fd, c.encode() + b"\n")
        return drain(fd, settle, sink)
    screens = 0
    cycles = 0
    stats = []
    deadline = time.monotonic() + a.minutes * 60
    cmd("js pulp", 4)
    while time.monotonic() < deadline:
        for y in ROWS:
            if time.monotonic() > deadline:
                break
            out = cmd(f"js tap 270 {y}", 2.5)
            screens += out.count("pulp screen:")
            if y == 349:
                cmd("js pulp", 3)   # 2048 traps G.DOWN
            else:
                cmd("js swipe 5", 2)
        cycles += 1
        st = cmd("js status", 2)
        m = re.search(r"arena_used=(\d+).*exceptions=(\d+)", st)
        if m:
            stats.append((cycles, int(m.group(1)), int(m.group(2))))
            sink.write(f"# cycle {cycles}: arena={m.group(1)} "
                       f"exc={m.group(2)}\n")
    sink.close()
    print(f"soak done: cycles={cycles} screen_lines={screens}")
    for c, ar, ex in stats:
        print(f"  cycle {c}: arena_used={ar} exceptions={ex}")
    return 0

if __name__ == "__main__":
    sys.exit(main())
