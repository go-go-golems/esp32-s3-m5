cmake_minimum_required(VERSION 3.16)

# ESP-60-M5STACKCHAN-NFC: native ESP-IDF feature explorer for the complete
# StackChan NFC Arduino example surface. The application creates the CoreS3
# internal I2C bus and gives that handle to M5Unit-NFC's native IDF adapter.
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(m5stackchan_nfc_feature_explorer)
