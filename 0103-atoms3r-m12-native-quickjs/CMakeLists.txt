# SPDX-License-Identifier: MIT
# 0103 — AtomS3R M12 native QuickJS firmware with PSRAM.
# Target firmware for ticket ATOMS3R-M12-NATIVE-QUICKJS.
cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../components/quickjs_native"
    "${CMAKE_CURRENT_LIST_DIR}/../components/qjs_service"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(0103-atoms3r-m12-native-quickjs)
