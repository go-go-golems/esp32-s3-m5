cmake_minimum_required(VERSION 3.16)

# 0071: Cardputer-ADV photo development timer.
set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../../M5Cardputer-UserDemo-ADV/components/M5GFX"
    "${CMAKE_CURRENT_LIST_DIR}/../../M5Cardputer-UserDemo-ADV/components/M5Unified"
    "${CMAKE_CURRENT_LIST_DIR}/../components/httpd_assets_embed"
    "${CMAKE_CURRENT_LIST_DIR}/../components/wifi_mgr"
    "${CMAKE_CURRENT_LIST_DIR}/../components/wifi_console"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(cardputer_adv_photo_timer_0071)
