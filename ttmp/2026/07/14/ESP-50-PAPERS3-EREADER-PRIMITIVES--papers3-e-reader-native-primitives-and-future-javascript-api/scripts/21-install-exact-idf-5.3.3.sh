#!/usr/bin/env bash
set -euo pipefail

TICKET_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
DEST='/home/manuel/esp/esp-idf-5.3.3'
EXPECTED_TAG='v5.3.3'
EXPECTED_COMMIT='6db3dc25df7325c1c81b7cd7d4e42babff7a818e'
OUTPUT="$TICKET_ROOT/scripts/output"
MODE=${1:---check}
STAMP=$(date -u +%Y%m%dT%H%M%SZ)
LOG="$OUTPUT/21-idf-5.3.3-install-$STAMP.log"
mkdir -p "$OUTPUT"

case "$MODE" in
  --check|--execute) ;;
  *) echo "usage: $0 [--check|--execute]" >&2; exit 2 ;;
esac

verify() {
  [[ -f "$DEST/export.sh" ]] || return 1
  [[ "$(git -C "$DEST" rev-parse HEAD)" == "$EXPECTED_COMMIT" ]] || return 1
  [[ "$(git -C "$DEST" describe --tags --exact-match)" == "$EXPECTED_TAG" ]] || return 1
  (
    unset IDF_PATH IDF_VERSION IDF_PYTHON_ENV_PATH
    # shellcheck disable=SC1091
    source "$DEST/export.sh" >/dev/null 2>&1
    [[ "$(idf.py --version)" == 'ESP-IDF v5.3.3' ]]
  )
}

if [[ "$MODE" == --check ]]; then
  if verify; then
    printf 'installed=yes\npath=%s\ntag=%s\ncommit=%s\nidf=ESP-IDF v5.3.3\n' "$DEST" "$EXPECTED_TAG" "$EXPECTED_COMMIT"
    exit 0
  fi
  printf 'installed=no\npath=%s\nexpected_tag=%s\nexpected_commit=%s\n' "$DEST" "$EXPECTED_TAG" "$EXPECTED_COMMIT"
  exit 1
fi

if [[ -e "$DEST" ]]; then
  if [[ ! -d "$DEST/.git" ]] || [[ "$(git -C "$DEST" rev-parse HEAD 2>/dev/null || true)" != "$EXPECTED_COMMIT" ]]; then
    echo "error: refusing to replace non-matching path $DEST" >&2
    exit 3
  fi
fi

if [[ ! -d "$DEST/.git" ]]; then
  echo "[install] cloning exact $EXPECTED_TAG; full output -> $LOG"
  set +e
  git clone --branch "$EXPECTED_TAG" --depth 1 --recursive --shallow-submodules \
    https://github.com/espressif/esp-idf.git "$DEST" >"$LOG" 2>&1
  status=$?
  set -e
  if [[ $status -ne 0 ]]; then
    echo "error: clone failed (exit $status); filtered tail:" >&2
    tail -n 60 "$LOG" | cut -c1-500 >&2
    exit "$status"
  fi
else
  printf 'existing_checkout=%s\n' "$DEST" >"$LOG"
fi

actual=$(git -C "$DEST" rev-parse HEAD)
[[ "$actual" == "$EXPECTED_COMMIT" ]] || { echo "error: expected $EXPECTED_COMMIT, got $actual" >&2; exit 4; }

echo "[install] installing ESP32-S3 tools/Python environment; output appended -> $LOG"
set +e
(
  unset IDF_PATH IDF_VERSION IDF_PYTHON_ENV_PATH
  cd "$DEST"
  ./install.sh esp32s3
) >>"$LOG" 2>&1
status=$?
set -e
perl -pi -e 's/[ \t]+$//' "$LOG"
if [[ $status -ne 0 ]]; then
  echo "error: install failed (exit $status); filtered tail:" >&2
  tail -n 80 "$LOG" | cut -c1-500 >&2
  exit "$status"
fi
verify || { echo "error: post-install verification failed; see $LOG" >&2; exit 5; }

printf 'installed=yes\npath=%s\ntag=%s\ncommit=%s\nidf=ESP-IDF v5.3.3\nlog=%s\nhardware_modified=no\n' \
  "$DEST" "$EXPECTED_TAG" "$EXPECTED_COMMIT" "$LOG"
