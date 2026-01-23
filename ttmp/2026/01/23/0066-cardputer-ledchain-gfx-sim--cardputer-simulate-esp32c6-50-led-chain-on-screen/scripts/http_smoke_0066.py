#!/usr/bin/env python3
import argparse
import datetime as dt
import json
import pathlib
import re
import time
import urllib.error
import urllib.request

import serial  # pyserial


def write_line(ser: serial.Serial, line: str) -> None:
    ser.write((line.rstrip("\n") + "\n").encode("utf-8"))
    ser.flush()


def read_until(ser: serial.Serial, needle: bytes, timeout_s: float) -> bytes:
    deadline = time.time() + timeout_s
    buf = bytearray()
    while time.time() < deadline:
        b = ser.read(1024)
        if b:
            buf.extend(b)
            if needle in buf:
                break
        else:
            time.sleep(0.01)
    return bytes(buf)


def http_get(url: str, timeout_s: float) -> bytes:
    req = urllib.request.Request(url, method="GET")
    with urllib.request.urlopen(req, timeout=timeout_s) as resp:
        return resp.read()


def http_post_json(url: str, obj: dict, timeout_s: float) -> bytes:
    body = json.dumps(obj).encode("utf-8")
    req = urllib.request.Request(
        url,
        data=body,
        method="POST",
        headers={"content-type": "application/json; charset=utf-8"},
    )
    with urllib.request.urlopen(req, timeout=timeout_s) as resp:
        return resp.read()


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", default="/dev/ttyACM0")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--timeout", type=float, default=0.2)
    ap.add_argument("--http-timeout", type=float, default=2.0)
    args = ap.parse_args()

    ticket_dir = pathlib.Path(
        "ttmp/2026/01/23/0066-cardputer-ledchain-gfx-sim--cardputer-simulate-esp32c6-50-led-chain-on-screen"
    ).resolve()
    out_dir = ticket_dir / "various"
    out_dir.mkdir(parents=True, exist_ok=True)
    out_path = out_dir / f"http-smoke-0066-{dt.datetime.now().strftime('%Y%m%d-%H%M%S')}.log"

    transcript: list[bytes] = []

    with serial.Serial(args.port, args.baud, timeout=args.timeout) as ser:
        ser.reset_input_buffer()
        ser.reset_output_buffer()

        time.sleep(0.8)
        transcript.append(read_until(ser, b"sim> ", 6.0))

        def run(cmd: str, wait_s: float = 0.05) -> bytes:
            write_line(ser, cmd)
            time.sleep(wait_s)
            out = read_until(ser, b"sim> ", 2.0)
            transcript.append(out)
            return out

        out = run("wifi status", wait_s=0.2)
        m = re.search(rb"\bip=([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)", out)
        if not m:
            transcript.append(b"\nerror: could not parse ip=... from wifi status\n")
            out_path.write_bytes(b"".join(transcript))
            print(out_path)
            return 2

        ip = m.group(1).decode("ascii")
        base = f"http://{ip}"

        try:
            status = http_get(base + "/api/status", timeout_s=args.http_timeout)
            transcript.append(b"\nGET /api/status\n")
            transcript.append(status + b"\n")
        except urllib.error.URLError as e:
            transcript.append(b"\nGET /api/status failed: " + str(e).encode("utf-8") + b"\n")

        try:
            apply = http_post_json(
                base + "/api/apply",
                {
                    "type": "rainbow",
                    "brightness_pct": 50,
                    "frame_ms": 16,
                    "rainbow": {"speed": 5, "sat": 100, "spread": 10},
                },
                timeout_s=args.http_timeout,
            )
            transcript.append(b"\nPOST /api/apply\n")
            transcript.append(apply + b"\n")
        except urllib.error.URLError as e:
            transcript.append(b"\nPOST /api/apply failed: " + str(e).encode("utf-8") + b"\n")

    out_path.write_bytes(b"".join(transcript))
    print(out_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

