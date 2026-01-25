#!/usr/bin/env bash
set -euo pipefail

# Captures the missing Inspector detail screens (PANIC + COREDUMP report) using real hardware.
# This script intentionally drives tmux keypresses to open the detail overlays.
#
# Output:
#   ttmp/.../various/screenshots/<stamp>/*.txt (+ *.png if ImageMagick "convert" exists)

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TICKET_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
REPO_ROOT="$(git -C "${TICKET_DIR}" rev-parse --show-toplevel)"

RUN_TUI="${REPO_ROOT}/ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/05-run-esper-tui.sh"
DETECT_PORT="${REPO_ROOT}/ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/01-detect-port.sh"

STAMP="${STAMP:-inspector_details_$(date +%Y%m%d-%H%M%S)}"
OUTDIR="${TICKET_DIR}/various/screenshots/${STAMP}"
mkdir -p "${OUTDIR}"

ESPPORT="${ESPPORT:-}"
if [[ -z "${ESPPORT}" ]]; then
  ESPPORT="$("${DETECT_PORT}")"
fi

have_convert() {
  command -v convert >/dev/null 2>&1
}

render_png() {
  local in_txt="$1"
  local out_png="$2"
  local cols="$3"
  local rows="$4"
  if ! have_convert; then
    return 0
  fi

  local text
  text="$(cat "${in_txt}")"

  local char_w=9
  local char_h=18
  local pad=20
  local px_w=$((cols * char_w + pad * 2))
  local px_h=$((rows * char_h + pad * 2))

  convert \
    -size "${px_w}x${px_h}" \
    xc:'#0f111a' \
    -fill '#e6e6e6' \
    -font 'DejaVu-Sans-Mono' \
    -pointsize 14 \
    -gravity NorthWest \
    -annotate "+${pad}+${pad}" "${text}" \
    "${out_png}"
}

capture() {
  local session="$1"
  local w="$2"
  local h="$3"
  local name="$4"

  local base="${OUTDIR}/${w}x${h}-${name}"
  local out_txt="${base}.txt"
  local out_png="${base}.png"

  tmux capture-pane -pN -t "${session}:0.0" >"${out_txt}"
  render_png "${out_txt}" "${out_png}" "${w}" "${h}"
  echo "captured: ${out_txt}"
  if have_convert; then
    echo "captured: ${out_png}"
  fi
}

send_keys() {
  local session="$1"
  shift
  tmux send-keys -t "${session}:0.0" "$@"
}

wait_for_monitor() {
  local session="$1"
  local tries="${2:-80}"

  for _ in $(seq 1 "${tries}"); do
    local pane
    pane="$(tmux capture-pane -pN -t "${session}:0.0")"
    if echo "${pane}" | rg -q "Connected:"; then
      return 0
    fi
    sleep 0.25
  done
  echo "ERROR: timed out waiting for monitor screen" >&2
  return 1
}

main() {
  local w=120
  local h=40
  local session="esper_tui_inspector_detail_$$"

  tmux new-session -d -x "${w}" -y "${h}" -s "${session}" -c "${REPO_ROOT}" "env ESPPORT='${ESPPORT}' '${RUN_TUI}'"
  sleep 2
  wait_for_monitor "${session}"

  # Trigger deterministic event suite + panic last (device will reboot).
  send_keys "${session}" C-u
  sleep 0.05
  send_keys "${session}" "emitall panic" Enter
  sleep 6

  # Switch to HOST mode + open inspector + focus inspector list.
  send_keys "${session}" C-t
  sleep 0.2
  send_keys "${session}" i
  sleep 0.2
  send_keys "${session}" Tab
  sleep 0.2

  capture "${session}" "${w}" "${h}" "01-inspector"

  # Open last event detail (expected: PANIC) and capture.
  send_keys "${session}" End
  sleep 0.1
  send_keys "${session}" Enter
  sleep 0.3
  capture "${session}" "${w}" "${h}" "02-panic-detail"

  # Close and open previous event detail (expected: COREDUMP report) and capture.
  send_keys "${session}" Escape
  sleep 0.2
  send_keys "${session}" Up
  sleep 0.1
  capture "${session}" "${w}" "${h}" "03a-inspector-coredump-selected"
  send_keys "${session}" Enter
  sleep 0.3
  capture "${session}" "${w}" "${h}" "03b-coredump-report"

  tmux kill-session -t "${session}" || true
  echo "outputs: ${OUTDIR}"
}

main "$@"
