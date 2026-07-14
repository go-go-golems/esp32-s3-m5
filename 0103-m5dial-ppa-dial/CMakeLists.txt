cmake_minimum_required(VERSION 3.16)

add_definitions(-DLGFX_FORCE_LEGACY_I2C_DRIVER)

set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../../M5Dial-UserDemo/components/LovyanGFX"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(m5dial_ppa_dial_0103)
