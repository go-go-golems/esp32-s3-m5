#!/usr/bin/env python3
"""Capture Printalyzer and PaperS3 serial lines on one host timebase.

Passive capture sends no input.  Read-only inventory is explicit.  Continuous
Printalyzer raw-sensor mode is separately gated because it enters remote mode,
changes transient sensor/light state, and does not produce calibrated density.
"""

from __future__ import annotations

import argparse
import fcntl
import json
import math
import os
import queue
import re
import select
import signal
import socket
import struct
import threading
import time
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Callable

import serial

TICKET = Path(__file__).resolve().parents[1]
DEFAULT_DENS_PORT = Path(
    "/dev/serial/by-id/usb-Dektronics_Printalyzer_Densitometer_"
    "323147103439323344002900-if00"
)
DEFAULT_FIRMWARE_PORT = Path(
    "/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:16:17:DC-if00"
)
READ_ONLY_DENS_COMMANDS = (
    "GS V",
    "GS B",
    "GS DEV",
    "GS UID",
    "GS ISEN",
    "GM REFL",
    "GC LIGHT",
    "GC GAIN",
    "GC SLOPE",
    "GC REFL",
    "GC TRAN",
)
DENSITY_RE = re.compile(r"^([RT])([+-])(\d+(?:\.\d+)?)D(?:,(.*))?$")
RAW_SENSOR_RE = re.compile(r"^GD S,(\d+),(\d+),(\d+),(\d+)$")
ESP_LOG_RE = re.compile(r"^[A-Z] \((\d+)\)")
TSL2591_LUX_DF = 408.0
TSL2591_LUX_GA = 1.16
TSL2591_ANALOG_SATURATION = 37888
TSL2591_DIGITAL_SATURATION = 65535
REFLECTION_MAX_D = 2.50


@dataclass(frozen=True)
class ReflectionCalibration:
    reflection_light_duty: int
    gain_ch0: tuple[float, float, float, float]
    gain_ch1: tuple[float, float, float, float]
    slope: tuple[float, float, float]
    lo_density: float
    lo_value: float
    hi_density: float
    hi_value: float

    def derive(
        self, channel_0: int, gain_index: int, integration_index: int, light_duty: int
    ) -> dict[str, Any]:
        integration_ms = (integration_index + 1) * 100
        saturation_limit = (
            TSL2591_ANALOG_SATURATION
            if integration_index == 0
            else TSL2591_DIGITAL_SATURATION
        )
        result: dict[str, Any] = {
            "integration_ms": integration_ms,
            "saturation_limit": saturation_limit,
            "saturated": channel_0 >= saturation_limit,
            "calibration_light_duty": self.reflection_light_duty,
            "light_duty_matches_calibration": light_duty == self.reflection_light_duty,
        }
        if not 0 <= gain_index < len(self.gain_ch0) or channel_0 <= 0:
            result["density_estimate_valid"] = False
            return result
        cpl = (integration_ms * self.gain_ch0[gain_index]) / (
            TSL2591_LUX_GA * TSL2591_LUX_DF
        )
        basic = channel_0 / cpl
        result["channel_0_basic"] = basic
        if (
            basic <= 0
            or result["saturated"]
            or light_duty != self.reflection_light_duty
        ):
            result["density_estimate_valid"] = False
            return result
        log_basic = math.log10(basic)
        b0, b1, b2 = self.slope
        corrected = 10 ** (b0 + b1 * log_basic + b2 * (log_basic**2))
        result["channel_0_corrected"] = corrected
        if corrected <= 0 or self.lo_value <= 0 or self.hi_value <= 0:
            result["density_estimate_valid"] = False
            return result
        calibration_slope = (self.hi_density - self.lo_density) / (
            math.log10(self.hi_value) - math.log10(self.lo_value)
        )
        density = (
            calibration_slope * (math.log10(corrected) - math.log10(self.lo_value))
            + self.lo_density
        )
        result["density_unclamped"] = density
        result["density_estimate"] = min(REFLECTION_MAX_D, max(0.0, density))
        result["density_estimate_valid"] = True
        result["estimate_kind"] = "single-sample-host-reproduction-of-v1.1.0-formula"
        return result


def decode_f32(value: str) -> float:
    if not re.fullmatch(r"[0-9A-Fa-f]{8}", value):
        raise ValueError(f"invalid encoded float: {value!r}")
    return struct.unpack(">f", bytes.fromhex(value))[0]


def utcstamp() -> str:
    return datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")


def format_utc_ns(epoch_ns: int) -> str:
    seconds, nanos = divmod(epoch_ns, 1_000_000_000)
    base = datetime.fromtimestamp(seconds, timezone.utc).strftime("%Y-%m-%dT%H:%M:%S")
    return f"{base}.{nanos:09d}Z"


def parse_line(source: str, line: str) -> dict[str, Any]:
    parsed: dict[str, Any] = {}
    if source == "densitometer":
        match = DENSITY_RE.match(line)
        if match:
            value = float(match.group(3))
            if match.group(2) == "-":
                value = -value
            parsed = {
                "kind": "density",
                "mode": "reflection" if match.group(1) == "R" else "transmission",
                "display_density": value,
                "extended_fields": match.group(4).split(",") if match.group(4) else [],
            }
        else:
            match = RAW_SENSOR_RE.match(line)
            if match:
                parsed = {
                    "kind": "raw_sensor",
                    "channel_0": int(match.group(1)),
                    "channel_1": int(match.group(2)),
                    "gain_index": int(match.group(3)),
                    "integration_index": int(match.group(4)),
                }
            elif re.match(r"^[GSI][SMCD] ", line):
                parsed = {"kind": "command_response"}
    elif source == "firmware":
        if line.startswith("{"):
            try:
                parsed = {"kind": "json", "value": json.loads(line)}
            except json.JSONDecodeError:
                parsed = {"kind": "invalid_json"}
        else:
            match = ESP_LOG_RE.match(line)
            if match:
                parsed = {"kind": "esp_log", "device_log_ms": int(match.group(1))}
            elif "FACTORY_TRACE_DUMP_" in line:
                parsed = {"kind": "factory_trace_marker"}
    return parsed


class EventSink:
    def __init__(self, path: Path) -> None:
        self.path = path
        self.path.parent.mkdir(parents=True, exist_ok=True)
        self.file = path.open("x", encoding="utf-8")
        self.anchor_monotonic_ns = time.monotonic_ns()
        self.anchor_realtime_ns = time.time_ns()
        self.lock = threading.Lock()
        self.sequence = 0

    def timestamp(self, monotonic_ns: int | None = None) -> dict[str, Any]:
        mono = time.monotonic_ns() if monotonic_ns is None else monotonic_ns
        realtime = self.anchor_realtime_ns + (mono - self.anchor_monotonic_ns)
        return {
            "host_monotonic_ns": mono,
            "host_utc_ns": realtime,
            "host_utc": format_utc_ns(realtime),
        }

    def emit(self, source: str, event: str, **fields: Any) -> None:
        with self.lock:
            record = {
                "schema": "esp50.synchronized-serial.v1",
                "sequence": self.sequence,
                "source": source,
                "event": event,
                **self.timestamp(fields.pop("_monotonic_ns", None)),
                **fields,
            }
            self.sequence += 1
            self.file.write(
                json.dumps(record, sort_keys=True, separators=(",", ":")) + "\n"
            )
            self.file.flush()

    def close(self) -> None:
        with self.lock:
            self.file.flush()
            os.fsync(self.file.fileno())
            self.file.close()


class SerialSource:
    def __init__(self, name: str, port: Path, sink: EventSink) -> None:
        self.name = name
        self.port = port
        self.real_port = port.resolve()
        self.sink = sink
        self.stop_event = threading.Event()
        self.responses: queue.Queue[tuple[int, str]] = queue.Queue()
        self.line_sequence = 0
        self.reflection_calibration: ReflectionCalibration | None = None
        self.raw_light_duty: int | None = None
        self.thread: threading.Thread | None = None
        self.serial: serial.Serial | None = None
        self.read_fd: int | None = None
        if self.name == "firmware":
            self._open_firmware_read_only()
        else:
            self.serial = self._open_densitometer()

    def _open_firmware_read_only(self) -> None:
        # Do not use pyserial here: opening ESP32-S3 USB Serial/JTAG while
        # applying DTR/RTS state can reset the chip or select ROM download
        # mode.  A read-only, non-controlling fd observes bytes without issuing
        # modem-control ioctls.  Firmware TX is deliberately unsupported.
        fd = os.open(self.port, os.O_RDONLY | os.O_NOCTTY | os.O_NONBLOCK)
        fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
        self.read_fd = fd
        self.sink.emit(
            self.name,
            "source_open",
            port=str(self.port),
            real_port=str(self.real_port),
            open_mode="read-only-os",
            modem_control_issued=False,
        )

    def _open_densitometer(self) -> serial.Serial:
        device = serial.Serial()
        device.port = str(self.port)
        device.baudrate = 115200
        device.bytesize = serial.EIGHTBITS
        device.parity = serial.PARITY_NONE
        device.stopbits = serial.STOPBITS_ONE
        device.timeout = 0.05
        device.write_timeout = 1.0
        device.exclusive = True
        # Printalyzer firmware only marks USB CDC as host-connected while DTR
        # is asserted.  It does not use DTR/RTS as a reset/boot strap.
        device.dtr = True
        device.rts = False
        device.open()
        fcntl.flock(device.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
        self.sink.emit(
            self.name,
            "source_open",
            port=str(self.port),
            real_port=str(self.real_port),
            open_mode="pyserial",
            baud=115200,
            dtr=True,
            rts=False,
        )
        return device

    def start(self) -> None:
        self.thread = threading.Thread(
            target=self._reader, name=f"serial-{self.name}", daemon=True
        )
        self.thread.start()

    def _reader(self) -> None:
        pending = bytearray()
        first_byte_ns: int | None = None
        try:
            while not self.stop_event.is_set():
                if self.serial is not None:
                    chunk = self.serial.read(4096)
                elif self.read_fd is not None:
                    readable, _, _ = select.select([self.read_fd], [], [], 0.05)
                    if not readable:
                        continue
                    try:
                        chunk = os.read(self.read_fd, 4096)
                    except BlockingIOError:
                        continue
                else:
                    raise RuntimeError("serial source has no open input")
                read_ns = time.monotonic_ns()
                if not chunk:
                    continue
                if not pending:
                    first_byte_ns = read_ns
                pending.extend(chunk)
                while b"\n" in pending:
                    raw_line, remainder = pending.split(b"\n", 1)
                    pending = bytearray(remainder)
                    line = raw_line.rstrip(b"\r\x00").decode("utf-8", "replace")
                    first_ns = first_byte_ns if first_byte_ns is not None else read_ns
                    event_fields: dict[str, Any] = {
                        "line_sequence": self.line_sequence,
                        "first_byte_monotonic_ns": first_ns,
                        "last_byte_monotonic_ns": read_ns,
                        "line": line,
                        "raw_hex": bytes(raw_line).hex(),
                    }
                    parsed = parse_line(self.name, line)
                    if (
                        parsed.get("kind") == "raw_sensor"
                        and self.reflection_calibration is not None
                        and self.raw_light_duty is not None
                    ):
                        parsed["derived"] = self.reflection_calibration.derive(
                            parsed["channel_0"],
                            parsed["gain_index"],
                            parsed["integration_index"],
                            self.raw_light_duty,
                        )
                    if parsed:
                        event_fields["parsed"] = parsed
                    self.sink.emit(
                        self.name, "rx_line", _monotonic_ns=read_ns, **event_fields
                    )
                    self.responses.put((read_ns, line))
                    self.line_sequence += 1
                    first_byte_ns = read_ns if pending else None
        except Exception as exc:  # captured in evidence before propagation to main
            self.sink.emit(self.name, "reader_error", error=repr(exc))
            self.stop_event.set()
        finally:
            if pending:
                read_ns = time.monotonic_ns()
                self.sink.emit(
                    self.name,
                    "rx_partial",
                    _monotonic_ns=read_ns,
                    first_byte_monotonic_ns=first_byte_ns,
                    last_byte_monotonic_ns=read_ns,
                    raw_hex=bytes(pending).hex(),
                    text=bytes(pending).decode("utf-8", "replace"),
                )

    def send(self, line: str) -> int:
        if self.serial is None:
            raise RuntimeError(f"source {self.name!r} is read-only")
        sent_ns = time.monotonic_ns()
        payload = line.encode("ascii") + b"\r\n"
        self.serial.write(payload)
        self.serial.flush()
        self.sink.emit(
            self.name,
            "tx_line",
            _monotonic_ns=sent_ns,
            line=line,
            raw_hex=payload.hex(),
        )
        return sent_ns

    def send_and_wait(
        self, line: str, predicate: Callable[[str], bool], timeout: float = 3.0
    ) -> str:
        sent_ns = self.send(line)
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            try:
                remaining = max(0.001, deadline - time.monotonic())
                received_ns, response = self.responses.get(timeout=min(0.1, remaining))
            except queue.Empty:
                continue
            if received_ns >= sent_ns and predicate(response):
                if response.endswith(",ERR") or response.endswith(",NAK"):
                    raise RuntimeError(f"command rejected: {line!r}: {response!r}")
                return response
        raise TimeoutError(f"timed out waiting for response to {line!r}")

    def close(self) -> None:
        self.stop_event.set()
        if self.thread is not None:
            self.thread.join(timeout=2)
        if self.serial is not None and self.serial.is_open:
            try:
                fcntl.flock(self.serial.fileno(), fcntl.LOCK_UN)
            finally:
                self.serial.close()
        if self.read_fd is not None:
            try:
                fcntl.flock(self.read_fd, fcntl.LOCK_UN)
            finally:
                os.close(self.read_fd)
                self.read_fd = None
        self.sink.emit(self.name, "source_close", port=str(self.port))


def command_prefix(command: str) -> str:
    return command.split(",", 1)[0] + ","


def run_inventory(dens: SerialSource) -> None:
    dens.sink.emit(
        "session", "inventory_begin", command_count=len(READ_ONLY_DENS_COMMANDS)
    )
    for command in READ_ONLY_DENS_COMMANDS:
        prefix = command_prefix(command)
        dens.send_and_wait(command, lambda line, prefix=prefix: line.startswith(prefix))
    dens.sink.emit("session", "inventory_end", result="ok")


def response_args(response: str) -> list[str]:
    try:
        return response.split(",", 1)[1].split(",")
    except IndexError as exc:
        raise ValueError(f"response has no arguments: {response!r}") from exc


def capture_reflection_calibration(dens: SerialSource) -> ReflectionCalibration:
    light = response_args(
        dens.send_and_wait("GC LIGHT", lambda line: line.startswith("GC LIGHT,"))
    )
    gain = [
        decode_f32(value)
        for value in response_args(
            dens.send_and_wait("GC GAIN", lambda line: line.startswith("GC GAIN,"))
        )
    ]
    slope = [
        decode_f32(value)
        for value in response_args(
            dens.send_and_wait("GC SLOPE", lambda line: line.startswith("GC SLOPE,"))
        )
    ]
    reflection = [
        decode_f32(value)
        for value in response_args(
            dens.send_and_wait("GC REFL", lambda line: line.startswith("GC REFL,"))
        )
    ]
    if len(light) != 2 or len(gain) != 8 or len(slope) != 3 or len(reflection) != 4:
        raise ValueError("unexpected Printalyzer calibration response shape")
    calibration = ReflectionCalibration(
        reflection_light_duty=int(light[0]),
        gain_ch0=(gain[0], gain[2], gain[4], gain[6]),
        gain_ch1=(gain[1], gain[3], gain[5], gain[7]),
        slope=(slope[0], slope[1], slope[2]),
        lo_density=reflection[0],
        lo_value=reflection[1],
        hi_density=reflection[2],
        hi_value=reflection[3],
    )
    dens.reflection_calibration = calibration
    dens.sink.emit(
        "session",
        "reflection_calibration_snapshot",
        reflection_light_duty=calibration.reflection_light_duty,
        gain_ch0=calibration.gain_ch0,
        gain_ch1=calibration.gain_ch1,
        slope=calibration.slope,
        lo_density=calibration.lo_density,
        lo_value=calibration.lo_value,
        hi_density=calibration.hi_density,
        hi_value=calibration.hi_value,
        formula_source="Printalyzer v1.1.0 firmware formula",
    )
    return calibration


def enter_raw_stream(
    dens: SerialSource, gain: int, integration: int, duty: int
) -> None:
    calibration = capture_reflection_calibration(dens)
    dens.raw_light_duty = duty
    dens.send_and_wait("IS REMOTE,1", lambda line: line == "IS REMOTE,1", timeout=5)
    dens.send_and_wait(
        f"SD S,CFG,{gain},{integration}",
        lambda line: line.startswith("SD S,"),
    )
    dens.send_and_wait(f"SD LR,{duty}", lambda line: line.startswith("SD LR,"))
    dens.send_and_wait("ID S,START", lambda line: line.startswith("ID S,"))
    dens.sink.emit(
        "session",
        "densitometer_raw_stream_begin",
        gain_index=gain,
        integration_index=integration,
        reflection_light_duty=duty,
        calibrated_density=False,
        host_density_estimate_enabled=duty == calibration.reflection_light_duty,
        calibration_light_duty=calibration.reflection_light_duty,
    )


def leave_raw_stream(dens: SerialSource) -> None:
    errors: list[str] = []
    for command, predicate in (
        ("ID S,STOP", lambda line: line.startswith("ID S,")),
        ("SD LR,0", lambda line: line.startswith("SD LR,")),
        ("IS REMOTE,0", lambda line: line == "IS REMOTE,0"),
    ):
        try:
            dens.send_and_wait(command, predicate, timeout=5)
        except Exception as exc:
            errors.append(f"{command}: {exc}")
    dens.sink.emit("session", "densitometer_raw_stream_end", cleanup_errors=errors)
    if errors:
        raise RuntimeError("raw stream cleanup incomplete: " + "; ".join(errors))


def owner_pids(port: Path) -> list[int]:
    real = port.resolve()
    result: set[int] = set()
    for pid_dir in Path("/proc").glob("[0-9]*"):
        fd_dir = pid_dir / "fd"
        try:
            for fd in fd_dir.iterdir():
                try:
                    target = fd.resolve()
                except OSError:
                    continue
                if target == real:
                    result.add(int(pid_dir.name))
        except (FileNotFoundError, PermissionError):
            continue
    return sorted(result)


def validate_port(label: str, port: Path) -> None:
    if not port.exists():
        raise SystemExit(f"missing {label} port: {port}")
    owners = owner_pids(port)
    if owners:
        raise SystemExit(f"{label} port already owned by PID(s): {owners}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--check", action="store_true")
    mode.add_argument("--execute", action="store_true")
    parser.add_argument("--dens-port", type=Path, default=DEFAULT_DENS_PORT)
    parser.add_argument("--firmware-port", type=Path, default=DEFAULT_FIRMWARE_PORT)
    parser.add_argument("--no-densitometer", action="store_true")
    parser.add_argument("--no-firmware", action="store_true")
    parser.add_argument(
        "--duration",
        type=float,
        default=10.0,
        help="seconds; 0 means until interrupted",
    )
    parser.add_argument("--output", type=Path)
    parser.add_argument(
        "--dens-inventory",
        action="store_true",
        help="send only the fixed read-only query allowlist",
    )
    parser.add_argument(
        "--dens-raw-stream",
        action="store_true",
        help="enter transient diagnostic raw streaming mode",
    )
    parser.add_argument(
        "--gain",
        type=int,
        choices=range(4),
        default=2,
        help="TSL2591 gain index; 2=HIGH is preregistered for reference characterization",
    )
    parser.add_argument("--integration", type=int, choices=range(6), default=0)
    parser.add_argument(
        "--light-duty",
        type=int,
        choices=range(1, 129),
        default=128,
        help="reflection LED duty; 128 matches this instrument's captured calibration",
    )
    parser.add_argument("--confirm", default="")
    args = parser.parse_args()
    if args.no_densitometer and args.no_firmware:
        parser.error("at least one source must be enabled")
    if (args.dens_inventory or args.dens_raw_stream) and args.no_densitometer:
        parser.error("densitometer actions require the densitometer source")
    if args.dens_raw_stream and args.confirm != "ENABLE-DENS-RAW-STREAM":
        parser.error("raw stream requires --confirm ENABLE-DENS-RAW-STREAM")
    if args.duration < 0:
        parser.error("duration must be nonnegative")
    return args


def main() -> None:
    args = parse_args()
    ports: list[tuple[str, Path]] = []
    if not args.no_densitometer:
        ports.append(("densitometer", args.dens_port))
    if not args.no_firmware:
        ports.append(("firmware", args.firmware_port))
    for label, port in ports:
        validate_port(label, port)

    output = (
        args.output
        or TICKET / "scripts/output" / f"29-synchronized-serial-{utcstamp()}.jsonl"
    )
    print(f"mode={'check' if args.check else 'execute'}")
    for label, port in ports:
        print(f"{label}_port={port}")
        print(f"{label}_real_port={port.resolve()}")
    print(f"output={output}")
    print(
        f"serial_input_planned={'yes' if args.dens_inventory or args.dens_raw_stream else 'no'}"
    )
    print(f"raw_stream_planned={'yes' if args.dens_raw_stream else 'no'}")
    if args.check:
        print("hardware_modified=no")
        return

    if output.exists():
        raise SystemExit(f"refusing to replace output: {output}")
    sink = EventSink(output)
    sources: dict[str, SerialSource] = {}
    raw_active = False
    interrupted = False

    def request_stop(_signum: int, _frame: Any) -> None:
        nonlocal interrupted
        interrupted = True

    signal.signal(signal.SIGINT, request_stop)
    signal.signal(signal.SIGTERM, request_stop)
    sink.emit(
        "session",
        "capture_begin",
        pid=os.getpid(),
        hostname=socket.gethostname(),
        anchor_monotonic_ns=sink.anchor_monotonic_ns,
        anchor_realtime_ns=sink.anchor_realtime_ns,
        duration_seconds=args.duration,
        densitometer_inventory=args.dens_inventory,
        densitometer_raw_stream=args.dens_raw_stream,
    )
    failure: BaseException | None = None
    try:
        for label, port in ports:
            sources[label] = SerialSource(label, port, sink)
        for source in sources.values():
            source.start()
        time.sleep(0.15)

        dens = sources.get("densitometer")
        if args.dens_inventory and dens is not None:
            run_inventory(dens)
        if args.dens_raw_stream and dens is not None:
            # Mark cleanup as required before the first state-changing command;
            # partial entry failures must still attempt STOP/light-off/exit.
            raw_active = True
            enter_raw_stream(dens, args.gain, args.integration, args.light_duty)

        deadline = None if args.duration == 0 else time.monotonic() + args.duration
        while not interrupted and (deadline is None or time.monotonic() < deadline):
            if any(source.stop_event.is_set() for source in sources.values()):
                raise RuntimeError("a serial reader stopped unexpectedly")
            time.sleep(0.05)
    except BaseException as exc:
        failure = exc
        sink.emit("session", "capture_error", error=repr(exc))
    finally:
        if raw_active and "densitometer" in sources:
            try:
                leave_raw_stream(sources["densitometer"])
            except BaseException as exc:
                sink.emit("session", "cleanup_error", error=repr(exc))
                if failure is None:
                    failure = exc
        for source in reversed(list(sources.values())):
            try:
                source.close()
            except BaseException as exc:
                sink.emit(
                    "session",
                    "source_close_error",
                    source_name=source.name,
                    error=repr(exc),
                )
                if failure is None:
                    failure = exc
        sink.emit(
            "session",
            "capture_end",
            result="error" if failure else "ok",
            interrupted=interrupted,
            serial_input_sent=args.dens_inventory or args.dens_raw_stream,
            calibrated_density_stream=False if args.dens_raw_stream else None,
        )
        sink.close()

    print(f"capture={output}")
    print(f"result={'error' if failure else 'ok'}")
    print(
        f"hardware_modified={'transient-densitometer-state-only' if args.dens_raw_stream else 'no'}"
    )
    if failure:
        raise failure


if __name__ == "__main__":
    main()
