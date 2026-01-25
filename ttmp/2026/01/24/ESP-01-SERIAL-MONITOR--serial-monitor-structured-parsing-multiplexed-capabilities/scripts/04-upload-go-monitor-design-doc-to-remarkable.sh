#!/usr/bin/env bash
set -euo pipefail

TICKET_ID="ESP-01-SERIAL-MONITOR"
TICKET_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"

DOC="${TICKET_DIR}/design-doc/02-go-serial-monitor-idf-py-compatible-design-implementation.md"
REMOTE_DIR="/ai/2026/01/24/${TICKET_ID}"
NAME="${TICKET_ID} — Go Serial Monitor (idf.py-compatible) — Design+Impl"

MODE="${1:-upload}" # upload | dry-run | force

case "${MODE}" in
  dry-run)
    remarquee upload bundle --dry-run \
      --name "${NAME}" \
      --remote-dir "${REMOTE_DIR}" \
      --toc-depth 3 \
      "${DOC}"
    ;;
  upload)
    remarquee upload bundle \
      --name "${NAME}" \
      --remote-dir "${REMOTE_DIR}" \
      --toc-depth 3 \
      "${DOC}"
    ;;
  force)
    remarquee upload bundle --force \
      --name "${NAME}" \
      --remote-dir "${REMOTE_DIR}" \
      --toc-depth 3 \
      "${DOC}"
    ;;
  *)
    echo "usage: $0 [upload|dry-run|force]" >&2
    exit 2
    ;;
esac
