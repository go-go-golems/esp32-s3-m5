#!/usr/bin/env bash
set -euo pipefail

upstream="${1:-}"
if [ -z "$upstream" ]; then
  upstream="$(git rev-parse --abbrev-ref --symbolic-full-name @{upstream} 2>/dev/null || true)"
fi
if [ -z "$upstream" ]; then
  upstream="origin/main"
fi

if ! git show-ref --verify --quiet "refs/remotes/$upstream" && ! git show-ref --verify --quiet "$upstream"; then
  echo "Unable to resolve upstream '$upstream'. Set one explicitly, e.g.: ./scripts/estimate-push-size.sh origin/main" >&2
  exit 1
fi

# Normalize for local branch refs as needed.
if git show-ref --verify --quiet "refs/remotes/$upstream"; then
  upstream_ref="refs/remotes/$upstream"
elif git show-ref --verify --quiet "$upstream"; then
  upstream_ref="$upstream"
else
  echo "Upstream reference '$upstream' not found." >&2
  exit 1
fi

range="$upstream_ref..HEAD"
read -r behind ahead <<<"$(git rev-list --left-right --count "$upstream_ref...HEAD")"
echo "Branch: $(git rev-parse --abbrev-ref HEAD)"
echo "Upstream: $upstream_ref"
echo "Commits ahead: $ahead"

tmpdir="$(mktemp -d)"
pack_file="$tmpdir/push-pack"
trap 'rm -rf "$tmpdir"' EXIT

if [ "$ahead" -eq 0 ]; then
  echo "No new commits to push."
  echo "Estimated transfer (pack): 0.00 MB (0 bytes)"
  exit 0
fi

if printf '%s\n' "$range" | git pack-objects --stdout --revs >"$pack_file" 2>/dev/null; then
  pack_bytes=$(wc -c <"$pack_file")
  pack_mib=$(awk -v b="$pack_bytes" 'BEGIN { printf "%.2f", b / 1024 / 1024 }')
  echo "Estimated transfer (pack): ${pack_mib} MB (${pack_bytes} bytes)"
else
  echo "pack-objects estimate failed; falling back to raw blob-size estimate." >&2
  pack_bytes=0
  pack_mib="0.00"
fi

fallback_bytes=0
while IFS= read -r file; do
  if git cat-file -e "HEAD:$file" 2>/dev/null; then
    size=$(git cat-file -s "HEAD:$file")
    fallback_bytes=$((fallback_bytes + size))
  fi
done < <(git diff --name-only --diff-filter=AM "$range" | sort -u)

fallback_mib=$(awk -v b="$fallback_bytes" 'BEGIN { printf "%.2f", b / 1024 / 1024 }')
echo "Fallback raw added/modified file bytes: ${fallback_mib} MB (${fallback_bytes} bytes)"
