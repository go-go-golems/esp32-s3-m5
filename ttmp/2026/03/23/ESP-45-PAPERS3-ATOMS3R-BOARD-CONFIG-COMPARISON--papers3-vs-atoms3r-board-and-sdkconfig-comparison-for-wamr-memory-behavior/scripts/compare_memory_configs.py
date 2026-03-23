#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


ROOT = Path("/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5")
ATOM = ROOT / "0081-atoms3r-wamr-probe-console" / "sdkconfig"
PAPER = ROOT / "0082-papers3-wamr-allocator-control" / "build-internal-pool" / "sdkconfig.variant"

PREFIXES = (
    "CONFIG_IDF_TARGET",
    "CONFIG_ESPTOOLPY_FLASH",
    "CONFIG_ESPTOOLPY_FLASHSIZE",
    "CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ",
    "CONFIG_FREERTOS_HZ",
    "CONFIG_SPIRAM",
    "CONFIG_SPIRAM_",
    "CONFIG_ESP_SYSTEM_PANIC",
    "CONFIG_COMPILER_OPTIMIZATION",
    "CONFIG_ESP_CONSOLE",
    "CONFIG_WAMR",
    "CONFIG_PAPERS3_WAMR",
)


def load(path: Path) -> dict[str, str]:
    out: dict[str, str] = {}
    for line in path.read_text().splitlines():
        line = line.strip()
        if not line:
            continue
        if line.startswith("# ") and line.endswith(" is not set"):
            key = line[2 : -len(" is not set")]
            out[key] = "n"
            continue
        if line.startswith("CONFIG_") and "=" in line:
            key, value = line.split("=", 1)
            out[key] = value
    return out


def wanted(key: str) -> bool:
    return any(key.startswith(prefix) for prefix in PREFIXES)


def main() -> int:
    atom = load(ATOM)
    paper = load(PAPER)
    keys = sorted({*atom.keys(), *paper.keys()})
    keys = [k for k in keys if wanted(k)]

    print(f"atom_config={ATOM}")
    print(f"paper_config={PAPER}")
    print()
    print(f"{'key':58} {'atom':28} {'paper':28} status")
    print("-" * 130)

    for key in keys:
        a = atom.get(key, "-")
        p = paper.get(key, "-")
        status = "same" if a == p else "diff"
        print(f"{key:58} {a[:28]:28} {p[:28]:28} {status}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
