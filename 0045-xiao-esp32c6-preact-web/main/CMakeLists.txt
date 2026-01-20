idf_component_register(
    SRCS
        "app_main.c"
        "wifi_mgr.c"
        "wifi_console.c"
        "http_server.c"
    PRIV_REQUIRES
        nvs_flash
        esp_event
        esp_netif
        esp_wifi
        esp_http_server
        esp_timer
        esp_driver_usb_serial_jtag
        console
    EMBED_TXTFILES
        "assets/index.html"
        "assets/assets/app.js"
        "assets/assets/app.css"
    INCLUDE_DIRS "."
)

