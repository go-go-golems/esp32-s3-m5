# The M5Unified component is named differently depending on how the app
# obtains it: "M5Unified" (local Arduino-style checkout, e.g. 0071) vs
# "m5stack__m5unified" (component-registry managed component, e.g. 0114).
# Apps using the registry name set SCREENSHOT_QOI_M5_COMPONENT in their
# top-level CMakeLists before project() (ESP-56).
# (env var: the requirements-expansion pass runs in a separate cmake
# process, so plain/cache variables from the project scope do not reach
# it; the environment does.)
if(DEFINED ENV{SCREENSHOT_QOI_M5_COMPONENT})
    set(SCREENSHOT_QOI_M5_COMPONENT $ENV{SCREENSHOT_QOI_M5_COMPONENT})
else()
    set(SCREENSHOT_QOI_M5_COMPONENT M5Unified)
endif()

idf_component_register(
    SRCS
        "screenshot_qoi.cpp"
    INCLUDE_DIRS
        "include"
    PRIV_REQUIRES
        esp_driver_usb_serial_jtag
    REQUIRES
        ${SCREENSHOT_QOI_M5_COMPONENT}
)
