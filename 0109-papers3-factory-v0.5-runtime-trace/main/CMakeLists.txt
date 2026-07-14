file(GLOB_RECURSE MY_SRCS
    "./hal/*.c"
    "./hal/*.cc"
    "./hal/*.cpp"
    "./apps/*.cpp"
)

set(MY_INCLUDE_DIRS
    "."
    "./assets"
    "./hal"
    "./apps"
)

file(GLOB EXTERN_FILES assets/*)

idf_component_register(SRCS "main.cpp" "factory_trace_runtime.cpp" ${MY_SRCS}
                    INCLUDE_DIRS ${MY_INCLUDE_DIRS}
                    EMBED_FILES ${EXTERN_FILES}
                    REQUIRES
                        M5Unified mooncake mooncake_log
                        console esp_driver_usb_serial_jtag
                        driver esp_adc fatfs esp_netif esp_wifi nvs_flash sdmmc
)
