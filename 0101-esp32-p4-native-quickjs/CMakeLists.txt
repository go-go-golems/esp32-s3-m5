# SPDX-License-Identifier: MIT
# 0101 — ESP32-P4 native QuickJS firmware.
# Target firmware for ticket ESP32-P4-NATIVE-QUICKJS.
cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../components/quickjs_native"
    "${CMAKE_CURRENT_LIST_DIR}/../components/qjs_service"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(0101-esp32-p4-native-quickjs)
