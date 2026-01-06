#!/usr/bin/env python3
import argparse
import os
import re
import sys
import time

import serial


PROMPT_RE = re.compile(rb"\bgw>\s*$", re.MULTILINE)


def read_until_prompt(s: serial.Serial, timeout_s: float) -> bytes:
    end = time.time() + timeout_s
    buf = bytearray()
    while time.time() < end:
        chunk = s.read(4096)
        if chunk:
            buf.extend(chunk)
            if PROMPT_RE.search(bytes(buf)):
                return bytes(buf)
        else:
            time.sleep(0.02)
    return bytes(buf)


def send_cmd(s: serial.Serial, out_f, cmd: str, timeout_s: float) -> None:
    out_f.write(f"\n#> {cmd}\n".encode("utf-8"))
    out_f.flush()
    s.write(cmd.encode("utf-8") + b"\r\n")
    s.flush()
    out = read_until_prompt(s, timeout_s=timeout_s)
    out_f.write(out)
    out_f.flush()


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Send a sequence of gw/zb commands, then capture host console output."
    )
    ap.add_argument("--port", required=True)
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--out", required=True)
    ap.add_argument("--seconds", type=float, default=300.0)
    ap.add_argument(
        "--cmd",
        action="append",
        default=[],
        help="Command to send (can be repeated).",
    )
    ap.add_argument("--prompt-timeout", type=float, default=10.0)
    args = ap.parse_args()

    if not args.cmd:
        args.cmd = [
            "monitor on",
            "zb tclk set default",
            "zb lke off",
            "gw post permit_join 180",
        ]

    os.makedirs(os.path.dirname(args.out), exist_ok=True)

    sys.stderr.write(
        f"[hostcap] port={args.port} baud={args.baud} seconds={args.seconds} out={args.out}\n"
    )

    start = time.time()
    end = start + args.seconds

    with serial.Serial(args.port, args.baud, timeout=0.1) as s:
        # Best effort: avoid asserting DTR/RTS (can reset some boards).
        try:
            s.dtr = False
            s.rts = False
        except Exception:
            pass

        s.reset_input_buffer()

        with open(args.out, "wb") as out_f:
            # Get prompt
            s.write(b"\r\n")
            s.flush()
            out_f.write(read_until_prompt(s, timeout_s=8.0))
            out_f.flush()

            out_f.write(b"\n[hostcap] driving commands...\n")
            out_f.flush()
            for cmd in args.cmd:
                send_cmd(s, out_f, cmd, timeout_s=args.prompt_timeout)

            out_f.write(
                f"\n[hostcap] capturing for {args.seconds:.1f}s...\n".encode("utf-8")
            )
            out_f.flush()

            while time.time() < end:
                chunk = s.read(4096)
                if chunk:
                    out_f.write(chunk)
                    out_f.flush()
                else:
                    time.sleep(0.02)

    sys.stderr.write("[hostcap] done\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
