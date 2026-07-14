#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 || ! "$1" =~ ^[A-D]$ ]]; then
  echo "usage: $0 A|B|C|D" >&2
  exit 2
fi

CELL="$1"
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
RESULT_DIR="$PROJECT_DIR/matrix-results"
mkdir -p "$RESULT_DIR"

case "$CELL" in
  A) IDF_VERSION=5.3.3; PROFILE=legacy ;;
  B) IDF_VERSION=5.3.3; PROFILE=current ;;
  C) IDF_VERSION=5.3.4; PROFILE=current ;;
  D) IDF_VERSION=5.4.2; PROFILE=current ;;
esac

IDF_DIR="$HOME/esp/esp-idf-$IDF_VERSION"
COMPONENTS_DIR="$PROJECT_DIR/.component-matrix/$PROFILE"
if [[ ! -f "$IDF_DIR/export.sh" ]]; then
  printf 'error: ESP-IDF %s is not installed at %s\n' "$IDF_VERSION" "$IDF_DIR" >&2
  printf 'install the exact toolchain before building matrix cell %s\n' "$CELL" >&2
  exit 1
fi
if [[ ! -d "$COMPONENTS_DIR/M5GFX/.git" || ! -d "$COMPONENTS_DIR/M5Unified/.git" ]]; then
  printf 'error: matrix components are missing; run tools/prepare_matrix_components.sh\n' >&2
  exit 1
fi
if [[ -n "$(git -C "$COMPONENTS_DIR/M5GFX" status --porcelain)" ||
      -n "$(git -C "$COMPONENTS_DIR/M5Unified" status --porcelain)" ]]; then
  printf 'error: refusing to build with dirty matrix component checkouts\n' >&2
  exit 1
fi

export PAPERS3_MATRIX_CELL="$CELL"
export PAPERS3_COMPONENTS_DIR="$COMPONENTS_DIR"
export PAPERS3_M5GFX_SHA="$(git -C "$COMPONENTS_DIR/M5GFX" rev-parse HEAD)"
export PAPERS3_M5UNIFIED_SHA="$(git -C "$COMPONENTS_DIR/M5Unified" rev-parse HEAD)"

# IDF export scripts may inspect an inherited environment from another IDF.
unset IDF_PYTHON_ENV_PATH IDF_TOOLS_PATH IDF_VERSION IDF_PATH
# shellcheck disable=SC1090
source "$IDF_DIR/export.sh" >/dev/null 2>&1

BUILD_DIR="$PROJECT_DIR/build-cell-$CELL"
SDKCONFIG="$PROJECT_DIR/sdkconfig.cell-$CELL"
METADATA="$RESULT_DIR/build-cell-$CELL.txt"

{
  printf 'cell=%s\n' "$CELL"
  printf 'captured_at=%s\n' "$(date --iso-8601=seconds)"
  printf 'idf=%s\n' "$(idf.py --version)"
  printf 'idf_path=%s\n' "$IDF_PATH"
  printf 'component_profile=%s\n' "$PROFILE"
  printf 'm5gfx_tag=%s\n' "$(git -C "$COMPONENTS_DIR/M5GFX" describe --tags --exact-match)"
  printf 'm5gfx_sha=%s\n' "$PAPERS3_M5GFX_SHA"
  printf 'm5unified_tag=%s\n' "$(git -C "$COMPONENTS_DIR/M5Unified" describe --tags --exact-match)"
  printf 'm5unified_sha=%s\n' "$PAPERS3_M5UNIFIED_SHA"
  printf 'build_dir=%s\n' "$BUILD_DIR"
  printf 'sdkconfig=%s\n' "$SDKCONFIG"
} | tee "$METADATA"

LOG="$RESULT_DIR/build-cell-$CELL.log"
echo "[build] matrix cell $CELL; full output -> $LOG"
set +e
{
  if [[ ! -f "$SDKCONFIG" ]]; then
    idf.py -C "$PROJECT_DIR" -B "$BUILD_DIR" -D "SDKCONFIG=$SDKCONFIG" set-target esp32s3
  fi
  idf.py -C "$PROJECT_DIR" -B "$BUILD_DIR" -D "SDKCONFIG=$SDKCONFIG" build
} >"$LOG" 2>&1
status=$?
set -e
perl -pi -e 's/[ \t]+$//' "$LOG"
if [[ $status -ne 0 ]]; then
  echo "error: matrix cell $CELL failed (exit $status); filtered tail:" >&2
  tail -n 80 "$LOG" | cut -c1-500 >&2
  exit "$status"
fi
APP="$BUILD_DIR/papers3_epd_qualification.bin"
ELF="$BUILD_DIR/papers3_epd_qualification.elf"
printf '[build] cell=%s complete app_sha256=%s elf_sha256=%s hardware_modified=no\n' \
  "$CELL" "$(sha256sum "$APP" | awk '{print $1}')" "$(sha256sum "$ELF" | awk '{print $1}')"
