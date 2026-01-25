#!/usr/bin/env bash
set -euo pipefail

# Runs the standard tmux capture harness against real hardware, auto-triggering
# the esp32s3-test REPL commands to generate a deterministic event suite.
#
# Usage:
#   ./ttmp/.../scripts/01-capture-hw-trigger-suite.sh
#
# Optional:
#   STAMP=myname ./ttmp/.../scripts/01-capture-hw-trigger-suite.sh
#   FIRMWARE_TRIGGERS=logdemo,partial,gdbstub,coredumpfake,panic ./ttmp/.../scripts/01-capture-hw-trigger-suite.sh

REPO_ROOT="$(git rev-parse --show-toplevel)"

DEFAULT_TRIGGERS="logdemo,partial,gdbstub,coredumpfake"
FIRMWARE_TRIGGERS="${FIRMWARE_TRIGGERS:-${DEFAULT_TRIGGERS}}"

STAMP="${STAMP:-hw_trigger_suite_$(date +%Y%m%d-%H%M%S)}"

exec env \
  STAMP="${STAMP}" \
  FIRMWARE_TRIGGERS="${FIRMWARE_TRIGGERS}" \
  "${REPO_ROOT}/ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/09-tmux-capture-esper-tui.sh"

