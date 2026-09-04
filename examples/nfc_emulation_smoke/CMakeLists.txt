cmake_minimum_required(VERSION 3.16)
set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../../components/gogolem_nfc"
    "${CMAKE_CURRENT_LIST_DIR}/../../components/gogolem_nfc_engine")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(nfc_emulation_smoke)
