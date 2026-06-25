#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
JS_DIR="$ROOT/0102-esp32-p4-visual-quickjs-repl/js"

hello_out="$(mktemp)"
dash_out="$(mktemp)"
layout_out="$(mktemp)"
hello_err="$(mktemp)"
dash_err="$(mktemp)"
layout_err="$(mktemp)"
trap 'rm -f "$hello_out" "$dash_out" "$layout_out" "$hello_err" "$dash_err" "$layout_err"' EXIT

(printf 'a'; sleep 0.2; printf 'q') \
  | "$JS_DIR/tests/run-native-host.sh" hello-native >"$hello_out" 2>"$hello_err"

(printf 'o'; sleep 0.2; printf 'q') \
  | "$JS_DIR/tests/run-native-host.sh" dashboard-native >"$dash_out" 2>"$dash_err"

(sleep 0.2; printf 'q') \
  | "$JS_DIR/tests/run-native-host.sh" layout-native >"$layout_out" 2>"$layout_err"

if [[ -s "$hello_err" || -s "$dash_err" || -s "$layout_err" ]]; then
  cat "$hello_err" "$dash_err" "$layout_err" >&2
  exit 1
fi

grep -qa 'last key: a' "$hello_out"
grep -qa 'launch -> notes' "$dash_out"
grep -qa 'layout regions' "$layout_out"

echo "PASS native hello-native"
echo "PASS native dashboard-native"
echo "PASS native layout-native"
