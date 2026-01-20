#!/usr/bin/env bash
set -euo pipefail

ROOT="${1:-.}"

say() { printf "\n== %s ==\n" "$1"; }

say "Core files (known-good example)"
printf "%s\n" \
  "docs/playbook-embedded-preact-zustand-webui.md" \
  "0017-atoms3r-web-ui/web/vite.config.ts" \
  "0017-atoms3r-web-ui/main/CMakeLists.txt" \
  "0017-atoms3r-web-ui/main/http_server.cpp" \
  "0017-atoms3r-web-ui/web/src/store/ws.ts"

if command -v docmgr >/dev/null 2>&1; then
  say "docmgr hits"
  (cd "$ROOT" && docmgr doc search --query "preact" || true)
  (cd "$ROOT" && docmgr doc search --query "zustand" || true)
  (cd "$ROOT" && docmgr doc search --query "deterministic filenames" || true)
else
  say "docmgr not found (skipping)"
fi

if command -v rg >/dev/null 2>&1; then
  say "rg hits (implementation jump points)"
  (cd "$ROOT" && rg -n -S "outDir: '../main/assets'|inlineDynamicImports|entryFileNames: 'assets/app.js'|EMBED_TXTFILES|/assets/app.js|/ws" 0017-atoms3r-web-ui || true)
else
  say "rg not found (skipping)"
fi

