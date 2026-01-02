cmake_minimum_required(VERSION 3.16)

# Calibrator firmware uses M5GFX (LovyanGFX) as an ESP-IDF component.
set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../../M5Cardputer-UserDemo/components/M5GFX"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(cardputer_kb_scancode_calibrator)

