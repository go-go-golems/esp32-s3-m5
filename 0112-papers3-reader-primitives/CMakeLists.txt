cmake_minimum_required(VERSION 3.16)

# s3paper_core / s3paper_m5 are shared, hardware-proven components promoted to
# the repo-level components/ directory (ESP-51 Phase 1). Point at them
# explicitly rather than the whole shared directory so unrelated components
# never enter this build. The engine copy (components/mquickjs, local) stays
# per-firmware: its atom table is generated from THIS firmware's stdlib.
set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../components/s3paper_core"
    "${CMAKE_CURRENT_LIST_DIR}/../components/s3paper_m5")

# Trim the build to main + resolved dependencies. esp_psram must be NAMED:
# a trimmed COMPONENTS list silently drops its Kconfig symbols otherwise
# (ESP-50 diary Step 16) and PSRAM vanishes with only a configure-time note.
set(COMPONENTS main esp_psram)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(papers3_reader_primitives)
