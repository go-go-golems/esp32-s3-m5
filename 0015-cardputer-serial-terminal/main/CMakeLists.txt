idf_component_register(
    SRCS "hello_world_main.cpp"
    PRIV_REQUIRES spi_flash esp_driver_gpio esp_timer esp_driver_uart esp_driver_usb_serial_jtag
    REQUIRES M5GFX
    INCLUDE_DIRS ""
)



