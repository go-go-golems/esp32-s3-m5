# SPDX-License-Identifier: MIT
#
# gogolem_nfc — reusable native ESP-IDF NFC component (ESP-61).
#
# Host-testable pure-logic layers (no M5Unit-NFC dependency):
#   Phase 1: types, Result, version
#   Phase 2 (pure): lifecycle, safety, PICC→TagInfo conversion
#   Phase 3: NDEF public model and codec
#   Phase 6: mutation permits and write reports
#   Phase 8: MIFARE Classic value-block codec and credentials
#
# The M5Unit-NFC-wrapped synchronous Engine (target only) is added in the
# Engine wiring step; the worker Service and emulation follow.

idf_component_register(
    SRCS
        "src/gogolem_nfc.cpp"
        "src/lifecycle.cpp"
        "src/safety.cpp"
        "src/ndef.cpp"
        "src/mutation.cpp"
        "src/classic.cpp"
        "src/picc_map.cpp"
    INCLUDE_DIRS
        "include"
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
