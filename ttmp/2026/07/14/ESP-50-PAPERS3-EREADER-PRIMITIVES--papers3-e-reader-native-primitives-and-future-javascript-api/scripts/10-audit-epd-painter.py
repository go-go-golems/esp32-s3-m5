#!/usr/bin/env python3
"""Audit the pinned EPD_Painter PaperS3 path before any hardware execution.

The script intentionally emits a committed Markdown artifact. It checks source
invariants that are easy to lose during manual review and fails if the pinned
snapshot no longer contains the expected PaperS3 control path.
"""

from __future__ import annotations

from collections import Counter
from dataclasses import dataclass
from datetime import datetime, timezone
import pathlib
import re

ROOT = pathlib.Path(__file__).resolve().parents[1]
EPD = ROOT / "sources/code/epd-painter-753c521da8aef59756df07c1a4eb88f1c64c8227"
M5 = ROOT / "sources/code/m5gfx-lut-comparison"
OUT = ROOT / "scripts/output/10-epd-painter-pre-hardware-audit.md"

FILES = {
    "cpp": EPD / "src/EPD_Painter.cpp",
    "header": EPD / "src/EPD_Painter.h",
    "preset": EPD / "src/EPD_Painter_presets.h",
    "power_h": EPD / "src/epd_painter_powerctl.h",
    "adafruit": EPD / "src/EPD_Painter_Adafruit.h",
    "m5gfx": M5 / "M5GFX-0.2.25.cpp",
}


@dataclass
class Finding:
    severity: str
    title: str
    evidence: str
    consequence: str
    required_action: str


def line_number(text: str, needle: str) -> int:
    index = text.find(needle)
    if index < 0:
        raise RuntimeError(f"missing expected source text: {needle}")
    return text.count("\n", 0, index) + 1


def extract_block(text: str, start: str, end: str) -> str:
    a = text.find(start)
    if a < 0:
        raise RuntimeError(f"missing block start: {start}")
    b = text.find(end, a)
    if b < 0:
        raise RuntimeError(f"missing block end after {start}: {end}")
    return text[a:b]


def parse_assignment(block: str, name: str) -> int:
    match = re.search(rf"\.{re.escape(name)}\s*=\s*(-?\d+)", block)
    if not match:
        raise RuntimeError(f"missing assignment {name}")
    return int(match.group(1))


def parse_gpio_assignment(block: str, name: str) -> int:
    match = re.search(rf"bus_cfg\.{re.escape(name)}\s*=\s*GPIO_NUM_(\d+)", block)
    if not match:
        raise RuntimeError(f"missing M5GFX assignment {name}")
    return int(match.group(1))


def parse_waveform(block: str, name: str, next_name: str | None) -> list[list[int]]:
    start = block.find(f".{name}")
    if start < 0:
        raise RuntimeError(f"missing waveform {name}")
    end = block.find(f".{next_name}", start) if next_name else block.find("},\n        },", start)
    if end < 0:
        raise RuntimeError(f"cannot terminate waveform {name}")
    payload = block[start:end]
    rows = re.findall(r"\{\s*([0-3](?:\s*,\s*[0-3])+)\s*\}", payload)
    result = [[int(value) for value in re.findall(r"[0-3]", row)] for row in rows]
    if len(result) != 3:
        raise RuntimeError(f"expected 3 rows in {name}, got {len(result)}")
    return result


def main() -> None:
    for path in FILES.values():
        if not path.is_file():
            raise SystemExit(f"missing audit input: {path}")

    source = {name: path.read_text(encoding="utf-8") for name, path in FILES.items()}
    preset_block = extract_block(
        source["preset"],
        "inline EPD_Painter::Config EPD_M5PAPER_S3_PRESET",
        "#if defined(EPD_PAINTER_PRESET_LILYGO_T5_S3_GPS)",
    )
    board_block = extract_block(
        source["m5gfx"],
        "if (gt911_found)",
        "goto init_clear;",
    )

    epd_pins = {
        "pin_pwr": parse_assignment(preset_block, "pin_pwr"),
        "pin_spv": parse_assignment(preset_block, "pin_spv"),
        "pin_ckv": parse_assignment(preset_block, "pin_ckv"),
        "pin_sph": parse_assignment(preset_block, "pin_sph"),
        "pin_oe": parse_assignment(preset_block, "pin_oe"),
        "pin_le": parse_assignment(preset_block, "pin_le"),
        "pin_cl": parse_assignment(preset_block, "pin_cl"),
    }
    m5_pins = {name: parse_gpio_assignment(board_block, name) for name in epd_pins}
    epd_data_match = re.search(r"\.data_pins\s*=\s*\{([^}]+)\}", preset_block)
    if not epd_data_match:
        raise RuntimeError("missing EPD_Painter data_pins")
    epd_data = [int(x) for x in re.findall(r"\d+", epd_data_match.group(1))]
    m5_data = [
        int(x)
        for _, x in sorted(
            (int(i), pin)
            for i, pin in re.findall(
                r"bus_cfg\.pin_data\[(\d+)\]\s*=\s*GPIO_NUM_(\d+)", board_block
            )
        )
    ]
    pins_match = epd_pins == m5_pins and epd_data == m5_data

    cpp = source["cpp"]
    power_h = source["power_h"]
    adafruit = source["adafruit"]

    findings: list[Finding] = []
    if pins_match:
        findings.append(
            Finding(
                "PASS",
                "PaperS3 scan and power pins match M5GFX 0.2.25",
                f"EPD_Painter preset line {line_number(source['preset'], 'inline EPD_Painter::Config EPD_M5PAPER_S3_PRESET')}; "
                f"M5GFX board block line {line_number(source['m5gfx'], 'if (gt911_found)')}. Data={epd_data}; controls={epd_pins}.",
                "The independent driver targets the same physical signals as the qualified M5GFX path.",
                "Keep these values pinned and assert them in the firmware build metadata.",
            )
        )
    else:
        findings.append(Finding("BLOCKER", "Pin maps differ", f"EPD={epd_pins}/{epd_data}; M5={m5_pins}/{m5_data}", "Wrong pins can corrupt scans or power control.", "Do not build or flash."))

    bad_mux_call = "epd_gpio_func_sel(GPIO_PIN_MUX_REG[pin])" in cpp
    if bad_mux_call:
        findings.append(
            Finding(
                "BLOCKER",
                "GPIO pad-selection helper receives an IOMUX register address instead of a GPIO number",
                f"`EPD_Painter.cpp:{line_number(cpp, 'epd_gpio_func_sel(GPIO_PIN_MUX_REG[pin])')}` calls "
                "`esp_rom_gpio_pad_select_gpio()` with `GPIO_PIN_MUX_REG[pin]`; the ESP-IDF API takes the GPIO number.",
                "The raw pinned driver must not be run unchanged. Pin mux configuration can address invalid GPIO indices or leave data pins misconfigured.",
                "Patch both calls to pass `pin` and `_config.pin_cl`, then compile and inspect the generated path before hardware use.",
            )
        )

    epd_on = extract_block(power_h, "bool powerOn() override", "void powerOff() override")
    epd_off = extract_block(power_h, "void powerOff() override", "private:")
    on_order_ok = epd_on.find("EPD_PIN_HIGH(_pin_oe)") < epd_on.find("EPD_PIN_HIGH(_pin_pwr)")
    if on_order_ok:
        findings.append(Finding("PASS", "Direct-GPIO power-on ordering matches M5GFX", f"`epd_painter_powerctl.h:{line_number(power_h, 'EPD_PIN_HIGH(_pin_oe)')}` raises OE, waits 100 µs, then raises PWR.", "The tested enable order is retained.", "Preserve this sequence."))

    off_reversed = epd_off.find("EPD_PIN_LOW(_pin_oe)") < epd_off.find("EPD_PIN_LOW(_pin_pwr)")
    if off_reversed:
        findings.append(
            Finding(
                "REVIEW",
                "Power-off order differs from M5GFX and does not lower SPV",
                f"`epd_painter_powerctl.h:{line_number(power_h, 'EPD_PIN_LOW(_pin_oe)')}` lowers OE before PWR. "
                "M5GFX lowers PWR, then OE, then SPV. `EPD_Painter::powerOff()` only delegates to the power driver.",
                "The alternative sequence may be safer, equivalent, or incorrect, but it is an uncontrolled difference in a physical experiment.",
                "Use an explicit local safe-state function, document the chosen order, and lower LE/SPV/SPH around power-off.",
            )
        )

    if "memset(packed_screenbuffer" not in cpp[cpp.find("packed_screenbuffer ="):cpp.find("// ── Create the power driver")]:
        findings.append(
            Finding(
                "BLOCKER",
                "Packed physical-state buffers are allocated without initialization",
                f"Allocations begin near `EPD_Painter.cpp:{line_number(cpp, 'packed_screenbuffer =')}`; no zeroing occurs before the paint task starts.",
                "The first differential update can compare a target against indeterminate PSRAM and drive arbitrary transitions.",
                "Zero fast, screen, paint, and bitmask buffers before task creation; begin every experiment with a documented hard clear.",
            )
        )

    allocation_check = re.search(r"if \(!\(dma_buffer.*?\)\) return false;", cpp, re.S)
    check_text = allocation_check.group(0) if allocation_check else ""
    if "packed_paintbuffer" not in check_text or "bitmask" not in check_text:
        findings.append(
            Finding(
                "BLOCKER",
                "Initialization does not validate every required allocation",
                f"`EPD_Painter.cpp:{line_number(cpp, 'if (!(dma_buffer && packed_fastbuffer && packed_screenbuffer))')}` omits `packed_paintbuffer` and `bitmask`.",
                "Allocation failure can become a null dereference in the background task.",
                "Extend the guard and fail cleanly before creating semaphores or tasks.",
            )
        )

    dma_alloc = cpp.find("dma_buffer1 = static_cast")
    dma_memset = cpp.find("memset(dma_buffer1")
    dma_guard = cpp.find("if (!(dma_buffer")
    if -1 not in (dma_alloc, dma_memset, dma_guard) and dma_alloc < dma_memset < dma_guard:
        findings.append(
            Finding(
                "BLOCKER",
                "DMA buffers are dereferenced before their delayed allocation guard",
                f"`EPD_Painter.cpp:{line_number(cpp, 'memset(dma_buffer1')}` zeroes both row buffers before the guard near line {line_number(cpp, 'if (!(dma_buffer')}.",
                "A DMA-capable internal-memory allocation failure crashes in `begin()` instead of returning a safe diagnostic.",
                "Check both DMA allocations immediately, before `memset`, descriptor construction, power-driver creation, or task setup.",
            )
        )

    if "log_w(" in cpp and '#include "esp_log.h"' not in cpp:
        findings.append(
            Finding(
                "BLOCKER",
                "The advertised pure ESP-IDF path contains an Arduino-only logging macro",
                f"`EPD_Painter.cpp:{line_number(cpp, 'log_w(')}` calls `log_w` while the file includes Arduino logging support only under `#ifdef ARDUINO`.",
                "A minimal pure ESP-IDF component can fail to compile before the hardware control is reproducible.",
                "Replace the fallback message with `ESP_LOGW`/`printf` and build with the explicit M5PaperS3 preset definition.",
            )
        )

    if "xSemaphoreCreateBinary()" in cpp and "xTaskCreatePinnedToCore(" in cpp:
        findings.append(
            Finding(
                "BLOCKER",
                "Semaphore and paint-task creation results are not validated",
                f"Resource creation starts near `EPD_Painter.cpp:{line_number(cpp, '_paint_active_sem = xSemaphoreCreateBinary()')}` and ignores the return code from `xTaskCreatePinnedToCore`.",
                "Low-memory or task-creation failure can leave a partially initialized driver that later blocks or dereferences invalid handles.",
                "Check both semaphore handles and the task creation result; return failure before any command can energize the panel.",
            )
        )

    if "while(paintStage==(interlace_mode?3:2))" in cpp:
        findings.append(
            Finding(
                "BLOCKER",
                "`paint()` returns after buffer pickup, not after scan completion",
                f"`EPD_Painter.cpp:{line_number(cpp, 'while(paintStage==(interlace_mode?3:2))')}` waits only until the initial stage value changes.",
                "Optical timing, heap checks, cleanup ordering, and power-down cannot be bounded by the caller.",
                "Add a public `waitIdle(timeout)` and require it after every experimental operation.",
            )
        )

    if "_painter.setInterlaceMode(true)" in adafruit:
        findings.append(
            Finding(
                "REVIEW",
                "Adafruit binding unconditionally enables three-stage convergence",
                f"`EPD_Painter_Adafruit.h:{line_number(adafruit, '_painter.setInterlaceMode(true)')}` sets `paintStage` to 3 per paint.",
                "One requested paint can execute multiple waveform scans, complicating pulse-dose comparison with M5GFX.",
                "Expose the stage policy explicitly in the control firmware and report it with each result.",
            )
        )

    if "memset(buffer" in adafruit and "if (!buffer)" not in adafruit:
        findings.append(
            Finding(
                "BLOCKER",
                "Adafruit framebuffer allocation is dereferenced without a null check",
                f"`EPD_Painter_Adafruit.h:{line_number(adafruit, 'buffer = static_cast<uint8_t *>')}` allocates, then immediately calls `memset`.",
                "A PSRAM allocation failure crashes before diagnostics.",
                "Exclude the Adafruit binding from the pure ESP-IDF control; if it is later enabled, patch allocation ownership and failure handling first.",
            )
        )

    clear_balanced = all(token in cpp for token in ("totpass[0] = 6", "totpass[1] = 2", "totpass[2] = 4", "totpass[3] = 8"))
    if clear_balanced:
        findings.append(
            Finding(
                "PASS",
                "HARD clear alternates 20 full-panel actions with equal aggregate polarity counts",
                f"`EPD_Painter.cpp:{line_number(cpp, 'totpass[0] = 6')}` uses phase counts 6, 2, 4, 8; alternating phases total 10 actions per polarity.",
                "The explicit clear is a suitable controlled starting and ending operation, subject to verifying code-to-voltage polarity and endpoint.",
                "Use HARD clear sparingly and record its duration and final optical state.",
            )
        )

    findings.append(
        Finding(
            "PASS",
            "Automatic shutdown can be disabled for the experiment",
            f"`EPD_Painter.h:{line_number(source['header'], 'void setAutoShutdown(bool v)')}` exposes the control.",
            "The test can avoid reset-toggle, NVS, shutdown-image, and automatic unpaint behavior.",
            "Call `setAutoShutdown(false)` before `begin()` and implement no system-power-off command in the first firmware.",
        )
    )

    names = ["fast_lighter", "fast_darker", "normal_lighter", "normal_darker", "high_lighter", "high_darker"]
    waves: dict[str, list[list[int]]] = {}
    for index, name in enumerate(names):
        waves[name] = parse_waveform(preset_block, name, names[index + 1] if index + 1 < len(names) else None)

    blockers = sum(f.severity == "BLOCKER" for f in findings)
    reviews = sum(f.severity == "REVIEW" for f in findings)
    gate = "BLOCKED — create and review a narrow local hardening patch before build/flash" if blockers else "PASS"

    generated = datetime.now(timezone.utc).isoformat()
    lines = [
        "---",
        "Title: EPD Painter Pre-Hardware Audit Output",
        "Ticket: ESP-50-PAPERS3-EREADER-PRIMITIVES",
        "Status: active",
        "Topics:",
        "    - papers3",
        "    - eink",
        "    - m5gfx",
        "    - debugging",
        "DocType: reference",
        "Intent: long-term",
        "Owners: []",
        "RelatedFiles: []",
        "ExternalSources:",
        "    - https://github.com/tonywestonuk/EPD_Painter/commit/753c521da8aef59756df07c1a4eb88f1c64c8227",
        'Summary: "Generated source audit and hardware-use gate for pinned EPD_Painter."',
        f"LastUpdated: {generated}",
        'WhatFor: "Reproduce the source-level decision that blocks unmodified EPD_Painter from PaperS3 hardware execution."',
        'WhenToUse: "Regenerate and review before changing the local hardening patch or flashing the independent control."',
        "---",
        "",
        "# EPD_Painter pre-hardware audit",
        "",
        f"Generated: {generated}",
        "",
        "## Audit gate",
        "",
        f"**{gate}**",
        "",
        f"Blockers: {blockers}; review items: {reviews}.",
        "",
        "This audit evaluates commit `753c521da8aef59756df07c1a4eb88f1c64c8227`. It does not establish optical quality or panel safety. It determines whether the unmodified source is suitable for the first controlled hardware run.",
        "",
        "## Pin comparison",
        "",
        "| Signal | EPD_Painter | M5GFX 0.2.25 |",
        "|---|---:|---:|",
    ]
    for name in epd_pins:
        lines.append(f"| `{name}` | {epd_pins[name]} | {m5_pins[name]} |")
    lines.append(f"| `data[0..7]` | `{epd_data}` | `{m5_data}` |")
    lines.extend(["", "## Waveform action counts", "", "EPD_Painter documents `0=float`, `1=whiten`, `2=darken`, and `3=both`. Counts below are descriptive only; equal counts do not prove equal physical dose.", "", "| Table | Row | 0 float | 1 whiten | 2 darken | 3 both |", "|---|---:|---:|---:|---:|---:|"])
    for name, rows in waves.items():
        for index, row in enumerate(rows):
            count = Counter(row)
            lines.append(f"| `{name}` | {index} | {count[0]} | {count[1]} | {count[2]} | {count[3]} |")

    lines.extend(["", "## Findings", ""])
    for index, finding in enumerate(findings, 1):
        lines.extend(
            [
                f"### {index}. [{finding.severity}] {finding.title}",
                "",
                f"**Evidence:** {finding.evidence}",
                "",
                f"**Consequence:** {finding.consequence}",
                "",
                f"**Required action:** {finding.required_action}",
                "",
            ]
        )

    lines.extend(
        [
            "## Approved next step",
            "",
            "Create the firmware in a numbered repository directory, not in a temporary directory. Vendor or pin the exact EPD_Painter commit through a reproducible ticket script. Apply only the audited hardening changes: correct GPIO pad selection, initialize and validate all buffers, add bounded idle waiting, make stage count explicit, disable automatic shutdown, and drive control pins to a documented safe state. Build and inspect the binary before any flash.",
            "",
            "The first hardware operation remains a HARD white cleanup followed by idle wait. No black waveform runs until boot diagnostics, buffer initialization, command gating, and cleanup completion are visible on the serial console.",
        ]
    )

    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")
    print(OUT)
    print(f"gate={gate}")


if __name__ == "__main__":
    main()
