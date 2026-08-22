#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Host unit tests for gogolem_nfc (types, Result, lifecycle, safety).
# No ESP-IDF required.
set -euo pipefail
cd "$(dirname "$0")/.."

SRC="src/gogolem_nfc.cpp src/lifecycle.cpp src/safety.cpp"
INC=include

echo "Compiling host tests..."
g++ -std=c++17 -Wall -Wextra -Werror -O2 -I"$INC" $SRC test_host/test_types.cpp    -o test_host/test_types
g++ -std=c++17 -Wall -Wextra -Werror -O2 -I"$INC" $SRC test_host/test_result.cpp  -o test_host/test_result
g++ -std=c++17 -Wall -Wextra -Werror -O2 -I"$INC" $SRC test_host/test_lifecycle.cpp -o test_host/test_lifecycle
g++ -std=c++17 -Wall -Wextra -Werror -O2 -I"$INC" $SRC test_host/test_safety.cpp   -o test_host/test_safety

echo "Running host tests..."
test_host/test_types
test_host/test_result
test_host/test_lifecycle
test_host/test_safety
