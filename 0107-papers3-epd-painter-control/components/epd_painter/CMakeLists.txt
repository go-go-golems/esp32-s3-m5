idf_component_register(
    SRCS
        "src/EPD_Painter.cpp"
        "src/EPD_Painter.S"
        "src/epd_painter_powerctl.cpp"
        "src/epd_pin_driver.cpp"
    INCLUDE_DIRS "src"
    REQUIRES
        esp_driver_gpio
        esp_hw_support
        esp_timer
        esp_system
        freertos
        heap
        log
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
target_compile_definitions(${COMPONENT_LIB} PUBLIC
    EPD_PAINTER_PRESET_M5PAPER_S3=1
    EPD_PAINTER_DISABLE_BOOTCTL=1
)
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall
    -Wextra
    -Wno-missing-field-initializers
    -Werror=return-type
    -Werror=uninitialized
)
