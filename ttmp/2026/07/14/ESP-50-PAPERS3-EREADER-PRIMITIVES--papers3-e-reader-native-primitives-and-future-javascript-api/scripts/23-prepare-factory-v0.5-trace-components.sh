#!/usr/bin/env bash
set -euo pipefail

TICKET_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
REPO_ROOT=$(git -C "$TICKET_ROOT" rev-parse --show-toplevel)
PROJECT="$REPO_ROOT/0109-papers3-factory-v0.5-runtime-trace"
MATRIX="$REPO_ROOT/0106-papers3-epd-qualification/.component-matrix/legacy"
FACTORY_SIBLING="$REPO_ROOT/../M5PaperS3-UserDemo"
DEST="$PROJECT/.components"
PATCH="$TICKET_ROOT/scripts/patches/23-m5gfx-0.2.15-factory-runtime-trace.patch"
M5GFX_SHA='c6f92dc03226cdc04d67c705a2020f62ad21ad01'
M5UNIFIED_SHA='5580ff6923a868cc71d5b30c962186bde2c85b67'
MOONCAKE_SHA='0dfc177b72fcc55095fd222b0bdc69595b001d9f'
MOONCAKE_LOG_SHA='fe97aafe5432a9f22c1414b45b59fad37a4bf27e'

for spec in \
  "$MATRIX/M5GFX:$M5GFX_SHA" \
  "$MATRIX/M5Unified:$M5UNIFIED_SHA" \
  "$FACTORY_SIBLING/components/mooncake:$MOONCAKE_SHA" \
  "$FACTORY_SIBLING/components/mooncake_log:$MOONCAKE_LOG_SHA"; do
  path=${spec%%:*}; expected=${spec##*:}
  [[ -d "$path/.git" ]] || { echo "error: missing component $path" >&2; exit 2; }
  [[ "$(git -C "$path" rev-parse HEAD)" == "$expected" ]] || { echo "error: SHA mismatch for $path" >&2; exit 2; }
  [[ -z "$(git -C "$path" status --porcelain)" ]] || { echo "error: dirty component $path" >&2; exit 2; }
done
[[ -f "$PATCH" ]] || { echo "error: missing patch $PATCH" >&2; exit 2; }

rm -rf "$DEST"
for profile in clean trace; do
  mkdir -p "$DEST/$profile"
  cp -a "$MATRIX/M5GFX" "$DEST/$profile/M5GFX"
  cp -a "$MATRIX/M5Unified" "$DEST/$profile/M5Unified"
  cp -a "$FACTORY_SIBLING/components/mooncake" "$DEST/$profile/mooncake"
  cp -a "$FACTORY_SIBLING/components/mooncake_log" "$DEST/$profile/mooncake_log"
done

git -C "$DEST/trace/M5GFX" apply --check "$PATCH"
git -C "$DEST/trace/M5GFX" apply "$PATCH"
git -C "$DEST/trace/M5GFX" apply --reverse --check "$PATCH"
git -C "$DEST/trace/M5GFX" diff --check

cat > "$DEST/PREPARED_MANIFEST.txt" <<EOF
prepared_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)
factory_source_commit=5e275ad4b70abb85f7193fda137844730e64c4db
idf_tag=v5.3.3
idf_commit=6db3dc25df7325c1c81b7cd7d4e42babff7a818e
m5gfx_sha=$M5GFX_SHA
m5unified_sha=$M5UNIFIED_SHA
mooncake_sha=$MOONCAKE_SHA
mooncake_log_sha=$MOONCAKE_LOG_SHA
trace_patch_sha256=$(sha256sum "$PATCH" | awk '{print $1}')
hardware_modified=no
EOF
printf 'components=%s\npatch_sha256=%s\nhardware_modified=no\n' "$DEST" "$(sha256sum "$PATCH" | awk '{print $1}')"
