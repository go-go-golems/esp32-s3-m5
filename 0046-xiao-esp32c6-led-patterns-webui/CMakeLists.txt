cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../components/wifi_mgr"
    "${CMAKE_CURRENT_LIST_DIR}/../components/wifi_console"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(xiao_esp32c6_led_patterns_webui_0046)
