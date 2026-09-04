cmake_minimum_required(VERSION 3.16)

# ESP-60-M5STACKCHAN-NFC Phase 1: standalone NFC reader for the M5StackChan.
# Uses the same I2C bus as the StackChan firmware (port 1, SDA=GPIO12, SCL=GPIO11).
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(m5stackchan_nfc_reader)
