cmake_minimum_required(VERSION 3.16)

add_definitions(-DLGFX_FORCE_LEGACY_I2C_DRIVER)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(m5dial_web_remote_0074)
