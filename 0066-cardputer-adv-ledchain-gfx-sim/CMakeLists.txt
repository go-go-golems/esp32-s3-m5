cmake_minimum_required(VERSION 3.16)

# Tutorial 0066 uses M5GFX (LovyanGFX) as an ESP-IDF component.
set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../../M5Cardputer-UserDemo/components/M5GFX"
    "${CMAKE_CURRENT_LIST_DIR}/../components/ws281x"
    "${CMAKE_CURRENT_LIST_DIR}/../components/wifi_mgr"
    "${CMAKE_CURRENT_LIST_DIR}/../components/wifi_console"
    "${CMAKE_CURRENT_LIST_DIR}/../components/httpd_assets_embed"
    "${CMAKE_CURRENT_LIST_DIR}/../components/httpd_ws_hub"
    "${CMAKE_CURRENT_LIST_DIR}/../components/cardputer_kb"
    "${CMAKE_CURRENT_LIST_DIR}/../components/mqjs_service"
    "${CMAKE_CURRENT_LIST_DIR}/../imports/esp32-mqjs-repl/mqjs-repl/components"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(cardputer_adv_ledchain_gfx_sim_0066)
