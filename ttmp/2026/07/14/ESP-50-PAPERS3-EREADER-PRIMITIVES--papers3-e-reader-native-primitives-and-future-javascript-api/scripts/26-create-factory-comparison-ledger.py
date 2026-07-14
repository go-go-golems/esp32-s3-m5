#!/usr/bin/env python3
"""Create immutable preregistration ledgers for FactoryTest F0/F1/F2 runs."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path

TICKET = Path(__file__).resolve().parents[1]
ROOT = TICKET.parents[4]
BASE = TICKET / "scripts/experiments"
PORT = "/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:16:17:DC-if00"
COMMON = {
    "schema": "esp50.epd-experiment.v1",
    "ticket": "ESP-50-PAPERS3-EREADER-PRIMITIVES",
    "board": {"model": "M5Stack PaperS3", "usb_serial": "D0:CF:13:16:17:DC", "port": PORT},
    "panel": {"model": "ED047TC1", "assigned_vcom": None},
    "optical_protocol": {
        "video_required": True,
        "camera_fixed": True,
        "exposure_locked": True,
        "white_balance_locked": True,
        "focus_locked": True,
        "minimum_fps": 60,
        "stable_illumination": True,
        "reference_patches_in_frame": ["matte-white", "matte-dark"],
        "record_from_before_reset_through_dashboard": True,
    },
    "automatic_and_optical_dispositions_are_separate": True,
    "stop_conditions": ["unexpected heat", "odor", "sound", "power instability", "reset loop", "visible worsening outside expected sequence"],
}

RUNS = [
    {
        "id": "EXP-20260714-001-factory-v05-exact-f0", "treatment": "F0-exact-vendor",
        "hypothesis": "The exact released FactoryTest V0.5 black-to-white sequence establishes whether vendor firmware erases its immediately preceding full-black field cleanly.",
        "decision_rule": "Pass white only if the full-black field disappears without material retained field, gradient, or edge residue; dashboard quality is scored separately.",
        "firmware": {"kind": "exact-merged-release", "flash_offset": "0x0", "sha256": "d6733a0ca378f95335fa5fba4d4d992fb1dd97c17557b20e9aebfca08ba6d624", "ring": False},
        "starting_state": "0107 EPD_Painter commanded-white endpoint with substantial retained FactoryTest dashboard ghosting",
        "telemetry": "video and operator observation only; exact binary has no ring",
    },
    {
        "id": "EXP-20260714-002-factory-v05-source-f1-off", "treatment": "F1-stock-source-trace-off",
        "hypothesis": "The exact-source/toolchain trace-off rebuild reproduces F0 optical behavior closely enough to serve as its source-derived proxy.",
        "decision_rule": "Compare F1 white and dashboard against F0 video under identical camera settings; reject proxy status if endpoint class or spatial artifacts differ materially.",
        "firmware": {"kind": "source-derived", "source_commit": "5e275ad4b70abb85f7193fda137844730e64c4db", "idf": "v5.3.3", "m5gfx": "0.2.15", "m5unified": "0.2.10", "sha256": "3d9bf37a5c5faa120fa1dccf357e8d0676a77495359754d062a5fa654dd2d2b3", "ring": False},
        "starting_state": "F0 final dashboard; F1 commands full black immediately before judged white",
        "telemetry": "video and operator observation; trace compiled completely out",
    },
    {
        "id": "EXP-20260714-003-factory-v05-source-f2-trace", "treatment": "F2-stock-source-trace-timing",
        "hypothesis": "Fixed-ring boundary tracing does not materially change F1 optical behavior and reveals the actual source-derived eraser/target/frame/power schedule.",
        "decision_rule": "Reject timing interpretation if F2 optical endpoint differs materially from F1, ring overwrites/invalid records occur, or trace dump begins before DISPLAY_IDLE/POWER_OFF_END.",
        "firmware": {"kind": "source-derived-instrumented", "source_commit": "5e275ad4b70abb85f7193fda137844730e64c4db", "idf": "v5.3.3", "m5gfx": "0.2.15", "m5unified": "0.2.10", "sha256": "95334c261762205ab95d3f578a5d3d0a0eac4fe7fffdfd1ada0e836ba8a2d755", "ring": {"records": 1024, "record_bytes": 48, "schema": "esp50.factory-v05-runtime-trace.v1"}},
        "starting_state": "F1 final dashboard; F2 commands full black immediately before judged white",
        "telemetry": "fixed video, operator observation, and post-idle USB JSONL ring dump",
    },
]

FRONTMATTER = """---
Title: "{title}"
Ticket: ESP-50-PAPERS3-EREADER-PRIMITIVES
Status: active
Topics:
    - papers3
    - eink
    - hardware-qualification
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "{summary}"
LastUpdated: 2026-07-14T22:50:00Z
WhatFor: "Preserve a preregistered FactoryTest F0/F1/F2 experiment treatment and its evidence."
WhenToUse: "Use during and after the corresponding physical treatment; never substitute evidence from another treatment."
---

"""

PREREG = """# Preregistration: {id}

## Hypothesis

{hypothesis}

## Decision rule

{decision_rule}

## Fixed sequence

1. Confirm artifact SHA, audit PASS, stable by-id port, and no serial owner.
2. Start fixed video before flash-triggered reset; do not change camera or lighting between F0/F1/F2.
3. Observe title, full black, full white, sixteen grayscale bars, and dashboard.
4. Judge the two-second white interval specifically against the immediately preceding black field.
5. Record automatic and optical dispositions separately and verbatim.
6. Stop on any preregistered safety condition; do not skip ahead to a later treatment.

## Forbidden post-hoc changes

Do not change exposure, white balance, focus, illumination, waveform, dwell, source, SDK, or treatment order after seeing an endpoint. A necessary correction creates a new experiment ID.
"""

OBS = """# Operator observation: {id}

Status: pending

- Video file:
- Camera/device:
- FPS/resolution:
- Exposure/ISO/shutter:
- White balance:
- Focus:
- Illumination/reference patches:
- Ambient/panel temperature and method:

## Verbatim observation

Pending.

## Endpoint scores

- Full black darkness/uniformity:
- Full white retained black field:
- Full white gradients/edges:
- Grayscale ordering/separation:
- Dashboard text/background:
- Heat/odor/sound/power anomaly:
"""

DISP = """# Disposition: {id}

- Execution status: pending
- Automatic transaction disposition: pending
- Optical disposition: pending
- Stop reason: pending
- Eligible to proceed to next treatment: no (pending review)
- Reviewer notes: pending
"""


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> None:
    BASE.mkdir(parents=True, exist_ok=True)
    created = []
    for run in RUNS:
        directory = BASE / run["id"]
        if directory.exists():
            raise SystemExit(f"refusing to overwrite existing experiment: {directory}")
        directory.mkdir()
        manifest = dict(COMMON)
        manifest.update(run)
        manifest["status"] = "preregistered"
        manifest["evidence"] = {"flash_log": None, "serial_log": None, "video": None, "ring_jsonl": None}
        manifest_path = directory / "manifest.json"
        prereg_path = directory / "01-preregistration.md"
        manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
        fm = lambda title, summary: FRONTMATTER.format(title=title, summary=summary)
        prereg_path.write_text(fm(f"Preregistration - {run['id']}", f"Frozen hypothesis, decision rule, and protocol for {run['treatment']}.") + PREREG.format(**run))
        (directory / "02-operator-observation.md").write_text(fm(f"Operator Observation - {run['id']}", f"Pending verbatim optical observation for {run['treatment']}.") + OBS.format(**run))
        (directory / "03-disposition.md").write_text(fm(f"Disposition - {run['id']}", f"Separate automatic and optical dispositions for {run['treatment']}.") + DISP.format(**run))
        (directory / "00-index.md").write_text(
            fm(f"Experiment - {run['id']}", f"Index for preregistered treatment {run['treatment']}.") +
            f"# {run['id']}\n\nTreatment: `{run['treatment']}`. The manifest and preregistration are immutable after creation. Runtime evidence and observations are added as new files or completed pending templates; corrections require an amendment file.\n"
        )
        (directory / "preregistration.sha256").write_text(
            f"{digest(manifest_path)}  manifest.json\n{digest(prereg_path)}  01-preregistration.md\n"
        )
        created.append(str(directory))
    print("\n".join(f"created={path}" for path in created))
    print("hardware_modified=no")

if __name__ == "__main__":
    main()
