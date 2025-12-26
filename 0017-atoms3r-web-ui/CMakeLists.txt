cmake_minimum_required(VERSION 3.16)

# Tutorial 0017 reuses the vendored M5GFX component already present in this repo
# (from the Cardputer demo) to avoid copying it into each tutorial.
set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../../M5Cardputer-UserDemo/components/M5GFX"
    "${CMAKE_CURRENT_LIST_DIR}/../components"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(atoms3r_web_ui_0017)


