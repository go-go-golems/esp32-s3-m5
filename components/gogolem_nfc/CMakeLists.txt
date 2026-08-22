# SPDX-License-Identifier: MIT
#
# gogolem_nfc — reusable native ESP-IDF NFC component (ESP-61).
#
# Phase 1: dependency-free foundation (types, Result, version). No M5Unit-NFC
# dependency yet; that is added in Phase 2 when the synchronous Engine wraps
# the upstream protocol layer.

idf_component_register(
    SRCS
        "src/gogolem_nfc.cpp"
    INCLUDE_DIRS
        "include"
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
