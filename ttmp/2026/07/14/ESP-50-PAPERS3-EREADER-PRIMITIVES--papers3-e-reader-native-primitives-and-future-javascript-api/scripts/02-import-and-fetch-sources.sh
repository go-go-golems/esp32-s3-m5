#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TICKET_DIR="$(dirname "$SCRIPT_DIR")"
LOCAL_DIR="$TICKET_DIR/sources/local"
WEB_DIR="$TICKET_DIR/sources/web"
mkdir -p "$LOCAL_DIR" "$WEB_DIR"

# The prompt repeated s3paper-api-design.md twice but said "files". The only
# companion s3paper file in Downloads was s3paper-studio.jsx, so preserve both.
cp "$HOME/Downloads/s3paper-api-design.md" "$LOCAL_DIR/s3paper-api-design.md"
cp "$HOME/Downloads/s3paper-studio.jsx" "$LOCAL_DIR/s3paper-studio.jsx"

fetch() {
  local url="$1"
  local output="$2"
  local tmp
  tmp="$(mktemp)"
  printf 'defuddle: %s -> %s\n' "$url" "$output"
  if defuddle parse "$url" --md -o "$tmp"; then
    mv "$tmp" "$WEB_DIR/$output"
    return 0
  fi

  # Defuddle can emit useful content and then fail while parsing GitHub
  # metadata. Keep substantial output, but reject tiny error-only files.
  if [[ -s "$tmp" ]] && (( $(wc -c < "$tmp") >= 500 )); then
    printf 'warning: parser returned non-zero but emitted useful content\n' >&2
    mv "$tmp" "$WEB_DIR/$output"
    return 0
  fi
  rm -f "$tmp"
  printf 'error: no useful extraction for %s\n' "$url" >&2
  return 1
}

fetch 'https://docs.m5stack.com/en/core/PaperS3' \
  '01-m5stack-papers3-hardware.md'
fetch 'https://github.com/m5stack/M5PaperS3-UserDemo' \
  '02-m5papers3-userdemo.md'
fetch 'https://github.com/m5stack/M5GFX/blob/master/docs/M5PaperS3.md' \
  '03-m5gfx-papers3-driver.md'
fetch 'https://github.com/m5stack/M5GFX/issues/181' \
  '04-m5gfx-issue-181-panel-epd-heap-corruption.md'
fetch 'https://github.com/m5stack/M5GFX/issues/152' \
  '05-m5gfx-issue-152-waveform-and-ghosting.md'
fetch 'https://github.com/bellard/mquickjs' \
  '06-mquickjs-readme.md'
fetch 'https://github.com/atomic14/diy-esp32-epub-reader' \
  '07-diy-esp32-epub-reader.md'
fetch 'https://www.atomic14.com/videos/posts/VLiCgB0odOQ' \
  '08-diy-ebook-reader-article.md'
fetch 'https://crossink.uxj.io/contributing/architecture.html' \
  '09-crossink-architecture.md'
fetch 'https://docs.m5stack.com/en/arduino/m5papers3/touch' \
  '10-m5stack-papers3-touch.md'
fetch 'https://github.com/m5stack/M5GFX/releases/' \
  '11-m5gfx-releases.md'

wc -lc "$LOCAL_DIR"/* "$WEB_DIR"/*
