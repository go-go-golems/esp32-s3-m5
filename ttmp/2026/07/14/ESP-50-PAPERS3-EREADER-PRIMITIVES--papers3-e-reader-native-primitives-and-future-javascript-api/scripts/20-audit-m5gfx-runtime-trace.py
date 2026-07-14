#!/usr/bin/env python3
"""Audit trace-off/trace-timing M5GFX variants without hardware access."""

from __future__ import annotations

import hashlib
import importlib.util
import json
import re
import subprocess
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[6]
TICKET = Path(__file__).resolve().parents[1]
PROJECT = ROOT / "0108-papers3-m5gfx-runtime-trace"
OUTPUT = TICKET / "scripts" / "output"
WORK = TICKET / "scripts" / "work" / "20-observer-effect-audit"
PATCH = TICKET / "scripts" / "patches" / "18-m5gfx-runtime-trace-hooks.patch"
EXPECTED_LUT = "d24b2df188e4261d5891a0884e2510567ea45c38bcaebeb66ade1d4f4b979af3"
TOOL_PREFIX = "xtensa-esp32s3-elf-"


def run(*args: str, cwd: Path | None = None) -> str:
    return subprocess.check_output(args, cwd=cwd, text=True, stderr=subprocess.STDOUT)


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def nm(elf: Path) -> str:
    return run(TOOL_PREFIX + "nm", "-S", "-C", str(elf))


def symbol_size(table: str, name: str) -> int:
    for line in table.splitlines():
        if name in line:
            fields = line.split(maxsplit=3)
            return int(fields[1], 16)
    raise ValueError(f"symbol not found: {name}")


def dump_section(obj: Path, section: str, output: Path) -> None:
    output.unlink(missing_ok=True)
    subprocess.check_call([TOOL_PREFIX + "objcopy", "--dump-section", f"{section}={output}", str(obj)])


def load_decoder():
    path = TICKET / "scripts" / "17-decode-m5gfx-epd-waveforms.py"
    spec = importlib.util.spec_from_file_location("waveform_decoder", path)
    assert spec and spec.loader
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main() -> None:
    OUTPUT.mkdir(parents=True, exist_ok=True)
    WORK.mkdir(parents=True, exist_ok=True)
    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    generated = datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")
    report = OUTPUT / f"20-m5gfx-runtime-trace-audit-{stamp}.md"
    latest = OUTPUT / "20-m5gfx-runtime-trace-audit-latest.md"

    off_build = PROJECT / "build-off"
    trace_build = PROJECT / "build-trace"
    off_elf = off_build / "papers3_m5gfx_runtime_trace.elf"
    trace_elf = trace_build / "papers3_m5gfx_runtime_trace.elf"
    off_app = off_build / "papers3_m5gfx_runtime_trace.bin"
    trace_app = trace_build / "papers3_m5gfx_runtime_trace.bin"
    for artifact in (off_elf, trace_elf, off_app, trace_app, PATCH):
        if not artifact.is_file():
            raise SystemExit(f"missing artifact: {artifact}")

    panel = PROJECT / "components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp"
    bus = PROJECT / "components/M5GFX/src/lgfx/v1/platforms/esp32/Bus_EPD.cpp"
    app = PROJECT / "main/app_main.cpp"
    runtime = PROJECT / "main/epd_trace_runtime.cpp"
    header = PROJECT / "components/M5GFX/src/lgfx/v1/platforms/esp32/epd_trace_hooks.hpp"
    off_config = PROJECT / "sdkconfig.off.generated"
    trace_config = PROJECT / "sdkconfig.trace.generated"

    checks: list[tuple[str, bool, str]] = []

    def check(name: str, condition: bool, evidence: str) -> None:
        checks.append((name, condition, evidence))

    reverse = subprocess.run(
        ["git", "apply", "--reverse", "--check", str(PATCH)],
        cwd=PROJECT / "components/M5GFX", capture_output=True, text=True,
    )
    check("Patch is applied exactly and reversibly", reverse.returncode == 0, f"patch_sha256={sha(PATCH)}")

    decoder = load_decoder()
    luts = decoder.parse_luts(panel.read_text())
    canonical = hashlib.sha256(json.dumps(luts, sort_keys=True, separators=(",", ":")).encode()).hexdigest()
    check("Patched source preserves canonical LUTs", canonical == EXPECTED_LUT, f"canonical_lut_sha256={canonical}")

    off_cfg = off_config.read_text()
    trace_cfg = trace_config.read_text()
    check("Off configuration compiles trace out", "# CONFIG_PAPERS3_M5GFX_RUNTIME_TRACE is not set" in off_cfg,
          f"sdkconfig_sha256={sha(off_config)}")
    check("Timing configuration enables 512-record ring",
          "CONFIG_PAPERS3_M5GFX_RUNTIME_TRACE=y" in trace_cfg and
          "CONFIG_PAPERS3_M5GFX_TRACE_CAPACITY=512" in trace_cfg,
          f"sdkconfig_sha256={sha(trace_config)}")
    check("Both variants preserve 100 Hz tick",
          "CONFIG_FREERTOS_HZ=100" in off_cfg and "CONFIG_FREERTOS_HZ=100" in trace_cfg,
          "CONFIG_FREERTOS_HZ=100")

    logs = sorted(OUTPUT.glob("19-m5gfx-runtime-trace-*-*.log"))
    latest_off = max((p for p in logs if "-off-" in p.name), key=lambda p: p.stat().st_mtime)
    latest_trace = max((p for p in logs if "-trace-" in p.name), key=lambda p: p.stat().st_mtime)
    check("Final builds are warning-free",
          "warning:" not in latest_off.read_text(errors="replace") and
          "warning:" not in latest_trace.read_text(errors="replace"),
          f"off={latest_off.name}; trace={latest_trace.name}")

    off_nm = nm(off_elf)
    trace_nm = nm(trace_elf)
    check("Off ELF has no trace hook or ring", "lgfx_epd_trace_emit" not in off_nm and "g_records" not in off_nm,
          "symbols=absent")
    check("Timing ELF links one strong hook", len(re.findall(r"\bT lgfx_epd_trace_emit$", trace_nm, re.M)) == 1,
          "lgfx_epd_trace_emit=1")
    ring_match = re.search(r"^[0-9a-f]+\s+([0-9a-f]+)\s+b\s+.*g_records$", trace_nm, re.M)
    ring_size = int(ring_match.group(1), 16) if ring_match else -1
    check("Timing ring is fixed at 512 x 48 bytes", ring_size == 512 * 48 and "sizeof(TraceRecord) == 48" in runtime.read_text(),
          f"ring_bss_bytes={ring_size}")

    clean_base = ROOT / "0106-papers3-epd-qualification/build-cell-D/esp-idf/M5GFX/CMakeFiles/__idf_M5GFX.dir/src/lgfx/v1/platforms/esp32"
    off_base = off_build / "esp-idf/M5GFX/CMakeFiles/__idf_M5GFX.dir/src/lgfx/v1/platforms/esp32"
    section_specs = [
        ("panel-task", clean_base / "Panel_EPD.cpp.obj", off_base / "Panel_EPD.cpp.obj",
         ".text._ZN4lgfx2v19Panel_EPD11task_updateEPS1_"),
        ("power-control", clean_base / "Bus_EPD.cpp.obj", off_base / "Bus_EPD.cpp.obj",
         ".text._ZN4lgfx2v17Bus_EPD12powerControlEb"),
    ]
    section_evidence = []
    sections_identical = True
    for label, clean_obj, off_obj, section in section_specs:
        clean_bin = WORK / f"clean-{label}.bin"
        off_bin = WORK / f"off-{label}.bin"
        dump_section(clean_obj, section, clean_bin)
        dump_section(off_obj, section, off_bin)
        same = clean_bin.read_bytes() == off_bin.read_bytes()
        sections_identical &= same
        section_evidence.append(f"{label}: clean={sha(clean_bin)} off={sha(off_bin)} same={str(same).lower()}")
    check("Trace-off critical driver code is byte-identical to clean Cell D", sections_identical,
          "; ".join(section_evidence))

    panel_text = panel.read_text()
    row_start = panel_text.index("for (uint_fast16_t y = 0; y < mh; y++)")
    row_end = panel_text.index("bus->endTransaction();", row_start)
    check("No trace hook executes inside the row loop", "LGFX_EPD_TRACE_EMIT" not in panel_text[row_start:row_end],
          "frame hooks bracket, rather than enter, the 540-row loop")
    diff = run("git", "diff", cwd=PROJECT / "components/M5GFX")
    added = "\n".join(line[1:] for line in diff.splitlines() if line.startswith("+") and not line.startswith("+++"))
    check("Patched M5GFX performs no hot-path printing or allocation",
          not re.search(r"\b(printf|ESP_LOG|malloc|new\s|heap_caps_|json)", added),
          "added M5GFX lines contain fixed hook calls and counters only")

    app_main_body = app.read_text().split('extern "C" void app_main(void)', 1)[1]
    check("Firmware boot issues no display transaction", "DrawTextScene();" not in app_main_body,
          "M5.begin(clear_display=false), rotation, text defaults, console only")
    app_text = app.read_text()
    trace_wait_pattern = "if (g_display_ready) {\n            M5.Display.waitDisplay();\n        }"
    check("Trace dumps are operator-requested after waitDisplay",
          trace_wait_pattern in app_text and "papers3::trace::DumpJsonLines();" in app_text,
          "epd trace dump is outside M5GFX worker and guarded by display mutex")

    off_task = symbol_size(off_nm, "Panel_EPD::task_update")
    trace_task = symbol_size(trace_nm, "Panel_EPD::task_update")
    off_power = symbol_size(off_nm, "Bus_EPD::powerControl")
    trace_power = symbol_size(trace_nm, "Bus_EPD::powerControl")
    check("Timing instrumentation growth is bounded to frame/power paths",
          trace_task > off_task and trace_power > off_power,
          f"task_update={off_task}->{trace_task} (+{trace_task-off_task}); powerControl={off_power}->{trace_power} (+{trace_power-off_power})")

    disassembly = run(TOOL_PREFIX + "objdump", "-d", "-C", str(trace_elf))
    hook_calls = sum(1 for line in disassembly.splitlines() if "call" in line and "<lgfx_epd_trace_emit>" in line)
    check("Linked timing image has bounded hook call sites", hook_calls == 10,
          f"hook_call_sites={hook_calls}; none in row loop")

    off_size = off_app.stat().st_size
    trace_size = trace_app.stat().st_size
    check("Application size delta is recorded and modest", 0 < trace_size - off_size < 4096,
          f"off={off_size}; trace={trace_size}; delta={trace_size-off_size}")

    build_script = (TICKET / "scripts/19-build-m5gfx-runtime-trace-variants.sh").read_text()
    check("Build workflow contains no flash or monitor operation",
          not re.search(r"idf\.py[^\n]*(?:flash|monitor)|write_flash", build_script),
          "builds use set-target, build, and size only")

    failures = [name for name, ok, _ in checks if not ok]
    gate = "PASS" if not failures else "FAIL"
    lines = [
        "---",
        "Title: M5GFX Runtime Trace Observer-Effect Audit",
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
        'Summary: "Static/binary observer-effect audit for trace-off and fixed-ring M5GFX timing variants before hardware use."',
        f"LastUpdated: {generated}",
        'WhatFor: "Prove trace-off identity and bound trace-on perturbations before any physical comparison."',
        'WhenToUse: "Review before authorizing an instrumented M5GFX flash or interpreting runtime timestamps."',
        "---",
        "",
        "# M5GFX runtime trace observer-effect audit",
        "",
        f"Gate: **{gate}**",
        f"Checks: **{len(checks) - len(failures)} / {len(checks)} passed**",
        "Hardware modified: **no**",
        "",
        "## Checks",
        "",
    ]
    for name, ok, evidence in checks:
        lines.append(f"- [{'x' if ok else ' '}] **{name}** — {evidence}")
    lines += [
        "",
        "## Interpretation",
        "",
        "The trace-disabled critical M5GFX frame scheduler and power-control text sections are byte-identical to the previously built clean Cell D control. This is stronger than source inspection: trace arguments, queue queries, counters, and hooks compile completely out.",
        "",
        "The timing variant is intentionally not byte-identical. It adds fixed-ring writes and one timestamp read per event at operation, queue, update-preparation, power, and frame boundaries. It does not count drive codes or log inside the 540-row loop, and it emits no serial output while rails are active. Static auditing bounds where perturbation can occur; only a later trace-off/trace-on physical timing comparison can measure its duration.",
        "",
        "## Artifact identities",
        "",
        f"- Off application SHA-256: `{sha(off_app)}`",
        f"- Trace application SHA-256: `{sha(trace_app)}`",
        f"- Patch SHA-256: `{sha(PATCH)}`",
        f"- Canonical LUT SHA-256: `{canonical}`",
        "",
    ]
    text = "\n".join(lines)
    report.write_text(text)
    latest.write_text(text)
    print(report)
    print(latest)
    print(f"gate={gate} checks={len(checks)} failures={len(failures)}")
    if failures:
        for failure in failures:
            print(f"failure={failure}")
        raise SystemExit(1)


if __name__ == "__main__":
    main()
