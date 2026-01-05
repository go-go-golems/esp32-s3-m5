#!/usr/bin/env bash
set -euo pipefail

# Writes WiFi credentials into the project-local sdkconfig (gitignored).
#
# Usage:
#   WIFI_SSID="..." WIFI_PASSWORD="..." ./scripts/configure_wifi_sdkconfig.sh

proj_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
sdkconfig="${proj_dir}/sdkconfig"

if [[ -z "${WIFI_SSID:-}" ]]; then
  echo "ERROR: WIFI_SSID is empty" >&2
  exit 2
fi

if [[ -z "${WIFI_PASSWORD:-}" ]]; then
  echo "ERROR: WIFI_PASSWORD is empty" >&2
  exit 2
fi

if [[ ! -f "${sdkconfig}" ]]; then
  echo "sdkconfig not found; run once: idf.py -C \"${proj_dir}\" set-target esp32s3" >&2
  exit 2
fi

tmp="$(mktemp)"
trap 'rm -f "$tmp"' EXIT

python3 - "$sdkconfig" "$tmp" "$WIFI_SSID" "$WIFI_PASSWORD" <<'PY'
import sys
from pathlib import Path

src = Path(sys.argv[1]).read_text(encoding="utf-8", errors="ignore").splitlines(True)
dst = Path(sys.argv[2])
ssid = sys.argv[3]
pw = sys.argv[4]

def set_kv(lines, key, value):
    out = []
    found = False
    for line in lines:
        if line.startswith(key + "="):
            out.append(f'{key}="{value}"\n')
            found = True
        else:
            out.append(line)
    if not found:
        out.append(f'{key}="{value}"\n')
    return out

src = set_kv(src, "CONFIG_TUTORIAL_0029_WIFI_SSID", ssid)
src = set_kv(src, "CONFIG_TUTORIAL_0029_WIFI_PASSWORD", pw)

dst.write_text("".join(src), encoding="utf-8")
PY

mv "$tmp" "$sdkconfig"
echo "Wrote WiFi credentials to ${sdkconfig} (not tracked by git)."

