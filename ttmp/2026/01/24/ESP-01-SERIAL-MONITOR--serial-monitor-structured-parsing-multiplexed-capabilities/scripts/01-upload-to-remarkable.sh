#!/usr/bin/env bash
set -euo pipefail

TICKET_ID="ESP-01-SERIAL-MONITOR"
TICKET_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"

# Keep uploads grouped under the ticket's workspace date for consistency.
REMOTE_DIR="/ai/2026/01/24/${TICKET_ID}"
NAME="${TICKET_ID} — Serial Monitor Research"

MODE="${1:-upload}" # upload | dry-run | force

FILES=(
  "${TICKET_DIR}/index.md"
  "${TICKET_DIR}/design-doc/01-serial-monitor-as-stream-processor-parsing-multiplexing-screenshots.md"
  "${TICKET_DIR}/reference/01-diary.md"
  "${TICKET_DIR}/scripts/README.md"
  "${TICKET_DIR}/tasks.md"
  "${TICKET_DIR}/changelog.md"
  "${TICKET_DIR}/README.md"
)

case "${MODE}" in
  dry-run)
    remarquee upload bundle --dry-run \
      --name "${NAME}" \
      --remote-dir "${REMOTE_DIR}" \
      --toc-depth 3 \
      "${FILES[@]}"
    ;;
  upload)
    remarquee upload bundle \
      --name "${NAME}" \
      --remote-dir "${REMOTE_DIR}" \
      --toc-depth 3 \
      "${FILES[@]}"
    ;;
  force)
    remarquee upload bundle --force \
      --name "${NAME}" \
      --remote-dir "${REMOTE_DIR}" \
      --toc-depth 3 \
      "${FILES[@]}"
    ;;
  *)
    echo "usage: $0 [upload|dry-run|force]" >&2
    exit 2
    ;;
esac
