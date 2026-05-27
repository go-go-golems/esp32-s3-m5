cmake_minimum_required(VERSION 3.16)

# No partition overrides — CoreS3 has 16 MB flash, default table works
# set(IDF_PARTITION_TABLE_CUSTOM YES)
# set(IDF_PARTITION_TABLE_CUSTOM_CSV "${CMAKE_CURRENT_SOURCE_DIR}/partitions/partitions_2m_app.csv")
# set(IDF_PARTITION_TABLE_OFFSET 0x8000)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(cores3_wifi_bench)
