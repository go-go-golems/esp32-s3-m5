# SPDX-License-Identifier: MIT
#
# gogolem_nfc_engine — synchronous Engine wrapping the pinned M5Unit-NFC
# protocol layer (ESP-61 Phase 2 wiring). Target-only: requires M5Unit-NFC.
# The pure logic lives in gogolem_nfc and stays host-testable.

idf_component_register(
    SRCS
        "src/engine.cpp"
        "src/service.cpp"
    INCLUDE_DIRS
        "include"
    REQUIRES
        gogolem_nfc
        esp_driver_i2c
        freertos
        m5stack__M5Unit-NFC
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
