cmake_minimum_required(VERSION 3.16)

# gogolem_nfc feature explorer — the migrated 0117 using the reusable component.
# Console, NVS mode persistence, and demo data live in the example layer; the
# component owns NFC protocol logic.

set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../../components/gogolem_nfc"
    "${CMAKE_CURRENT_LIST_DIR}/../../components/gogolem_nfc_engine")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(nfc_feature_explorer)
