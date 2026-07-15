cmake_minimum_required(VERSION 3.16)

# Keep the flashed image independent of unrelated repository dirtiness. Hardware
# evidence identifies the source commit and binary SHA separately.
set(PROJECT_VER "esp50-p0.16-e9f3769")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(papers3_epd_density_step_response)
