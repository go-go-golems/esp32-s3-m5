idf_component_register(SRCS "hello_world_main.c"
                    PRIV_REQUIRES spi_flash esp_driver_gpio esp_driver_usb_serial_jtag
                    INCLUDE_DIRS "")
