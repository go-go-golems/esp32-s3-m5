#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# ESP-62 — 01-probe-qrcode-uart.py
#
# Reusable host-side probe for the Module13.2 QRCode (M145) UART protocol.
# Use this to verify the module talks the protocol *before* flashing any
# CoreS3 firmware: connect the module's PORT.C (UART_TX/UART_RX) to a
# USB-TTL adapter (3V3) OR run it against the CoreS3's USB-CDC passthrough.
#
# It implements the key commands from sources/protocol-pdf/Module13.2-QRCode-Protocol-EN.txt:
#   - Status Read firmware version:  43 02 C1  -> reply 44 02 C1 <len_hi> <len_lo> <data...>
#   - Control start decode:          32 75 01  (no reply)
#   - Control stop decode:           32 75 02  -> reply 33 75 02 00 00
#   - Set trigger mode continuous:   21 61 41 02 -> reply 22 61 41 02 00
#
# Reference implementation: sources/arduino-lib/src/qrcode_m14.cpp (sendCmd).
#
# Usage:
#   python3 01-probe-qrcode-uart.py --port /dev/ttyUSB0 --baud 115200
#   python3 01-probe-qrcode-uart.py --port /dev/ttyACM0 --scan   # stream scan results
#
# Requires: pyserial  (pip install pyserial)

import argparse
import serial
import sys
import time

CMD_GET_FW    = bytes([0x43, 0x02, 0xC1])
CMD_GET_SW    = bytes([0x43, 0x02, 0xC2])
CMD_GET_SN    = bytes([0x43, 0x02, 0xC5])
CMD_START     = bytes([0x32, 0x75, 0x01])
CMD_STOP      = bytes([0x32, 0x75, 0x02])
CMD_TRIG_CONT = bytes([0x21, 0x61, 0x41, 0x02])
ACK_STOP      = bytes([0x33, 0x75, 0x02, 0x00, 0x00])
ACK_TRIG_CONT = bytes([0x22, 0x61, 0x41, 0x02, 0x00])


def drain(s):
    while s.in_waiting:
        s.read(s.in_waiting)


def send(s, cmd, expect=None, timeout=1.0):
    drain(s)
    sys.stdout.write(f"TX {' '.join(f'{b:02X}' for b in cmd)}\n")
    s.write(cmd)
    if expect is None:
        return None
    deadline = time.time() + timeout
    buf = b""
    while time.time() < deadline:
        if s.in_waiting:
            buf += s.read(s.in_waiting)
            if len(buf) >= len(expect):
                break
        time.sleep(0.005)
    sys.stdout.write(f"RX {' '.join(f'{b:02X}' for b in buf)}\n")
    if expect and buf[:len(expect)] == expect:
        sys.stdout.write("  -> ACK OK\n")
    elif expect:
        sys.stdout.write(f"  -> ACK MISMATCH (expected {' '.join(f'{b:02X}' for b in expect)})\n")
    return buf


def read_status(s, cmd, label):
    """Status Read (0x43) -> Status Reply (0x44): <44> <PID> <FID> <len_hi> <len_lo> <data...>"""
    drain(s)
    sys.stdout.write(f"TX {' '.join(f'{b:02X}' for b in cmd)}  ({label})\n")
    s.write(cmd)
    deadline = time.time() + 1.0
    buf = b""
    while time.time() < deadline:
        if s.in_waiting:
            buf += s.read(s.in_waiting)
            if len(buf) >= 5 and len(buf) >= 5 + ((buf[3] << 8) | buf[4]):
                break
        time.sleep(0.01)
    sys.stdout.write(f"RX {' '.join(f'{b:02X}' for b in buf)}\n")
    if len(buf) >= 5 and buf[0] == 0x44:
        ln = (buf[3] << 8) | buf[4]
        data = buf[5:5 + ln]
        try:
            sys.stdout.write(f"  -> {label}: {data.decode('utf-8', 'replace')!r}\n")
        except Exception:
            sys.stdout.write(f"  -> {label}: {data!r}\n")
    else:
        sys.stdout.write("  -> no valid 0x44 reply (module may be in USB mode or not connected)\n")
    return buf


def stream_scan(s):
    sys.stdout.write("--- streaming scan results (Ctrl-C to stop) ---\n")
    s.write(CMD_TRIG_CONT)
    time.sleep(0.2)
    s.write(CMD_START)
    try:
        while True:
            if s.in_waiting:
                data = s.read(s.in_waiting)
                sys.stdout.write(data.decode("utf-8", "replace"))
                sys.stdout.flush()
            time.sleep(0.02)
    except KeyboardInterrupt:
        sys.stdout.write("\n--- stopping ---\n")
        s.write(CMD_STOP)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", required=True)
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--scan", action="store_true", help="start continuous scan and stream results")
    args = ap.parse_args()

    with serial.Serial(args.port, args.baud, timeout=0.1) as s:
        sys.stdout.write(f"opened {args.port} @ {args.baud} 8N1\n")
        read_status(s, CMD_GET_FW, "firmware")
        read_status(s, CMD_GET_SW, "software")
        read_status(s, CMD_GET_SN, "serial no")
        send(s, CMD_TRIG_CONT, ACK_TRIG_CONT)
        send(s, CMD_START)
        time.sleep(2.0)
        send(s, CMD_STOP, ACK_STOP)
        if args.scan:
            stream_scan(s)


if __name__ == "__main__":
    main()
