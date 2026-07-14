cmake_minimum_required(VERSION 3.16)

# Cardcore is an ESP-IDF application. Arduino-ESP32 is intentionally confined
# to meshcore_compat in later tasks; it is included here only to prove the
# selected ESP-IDF/Arduino component combination builds and starts.
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(cardcore_mesh_terminal)
