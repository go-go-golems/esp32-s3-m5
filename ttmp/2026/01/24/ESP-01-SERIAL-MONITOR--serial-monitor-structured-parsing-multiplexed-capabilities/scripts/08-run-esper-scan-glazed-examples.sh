#!/usr/bin/env bash
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
workspace_root="$(cd "$here/../../../../../../" && pwd)"
esper_root="$workspace_root/esper"
out_dir="$workspace_root/ttmp/2026/01/24/ESP-01-SERIAL-MONITOR--serial-monitor-structured-parsing-multiplexed-capabilities/sources"

mkdir -p "$out_dir"

{
  echo "# esper scan (glazed) examples"
  echo "# timestamp: $(date -Iseconds)"
  echo

  echo "## 1) Table output (no probe)"
  (cd "$esper_root" && go run ./cmd/esper scan --output table --fields preferred_path,vidpid,score,reasons)
  echo

  echo "## 2) Table output (with esptool probe)"
  (cd "$esper_root" && go run ./cmd/esper scan --probe-esptool --output table --fields preferred_path,vidpid,chip_description,usb_mode)
  echo

  echo "## 3) JSON output (with esptool probe)"
  (cd "$esper_root" && go run ./cmd/esper scan --probe-esptool --output json)
  echo
} | tee "$out_dir/esper-scan-glazed-example-output.txt"

