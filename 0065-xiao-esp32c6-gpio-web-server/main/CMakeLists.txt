idf_component_register(
    SRCS
        "app_main.c"
        "gpio_ctl.c"
        "http_server.c"
    PRIV_REQUIRES
        httpd_assets_embed
        wifi_mgr
        wifi_console
        nvs_flash
        esp_event
        esp_netif
        esp_wifi
        esp_http_server
        driver
        esp_driver_usb_serial_jtag
        console
        json
    EMBED_TXTFILES
        "assets/index.html"
    INCLUDE_DIRS "."
)

