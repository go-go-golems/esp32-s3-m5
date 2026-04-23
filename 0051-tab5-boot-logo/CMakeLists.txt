cmake_minimum_required(VERSION 3.16)

# Override the partition table to a 2 MB factory app partition.
# The default single-app partition is only 1 MB; this firmware (display + LVGL +
# ESP-Hosted Wi-Fi + m5stack_tab5 BSP) is ~1.8 MB and needs the extra space.
set(IDF_PARTITION_TABLE_CUSTOM YES)
set(IDF_PARTITION_TABLE_CUSTOM_CSV "${CMAKE_CURRENT_SOURCE_DIR}/partitions/partitions_2m_app.csv")
set(IDF_PARTITION_TABLE_OFFSET 0x8000)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(tab5_boot_logo)
