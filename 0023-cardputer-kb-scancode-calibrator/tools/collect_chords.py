#!/usr/bin/env python3

import argparse
import json
import re
import sys
import time

import serial  # type: ignore


LINE_RE = re.compile(r"pressed keynums=\[(?P<nums>[0-9,]*)\]\s+pinset=(?P<pinset>.+)$")


def parse_nums(s: str) -> list[int]:
    s = s.strip()
    if not s:
        return []
    return [int(x) for x in s.split(",") if x]


def main() -> int:
    ap = argparse.ArgumentParser(description="Collect Cardputer key chords from firmware logs.")
    ap.add_argument("port", help="Serial port (prefer /dev/serial/by-id/...)")
    ap.add_argument("--baud", type=int, default=115200, help="Baud rate (default: 115200)")
    ap.add_argument("--out", default="chords.json", help="Output JSON file (default: chords.json)")
    args = ap.parse_args()

    seen: dict[str, dict] = {}
    started = time.time()

    with serial.Serial(args.port, args.baud, timeout=0.5) as ser:
        ser.reset_input_buffer()
        print(f"# Listening on {args.port} @ {args.baud}. Press keys/chords; Ctrl+C to stop.", file=sys.stderr)
        while True:
            try:
                raw = ser.readline()
            except serial.SerialException as e:
                print(f"ERROR: serial read failed: {e}", file=sys.stderr)
                return 2

            if not raw:
                continue

            try:
                line = raw.decode("utf-8", errors="replace").strip()
            except Exception:
                continue

            m = LINE_RE.search(line)
            if not m:
                continue

            nums = parse_nums(m.group("nums"))
            if not nums:
                continue
            key = ",".join(str(n) for n in nums)
            entry = seen.get(key)
            if entry is None:
                entry = {
                    "required_keynums": nums,
                    "count": 0,
                    "first_seen_s": time.time() - started,
                    "last_seen_s": 0.0,
                    "pinset": m.group("pinset"),
                }
                seen[key] = entry

            entry["count"] += 1
            entry["last_seen_s"] = time.time() - started

            print(f"chord [{key}] count={entry['count']} pinset={entry['pinset']}", file=sys.stderr)

    # Unreachable


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except KeyboardInterrupt:
        # Best-effort: when interrupted, we might be in the middle of a readline.
        # Just exit with success.
        raise SystemExit(0)

