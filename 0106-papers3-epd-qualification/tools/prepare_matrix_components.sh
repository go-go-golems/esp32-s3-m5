#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MATRIX_DIR="$PROJECT_DIR/.component-matrix"

clone_exact() {
  local url="$1" tag="$2" dest="$3"
  if [[ ! -d "$dest/.git" ]]; then
    git clone --depth 1 --branch "$tag" "$url" "$dest"
  fi
  local actual_tag
  actual_tag="$(git -C "$dest" describe --tags --exact-match 2>/dev/null || true)"
  if [[ "$actual_tag" != "$tag" ]]; then
    printf 'error: %s is at tag %s, expected %s\n' "$dest" "${actual_tag:-<none>}" "$tag" >&2
    exit 1
  fi
  if [[ -n "$(git -C "$dest" status --porcelain)" ]]; then
    printf 'error: component checkout is dirty: %s\n' "$dest" >&2
    exit 1
  fi
}

mkdir -p "$MATRIX_DIR/legacy" "$MATRIX_DIR/current"
clone_exact https://github.com/m5stack/M5GFX.git 0.2.15 "$MATRIX_DIR/legacy/M5GFX"
clone_exact https://github.com/m5stack/M5Unified.git 0.2.10 "$MATRIX_DIR/legacy/M5Unified"
clone_exact https://github.com/m5stack/M5GFX.git 0.2.25 "$MATRIX_DIR/current/M5GFX"
clone_exact https://github.com/m5stack/M5Unified.git 0.2.18 "$MATRIX_DIR/current/M5Unified"

for profile in legacy current; do
  printf '%s\n' "profile=$profile"
  printf 'm5gfx_tag=%s\n' "$(git -C "$MATRIX_DIR/$profile/M5GFX" describe --tags --exact-match)"
  printf 'm5gfx_sha=%s\n' "$(git -C "$MATRIX_DIR/$profile/M5GFX" rev-parse HEAD)"
  printf 'm5unified_tag=%s\n' "$(git -C "$MATRIX_DIR/$profile/M5Unified" describe --tags --exact-match)"
  printf 'm5unified_sha=%s\n' "$(git -C "$MATRIX_DIR/$profile/M5Unified" rev-parse HEAD)"
done
