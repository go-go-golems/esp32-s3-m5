cmake_minimum_required(VERSION 3.16)

if(DEFINED ENV{PAPERS3_COMPONENTS_DIR} AND NOT "$ENV{PAPERS3_COMPONENTS_DIR}" STREQUAL "")
    set(PAPERS3_COMPONENTS_DIR "$ENV{PAPERS3_COMPONENTS_DIR}")
else()
    get_filename_component(PAPERS3_COMPONENTS_DIR
        "${CMAKE_CURRENT_LIST_DIR}/../../M5PaperS3-UserDemo/components" ABSOLUTE)
endif()

if(NOT EXISTS "${PAPERS3_COMPONENTS_DIR}/M5GFX/CMakeLists.txt")
    message(FATAL_ERROR "M5GFX not found under PAPERS3_COMPONENTS_DIR=${PAPERS3_COMPONENTS_DIR}")
endif()
if(NOT EXISTS "${PAPERS3_COMPONENTS_DIR}/M5Unified/CMakeLists.txt")
    message(FATAL_ERROR "M5Unified not found under PAPERS3_COMPONENTS_DIR=${PAPERS3_COMPONENTS_DIR}")
endif()

message(STATUS "PaperS3 component root: ${PAPERS3_COMPONENTS_DIR}")
set(EXTRA_COMPONENT_DIRS
    "${PAPERS3_COMPONENTS_DIR}/M5GFX"
    "${PAPERS3_COMPONENTS_DIR}/M5Unified")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(papers3_epd_qualification)
