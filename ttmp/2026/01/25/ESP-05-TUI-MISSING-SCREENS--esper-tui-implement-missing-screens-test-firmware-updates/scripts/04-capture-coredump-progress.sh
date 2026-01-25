#!/usr/bin/env bash
set -euo pipefail

# Captures the Core Dump Capture In Progress overlay on real hardware by
# auto-triggering a long core dump stream from the esp32s3-test firmware.

STAMP="${STAMP:-hw_coredump_progress_$(date +%Y%m%d-%H%M%S)}"

exec env \
  STAMP="${STAMP}" \
  FIRMWARE_TRIGGERS="coredumpfakeslow 800" \
  ./ttmp/2026/01/25/ESP-05-TUI-MISSING-SCREENS--esper-tui-implement-missing-screens-test-firmware-updates/scripts/01-capture-hw-trigger-suite.sh

