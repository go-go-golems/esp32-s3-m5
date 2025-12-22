idf_component_register(SRCS "hello_world_main.c"
                    PRIV_REQUIRES spi_flash esp_lcd esp_driver_gpio esp_driver_spi
                    INCLUDE_DIRS "")
