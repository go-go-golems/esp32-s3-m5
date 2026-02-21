cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../components/httpd_assets_embed"
    "${CMAKE_CURRENT_LIST_DIR}/../components/mqjs_service"
    "${CMAKE_CURRENT_LIST_DIR}/../components/wifi_mgr"
    "${CMAKE_CURRENT_LIST_DIR}/../components/wifi_console"
    "${CMAKE_CURRENT_LIST_DIR}/../imports/esp32-mqjs-repl/mqjs-repl/components"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp_c3_led_matrix_http_0067)
