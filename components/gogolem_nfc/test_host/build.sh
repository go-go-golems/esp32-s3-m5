#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Host unit tests for gogolem_nfc. Compiles every test_host/test_*.cpp against
# all src/*.cpp. No ESP-IDF required.
set -euo pipefail
cd "$(dirname "$0")/.."

INC=include
SRCS=$(ls src/*.cpp)

echo "Compiling host tests..."
for t in test_host/test_*.cpp; do
    name=$(basename "$t" .cpp)
    g++ -std=c++17 -Wall -Wextra -Werror -O2 -I"$INC" $SRCS "$t" -o "test_host/$name"
done

echo "Running host tests..."
for t in test_host/test_*.cpp; do
    name=$(basename "$t" .cpp)
    "test_host/$name"
done
