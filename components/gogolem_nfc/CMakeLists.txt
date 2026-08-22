# SPDX-License-Identifier: MIT
#
# gogolem_nfc — reusable native ESP-IDF NFC component (ESP-61).
#
# Phase 1: dependency-free foundation (types, Result, version).
# Phase 2: host-testable pure logic (lifecycle, safety). The M5Unit-NFC-wrapped
# synchronous Engine is added when its hardware validation is available.

idf_component_register(
    SRCS
        "src/gogolem_nfc.cpp"
        "src/lifecycle.cpp"
        "src/safety.cpp"
    INCLUDE_DIRS
        "include"
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
