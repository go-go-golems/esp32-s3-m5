#!/usr/bin/env python3

import argparse
import asyncio
import struct
import sys
from dataclasses import dataclass
from typing import Optional

from bleak import BleakClient, BleakScanner


SVC_UUID_16 = 0xFFF0
TEMP_UUID_16 = 0xFFF1
CTRL_UUID_16 = 0xFFF2

# Bleak normalizes UUIDs as full 128-bit strings for 16-bit UUIDs.
SVC_UUID = f"0000{SVC_UUID_16:04x}-0000-1000-8000-00805f9b34fb"
TEMP_UUID = f"0000{TEMP_UUID_16:04x}-0000-1000-8000-00805f9b34fb"
CTRL_UUID = f"0000{CTRL_UUID_16:04x}-0000-1000-8000-00805f9b34fb"


def decode_temp_centi(payload: bytes) -> float:
    if payload is None or len(payload) < 2:
        raise ValueError(f"temp payload too short: {payload!r}")
    (v,) = struct.unpack("<h", payload[:2])
    return float(v) / 100.0


@dataclass
class Target:
    address: str
    name: str


async def scan_for_target(name_substr: str, timeout: float) -> Optional[Target]:
    devices = await BleakScanner.discover(timeout=timeout)
    for d in devices:
        name = d.name or ""
        if name_substr.lower() in name.lower():
            return Target(address=d.address, name=name)
    return None


async def do_scan(args: argparse.Namespace) -> int:
    devices = await BleakScanner.discover(timeout=args.timeout)
    for d in devices:
        print(f"{d.address}\t{d.name}")
    return 0


async def connect_once(address: str, set_period_ms: Optional[int], do_read: bool, do_notify: bool, duration: float) -> int:
    async with BleakClient(address) as client:
        print(f"connected: {address} (mtu={client.mtu_size})")

        if set_period_ms is not None:
            payload = struct.pack("<H", int(set_period_ms) & 0xFFFF)
            await client.write_gatt_char(CTRL_UUID, payload, response=True)
            print(f"wrote control: period_ms={set_period_ms}")

        if do_read:
            v = await client.read_gatt_char(TEMP_UUID)
            t = decode_temp_centi(bytes(v))
            print(f"read temp: {t:.2f} C (raw={bytes(v).hex()})")

        if do_notify:
            def on_notify(_handle: int, data: bytearray) -> None:
                try:
                    t = decode_temp_centi(bytes(data))
                    print(f"notify temp: {t:.2f} C (raw={bytes(data).hex()})")
                except Exception as e:
                    print(f"notify decode error: {e} raw={bytes(data).hex()}", file=sys.stderr)

            await client.start_notify(TEMP_UUID, on_notify)
            print(f"subscribed for {duration:.1f}s ...")
            await asyncio.sleep(duration)
            await client.stop_notify(TEMP_UUID)
            print("unsubscribed")

    return 0


async def stress_reconnect(args: argparse.Namespace) -> int:
    if args.address is None:
        t = await scan_for_target(args.name, timeout=args.timeout)
        if t is None:
            print(f"target not found (name contains {args.name!r})", file=sys.stderr)
            return 2
        address = t.address
        print(f"selected: {t.address} {t.name}")
    else:
        address = args.address

    for i in range(args.reconnects):
        print(f"reconnect {i+1}/{args.reconnects}")
        rc = await connect_once(
            address=address,
            set_period_ms=args.set_period_ms,
            do_read=args.read,
            do_notify=args.notify,
            duration=args.duration,
        )
        if rc != 0:
            return rc
        await asyncio.sleep(args.pause)
    return 0


def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(description="BLE client for Cardputer temp logger (tutorial 0019).")
    p.add_argument("--timeout", type=float, default=5.0, help="scan timeout seconds")

    sub = p.add_subparsers(dest="cmd", required=True)

    scan = sub.add_parser("scan", help="scan and print devices")
    scan.set_defaults(func=do_scan)

    run = sub.add_parser("run", help="connect once (scan-by-name unless --address)")
    run.add_argument("--name", default="CP_TEMP_LOGGER", help="match substring for scan selection")
    run.add_argument("--address", default=None, help="direct BLE address (skip scan)")
    run.add_argument("--read", action="store_true", help="read temperature once")
    run.add_argument("--notify", action="store_true", help="subscribe to notifications")
    run.add_argument("--duration", type=float, default=10.0, help="notify duration seconds")
    run.add_argument("--set-period-ms", type=int, default=None, help="write control period_ms (0 disables)")

    stress = sub.add_parser("stress", help="repeat connect/read/notify N times")
    stress.add_argument("--name", default="CP_TEMP_LOGGER", help="match substring for scan selection")
    stress.add_argument("--address", default=None, help="direct BLE address (skip scan)")
    stress.add_argument("--reconnects", type=int, default=25)
    stress.add_argument("--pause", type=float, default=0.5)
    stress.add_argument("--read", action="store_true")
    stress.add_argument("--notify", action="store_true")
    stress.add_argument("--duration", type=float, default=2.0)
    stress.add_argument("--set-period-ms", type=int, default=None)
    stress.set_defaults(func=stress_reconnect)

    return p


async def main_async(argv: list[str]) -> int:
    p = build_parser()
    args = p.parse_args(argv)

    if args.cmd == "run":
        if args.address is None:
            t = await scan_for_target(args.name, timeout=args.timeout)
            if t is None:
                print(f"target not found (name contains {args.name!r})", file=sys.stderr)
                return 2
            address = t.address
            print(f"selected: {t.address} {t.name}")
        else:
            address = args.address

        return await connect_once(
            address=address,
            set_period_ms=args.set_period_ms,
            do_read=args.read,
            do_notify=args.notify,
            duration=args.duration,
        )

    return await args.func(args)


def main() -> int:
    try:
        return asyncio.run(main_async(sys.argv[1:]))
    except KeyboardInterrupt:
        return 130


if __name__ == "__main__":
    raise SystemExit(main())


