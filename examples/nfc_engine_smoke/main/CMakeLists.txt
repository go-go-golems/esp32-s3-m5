# SPDX-License-Identifier: MIT
idf_component_register(
    SRCS "smoke_main.cpp"
    INCLUDE_DIRS "."
    REQUIRES gogolem_nfc gogolem_nfc_engine esp_driver_i2c freertos
)
