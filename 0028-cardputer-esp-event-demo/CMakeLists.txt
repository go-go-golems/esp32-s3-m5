cmake_minimum_required(VERSION 3.16)

# Tutorial 0028 uses M5GFX (LovyanGFX) and the reusable Cardputer keyboard component.
set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../../M5Cardputer-UserDemo/components/M5GFX"
    "${CMAKE_CURRENT_LIST_DIR}/../components/cardputer_kb"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(cardputer_esp_event_demo)

