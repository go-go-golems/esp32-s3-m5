#!/usr/bin/env python3
"""Audit the prepared no-drive P0.15 control and its latest clean build."""

from __future__ import annotations

import hashlib
import json
import re
import subprocess
from datetime import datetime, timezone
from pathlib import Path

TICKET = Path(__file__).resolve().parents[1]
REPO = Path(subprocess.check_output(["git", "-C", str(TICKET), "rev-parse", "--show-toplevel"], text=True).strip())
PROJECT = REPO / "0107-papers3-epd-painter-control"
BUILD = PROJECT / "build-ticket"
UPSTREAM = TICKET / "sources/code/epd-painter-753c521da8aef59756df07c1a4eb88f1c64c8227/src"
PREPARED = PROJECT / "components/epd_painter/src"
OUTPUT = TICKET / "scripts/output/13-built-control-audit-latest.md"


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def check(name: str, passed: bool, detail: str, rows: list[tuple[str, bool, str]]) -> None:
    rows.append((name, passed, detail))


def main() -> None:
    required = [
        PROJECT / "main/app_main.cpp",
        PROJECT / "main/fixtures/reader_page.bin",
        PROJECT / "sdkconfig.ticket",
        PREPARED / "EPD_Painter.cpp",
        PREPARED / "EPD_Painter_presets.h",
        BUILD / "papers3_epd_painter_control.bin",
        BUILD / "papers3_epd_painter_control.elf",
        BUILD / "compile_commands.json",
    ]
    missing = [str(path) for path in required if not path.is_file()]
    if missing:
        raise SystemExit("missing required build inputs:\n" + "\n".join(missing))

    main_cpp = (PROJECT / "main/app_main.cpp").read_text(encoding="utf-8")
    app_main_body = main_cpp[main_cpp.find('extern "C" void app_main(void)'):]
    driver_cpp = (PREPARED / "EPD_Painter.cpp").read_text(encoding="utf-8")
    sdkconfig = (PROJECT / "sdkconfig.ticket").read_text(encoding="utf-8")
    compile_commands = json.loads((BUILD / "compile_commands.json").read_text(encoding="utf-8"))
    commands = "\n".join(item["command"] for item in compile_commands)

    logs = sorted((TICKET / "scripts/output").glob("12-epd-painter-build-*.log"))
    if not logs:
        raise SystemExit("no build logs found")
    latest_log = logs[-1]
    log_text = latest_log.read_text(encoding="utf-8", errors="replace")

    tool = Path.home() / ".espressif/tools/xtensa-esp-elf/esp-14.2.0_20241119/xtensa-esp-elf/bin/xtensa-esp32s3-elf-nm"
    if not tool.is_file():
        raise SystemExit(f"missing nm tool: {tool}")
    symbols = subprocess.check_output([str(tool), "-C", str(BUILD / "papers3_epd_painter_control.elf")], text=True)
    component_symbols = subprocess.check_output(
        [str(tool), "-C", str(BUILD / "esp-idf/epd_painter/libepd_painter.a")], text=True
    )

    rows: list[tuple[str, bool, str]] = []
    check("Waveform/preset bytes unchanged", sha(UPSTREAM / "EPD_Painter_presets.h") == sha(PREPARED / "EPD_Painter_presets.h"), sha(PREPARED / "EPD_Painter_presets.h"), rows)
    check("Explicit PaperS3 preset compile definition", "EPD_PAINTER_PRESET_M5PAPER_S3=1" in commands, "compile_commands.json", rows)
    check("Automatic boot controller excluded", "EPD_PAINTER_DISABLE_BOOTCTL=1" in commands and "EPD_BootCtl" not in symbols, "compile definition and ELF symbols", rows)
    check("No M5GFX/M5Unified/Adafruit/Arduino symbols", not re.search(r"M5GFX|M5Unified|Adafruit|Arduino", symbols, re.I), "ELF symbol scan", rows)
    check("Exact 1000 Hz waveform-delay tick", "CONFIG_FREERTOS_HZ=1000" in sdkconfig, "sdkconfig.ticket", rows)
    check("USB Serial/JTAG console", "CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y" in sdkconfig and "CONFIG_ESP_CONSOLE_UART_DEFAULT=y" not in sdkconfig, "sdkconfig.ticket", rows)
    check("Octal PSRAM enabled", "CONFIG_SPIRAM=y" in sdkconfig and "CONFIG_SPIRAM_MODE_OCT=y" in sdkconfig, "sdkconfig.ticket", rows)
    check("No-drive boot path", all(token not in app_main_body for token in (".clear(", ".paint(", ".paintPacked(", ".unpaintPacked(", ".powerDown(")), "app_main() initializes and starts the console only", rows)
    expected_commands = ("cleanup CONFIRM", "target full", "target area", "target checker", "target page", "EPD_OP_BEGIN", "EPD_OP_END", "FAULT_NO_AUTOMATIC_CLEANUP")
    check("Bounded command and evidence surface", all(token in main_cpp for token in expected_commands), "fixed command grammar and timeout terminal record", rows)
    check("Reader fixture identity", sha(PROJECT / "main/fixtures/reader_page.bin") == "14dcffa9d13e0daabda8dc56c038bcec2eb8b01c4d8ac97ae170de5509207e90" and (PROJECT / "main/fixtures/reader_page.bin").stat().st_size == 129600, "129600 bytes; SHA-256 14dcffa9...", rows)
    check("Driver initializes packed buffers", all(token in driver_cpp for token in ("memset(packed_fastbuffer, 0, packed_size)", "memset(packed_screenbuffer, 0, packed_size)", "memset(packed_paintbuffer, 0, packed_size)")), "prepared EPD_Painter.cpp", rows)
    check("Bounded idle API linked", "EPD_Painter::waitIdle(unsigned long)" in symbols or "EPD_Painter::waitIdle(unsigned int)" in symbols, "ELF symbol scan", rows)
    check("Clean build has zero warnings", "warning:" not in log_text, latest_log.name, rows)
    check("Build did not flash hardware", "Project build complete" in log_text and "Writing at" not in log_text, latest_log.name, rows)

    iram_match = re.search(r"^│ IRAM\s+│\s+(\d+)\s+│\s+([0-9.]+)\s+│\s+(\d+)", log_text, re.M)
    iram_detail = "not parsed"
    if iram_match:
        iram_detail = f"used={iram_match.group(1)} percent={iram_match.group(2)} remaining={iram_match.group(3)}"

    failures = [name for name, passed, _ in rows if not passed]
    generated = datetime.now(timezone.utc).isoformat()
    lines = [
        "---",
        "Title: Built Independent EPD Control Audit",
        "Ticket: ESP-50-PAPERS3-EREADER-PRIMITIVES",
        "Status: active",
        "Topics:",
        "    - papers3",
        "    - eink",
        "    - esp-idf",
        "    - hardware-qualification",
        "DocType: reference",
        "Intent: long-term",
        "Owners: []",
        "RelatedFiles: []",
        "ExternalSources: []",
        'Summary: "Static and binary audit of the bounded P0.16 independent EPD control."',
        f"LastUpdated: {generated}",
        'WhatFor: "Prove that the independent-control build is pinned, waveform-identical, no-drive at boot, and exposes only bounded state-gated physical commands."',
        'WhenToUse: "Run after every P0.15/P0.16 source or build change and before creating a flash command."',
        "---",
        "",
        "# Built independent EPD control audit",
        "",
        f"Generated: {generated}",
        "",
        f"Gate: **{'PASS' if not failures else 'FAIL'}**",
        "",
        "| Check | Result | Evidence |",
        "|---|---|---|",
    ]
    for name, passed, detail in rows:
        lines.append(f"| {name} | {'PASS' if passed else 'FAIL'} | `{detail}` |")
    lines += [
        "",
        "## Binary identity",
        "",
        f"- Application SHA-256: `{sha(BUILD / 'papers3_epd_painter_control.bin')}`",
        f"- ELF SHA-256: `{sha(BUILD / 'papers3_epd_painter_control.elf')}`",
        f"- Prepared preset SHA-256: `{sha(PREPARED / 'EPD_Painter_presets.h')}`",
        f"- Latest build log: `{latest_log.relative_to(REPO)}`",
        "- Hardware modified: **no**",
        "",
        "## Review item",
        "",
        f"IRAM utilization is nearly saturated: `{iram_detail}`. P0.16 must avoid adding IRAM-attributed code and must rerun the size gate.",
        "",
        "This is a static/build gate. It does not prove runtime boot behavior or optical safety; those remain P0.17 hardware observations.",
    ]
    if failures:
        lines += ["", "## Failures", ""] + [f"- {name}" for name in failures]

    rendered = "\n".join(lines).rstrip() + "\n"
    snapshot = OUTPUT.with_name(f"13-built-control-audit-{generated.replace('-', '').replace(':', '').split('.')[0]}Z.md")
    snapshot.write_text(rendered, encoding="utf-8")
    OUTPUT.write_text(rendered, encoding="utf-8")
    print(snapshot)
    print(OUTPUT)
    print(f"gate={'PASS' if not failures else 'FAIL'} checks={len(rows)} failures={len(failures)}")
    if failures:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
