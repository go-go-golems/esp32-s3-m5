#!/usr/bin/env python3
"""PPA module simulator for testing the PPA Dial firmware without amplifiers.

Implements the reconstructed Four Audio PPA UDP protocol (ticket
M5DIAL-PPA-CONTROL design doc §4): answers pings with this module's uid and
answers preset recalls according to a scripted behavior.

Usage:
  python3 ppa_sim.py --uid 0x1234ABCD                # always ack
  python3 ppa_sim.py --uid 0x1234ABCD --behavior busy2   # busy twice, then ok
  python3 ppa_sim.py --uid 0x1234ABCD --behavior error   # hard error
  python3 ppa_sim.py --uid 0x1234ABCD --behavior silent  # never answer recalls
"""

import argparse
import socket
import struct
import sys

PPA_PORT = 5001
TYPE_PING = 0x00
TYPE_RECALL = 0x04
KIND_OK = 0x01
KIND_ERROR = 0x09
ERR_SUB_BUSY = 0x03


def build_header(msg_type: int, status: int, uid: int, seq: int, comp: int, res: int) -> bytes:
    return struct.pack("<BBHIHBB", msg_type, 0x01, status, uid, seq, comp, res)


def parse_header(data: bytes):
    if len(data) < 12:
        return None
    msg_type, _ver, status, uid, seq, comp, res = struct.unpack("<BBHIHBB", data[:12])
    return {"type": msg_type, "status": status, "uid": uid, "seq": seq, "comp": comp, "res": res}


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--uid", type=lambda v: int(v, 0), required=True, help="DeviceUniqueId (e.g. 0x1234ABCD)")
    ap.add_argument("--behavior", choices=["ok", "busy2", "error", "silent"], default="ok",
                    help="recall behavior: ok=ack, busy2=busy twice then ok, error=hard error, silent=timeout")
    ap.add_argument("--bind", default="0.0.0.0")
    args = ap.parse_args()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    sock.bind((args.bind, PPA_PORT))
    print(f"ppa_sim uid=0x{args.uid:08X} behavior={args.behavior} listening on {args.bind}:{PPA_PORT}")

    busy_count = 0
    while True:
        data, addr = sock.recvfrom(1024)
        hdr = parse_header(data)
        if hdr is None:
            continue
        # Ignore our own replies looping back via broadcast.
        if hdr["uid"] == args.uid:
            continue
        print(f"rx {addr[0]}:{addr[1]} type=0x{hdr['type']:02X} status=0x{hdr['status']:04X} "
              f"seq=0x{hdr['seq']:04X} raw={data.hex()}")

        if hdr["type"] == TYPE_PING:
            reply = build_header(TYPE_PING, KIND_OK, args.uid, hdr["seq"], 0xFE, 0x00) + b"\x00" * 4
            sock.sendto(reply, addr)
            print(f"tx ping-ack -> {addr[0]}")
        elif hdr["type"] == TYPE_RECALL:
            preset_id, preset_sub = data[13], data[14]
            print(f"   recall preset id={preset_id} sub={preset_sub}")
            if args.behavior == "silent":
                print("   (silent: no reply)")
                continue
            if args.behavior == "error":
                reply = build_header(TYPE_RECALL, KIND_ERROR, args.uid, hdr["seq"], 0xFF, 0x01) + bytes([0x00])
                sock.sendto(reply, addr)
                print("   tx error")
                continue
            if args.behavior == "busy2" and busy_count < 2:
                busy_count += 1
                reply = build_header(TYPE_RECALL, KIND_ERROR, args.uid, hdr["seq"], 0xFF, 0x01) + bytes([ERR_SUB_BUSY])
                sock.sendto(reply, addr)
                print(f"   tx busy ({busy_count}/2)")
                continue
            reply = build_header(TYPE_RECALL, KIND_OK, args.uid, hdr["seq"], 0xFF, 0x01)
            sock.sendto(reply, addr)
            print("   tx ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
