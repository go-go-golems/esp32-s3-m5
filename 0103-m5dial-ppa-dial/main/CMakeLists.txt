idf_component_register(
    SRCS
        "app_main.cpp"
        "lvgl_port_m5dial.cpp"
        "m5dial_board.cpp"
        "config_store.cpp"
        "scene_model.cpp"
        "ppa_proto.c"
        "ppa_client.cpp"
        "wifi_mgr.cpp"
        "web_setup.cpp"
        "ui_screens.cpp"
        "buzzer.cpp"
        "/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5Dial-UserDemo/main/hal/utils/Button/Button.cpp"
        "/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5Dial-UserDemo/main/hal/utils/ESP32Encoder/src/ESP32Encoder.cpp"
    REQUIRES lvgl LovyanGFX esp_wifi esp_netif nvs_flash esp_http_server json espressif__mdns lwip
    PRIV_REQUIRES esp_timer driver
    INCLUDE_DIRS
        "."
        "/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5Dial-UserDemo/main/hal/utils/Button"
        "/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5Dial-UserDemo/main/hal/utils/ESP32Encoder/src"
)
