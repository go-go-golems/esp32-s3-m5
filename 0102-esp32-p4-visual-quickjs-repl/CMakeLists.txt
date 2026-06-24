# SPDX-License-Identifier: MIT
# 0102 — ESP32-P4 visual QuickJS REPL firmware.
cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../components/quickjs_native"
    "${CMAKE_CURRENT_LIST_DIR}/../components/qjs_service"
    "${CMAKE_CURRENT_LIST_DIR}/../components/picocalc_lcd"
    "${CMAKE_CURRENT_LIST_DIR}/../components/picocalc_keyboard"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(0102-esp32-p4-visual-quickjs-repl)
