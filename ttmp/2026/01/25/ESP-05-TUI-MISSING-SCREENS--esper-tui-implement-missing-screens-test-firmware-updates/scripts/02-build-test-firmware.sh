#!/usr/bin/env bash
set -euo pipefail

# Wrapper to keep ESP-05 work reproducible while reusing the canonical ESP-02 scripts.

exec ./ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/02-build-firmware.sh

