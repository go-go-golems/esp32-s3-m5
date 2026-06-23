# SPDX-License-Identifier: MIT
# 0100 — ESP32-P4 QuickJS-WASM firmware scaffold.
# Target firmware for ticket ESP32-P4-QUICKJS-WASM.
# See: ttmp/2026/06/23/ESP32-P4-QUICKJS-WASM--run-quickjs-compiled-to-wasm-on-the-esp32-p4-intern-implementation-guide/design/01-quickjs-wasm-esp32p4-analysis-design-and-implementation-guide.md
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(0100-esp32-p4-quickjs-wasm)
