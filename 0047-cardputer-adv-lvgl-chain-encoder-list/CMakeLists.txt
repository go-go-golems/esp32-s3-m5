cmake_minimum_required(VERSION 3.16)

# Reuse vendored M5GFX component already present in this repo.
set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../../M5Cardputer-UserDemo/components/M5GFX"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(cardputer_adv_lvgl_chain_encoder_list)

