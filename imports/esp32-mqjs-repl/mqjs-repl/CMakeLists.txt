# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(mqjs-repl)

# Bundle a small SPIFFS image into the firmware so `:autoload` can load real files
# on both device and QEMU (via merged flash image).
spiffs_create_partition_image(storage spiffs_image FLASH_IN_PROJECT)
