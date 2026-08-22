# SPDX-License-Identifier: MIT
#
# gogolem_nfc Engine smoke: creates the CoreS3 internal I2C bus, attaches the
# Engine, and proves begin()/scan()/end() run on the real ST25R3916. With no
# tag present, scan returns success with an empty tag list.

cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../../components/gogolem_nfc"
    "${CMAKE_CURRENT_LIST_DIR}/../../components/gogolem_nfc_engine")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(nfc_engine_smoke)
