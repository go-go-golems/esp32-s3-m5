# SPDX-License-Identifier: MIT
#
# Minimal ESP-IDF smoke project for gogolem_nfc Phase 1.
# Proves the component is discoverable, buildable, and linkable under ESP-IDF
# 5.5.4 without touching any existing tutorial project.

cmake_minimum_required(VERSION 3.16)

# Point only at gogolem_nfc to avoid pulling unrelated shared components whose
# managed dependencies are not declared in this minimal project.
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/../../components/gogolem_nfc")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(nfc_types_smoke)
