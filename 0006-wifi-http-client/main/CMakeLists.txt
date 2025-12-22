idf_component_register(SRCS "hello_world_main.c"
                    PRIV_REQUIRES spi_flash esp_wifi esp_event esp_netif nvs_flash esp_http_client mbedtls
                    INCLUDE_DIRS "")
