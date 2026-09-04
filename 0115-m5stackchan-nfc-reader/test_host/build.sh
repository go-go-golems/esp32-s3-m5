#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Host unit tests for st25r_trace. No ESP-IDF required.
set -euo pipefail
cd "$(dirname "$0")/.."
SRC=main/st25r_trace/st25r_trace.c
TEST=test_host/test_st25r_trace.c
OUT=test_host/test_st25r_trace
echo "Compiling host tests..."
gcc -std=c11 -Wall -Wextra -Werror -O2 -Imain/st25r_trace "$SRC" "$TEST" -o "$OUT"
echo "Running host tests..."
"$OUT"
