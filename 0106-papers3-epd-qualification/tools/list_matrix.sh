#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
printf '%-5s %-8s %-9s %-10s %-11s %s\n' CELL IDF PROFILE M5GFX M5UNIFIED PURPOSE
while IFS=$'\t' read -r cell idf profile gfx unified purpose; do
  [[ "$cell" == \#* ]] && continue
  printf '%-5s %-8s %-9s %-10s %-11s %s\n' "$cell" "$idf" "$profile" "$gfx" "$unified" "$purpose"
done < "$PROJECT_DIR/matrix/cells.tsv"

printf '\nInstalled ESP-IDF versions:\n'
find "$HOME/esp" -maxdepth 1 -mindepth 1 -type d -name 'esp-idf-*' -printf '  %f\n' | sort -V
