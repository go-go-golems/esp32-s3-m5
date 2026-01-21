#!/usr/bin/env python3

import argparse
import os
import random
import socket
import struct
import time

MCAST_GRP = "239.255.32.6"
MCAST_PORT = 4626

HDR_FMT = "<4sBBBBIIIIIHH"  # 32 bytes
HDR_LEN = struct.calcsize(HDR_FMT)

PONG_FMT = "<IbBBBHIII16s5x"  # 43 bytes (5 padding bytes)
PONG_LEN = struct.calcsize(PONG_FMT)


def build_header(msg_type: int, *, epoch_id: int, msg_id: int, sender_id: int, flags: int = 0, target: int = 0, execute_at_ms: int = 0, payload_len: int = 0) -> bytes:
    return struct.pack(
        HDR_FMT,
        b"MLED",
        1,  # version
        msg_type & 0xFF,
        flags & 0xFF,
        HDR_LEN,
        epoch_id & 0xFFFFFFFF,
        msg_id & 0xFFFFFFFF,
        sender_id & 0xFFFFFFFF,
        target & 0xFFFFFFFF,
        execute_at_ms & 0xFFFFFFFF,
        payload_len & 0xFFFF,
        0,  # reserved
    )


def parse_header(pkt: bytes):
    if len(pkt) < HDR_LEN:
        return None
    fields = struct.unpack(HDR_FMT, pkt[:HDR_LEN])
    magic, version, msg_type, flags, hdr_len, epoch_id, msg_id, sender_id, target, execute_at_ms, payload_len, _reserved = fields
    if magic != b"MLED" or version != 1 or hdr_len != HDR_LEN:
        return None
    if HDR_LEN + payload_len > len(pkt):
        return None
    return {
        "type": msg_type,
        "flags": flags,
        "epoch_id": epoch_id,
        "msg_id": msg_id,
        "sender_id": sender_id,
        "target": target,
        "execute_at_ms": execute_at_ms,
        "payload_len": payload_len,
        "payload": pkt[HDR_LEN : HDR_LEN + payload_len],
    }


def parse_pong(payload: bytes):
    if len(payload) < PONG_LEN:
        return None
    (
        uptime_ms,
        rssi_dbm,
        state_flags,
        brightness_pct,
        pattern_type,
        frame_ms,
        active_cue_id,
        controller_epoch,
        show_ms_now,
        name_raw,
    ) = struct.unpack(PONG_FMT, payload[:PONG_LEN])
    name = name_raw.split(b"\x00", 1)[0].decode("utf-8", errors="replace")
    return {
        "uptime_ms": uptime_ms,
        "rssi_dbm": rssi_dbm,
        "state_flags": state_flags,
        "brightness_pct": brightness_pct,
        "pattern_type": pattern_type,
        "frame_ms": frame_ms,
        "active_cue_id": active_cue_id,
        "controller_epoch": controller_epoch,
        "show_ms_now": show_ms_now,
        "name": name,
    }


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--group", default=MCAST_GRP)
    ap.add_argument("--port", type=int, default=MCAST_PORT)
    ap.add_argument("--bind-ip", default=None, help="Bind the UDP socket to a specific local IPv4 (fixes multi-NIC multicast routing)")
    ap.add_argument("--timeout", type=float, default=2.0, help="Seconds to wait for PONGs")
    ap.add_argument("--repeat", type=int, default=1)
    args = ap.parse_args()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 1)
    sock.settimeout(0.2)

    if args.bind_ip:
        sock.bind((args.bind_ip, 0))
        sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_IF, socket.inet_aton(args.bind_ip))

    msg_id = random.randint(1, 0xFFFFFFFF)
    hdr = build_header(0x20, epoch_id=0, msg_id=msg_id, sender_id=0, payload_len=0)  # PING

    for i in range(args.repeat):
        sock.sendto(hdr, (args.group, args.port))
        time.sleep(0.02)

    local_ip, local_port = sock.getsockname()
    print(f"sent PING msg_id={msg_id} to {args.group}:{args.port} from {local_ip}:{local_port}")

    deadline = time.time() + args.timeout
    seen = set()
    while time.time() < deadline:
        try:
            pkt, addr = sock.recvfrom(2048)
        except socket.timeout:
            continue

        h = parse_header(pkt)
        if not h:
            continue
        if h["type"] != 0x21:  # PONG
            continue
        pong = parse_pong(h["payload"])
        if not pong:
            continue

        node_id = h["sender_id"]
        key = (addr[0], addr[1], node_id)
        if key in seen:
            continue
        seen.add(key)

        print(
            f"pong from {addr[0]}:{addr[1]} node_id={node_id} "
            f"name={pong['name']!r} rssi={pong['rssi_dbm']} "
            f"epoch={pong['controller_epoch']} show_ms={pong['show_ms_now']} "
            f"pat={pong['pattern_type']} bri={pong['brightness_pct']} cue={pong['active_cue_id']}"
        )

    if not seen:
        print("no PONGs received")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
