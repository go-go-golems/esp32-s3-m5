# M5Dial Dithered 3D Scene Viewer
cmake_minimum_required(VERSION 3.16)

# Match the working 0074 M5Dial firmware setup.  Do not set COMPONENT_DIRS:
# that overrides ESP-IDF's built-in component search path and hides IDF
# components such as nvs_flash.
#
# Reuse the LovyanGFX component already tracked by the 0074 reference firmware
# instead of vendoring a second full copy into this ticket project.
set(EXTRA_COMPONENT_DIRS "../0074-m5dial-web-remote/firmware/components/LovyanGFX")
add_definitions(-DLGFX_FORCE_LEGACY_I2C_DRIVER)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(0096-m5dial-dithered-3d)
