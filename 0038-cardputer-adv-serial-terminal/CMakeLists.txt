cmake_minimum_required(VERSION 3.16)

# Tutorial 0038 uses M5Unified + M5GFX from the vendor ADV demo tree so the
# Cardputer-ADV display backlight/power comes up reliably.
set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../../M5Cardputer-UserDemo-ADV/components/M5GFX"
    "${CMAKE_CURRENT_LIST_DIR}/../../M5Cardputer-UserDemo-ADV/components/M5Unified"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(cardputer_adv_serial_terminal)
