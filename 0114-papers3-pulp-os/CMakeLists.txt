cmake_minimum_required(VERSION 3.16)

# PULP OS v2 (ESP-51): JS-first PaperS3 firmware on the shared s3paper
# components. Point at the specific component dirs (never the whole shared
# directory) so unrelated components never enter this build. The MicroQuickJS
# engine copy arrives in Phase 5 as a LOCAL component: its atom table is
# generated from THIS firmware's stdlib and must not be shared.
set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../components/s3paper_core"
    "${CMAKE_CURRENT_LIST_DIR}/../components/s3paper_m5"
    "${CMAKE_CURRENT_LIST_DIR}/../components/s3paper_storage"
    "${CMAKE_CURRENT_LIST_DIR}/../components/s3paper_runtime")

# Trim the build to main + resolved dependencies. esp_psram must be NAMED:
# a trimmed COMPONENTS list silently drops its Kconfig symbols otherwise
# and PSRAM vanishes with only a configure-time note.
set(COMPONENTS main esp_psram)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(papers3_pulp_os)
