cmake_minimum_required(VERSION 3.16)

add_definitions(-DLGFX_FORCE_LEGACY_I2C_DRIVER)

set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../../components/exercizer_control"
    "${CMAKE_CURRENT_LIST_DIR}/../../components/mqjs_service"
    "${CMAKE_CURRENT_LIST_DIR}/../../imports/esp32-mqjs-repl/mqjs-repl/components"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(m5dial_web_remote_0074)
