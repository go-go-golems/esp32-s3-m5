#!/usr/bin/env python3

import argparse
import random
import socket
import struct
import time

MCAST_GRP = "239.255.32.6"
MCAST_PORT = 4626

HDR_FMT = "<4sBBBBIIIIIHH"  # 32 bytes
HDR_LEN = struct.calcsize(HDR_FMT)

ACK_FMT = "<IHH"
ACK_LEN = struct.calcsize(ACK_FMT)

PONG_FMT = "<IbBBBHIII16s5x"  # 43 bytes (with padding)
PONG_LEN = struct.calcsize(PONG_FMT)

PATTERN_FMT = "<BBBBI12s"  # 20 bytes
PATTERN_LEN = struct.calcsize(PATTERN_FMT)

CUE_PREPARE_FMT = "<IHH"  # then 20-byte PatternConfig
CUE_PREPARE_LEN = struct.calcsize(CUE_PREPARE_FMT) + PATTERN_LEN


def now_ms_u32() -> int:
    return int(time.monotonic() * 1000) & 0xFFFFFFFF


def build_header(
    msg_type: int,
    *,
    epoch_id: int,
    msg_id: int,
    sender_id: int,
    flags: int = 0,
    target: int = 0,
    execute_at_ms: int = 0,
    payload_len: int = 0,
) -> bytes:
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


def parse_rgb(s: str):
    s = s.strip()
    if s.startswith("#"):
        s = s[1:]
    if len(s) == 6 and all(c in "0123456789abcdefABCDEF" for c in s):
        r = int(s[0:2], 16)
        g = int(s[2:4], 16)
        b = int(s[4:6], 16)
        return (r, g, b)
    parts = [p.strip() for p in s.split(",")]
    if len(parts) != 3:
        raise ValueError("RGB must be '#RRGGBB' or 'R,G,B'")
    r, g, b = (int(parts[0]), int(parts[1]), int(parts[2]))
    for v in (r, g, b):
        if v < 0 or v > 255:
            raise ValueError("RGB components must be 0..255")
    return (r, g, b)


def build_pattern_off(*, brightness_pct: int = 1) -> bytes:
    data = bytes(12)
    return struct.pack(PATTERN_FMT, 0, brightness_pct & 0xFF, 0, 0, 0, data)


def build_pattern_rainbow(*, speed: int = 5, saturation: int = 100, spread_x10: int = 10, brightness_pct: int = 25) -> bytes:
    data = bytearray(12)
    data[0] = speed & 0xFF
    data[1] = saturation & 0xFF
    data[2] = spread_x10 & 0xFF
    # pattern_type=1 (RAINBOW), flags=0, seed=0
    return struct.pack(PATTERN_FMT, 1, brightness_pct & 0xFF, 0, 0, 0, bytes(data))


def build_pattern_chase(
    *,
    speed: int = 30,
    tail_len: int = 6,
    gap_len: int = 4,
    trains: int = 2,
    fg=(255, 0, 0),
    bg=(0, 0, 0),
    direction: int = 0,
    fade_tail: bool = True,
    brightness_pct: int = 25,
) -> bytes:
    data = bytearray(12)
    data[0] = speed & 0xFF
    data[1] = max(1, tail_len) & 0xFF
    data[2] = max(0, gap_len) & 0xFF
    data[3] = max(1, trains) & 0xFF
    data[4] = fg[0] & 0xFF
    data[5] = fg[1] & 0xFF
    data[6] = fg[2] & 0xFF
    data[7] = bg[0] & 0xFF
    data[8] = bg[1] & 0xFF
    data[9] = bg[2] & 0xFF
    data[10] = direction & 0xFF  # 0=fwd,1=rev,2=bounce
    data[11] = 1 if fade_tail else 0
    return struct.pack(PATTERN_FMT, 2, brightness_pct & 0xFF, 0, 0, 0, bytes(data))


def build_pattern_breathing(
    *,
    speed: int = 6,
    color=(0, 128, 255),
    min_bri: int = 10,
    max_bri: int = 255,
    curve: int = 0,
    brightness_pct: int = 25,
) -> bytes:
    data = bytearray(12)
    data[0] = speed & 0xFF
    data[1] = color[0] & 0xFF
    data[2] = color[1] & 0xFF
    data[3] = color[2] & 0xFF
    data[4] = max(0, min(255, min_bri)) & 0xFF
    data[5] = max(0, min(255, max_bri)) & 0xFF
    data[6] = curve & 0xFF  # 0=sine,1=linear,2=ease_in_out
    return struct.pack(PATTERN_FMT, 3, brightness_pct & 0xFF, 0, 0, 0, bytes(data))


def build_pattern_sparkle(
    *,
    speed: int = 10,
    color=(255, 255, 255),
    density_pct: int = 20,
    fade_speed: int = 18,
    mode: int = 1,
    background=(0, 0, 0),
    brightness_pct: int = 25,
) -> bytes:
    data = bytearray(12)
    data[0] = speed & 0xFF
    data[1] = color[0] & 0xFF
    data[2] = color[1] & 0xFF
    data[3] = color[2] & 0xFF
    data[4] = max(0, min(100, density_pct)) & 0xFF
    data[5] = max(1, min(255, fade_speed)) & 0xFF
    data[6] = mode & 0xFF  # 0=fixed,1=random,2=rainbow
    data[7] = background[0] & 0xFF
    data[8] = background[1] & 0xFF
    data[9] = background[2] & 0xFF
    return struct.pack(PATTERN_FMT, 4, brightness_pct & 0xFF, 0, 0, 0, bytes(data))


def build_cue_prepare_payload(*, cue_id: int, fade_in_ms: int = 0, fade_out_ms: int = 0, pattern: bytes) -> bytes:
    if len(pattern) != PATTERN_LEN:
        raise ValueError("pattern must be 20 bytes")
    return struct.pack(CUE_PREPARE_FMT, cue_id & 0xFFFFFFFF, fade_in_ms & 0xFFFF, fade_out_ms & 0xFFFF) + pattern


def build_cue_fire_payload(*, cue_id: int) -> bytes:
    return struct.pack("<I", cue_id & 0xFFFFFFFF)


def build_time_resp_payload(*, req_msg_id: int, master_rx_show_ms: int, master_tx_show_ms: int) -> bytes:
    return struct.pack("<III", req_msg_id & 0xFFFFFFFF, master_rx_show_ms & 0xFFFFFFFF, master_tx_show_ms & 0xFFFFFFFF)


def discover(sock: socket.socket, *, group: str, port: int, timeout_s: float, repeat: int):
    msg_id = random.randint(1, 0xFFFFFFFF)
    pkt = build_header(0x20, epoch_id=0, msg_id=msg_id, sender_id=0, payload_len=0)  # PING
    for _ in range(repeat):
        sock.sendto(pkt, (group, port))
        time.sleep(0.02)

    deadline = time.time() + timeout_s
    nodes = {}
    while time.time() < deadline:
        try:
            data, addr = sock.recvfrom(2048)
        except socket.timeout:
            continue
        h = parse_header(data)
        if not h:
            continue
        if h["type"] != 0x21:
            continue
        pong = parse_pong(h["payload"])
        if not pong:
            continue
        node_id = h["sender_id"]
        nodes[node_id] = {"addr": addr, "pong": pong}
    return nodes


def pick_multicast_if_for_target(target_ip: str) -> str | None:
    # Best-effort: ask the OS which source IP it would use to reach the target.
    # This helps on multi-NIC hosts where multicast routing can be surprising.
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        try:
            s.connect((target_ip, 9))
            return s.getsockname()[0]
        finally:
            s.close()
    except OSError:
        return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--group", default=MCAST_GRP)
    ap.add_argument("--port", type=int, default=MCAST_PORT)
    ap.add_argument("--bind-ip", default=None, help="Bind the UDP socket to a specific local IPv4 (fixes multi-NIC multicast routing)")
    ap.add_argument("--timeout", type=float, default=2.0)
    ap.add_argument("--repeat", type=int, default=2)
    ap.add_argument("--epoch", type=lambda s: int(s, 0), default=None, help="epoch id (default: random u32)")
    ap.add_argument("--cue", type=lambda s: int(s, 0), default=42, help="cue id")
    ap.add_argument("--delay-ms", type=int, default=800, help="schedule fire delay (show-time ms)")
    ap.add_argument("--beacon-ms", type=int, default=1200, help="how long to send BEACONs (ms)")
    ap.add_argument("--node-id", type=lambda s: int(s, 0), default=None, help="target node_id (default: first discovered)")
    ap.add_argument("--no-time-resp", action="store_true", help="do not reply to TIME_REQ (node will stay BEACON-synced)")

    ap.add_argument("--pattern", choices=["off", "rainbow", "chase", "breathing", "sparkle"], default="rainbow")
    ap.add_argument("--brightness", type=int, default=25, help="pattern global brightness (1..100)")

    ap.add_argument("--rainbow-speed", type=int, default=5)
    ap.add_argument("--rainbow-saturation", type=int, default=100)
    ap.add_argument("--rainbow-spread-x10", type=int, default=10)

    ap.add_argument("--chase-speed", type=int, default=30)
    ap.add_argument("--chase-tail-len", type=int, default=6)
    ap.add_argument("--chase-gap-len", type=int, default=4)
    ap.add_argument("--chase-trains", type=int, default=2)
    ap.add_argument("--chase-fg", type=parse_rgb, default=(255, 0, 0))
    ap.add_argument("--chase-bg", type=parse_rgb, default=(0, 0, 0))
    ap.add_argument("--chase-dir", type=int, choices=[0, 1, 2], default=0, help="0=fwd,1=rev,2=bounce")
    ap.add_argument("--chase-fade-tail", action="store_true", default=True)
    ap.add_argument("--chase-no-fade-tail", action="store_true", default=False)

    ap.add_argument("--breathing-speed", type=int, default=6)
    ap.add_argument("--breathing-color", type=parse_rgb, default=(0, 128, 255))
    ap.add_argument("--breathing-min-bri", type=int, default=10)
    ap.add_argument("--breathing-max-bri", type=int, default=255)
    ap.add_argument("--breathing-curve", type=int, choices=[0, 1, 2], default=0, help="0=sine,1=linear,2=ease")

    ap.add_argument("--sparkle-speed", type=int, default=10)
    ap.add_argument("--sparkle-color", type=parse_rgb, default=(255, 255, 255))
    ap.add_argument("--sparkle-density", type=int, default=20)
    ap.add_argument("--sparkle-fade", type=int, default=18)
    ap.add_argument("--sparkle-mode", type=int, choices=[0, 1, 2], default=1, help="0=fixed,1=random,2=rainbow")
    ap.add_argument("--sparkle-bg", type=parse_rgb, default=(0, 0, 0))
    args = ap.parse_args()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 1)
    sock.settimeout(0.05)

    if args.bind_ip:
        sock.bind((args.bind_ip, 0))
        sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_IF, socket.inet_aton(args.bind_ip))

    # 1) Discover nodes
    nodes = discover(sock, group=args.group, port=args.port, timeout_s=args.timeout, repeat=args.repeat)
    if not nodes:
        print("no nodes discovered (no PONGs)")
        return 1
    for node_id, info in sorted(nodes.items()):
        pong = info["pong"]
        addr = info["addr"]
        print(f"found node_id={node_id} ip={addr[0]}:{addr[1]} name={pong['name']!r} rssi={pong['rssi_dbm']} epoch={pong['controller_epoch']}")

    target_node_id = args.node_id if args.node_id is not None else next(iter(nodes.keys()))
    if target_node_id not in nodes:
        print(f"node-id {target_node_id} not in discovered set")
        return 1

    if not args.bind_ip:
        target_ip = nodes[target_node_id]["addr"][0]
        mc_if = pick_multicast_if_for_target(target_ip)
        if mc_if:
            sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_IF, socket.inet_aton(mc_if))
            print(f"multicast egress: auto-selected {mc_if} (override with --bind-ip)")

    epoch = args.epoch if args.epoch is not None else random.randint(1, 0xFFFFFFFF)
    print(f"using epoch_id={epoch} target_node_id={target_node_id} cue_id={args.cue}")

    # 2) Send BEACONs (coarse time sync + controller addr discovery)
    msg_id = random.randint(1, 0xFFFFFFFF)
    start = time.monotonic()
    end = start + (args.beacon_ms / 1000.0)
    while time.monotonic() < end:
        show_ms = now_ms_u32()
        beacon = build_header(0x01, epoch_id=epoch, msg_id=msg_id, sender_id=0, execute_at_ms=show_ms, payload_len=0)
        sock.sendto(beacon, (args.group, args.port))
        msg_id = (msg_id + 1) & 0xFFFFFFFF

        # respond to TIME_REQs (optional)
        t_deadline = time.monotonic() + 0.02
        while time.monotonic() < t_deadline:
            try:
                data, addr = sock.recvfrom(2048)
            except socket.timeout:
                break
            h = parse_header(data)
            if not h:
                continue
            if h["type"] == 0x03 and not args.no_time_resp:  # TIME_REQ
                rx = now_ms_u32()
                tx = now_ms_u32()
                payload = build_time_resp_payload(req_msg_id=h["msg_id"], master_rx_show_ms=rx, master_tx_show_ms=tx)
                resp = build_header(0x04, epoch_id=epoch, msg_id=h["msg_id"], sender_id=0, payload_len=len(payload))
                sock.sendto(resp + payload, addr)
            # ignore others during beacon phase

        time.sleep(0.05)

    # 3) Send CUE_PREPARE targeted to the node, request ACK
    bri = max(1, min(100, int(args.brightness)))
    fade_tail = bool(args.chase_fade_tail) and not bool(args.chase_no_fade_tail)
    if args.pattern == "off":
        pattern = build_pattern_off(brightness_pct=bri)
    elif args.pattern == "rainbow":
        pattern = build_pattern_rainbow(speed=args.rainbow_speed, saturation=args.rainbow_saturation, spread_x10=args.rainbow_spread_x10, brightness_pct=bri)
    elif args.pattern == "chase":
        pattern = build_pattern_chase(
            speed=args.chase_speed,
            tail_len=args.chase_tail_len,
            gap_len=args.chase_gap_len,
            trains=args.chase_trains,
            fg=args.chase_fg,
            bg=args.chase_bg,
            direction=args.chase_dir,
            fade_tail=fade_tail,
            brightness_pct=bri,
        )
    elif args.pattern == "breathing":
        pattern = build_pattern_breathing(
            speed=args.breathing_speed,
            color=args.breathing_color,
            min_bri=args.breathing_min_bri,
            max_bri=args.breathing_max_bri,
            curve=args.breathing_curve,
            brightness_pct=bri,
        )
    elif args.pattern == "sparkle":
        pattern = build_pattern_sparkle(
            speed=args.sparkle_speed,
            color=args.sparkle_color,
            density_pct=args.sparkle_density,
            fade_speed=args.sparkle_fade,
            mode=args.sparkle_mode,
            background=args.sparkle_bg,
            brightness_pct=bri,
        )
    else:
        raise RuntimeError(f"unknown pattern: {args.pattern}")
    prep_payload = build_cue_prepare_payload(cue_id=args.cue, fade_in_ms=0, fade_out_ms=0, pattern=pattern)
    flags = 0x01 | 0x04  # TARGET_NODE + ACK_REQ
    prep_hdr = build_header(0x10, epoch_id=epoch, msg_id=msg_id, sender_id=0, flags=flags, target=target_node_id, payload_len=len(prep_payload))
    sock.sendto(prep_hdr + prep_payload, (args.group, args.port))
    prep_msg_id = msg_id
    msg_id = (msg_id + 1) & 0xFFFFFFFF
    print(f"sent CUE_PREPARE cue={args.cue} msg_id={prep_msg_id} (awaiting ACK)")

    # Wait briefly for ACK
    ack_deadline = time.time() + 1.0
    while time.time() < ack_deadline:
        try:
            data, addr = sock.recvfrom(2048)
        except socket.timeout:
            continue
        h = parse_header(data)
        if not h:
            continue
        if h["type"] == 0x22 and h["payload_len"] >= ACK_LEN:  # ACK
            ack_for, code, _reserved = struct.unpack(ACK_FMT, h["payload"][:ACK_LEN])
            if ack_for == prep_msg_id:
                print(f"got ACK from {addr[0]}:{addr[1]} node_id={h['sender_id']} code={code}")
                break

    # 4) Send CUE_FIRE scheduled in the near future
    fire_show_ms = (now_ms_u32() + (args.delay_ms & 0xFFFFFFFF)) & 0xFFFFFFFF
    fire_payload = build_cue_fire_payload(cue_id=args.cue)
    fire_hdr = build_header(0x11, epoch_id=epoch, msg_id=msg_id, sender_id=0, execute_at_ms=fire_show_ms, payload_len=len(fire_payload))
    sock.sendto(fire_hdr + fire_payload, (args.group, args.port))
    print(f"sent CUE_FIRE cue={args.cue} execute_at_ms={fire_show_ms} (delay={args.delay_ms}ms)")

    # 5) PING again after the fire window and show updated PONG state
    time.sleep(max(0.0, (args.delay_ms / 1000.0) + 0.4))
    nodes2 = discover(sock, group=args.group, port=args.port, timeout_s=1.5, repeat=1)
    if target_node_id in nodes2:
        pong = nodes2[target_node_id]["pong"]
        print(f"post-fire status: node_id={target_node_id} pat={pong['pattern_type']} bri={pong['brightness_pct']} cue={pong['active_cue_id']}")
    else:
        if args.bind_ip:
            print("post-fire: did not receive updated PONG from target node")
        else:
            print("post-fire: did not receive updated PONG from target node (try --bind-ip <your-host-lan-ip>)")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
