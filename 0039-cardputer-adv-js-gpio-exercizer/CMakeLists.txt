cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../components/exercizer_control"
    "${CMAKE_CURRENT_LIST_DIR}/../components/exercizer_gpio"
    "${CMAKE_CURRENT_LIST_DIR}/../components/exercizer_i2c"
    "${CMAKE_CURRENT_LIST_DIR}/../imports/esp32-mqjs-repl/mqjs-repl/components"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(cardputer_adv_js_gpio_exercizer)

spiffs_create_partition_image(storage spiffs_image FLASH_IN_PROJECT)
