#!/usr/bin/env python3
import argparse
import os
import re
import sys
import time

import serial


HOST_PROMPT_RE = re.compile(rb"\bgw>\s*$", re.MULTILINE)
INTERESTING_RE = re.compile(
    r"(authorized|annce|announce|device update|leave|permit|commission|rejoin|short)",
    re.IGNORECASE,
)


def _best_effort_disable_modem_lines(s: serial.Serial) -> None:
    try:
        s.dtr = False
        s.rts = False
    except Exception:
        pass


def read_until_prompt(s: serial.Serial, timeout_s: float) -> bytes:
    end = time.time() + timeout_s
    buf = bytearray()
    while time.time() < end:
        chunk = s.read(4096)
        if chunk:
            buf.extend(chunk)
            if HOST_PROMPT_RE.search(bytes(buf)):
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
        description="Drive host commissioning commands and capture host+H2 logs concurrently."
    )
    ap.add_argument("--run-dir", required=True)
    ap.add_argument("--seconds", type=float, default=240.0)
    ap.add_argument("--host-port", default="/dev/ttyACM1")
    ap.add_argument("--h2-port", default="/dev/ttyACM0")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--permit-join-seconds", type=int, default=180)
    ap.add_argument(
        "--lke",
        choices=["on", "off", "skip"],
        default="off",
        help="Set link-key exchange required mode before opening permit-join.",
    )
    ap.add_argument(
        "--tclk",
        choices=["default", "skip"],
        default="default",
        help="Set Trust Center link key to ZigBeeAlliance09 (default) or skip.",
    )
    args = ap.parse_args()

    os.makedirs(args.run_dir, exist_ok=True)
    host_path = os.path.join(args.run_dir, "host.log")
    h2_path = os.path.join(args.run_dir, "h2.log")

    sys.stderr.write(f"[dualcap] run_dir={args.run_dir}\n")
    sys.stderr.write(f"[dualcap] host={args.host_port} h2={args.h2_port} baud={args.baud}\n")
    permit_join_seconds = args.permit_join_seconds
    if permit_join_seconds > 255:
        sys.stderr.write(
            f"[dualcap] NOTE: permit-join seconds clamped by NCP protocol (requested={permit_join_seconds} -> 255)\n"
        )
        permit_join_seconds = 255
    sys.stderr.write(f"[dualcap] duration={args.seconds}s permit_join={permit_join_seconds}s\n")
    sys.stderr.write(f"[dualcap] tclk={args.tclk} lke={args.lke}\n")
    sys.stderr.flush()

    with (
        serial.Serial(args.host_port, args.baud, timeout=0.05) as host,
        serial.Serial(args.h2_port, args.baud, timeout=0.05) as h2,
        open(host_path, "wb") as host_f,
        open(h2_path, "wb") as h2_f,
    ):
        _best_effort_disable_modem_lines(host)
        _best_effort_disable_modem_lines(h2)

        host.reset_input_buffer()
        h2.reset_input_buffer()

        # Drive host commissioning commands.
        host.write(b"\r\n")
        host.flush()
        host_f.write(read_until_prompt(host, timeout_s=8.0))
        host_f.flush()

        send_cmd(host, host_f, "monitor on", timeout_s=10.0)
        if args.tclk == "default":
            send_cmd(host, host_f, "zb tclk set default", timeout_s=10.0)
        if args.lke != "skip":
            send_cmd(host, host_f, f"zb lke {args.lke}", timeout_s=10.0)
        send_cmd(host, host_f, f"gw post permit_join {permit_join_seconds}", timeout_s=10.0)

        sys.stderr.write(
            "[dualcap] Commands sent. Put the device into pairing mode now.\n"
        )
        sys.stderr.flush()

        # Capture loop.
        end = time.time() + args.seconds
        while time.time() < end:
            h2_chunk = h2.read(4096)
            if h2_chunk:
                h2_f.write(h2_chunk)
                h2_f.flush()
                try:
                    txt = h2_chunk.decode("utf-8", errors="ignore")
                    for line in txt.splitlines():
                        if INTERESTING_RE.search(line):
                            sys.stdout.write(f"[h2] {line}\n")
                            sys.stdout.flush()
                except Exception:
                    pass

            host_chunk = host.read(4096)
            if host_chunk:
                host_f.write(host_chunk)
                host_f.flush()
                try:
                    txt = host_chunk.decode("utf-8", errors="ignore")
                    for line in txt.splitlines():
                        if INTERESTING_RE.search(line):
                            sys.stdout.write(f"[host] {line}\n")
                            sys.stdout.flush()
                except Exception:
                    pass

            if not h2_chunk and not host_chunk:
                time.sleep(0.02)

    sys.stderr.write("[dualcap] done\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
