cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED ENV{PAPERS3_COMPONENTS_DIR} OR "$ENV{PAPERS3_COMPONENTS_DIR}" STREQUAL "")
    message(FATAL_ERROR "PAPERS3_COMPONENTS_DIR must name the prepared, pinned component directory")
endif()
set(PAPERS3_COMPONENTS_DIR "$ENV{PAPERS3_COMPONENTS_DIR}")
foreach(component M5GFX M5Unified)
    if(NOT EXISTS "${PAPERS3_COMPONENTS_DIR}/${component}/CMakeLists.txt")
        message(FATAL_ERROR "${component} not found under PAPERS3_COMPONENTS_DIR=${PAPERS3_COMPONENTS_DIR}")
    endif()
endforeach()

set(EXTRA_COMPONENT_DIRS
    "${PAPERS3_COMPONENTS_DIR}/M5GFX"
    "${PAPERS3_COMPONENTS_DIR}/M5Unified")
set(PROJECT_VER "esp50-m5gfx-trace-p0.17")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(papers3_m5gfx_runtime_trace)
