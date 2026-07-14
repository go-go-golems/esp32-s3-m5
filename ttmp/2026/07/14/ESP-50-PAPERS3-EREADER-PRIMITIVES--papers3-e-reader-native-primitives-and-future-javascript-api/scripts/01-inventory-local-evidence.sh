#!/usr/bin/env bash
set -euo pipefail

ROOT="$(git rev-parse --show-toplevel)"
DEMO="$(realpath "$ROOT/../M5PaperS3-UserDemo")"

printf '%s\n' '== PaperS3 firmware directories =='
find "$ROOT" -maxdepth 1 -mindepth 1 -type d -printf '%f\n' \
  | sort -V | grep -Ei 'paper|epd|eink' || true

printf '%s\n' '== Local design inputs =='
find "$HOME/Downloads" -maxdepth 1 -type f -iname '*s3paper*' -printf '%f\n' | sort

printf '%s\n' '== Firmware source inventories =='
for dir in \
  0075-papers3-touch-draw-demo \
  0076-papers3-protractor-trainer \
  0077-papers3-alphabet-graffiti \
  0078-papers3-gnosis-layout \
  0079-papers3-wamr-assemblyscript-console \
  0080-papers3-ereader \
  0082-papers3-wamr-allocator-control; do
  printf '\n-- %s --\n' "$dir"
  find "$ROOT/$dir" -maxdepth 3 -type f \
    ! -path '*/build/*' ! -path '*/managed_components/*' \
    ! -name sdkconfig ! -name sdkconfig.old ! -name dependencies.lock \
    -printf '%P\n' | sort
 done

printf '%s\n' '== Existing PaperS3 ticket documents =='
find "$ROOT/ttmp" -type f | grep -Ei 'paper|epd|eink' | sort || true

printf '%s\n' '== M5PaperS3 UserDemo state =='
for repo in "$DEMO" "$DEMO/components/M5GFX" "$DEMO/components/M5Unified"; do
  printf '\n-- %s --\n' "$repo"
  git -C "$repo" status --short --branch || true
  git -C "$repo" remote -v | head -4 || true
  git -C "$repo" log -1 --format='%H%n%cs%n%s' || true
done

printf '%s\n' '== UserDemo pins and local toolchain drift =='
rg -n 'M5GFX|M5Unified|version|branch|idf' \
  "$DEMO/repos.json" "$DEMO/dependencies.lock" "$DEMO/.envrc" "$DEMO/README.md" || true

git -C "$DEMO" diff -- dependencies.lock || true
