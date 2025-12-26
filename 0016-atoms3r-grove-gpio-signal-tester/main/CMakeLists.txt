idf_component_register(
    SRCS
        "hello_world_main.cpp"
        "control_plane.cpp"
        "console_repl.cpp"
        "signal_state.cpp"
        "gpio_tx.cpp"
        "gpio_rx.cpp"
        "lcd_ui.cpp"
        "backlight.cpp"
        "display_hal.cpp"
    PRIV_REQUIRES
        spi_flash
        esp_driver_gpio
        esp_driver_spi
        esp_driver_i2c
        esp_timer
        console
    REQUIRES
        M5GFX
    INCLUDE_DIRS ""
)



