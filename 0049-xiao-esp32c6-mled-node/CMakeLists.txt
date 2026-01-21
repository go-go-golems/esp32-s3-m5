cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../components/mled_node"
    "${CMAKE_CURRENT_LIST_DIR}/../components/ws281x"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(xiao_esp32c6_mled_node_0049)
