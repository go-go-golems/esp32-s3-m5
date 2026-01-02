cmake_minimum_required(VERSION 3.16)

# Demo suite uses M5GFX (LovyanGFX) as an ESP-IDF component.
# We reuse the vendored component already present in this repo (from the Cardputer demo)
# to avoid copying it into each tutorial.
set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../../M5Cardputer-UserDemo/components/M5GFX"
    "${CMAKE_CURRENT_LIST_DIR}/../components/cardputer_kb"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(cardputer_m5gfx_demo_suite)
