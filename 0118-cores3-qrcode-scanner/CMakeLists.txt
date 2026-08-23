cmake_minimum_required(VERSION 3.16)

# ESP-62 CoreS3 + Module13.2 QRCode scanner firmware.
# ESP32-S3, IDF 5.3.4, M5Unified + M5GFX managed components.
# Pulls display + I2C-expander (PI4IOE5V6408) helpers via m5unified; the
# M14-Pro scanner engine is driven over UART1 (G17 TX / G18 RX).

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(cores3_qrcode_scanner)
