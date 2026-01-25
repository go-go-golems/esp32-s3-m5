#!/usr/bin/env bash
set -euo pipefail

# Captures:
# - Port Picker with nickname assignment
# - Assign Nickname overlay
# - Device Manager view
# - Device edit overlay
#
# Uses a temporary XDG_CONFIG_HOME so we don't touch the user's real registry.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TICKET_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
REPO_ROOT="$(git -C "${TICKET_DIR}" rev-parse --show-toplevel)"

STAMP="${STAMP:-device_mgr_$(date +%Y%m%d-%H%M%S)}"
OUTDIR="${TICKET_DIR}/various/screenshots/${STAMP}"
mkdir -p "${OUTDIR}"

TMPDIR="$(mktemp -d)"
XDG_CONFIG_HOME="${TMPDIR}/xdg_config"
XDG_CACHE_HOME="${TMPDIR}/xdg_cache"
XDG_STATE_HOME="${TMPDIR}/xdg_state"

mkdir -p "${XDG_CONFIG_HOME}/esper"
cat >"${XDG_CONFIG_HOME}/esper/devices.json" <<'JSON'
{
  "version": 1,
  "devices": []
}
JSON

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

wait_for_port_picker() {
  local session="$1"
  local tries="${2:-120}"
  for _ in $(seq 1 "${tries}"); do
    local pane
    pane="$(tmux capture-pane -pN -t "${session}:0.0")"
    if echo "${pane}" | rg -q "Select Serial Port"; then
      return 0
    fi
    sleep 0.25
  done
  echo "ERROR: timed out waiting for port picker" >&2
  return 1
}

main() {
  local w=120
  local h=40
  local session="esper_tui_device_mgr_$$"

  tmux new-session -d -x "${w}" -y "${h}" -s "${session}" -c "${REPO_ROOT}/esper" \
    "env GOTELEMETRY=off XDG_CONFIG_HOME='${XDG_CONFIG_HOME}' XDG_CACHE_HOME='${XDG_CACHE_HOME}' XDG_STATE_HOME='${XDG_STATE_HOME}' go run ./cmd/esper"

  sleep 2
  wait_for_port_picker "${session}"

  capture "${session}" "${w}" "${h}" "01-port-picker"

  # Assign nickname for the selected port.
  send_keys "${session}" n
  sleep 0.2
  capture "${session}" "${w}" "${h}" "02-assign-nickname"

  # Type nickname, tab through remaining fields, then save.
  send_keys "${session}" C-u
  sleep 0.05
  send_keys "${session}" cardputer
  sleep 0.1
  send_keys "${session}" Tab Tab Tab Tab Enter
  sleep 0.3

  capture "${session}" "${w}" "${h}" "03-port-picker-with-nickname"

  # Open Device Manager.
  send_keys "${session}" d
  sleep 1.0
  capture "${session}" "${w}" "${h}" "04-device-manager"

  # Edit selected entry.
  send_keys "${session}" e
  sleep 0.2
  capture "${session}" "${w}" "${h}" "05-edit-device"

  # Close overlay and exit.
  send_keys "${session}" Escape
  sleep 0.2
  send_keys "${session}" Escape
  sleep 0.2
  send_keys "${session}" q
  sleep 0.2
  tmux kill-session -t "${session}" >/dev/null 2>&1 || true

  echo "outputs: ${OUTDIR}"
  echo "xdg state left in: ${TMPDIR}"
}

main "$@"
