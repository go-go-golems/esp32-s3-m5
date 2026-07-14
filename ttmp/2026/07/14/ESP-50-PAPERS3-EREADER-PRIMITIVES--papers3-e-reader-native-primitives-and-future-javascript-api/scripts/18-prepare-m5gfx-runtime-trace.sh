#!/usr/bin/env bash
set -euo pipefail

TICKET_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
REPO_ROOT=$(git -C "$TICKET_ROOT" rev-parse --show-toplevel)
SOURCE="$REPO_ROOT/0106-papers3-epd-qualification/.component-matrix/current"
PROJECT="$REPO_ROOT/0108-papers3-m5gfx-runtime-trace"
DEST="$PROJECT/components"
PATCH="$TICKET_ROOT/scripts/patches/18-m5gfx-runtime-trace-hooks.patch"
EXPECTED_M5GFX='ad9b814264d4e2000e9f30070002310bbccaffc9'
EXPECTED_M5UNIFIED='b1ffcc677014ed8bd01e5a1f240736ae654bfe12'
EXPECTED_LUT_SHA='d24b2df188e4261d5891a0884e2510567ea45c38bcaebeb66ade1d4f4b979af3'

for component in M5GFX M5Unified; do
  [[ -d "$SOURCE/$component/.git" ]] || { echo "error: missing clean matrix component $SOURCE/$component" >&2; exit 2; }
  [[ -z "$(git -C "$SOURCE/$component" status --porcelain)" ]] || {
    echo "error: source component is dirty: $SOURCE/$component" >&2
    exit 2
  }
done
[[ "$(git -C "$SOURCE/M5GFX" rev-parse HEAD)" == "$EXPECTED_M5GFX" ]] || { echo 'error: M5GFX SHA mismatch' >&2; exit 2; }
[[ "$(git -C "$SOURCE/M5Unified" rev-parse HEAD)" == "$EXPECTED_M5UNIFIED" ]] || { echo 'error: M5Unified SHA mismatch' >&2; exit 2; }
[[ -f "$PATCH" ]] || { echo "error: missing patch $PATCH" >&2; exit 2; }

rm -rf "$DEST"
mkdir -p "$DEST"
cp -a "$SOURCE/M5GFX" "$DEST/M5GFX"
cp -a "$SOURCE/M5Unified" "$DEST/M5Unified"
git -C "$DEST/M5GFX" reset --hard "$EXPECTED_M5GFX" >/dev/null 2>&1
git -C "$DEST/M5GFX" clean -ffd >/dev/null 2>&1
git -C "$DEST/M5GFX" apply --check "$PATCH"
git -C "$DEST/M5GFX" apply "$PATCH"

git -C "$DEST/M5GFX" diff --check
[[ -z "$(git -C "$DEST/M5Unified" status --porcelain)" ]] || { echo 'error: prepared M5Unified became dirty' >&2; exit 3; }

PATCH_SHA=$(sha256sum "$PATCH" | awk '{print $1}')
cat > "$DEST/PREPARED_MANIFEST.txt" <<EOF
prepared_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)
m5gfx_sha=$EXPECTED_M5GFX
m5unified_sha=$EXPECTED_M5UNIFIED
trace_patch_sha256=$PATCH_SHA
canonical_lut_sha256=$EXPECTED_LUT_SHA
trace_modes=off,timing
hardware_modified=no
EOF

printf 'components=%s\nm5gfx_sha=%s\nm5unified_sha=%s\npatch_sha256=%s\ncanonical_lut_sha256=%s\nhardware_modified=no\n' \
  "$DEST" "$EXPECTED_M5GFX" "$EXPECTED_M5UNIFIED" "$PATCH_SHA" "$EXPECTED_LUT_SHA"
