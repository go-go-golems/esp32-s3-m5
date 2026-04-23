#pragma once

#include "esp_err.h"

#define ATOM_PRINTER_UART_NUM 2
#define ATOM_PRINTER_TX_GPIO  23
#define ATOM_PRINTER_RX_GPIO  33
#define ATOM_PRINTER_CTS_GPIO 19
#define ATOM_PRINTER_BAUD     9600

esp_err_t app_printer_init(void);
esp_err_t app_printer_reset(void);
esp_err_t app_printer_print_text(const char *text);
esp_err_t app_printer_feed_lines(unsigned int lines);
esp_err_t app_printer_print_wifi_status(const char *ip_address);
