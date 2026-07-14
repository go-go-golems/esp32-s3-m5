cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED ENV{PAPERS3_FACTORY_COMPONENTS_DIR} OR "$ENV{PAPERS3_FACTORY_COMPONENTS_DIR}" STREQUAL "")
    message(FATAL_ERROR "PAPERS3_FACTORY_COMPONENTS_DIR must name prepared FactoryTest V0.5 components")
endif()
set(PAPERS3_FACTORY_COMPONENTS_DIR "$ENV{PAPERS3_FACTORY_COMPONENTS_DIR}")
foreach(component M5GFX M5Unified mooncake mooncake_log)
    if(NOT EXISTS "${PAPERS3_FACTORY_COMPONENTS_DIR}/${component}/CMakeLists.txt")
        message(FATAL_ERROR "${component} missing under ${PAPERS3_FACTORY_COMPONENTS_DIR}")
    endif()
endforeach()

set(EXTRA_COMPONENT_DIRS
    "${PAPERS3_FACTORY_COMPONENTS_DIR}/M5GFX"
    "${PAPERS3_FACTORY_COMPONENTS_DIR}/M5Unified"
    "${PAPERS3_FACTORY_COMPONENTS_DIR}/mooncake"
    "${PAPERS3_FACTORY_COMPONENTS_DIR}/mooncake_log")
set(PROJECT_VER "esp50-factory-v0.5-lineage")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(papers3_factory_v05_trace)
