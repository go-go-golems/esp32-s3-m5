#!/usr/bin/env python3

import argparse
import json
import sys
from typing import Any


ACTION_ENUM = {
    "NavUp": "cardputer_kb::Action::NavUp",
    "NavDown": "cardputer_kb::Action::NavDown",
    "NavLeft": "cardputer_kb::Action::NavLeft",
    "NavRight": "cardputer_kb::Action::NavRight",
    "Back": "cardputer_kb::Action::Back",
    "Enter": "cardputer_kb::Action::Enter",
    "Tab": "cardputer_kb::Action::Tab",
    "Space": "cardputer_kb::Action::Space",
}


def load_cfg(path: str) -> dict[str, Any]:
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def main() -> int:
    ap = argparse.ArgumentParser(description="Convert CFG JSON to a C++ header binding table.")
    ap.add_argument("cfg_json", help="Path to JSON config emitted by the wizard")
    ap.add_argument("--symbol", default="kCapturedBindings", help="C++ symbol name (default: kCapturedBindings)")
    args = ap.parse_args()

    cfg = load_cfg(args.cfg_json)
    bindings = cfg.get("bindings", [])
    if not isinstance(bindings, list):
        print("ERROR: cfg.bindings must be a list", file=sys.stderr)
        return 2

    lines: list[str] = []
    lines.append("#pragma once")
    lines.append("")
    lines.append('#include "cardputer_kb/bindings.h"')
    lines.append("")
    lines.append("namespace cardputer_kb {")
    lines.append("")
    lines.append(f"static constexpr Binding {args.symbol}[] = {{")

    for b in bindings:
        if not isinstance(b, dict):
            continue
        name = b.get("name")
        req = b.get("required_keynums")
        if not isinstance(name, str) or not isinstance(req, list):
            continue
        if name not in ACTION_ENUM:
            print(f"WARNING: unknown binding name {name!r}; skipping", file=sys.stderr)
            continue
        nums = [int(x) for x in req][:4]
        if not nums:
            continue
        while len(nums) < 4:
            nums.append(0)
        lines.append(f"    {{{ACTION_ENUM[name]}, {len(req)}, {{{nums[0]}, {nums[1]}, {nums[2]}, {nums[3]}}}}},")

    lines.append("};")
    lines.append("")
    lines.append("} // namespace cardputer_kb")
    lines.append("")

    sys.stdout.write("\n".join(lines))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

