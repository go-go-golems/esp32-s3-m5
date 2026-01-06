#!/usr/bin/env bash
set -euo pipefail

# Offline helper: generate PDFs for reMarkable upload without requiring rmapi.
#
# Usage:
#   ./scripts/90-remarkable-pdf-only.sh

TICKET_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="${OUT_DIR:-$TICKET_DIR/exports/remarkable-pdf}"

mkdir -p "$OUT_DIR"

python3 /home/manuel/.local/bin/remarkable_upload.py --pdf-only --output-dir "$OUT_DIR" \
  --ticket-dir "$TICKET_DIR" \
  "$TICKET_DIR/reference/01-diary.md" \
  "$TICKET_DIR/analysis/01-analysis-evolve-0029-mock-hub-into-real-zigbee-orchestrator.md" \
  "$TICKET_DIR/design-doc/01-design-cardputer-zigbee-orchestrator-esp-event-bus-http-202-protobuf-ws.md" \
  "$TICKET_DIR/analysis/02-investigation-report-device-rejoin-loop-channel-selection-stuck-on-ch13.md"

echo "PDFs written to: $OUT_DIR"

