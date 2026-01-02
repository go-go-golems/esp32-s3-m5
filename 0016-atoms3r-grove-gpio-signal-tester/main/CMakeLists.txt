idf_component_register(
    SRCS
        "hello_world_main.cpp"
        "control_plane.cpp"
        "manual_repl.cpp"
        "signal_state.cpp"
        "gpio_tx.cpp"
        "gpio_rx.cpp"
        "uart_tester.cpp"
        "lcd_ui.cpp"
        "backlight.cpp"
        "display_hal.cpp"
    PRIV_REQUIRES
        spi_flash
        esp_driver_gpio
        esp_driver_spi
        esp_driver_i2c
        esp_driver_uart
        esp_driver_usb_serial_jtag
        esp_timer
    REQUIRES
        M5GFX
    INCLUDE_DIRS ""
)



