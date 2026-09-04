#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# ESP-62 — 03-serial-monitor.py
# Reusable single-owner USB Serial/JTAG monitor + console driver for the
# CoreS3. Catches boot logs by resetting via RTS, optionally sends console
# commands, and saves captured output. Kept here so it isn't rewritten each
# session. See AGENTS.md "Serial ownership": do not run another monitor on
# the same port at the same time.
#
# Usage:
#   python3 03-serial-monitor.py                    # reset + 20s boot capture
#   python3 03-serial-monitor.py --cmd "qr status"   # boot, then send a command
#   python3 03-serial-monitor.py --out /tmp/log.txt  # save captured output
#   python3 03-serial-monitor.py --no-reset          # don't reset, just listen
#
# Requires: pyserial
import argparse, serial, sys, time, threading

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", default="/dev/ttyACM0")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--secs", type=int, default=20, help="capture duration (s)")
    ap.add_argument("--cmd", help="console command to send after boot (e.g. 'qr status')")
    ap.add_argument("--cmd-after", type=float, default=7.0, help="send cmd N s after reset")
    ap.add_argument("--no-reset", action="store_true", help="don't reset, just listen")
    ap.add_argument("--out", help="save captured output to this file")
    args = ap.parse_args()

    s = serial.Serial(args.port, args.baud, timeout=0.2)
    captured = []
    stop = threading.Event()
    def reader():
        while not stop.is_set():
            if s.in_waiting:
                captured.append(s.read(s.in_waiting))
            time.sleep(0.01)
    t = threading.Thread(target=reader, daemon=True); t.start()

    if not args.no_reset:
        time.sleep(0.3)
        s.dtr = False; s.rts = True; time.sleep(0.1); s.rts = False; time.sleep(0.05)

    if args.cmd:
        time.sleep(args.cmd_after)
        s.write((args.cmd + "\r\n").encode())
        time.sleep(max(2.0, args.secs - args.cmd_after))
    else:
        time.sleep(args.secs)

    stop.set(); t.join(); s.close()
    data = b"".join(captured)
    text = data.decode("utf-8", "replace")
    print("=== captured %d bytes ===" % len(data))
    print(text)
    if args.out:
        open(args.out, "w").write(text)
        print("saved %s" % args.out, file=sys.stderr)

if __name__ == "__main__":
    main()
