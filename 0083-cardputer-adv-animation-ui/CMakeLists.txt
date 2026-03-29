cmake_minimum_required(VERSION 3.16)

# Tutorial 0083 uses the Cardputer-ADV vendor M5Unified/M5GFX stack for
# reliable display bring-up, plus the shared cardputer_kb component for
# backend-agnostic keyboard decoding.
set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../../M5Cardputer-UserDemo-ADV/components/M5GFX"
    "${CMAKE_CURRENT_LIST_DIR}/../../M5Cardputer-UserDemo-ADV/components/M5Unified"
    "${CMAKE_CURRENT_LIST_DIR}/../components/cardputer_kb"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(cardputer_adv_animation_ui_0083)
