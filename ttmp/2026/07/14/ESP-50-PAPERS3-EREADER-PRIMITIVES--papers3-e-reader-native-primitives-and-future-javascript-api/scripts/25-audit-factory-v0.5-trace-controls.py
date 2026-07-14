#!/usr/bin/env python3
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
PROJECT = ROOT / "0109-papers3-factory-v0.5-runtime-trace"
OUTPUT = TICKET / "scripts/output"
WORK = TICKET / "scripts/work/25-factory-trace-audit"
PATCH = TICKET / "scripts/patches/23-m5gfx-0.2.15-factory-runtime-trace.patch"
FACTORY = ROOT.parent / "M5PaperS3-UserDemo"
PREFIX = "xtensa-esp32s3-elf-"
EXPECTED_LUT = "d24b2df188e4261d5891a0884e2510567ea45c38bcaebeb66ade1d4f4b979af3"


def run(*args: str, cwd: Path | None = None) -> str:
    return subprocess.check_output(args, cwd=cwd, text=True, stderr=subprocess.STDOUT)


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def extract_function(text: str, signature: str, next_signature: str) -> str:
    return text[text.index(signature):text.index(next_signature, text.index(signature))]


def dump(obj: Path, section: str, dest: Path) -> None:
    dest.unlink(missing_ok=True)
    subprocess.check_call([PREFIX + "objcopy", "--dump-section", f"{section}={dest}", str(obj)])


def main() -> None:
    OUTPUT.mkdir(parents=True, exist_ok=True); WORK.mkdir(parents=True, exist_ok=True)
    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    generated = datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")
    report = OUTPUT / f"25-factory-v0.5-trace-audit-{stamp}.md"
    latest = OUTPUT / "25-factory-v0.5-trace-audit-latest.md"
    checks: list[tuple[str, bool, str]] = []
    def check(name: str, ok: bool, evidence: str) -> None: checks.append((name, ok, evidence))

    clean = PROJECT / "build-clean"; off = PROJECT / "build-off"; trace = PROJECT / "build-trace"
    clean_elf = clean / "papers3_factory_v05_trace.elf"
    off_elf = off / "papers3_factory_v05_trace.elf"
    trace_elf = trace / "papers3_factory_v05_trace.elf"
    apps = {v: PROJECT / f"build-{v}/papers3_factory_v05_trace.bin" for v in ("clean", "off", "trace")}
    for p in (*apps.values(), clean_elf, off_elf, trace_elf, PATCH):
        if not p.is_file(): raise SystemExit(f"missing {p}")

    check("Exact ESP-IDF 5.3.3 checkout is active",
          run("git", "-C", "/home/manuel/esp/esp-idf-5.3.3", "rev-parse", "HEAD").strip() == "6db3dc25df7325c1c81b7cd7d4e42babff7a818e",
          "v5.3.3 commit=6db3dc25df7325c1c81b7cd7d4e42babff7a818e")
    check("Factory source lineage is exact V0.5",
          run("git", "-C", str(FACTORY), "rev-parse", "V0.5^{}").strip() == "5e275ad4b70abb85f7193fda137844730e64c4db",
          "FactoryTest V0.5 commit=5e275ad4b70abb85f7193fda137844730e64c4db")

    upstream_main = run("git", "-C", str(FACTORY), "show", "V0.5:main/main.cpp")
    local_main = (PROJECT / "main/main.cpp").read_text()
    upstream_boot = extract_function(upstream_main, "void boot_display_test()", "void check_full_display_refresh_request")
    local_boot = extract_function(local_main, "void boot_display_test()", "void check_full_display_refresh_request")
    boot_sha = hashlib.sha256(local_boot.encode()).hexdigest()
    check("Built-in black-white-grayscale function is byte-identical to V0.5 source",
          local_boot == upstream_boot, f"boot_display_test_sha256={boot_sha}")
    call_order = [local_main.index(x) for x in ("boot_display_test();", "FactoryTraceDumpAfterDisplayIdle();", "// Install apps")]
    check("F2 dump is ordered after factory sequence and before dashboard app installation",
          call_order == sorted(call_order) and "M5.Display.waitDisplay();" in (PROJECT / "main/factory_trace_runtime.cpp").read_text(),
          "sequence -> waitDisplay/dump -> install apps")

    reverse = subprocess.run(["git", "apply", "--reverse", "--check", str(PATCH)],
                             cwd=PROJECT / ".components/trace/M5GFX", capture_output=True)
    check("Legacy trace patch is exactly and reversibly applied", reverse.returncode == 0,
          f"patch_sha256={sha(PATCH)}")

    decoder_path = TICKET / "scripts/17-decode-m5gfx-epd-waveforms.py"
    spec = importlib.util.spec_from_file_location("decoder", decoder_path); assert spec and spec.loader
    decoder = importlib.util.module_from_spec(spec); spec.loader.exec_module(decoder)
    panel = PROJECT / ".components/trace/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp"
    luts = decoder.parse_luts(panel.read_text())
    lut_sha = hashlib.sha256(json.dumps(luts, sort_keys=True, separators=(",", ":")).encode()).hexdigest()
    check("Legacy trace patch preserves canonical LUTs", lut_sha == EXPECTED_LUT,
          f"canonical_lut_sha256={lut_sha}")

    configs = {v: (PROJECT / f"sdkconfig.{v}.generated").read_text() for v in ("clean", "off", "trace")}
    check("Clean and F1 compile trace completely off",
          all("# CONFIG_PAPERS3_FACTORY_RUNTIME_TRACE is not set" in configs[v] for v in ("clean", "off")),
          "clean=off")
    check("F2 enables 1024 x 48-byte ring",
          "CONFIG_PAPERS3_FACTORY_RUNTIME_TRACE=y" in configs["trace"] and
          "CONFIG_PAPERS3_FACTORY_TRACE_CAPACITY=1024" in configs["trace"], "capacity=1024")
    check("All variants preserve 100 Hz tick and USB Serial/JTAG console",
          all("CONFIG_FREERTOS_HZ=100" in configs[v] and "CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y" in configs[v]
              for v in configs), "tick=100Hz; console=USB Serial/JTAG")

    logs = {v: max(OUTPUT.glob(f"24-factory-v0.5-{v}-*.log"), key=lambda p: p.stat().st_mtime)
            for v in ("clean", "off", "trace")}
    warning_sets = {}
    for v, log in logs.items():
        warning_sets[v] = sorted(re.sub(r"^.*warning:", "warning:", line)
                                 for line in log.read_text(errors="replace").splitlines() if "warning:" in line)
    warning_sha = hashlib.sha256("\n".join(warning_sets["clean"]).encode()).hexdigest()
    check("Tracing introduces no new build warning", warning_sets["clean"] == warning_sets["off"] == warning_sets["trace"],
          f"count={len(warning_sets['clean'])}; normalized_sha256={warning_sha}")

    obj_specs = [
        ("panel-task", "M5GFX/CMakeFiles/__idf_M5GFX.dir/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp.obj",
         ".text._ZN4lgfx2v19Panel_EPD11task_updateEPS1_"),
        ("power-control", "M5GFX/CMakeFiles/__idf_M5GFX.dir/src/lgfx/v1/platforms/esp32/Bus_EPD.cpp.obj",
         ".text._ZN4lgfx2v17Bus_EPD12powerControlEb"),
        ("app-main", "main/CMakeFiles/__idf_main.dir/main.cpp.obj", ".text.app_main"),
    ]
    section_notes=[]; identical=True
    for label, rel, section in obj_specs:
        c=clean/"esp-idf"/rel; o=off/"esp-idf"/rel
        cb=WORK/f"clean-{label}.bin"; ob=WORK/f"off-{label}.bin"
        dump(c,section,cb); dump(o,section,ob)
        same=cb.read_bytes()==ob.read_bytes(); identical &= same
        section_notes.append(f"{label}={sha(cb)} same={str(same).lower()}")
    check("F1 critical machine code is byte-identical to clean source control", identical, "; ".join(section_notes))

    off_nm=run(PREFIX+"nm","-S","-C",str(off_elf)); trace_nm=run(PREFIX+"nm","-S","-C",str(trace_elf))
    check("F1 contains no trace hook or ring", "lgfx_epd_trace_emit" not in off_nm and "g_records" not in off_nm,
          "symbols=absent")
    ring=re.search(r"^[0-9a-f]+\s+([0-9a-f]+)\s+b\s+.*g_records$",trace_nm,re.M)
    ring_size=int(ring.group(1),16) if ring else -1
    check("F2 links strong hook and exact ring BSS",
          len(re.findall(r"\bT lgfx_epd_trace_emit$",trace_nm,re.M))==1 and ring_size==1024*48,
          f"ring_bss={ring_size}")

    panel_text=panel.read_text(); start=panel_text.index("for (uint_fast16_t y = 0; y < mh; y++)"); end=panel_text.index("bus->endTransaction();",start)
    check("No F2 trace hook executes inside row loop", "LGFX_EPD_TRACE_EMIT" not in panel_text[start:end],
          "frame hooks bracket 540-row loop")
    diff=run("git","diff",cwd=PROJECT/".components/trace/M5GFX")
    added="\n".join(line[1:] for line in diff.splitlines() if line.startswith("+") and not line.startswith("+++"))
    check("Legacy M5GFX patch has no hot-path printing or allocation",
          not re.search(r"\b(printf|ESP_LOG|malloc|new\s|heap_caps_|json)",added), "hook/counter additions only")

    dis=run(PREFIX+"objdump","-d","-C",str(trace_elf))
    calls=sum(1 for line in dis.splitlines() if "call" in line and "<lgfx_epd_trace_emit>" in line)
    check("F2 has bounded hook call sites", calls==10, f"hook_call_sites={calls}")
    delta=apps["trace"].stat().st_size-apps["off"].stat().st_size
    check("F2 application growth is bounded", 0<delta<4096, f"off={apps['off'].stat().st_size}; trace={apps['trace'].stat().st_size}; delta={delta}")

    release=TICKET/"sources/hardware/factory-v0.5/C139-PaperS3-FactoryTest-V0.5_0x0.bin"
    check("Exact F0 merged release remains available",
          release.is_file() and sha(release)=="d6733a0ca378f95335fa5fba4d4d992fb1dd97c17557b20e9aebfca08ba6d624",
          "release_sha256=d6733a0ca378f95335fa5fba4d4d992fb1dd97c17557b20e9aebfca08ba6d624")
    script=(TICKET/"scripts/24-build-factory-v0.5-trace-variants.sh").read_text()
    check("Build workflow cannot flash or monitor hardware", not re.search(r"idf\.py[^\n]*(flash|monitor)|write_flash",script),
          "set-target/build/size only")

    failures=[n for n,o,_ in checks if not o]; gate="PASS" if not failures else "FAIL"
    lines=["---","Title: FactoryTest V0.5 Trace Control Audit","Ticket: ESP-50-PAPERS3-EREADER-PRIMITIVES","Status: active","Topics:","    - papers3","    - eink","    - esp-idf","    - hardware-qualification","DocType: reference","Intent: long-term","Owners: []","RelatedFiles: []","ExternalSources: []",'Summary: "Observer and provenance audit for exact-IDF FactoryTest clean, F1 trace-off, and F2 trace-timing controls."',f"LastUpdated: {generated}",'WhatFor: "Gate stock-source-derived runtime tracing before physical F0/F1/F2 comparisons."','WhenToUse: "Review before flashing any 0109 variant or attributing ring events to FactoryTest V0.5."',"---","","# FactoryTest V0.5 trace control audit","",f"Gate: **{gate}**",f"Checks: **{len(checks)-len(failures)} / {len(checks)} passed**","Hardware modified: **no**","","## Checks",""]
    lines += [f"- [{'x' if ok else ' '}] **{name}** — {evidence}" for name,ok,evidence in checks]
    lines += ["","## Disposition","","F0 remains the exact released-binary optical control and has no ring. F1 is eligible as the stock-source trace-off proxy because its factory boot function is source-identical and its app/driver critical text is byte-identical to the clean build. F2 is eligible for later operator-gated use only after F0 and F1 observations; its ring dump occurs after the built-in sequence reaches display idle.","","## Artifact identities","",f"- Clean app SHA-256: `{sha(apps['clean'])}`",f"- F1 app SHA-256: `{sha(apps['off'])}`",f"- F2 app SHA-256: `{sha(apps['trace'])}`",f"- Trace patch SHA-256: `{sha(PATCH)}`",f"- Canonical LUT SHA-256: `{lut_sha}`",""]
    text="\n".join(lines); report.write_text(text); latest.write_text(text)
    print(report); print(latest); print(f"gate={gate} checks={len(checks)} failures={len(failures)}")
    if failures:
        for f in failures: print(f"failure={f}")
        raise SystemExit(1)

if __name__ == "__main__": main()
