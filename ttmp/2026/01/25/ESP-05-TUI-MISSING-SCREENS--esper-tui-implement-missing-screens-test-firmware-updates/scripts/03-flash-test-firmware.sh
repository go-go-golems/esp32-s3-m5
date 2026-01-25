#!/usr/bin/env bash
set -euo pipefail

# Wrapper to keep ESP-05 work reproducible while reusing the canonical ESP-02 scripts.
#
# Optional:
#   ESPPORT=/dev/ttyACM0 ./ttmp/.../scripts/03-flash-test-firmware.sh

exec ./ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/03-flash-firmware.sh

