idf_component_register(
    SRCS
        "hello_world_main.cpp"
        "wifi_softap.cpp"
        "storage_fatfs.cpp"
        "display_app.cpp"
        "http_server.cpp"
        "uart_terminal.cpp"
        "button_input.cpp"
        "backlight.cpp"
        "display_hal.cpp"
    PRIV_REQUIRES
        spi_flash
        nvs_flash
        esp_event
        esp_netif
        esp_wifi
        fatfs
        wear_levelling
        esp_http_server
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


