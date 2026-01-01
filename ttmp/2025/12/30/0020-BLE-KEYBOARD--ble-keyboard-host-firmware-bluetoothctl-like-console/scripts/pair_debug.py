#!/usr/bin/env python3
import argparse
import datetime as dt
import glob
import json
import os
import re
import subprocess
import sys
import time
import urllib.request
from pathlib import Path
from urllib.parse import urlparse


def find_repo_root(start: Path) -> Path:
    p = start.resolve()
    for _ in range(20):
        if (p / ".git").exists():
            return p
        if p.parent == p:
            break
        p = p.parent
    raise RuntimeError("Could not find repo root (missing .git while walking parents)")


REPO_ROOT = find_repo_root(Path(__file__))
DEFAULT_PROJECT = str(REPO_ROOT / "0020-cardputer-ble-keyboard-host")
DEFAULT_EXPORT_SH = "/home/manuel/esp/esp-idf-5.4.1/export.sh"
DEFAULT_PLZ_CONFIRM_BASE_URL = os.environ.get("PLZ_CONFIRM_BASE_URL", "http://localhost:3000")


def _run(cmd: list[str], *, input_bytes: bytes | None = None, timeout_s: int | None = None) -> subprocess.CompletedProcess:
    return subprocess.run(
        cmd,
        input=input_bytes,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=timeout_s,
        check=False,
    )


def ensure_plz_confirm_server(base_url: str) -> None:
    try:
        urllib.request.urlopen(base_url, timeout=0.5).read()
        return
    except Exception:
        pass

    parsed = urlparse(base_url)
    if parsed.scheme not in ("http", "https") or not parsed.netloc:
        raise RuntimeError(f"Invalid plz-confirm base URL: {base_url}")
    host = parsed.hostname or "127.0.0.1"
    port = parsed.port or (443 if parsed.scheme == "https" else 80)
    addr = f"{host}:{port}"

    proc = subprocess.Popen(
        ["plz-confirm", "serve", "--addr", addr],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        start_new_session=True,
    )
    deadline = time.time() + 5
    while time.time() < deadline:
        try:
            urllib.request.urlopen(base_url, timeout=0.5).read()
            return
        except Exception:
            time.sleep(0.2)

    proc.terminate()
    raise RuntimeError(f"plz-confirm server not reachable at {base_url} (failed to start)")


def plz_confirm(title: str, message: str, *, base_url: str, wait_timeout_s: int) -> bool:
    cmd = [
        "plz-confirm",
        "confirm",
        "--base-url",
        base_url,
        "--title",
        title,
        "--message",
        message,
        "--wait-timeout",
        str(wait_timeout_s),
        "--output",
        "json",
    ]
    proc = _run(cmd, timeout_s=wait_timeout_s + 30)
    if proc.returncode != 0:
        raise RuntimeError(proc.stdout.decode(errors="replace").strip() or "plz-confirm confirm failed")

    data = json.loads(proc.stdout.decode())
    if isinstance(data, list) and data and isinstance(data[0], dict):
        data = data[0]
    if isinstance(data, dict):
        if "approved" in data:
            return bool(data["approved"])
        if "Approved" in data:
            return bool(data["Approved"])
    return False


def plz_form_json(title: str, schema: dict, *, base_url: str, wait_timeout_s: int) -> dict:
    proc = _run(
        [
            "plz-confirm",
            "form",
            "--base-url",
            base_url,
            "--title",
            title,
            "--schema",
            "-",
            "--wait-timeout",
            str(wait_timeout_s),
            "--output",
            "json",
        ],
        input_bytes=json.dumps(schema).encode(),
        timeout_s=wait_timeout_s + 30,
    )
    if proc.returncode != 0:
        raise RuntimeError(proc.stdout.decode(errors="replace").strip() or "plz-confirm form failed")
    out = json.loads(proc.stdout.decode())
    if isinstance(out, list) and out and isinstance(out[0], dict):
        out = out[0]
    if not isinstance(out, dict):
        raise RuntimeError(f"Unexpected form output: {out!r}")
    data_json = out.get("data_json")
    if not isinstance(data_json, str) or not data_json:
        raise RuntimeError(f"Unexpected form output (missing data_json): {out!r}")
    data = json.loads(data_json)
    if not isinstance(data, dict):
        raise RuntimeError(f"Unexpected form data: {data!r}")
    return data


def _sh_quote(s: str) -> str:
    return "'" + s.replace("'", "'\"'\"'") + "'"


def run_idf(project: str, export_sh: str, args: list[str], *, timeout_s: int | None = None) -> None:
    idf_args = " ".join(_sh_quote(a) for a in (["idf.py", "-C", project] + args))
    bash_cmd = f"source {_sh_quote(export_sh)} >/dev/null 2>&1 && {idf_args}"
    proc = _run(["bash", "-lc", bash_cmd], timeout_s=timeout_s)
    sys.stdout.write(proc.stdout.decode(errors="replace"))
    if proc.returncode != 0:
        raise RuntimeError(f"idf.py failed (rc={proc.returncode})")


def pick_serial_port(preferred: str | None) -> str:
    if preferred:
        return preferred
    candidates = sorted(set(glob.glob("/dev/ttyACM*") + glob.glob("/dev/ttyUSB*")))
    if len(candidates) == 1:
        return candidates[0]
    if not candidates:
        raise RuntimeError("No serial ports found (/dev/ttyACM* or /dev/ttyUSB*)")
    return candidates[0]


class SerialSession:
    def __init__(self, port: str, baud: int, log_path: Path | None):
        import serial  # pyserial

        self.port = port
        self.baud = baud
        self.log_path = log_path
        self._ser = serial.Serial(port, baudrate=baud, timeout=0.2)
        self._log_fp = None
        if log_path is not None:
            log_path.parent.mkdir(parents=True, exist_ok=True)
            self._log_fp = log_path.open("ab")

    def close(self) -> None:
        if self._log_fp:
            self._log_fp.close()
            self._log_fp = None
        self._ser.close()

    def write_line(self, line: str) -> None:
        if not line.endswith("\n"):
            line += "\n"
        self._ser.write(line.encode())
        self._ser.flush()

    def read_available_text(self) -> str:
        b = self._ser.read(4096)
        if not b:
            return ""
        if self._log_fp:
            self._log_fp.write(b)
            self._log_fp.flush()
        return b.decode(errors="replace")

    def read_until(self, needle: str, timeout_s: int) -> str:
        buf = ""
        deadline = time.time() + timeout_s
        while time.time() < deadline:
            chunk = self.read_available_text()
            if chunk:
                sys.stdout.write(chunk)
                sys.stdout.flush()
                buf += chunk
                if needle in buf:
                    return buf
            else:
                time.sleep(0.05)
        return buf

    def read_collect(self, *, timeout_s: int, idle_s: float) -> str:
        buf = ""
        deadline = time.time() + timeout_s
        last_rx = time.time()
        while time.time() < deadline:
            chunk = self.read_available_text()
            if chunk:
                sys.stdout.write(chunk)
                sys.stdout.flush()
                buf += chunk
                last_rx = time.time()
                continue
            if buf and (time.time() - last_rx) >= idle_s:
                break
            time.sleep(0.05)
        return buf


DEVICE_LINE_RE = re.compile(r"^\\s*(\\d+)\\s+([0-9a-fA-F:]{17})\\s+(pub|rand)\\s+(-?\\d+)\\s+(\\d+)\\s+(.*)$")


def parse_devices_table(text: str) -> list[dict]:
    rows: list[dict] = []
    for line in text.splitlines():
        m = DEVICE_LINE_RE.match(line)
        if not m:
            continue
        rows.append(
            {
                "idx": int(m.group(1)),
                "addr": m.group(2).lower(),
                "type": m.group(3),
                "rssi": int(m.group(4)),
                "age_ms": int(m.group(5)),
                "name": m.group(6).strip(),
            }
        )
    return rows


def format_devices_for_prompt(devs: list[dict], limit: int = 30) -> str:
    lines = []
    for d in devs[:limit]:
        lines.append(f'{d["idx"]}: {d["addr"]} {d["type"]} rssi={d["rssi"]} name={d["name"]}')
    if len(devs) > limit:
        lines.append(f"... ({len(devs) - limit} more)")
    return "\n".join(lines) if lines else "(none)"


def main() -> int:
    ap = argparse.ArgumentParser(description="Flash + scan + pair BLE keyboard host with plz-confirm prompts.")
    ap.add_argument("--project", default=DEFAULT_PROJECT, help="ESP-IDF project dir")
    ap.add_argument("--export-sh", default=DEFAULT_EXPORT_SH, help="Path to ESP-IDF export.sh")
    ap.add_argument("--port", default=None, help="Serial port (default: auto-detect)")
    ap.add_argument("--baud", type=int, default=115200, help="Serial baud rate")
    ap.add_argument("--scan-seconds", type=int, default=15, help="Scan duration")
    ap.add_argument("--plz-confirm-base-url", default=DEFAULT_PLZ_CONFIRM_BASE_URL, help="plz-confirm base URL")
    ap.add_argument("--plz-wait-timeout", type=int, default=300, help="plz-confirm wait timeout seconds")
    ap.add_argument("--log-file", default=None, help="Optional serial log path")
    args = ap.parse_args()

    ensure_plz_confirm_server(args.plz_confirm_base_url)

    port = pick_serial_port(args.port)
    approved = plz_confirm(
        "Ready to run BLE pairing flow?",
        f"Please confirm:\n\n- Your keyboard/device is in pairing mode\n- The ESP32-S3 is connected on {port}\n\nI will fullclean+build+flash, then scan and attempt to pair.",
        base_url=args.plz_confirm_base_url,
        wait_timeout_s=args.plz_wait_timeout,
    )
    if not approved:
        print("Cancelled.")
        return 2

    run_idf(args.project, args.export_sh, ["fullclean"], timeout_s=600)
    run_idf(args.project, args.export_sh, ["build"], timeout_s=600)
    run_idf(args.project, args.export_sh, ["-p", port, "flash"], timeout_s=600)

    log_path = Path(args.log_file) if args.log_file else None
    if log_path is None:
        ts = dt.datetime.now().strftime("%Y%m%d_%H%M%S")
        log_path = Path(__file__).resolve().parent / "logs" / f"pair_debug_{ts}.log"

    session = SerialSession(port=port, baud=args.baud, log_path=log_path)
    try:
        session.write_line("")
        session.write_line("")
        session.read_until("ble> ", timeout_s=20)
        session.write_line(f"scan on {args.scan_seconds}")
        time.sleep(max(1, args.scan_seconds + 2))
        session.write_line("devices")
        devices_out = session.read_collect(timeout_s=8, idle_s=0.4)
        devs = parse_devices_table(devices_out)

        if not devs:
            approved = plz_confirm(
                "No devices found",
                "No devices parsed from `devices` output. Scan again for 30s?",
                base_url=args.plz_confirm_base_url,
                wait_timeout_s=args.plz_wait_timeout,
            )
            if not approved:
                return 1
            session.write_line("scan on 30")
            time.sleep(32)
            session.write_line("devices")
            devices_out = session.read_collect(timeout_s=8, idle_s=0.4)
            devs = parse_devices_table(devices_out)

        if not devs:
            print("No devices discovered; aborting.")
            return 1

        prompt = format_devices_for_prompt(devs)
        form = plz_form_json(
            "Select device index",
            {
                "type": "object",
                "required": ["index"],
                "properties": {
                    "index": {
                        "type": "integer",
                        "minimum": 0,
                        "description": "Pick the device index to pair:\n\n" + prompt,
                    }
                },
            },
            base_url=args.plz_confirm_base_url,
            wait_timeout_s=args.plz_wait_timeout,
        )
        index = int(form["index"])
        chosen = next((d for d in devs if d["idx"] == index), None)
        if chosen is None:
            raise RuntimeError(f"Selected index {index} not present in parsed device list")

        addr = chosen["addr"]
        session.write_line(f"pair {index}")

        auth_deadline = time.time() + 90
        saw_auth = False
        while time.time() < auth_deadline:
            chunk = session.read_available_text()
            if not chunk:
                time.sleep(0.05)
                continue
            sys.stdout.write(chunk)
            sys.stdout.flush()

            if "passkey req:" in chunk:
                resp = plz_form_json(
                    "Passkey required",
                    {
                        "type": "object",
                        "required": ["passkey"],
                        "properties": {
                            "passkey": {
                                "type": "integer",
                                "minimum": 0,
                                "maximum": 999999,
                                "description": f"Enter the 6-digit passkey for {addr}.",
                            }
                        },
                    },
                    base_url=args.plz_confirm_base_url,
                    wait_timeout_s=args.plz_wait_timeout,
                )
                passkey = int(resp["passkey"])
                session.write_line(f"passkey {addr} {passkey:06d}")

            if "numeric compare req:" in chunk:
                approved = plz_confirm(
                    "Numeric comparison",
                    f"Accept numeric comparison for {addr}?\n\n(If you see a number on the keyboard/device, it should match the log.)",
                    base_url=args.plz_confirm_base_url,
                    wait_timeout_s=args.plz_wait_timeout,
                )
                session.write_line(f"confirm {addr} {'yes' if approved else 'no'}")

            if "auth complete: success" in chunk:
                saw_auth = True
                break
            if "auth complete: fail_reason=" in chunk:
                saw_auth = True
                break

        session.write_line("bonds")
        session.read_until("ble> ", timeout_s=5)

        plz_confirm(
            "Pairing flow finished",
            f"Finished. Serial log saved to:\n\n{log_path}\n\nDid pairing behave as expected (no 'not find peer_bdaddr' errors)?",
            base_url=args.plz_confirm_base_url,
            wait_timeout_s=args.plz_wait_timeout,
        )

        if not saw_auth:
            print("Timed out waiting for auth complete event.")
            return 1
        return 0
    finally:
        session.close()


if __name__ == "__main__":
    raise SystemExit(main())

