#!/usr/bin/env bash
set -euo pipefail

# Captures the "Reset Device" confirmation overlay.
# Default uses a virtual PTY so we don't actually reboot the connected device.
#
# Optional:
#   USE_VIRTUAL_PTY=0 bash ./ttmp/.../scripts/06-tmux-capture-reset-confirm.sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TICKET_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
REPO_ROOT="$(git -C "${TICKET_DIR}" rev-parse --show-toplevel)"

RUN_TUI="${REPO_ROOT}/ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/05-run-esper-tui.sh"
DETECT_PORT="${REPO_ROOT}/ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/01-detect-port.sh"

STAMP="${STAMP:-reset_confirm_$(date +%Y%m%d-%H%M%S)}"
OUTDIR="${TICKET_DIR}/various/screenshots/${STAMP}"
mkdir -p "${OUTDIR}"

USE_VIRTUAL_PTY="${USE_VIRTUAL_PTY:-1}"
ESPPORT="${ESPPORT:-}"

have_convert() { command -v convert >/dev/null 2>&1; }

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

start_virtual_pty() {
  local pty_a="${OUTDIR}/esper-pty-a"
  local pty_b="${OUTDIR}/esper-pty-b"
  rm -f "${pty_a}" "${pty_b}"

  socat -d -d \
    "pty,raw,echo=0,link=${pty_a}" \
    "pty,raw,echo=0,link=${pty_b}" \
    >/dev/null 2>&1 &
  SOCAT_PID=$!

  for _ in $(seq 1 30); do
    if [[ -e "${pty_a}" && -e "${pty_b}" ]]; then
      ESPPORT="${pty_a}"
      FEED_PORT="${pty_b}"
      return 0
    fi
    sleep 0.1
  done

  echo "ERROR: socat PTYs did not appear" >&2
  return 1
}

start_feeder() {
  local feed_port="$1"
  (
    for i in $(seq 1 40); do
      printf 'I (12) demo: hello %03d\n' "${i}"
      sleep 0.03
    done
  ) >"${feed_port}" &
  FEED_PID=$!
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

cleanup() {
  if [[ -n "${FEED_PID:-}" ]]; then
    kill "${FEED_PID}" >/dev/null 2>&1 || true
  fi
  if [[ -n "${SOCAT_PID:-}" ]]; then
    kill "${SOCAT_PID}" >/dev/null 2>&1 || true
  fi
}

SOCAT_PID=""
FEED_PID=""
FEED_PORT=""
trap cleanup EXIT

main() {
  if [[ "${USE_VIRTUAL_PTY}" == "1" ]]; then
    start_virtual_pty
    start_feeder "${FEED_PORT}"
  else
    if [[ -z "${ESPPORT}" ]]; then
      ESPPORT="$("${DETECT_PORT}")"
    fi
  fi

  local w=120
  local h=40
  local session="esper_tui_reset_confirm_$$"

  tmux new-session -d -x "${w}" -y "${h}" -s "${session}" -c "${REPO_ROOT}" "env ESPPORT='${ESPPORT}' '${RUN_TUI}'"
  sleep 2
  wait_for_monitor "${session}"

  # Enter HOST mode, open command palette, filter to "reset", run it.
  send_keys "${session}" C-t
  sleep 0.2
  send_keys "${session}" C-t
  sleep 0.05
  send_keys "${session}" t
  sleep 0.2
  send_keys "${session}" r e s e t
  sleep 0.2
  send_keys "${session}" Enter
  sleep 0.2

  capture "${session}" "${w}" "${h}" "01-reset-confirm"

  # Exit without confirming.
  send_keys "${session}" Escape
  sleep 0.1
  send_keys "${session}" C-c
  sleep 0.2
  tmux kill-session -t "${session}" >/dev/null 2>&1 || true

  echo "outputs: ${OUTDIR}"
}

main "$@"

