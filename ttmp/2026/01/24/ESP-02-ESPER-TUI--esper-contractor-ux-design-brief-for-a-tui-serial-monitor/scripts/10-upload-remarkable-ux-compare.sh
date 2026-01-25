#!/usr/bin/env bash
set -euo pipefail

# Upload the UX compare doc (and playbook) to reMarkable via remarquee.
#
# Defaults to dry-run. Set DRY_RUN=0 to actually upload.
#
# Usage:
#   ./ttmp/.../scripts/10-upload-remarkable-ux-compare.sh
#   DRY_RUN=0 ./ttmp/.../scripts/10-upload-remarkable-ux-compare.sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TICKET_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

COMPARE_DOC="${TICKET_DIR}/various/02-tui-current-vs-desired-compare.md"
PLAYBOOK_DOC="${TICKET_DIR}/playbooks/01-ux-iteration-loop.md"

DATE="${DATE:-2026/01/25}"
REMOTE_DIR="${REMOTE_DIR:-/ai/${DATE}/ESP-02-ESPER-TUI}"
NAME="${NAME:-ESP-02-ESPER-TUI — UX compare + iteration playbook}"

DRY_RUN="${DRY_RUN:-1}"
FORCE="${FORCE:-0}"

if [[ ! -f "${COMPARE_DOC}" ]]; then
  echo "missing: ${COMPARE_DOC}" >&2
  exit 1
fi

remarquee status >/dev/null

if [[ "${DRY_RUN}" == "1" ]]; then
  exec remarquee upload bundle --dry-run \
    "${COMPARE_DOC}" "${PLAYBOOK_DOC}" \
    --name "${NAME}" \
    --remote-dir "${REMOTE_DIR}" \
    --toc-depth 2
fi

FORCE_FLAG=()
if [[ "${FORCE}" == "1" ]]; then
  FORCE_FLAG+=(--force)
fi

exec remarquee upload bundle \
  "${COMPARE_DOC}" "${PLAYBOOK_DOC}" \
  "${FORCE_FLAG[@]}" \
  --name "${NAME}" \
  --remote-dir "${REMOTE_DIR}" \
  --toc-depth 2
