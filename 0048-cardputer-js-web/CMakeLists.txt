cmake_minimum_required(VERSION 3.16)

# 0048: Cardputer Web JS IDE (esp_http_server + MicroQuickJS).
#
# Pull in the MicroQuickJS component from the imports tree.
set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../components/httpd_assets_embed"
    "${CMAKE_CURRENT_LIST_DIR}/../components/httpd_ws_hub"
    "${CMAKE_CURRENT_LIST_DIR}/../components/mqjs_service"
    "${CMAKE_CURRENT_LIST_DIR}/../components/wifi_mgr"
    "${CMAKE_CURRENT_LIST_DIR}/../components/wifi_console"
    "${CMAKE_CURRENT_LIST_DIR}/../components/exercizer_control"
    "${CMAKE_CURRENT_LIST_DIR}/../imports/esp32-mqjs-repl/mqjs-repl/components"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(cardputer_js_web_0048)
