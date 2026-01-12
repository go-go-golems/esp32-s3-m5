#include "console/UartConsole.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "hal/uart_ll.h"

UartConsole::UartConsole(uart_port_t uart_num, int baud_rate) : uart_num_(uart_num) {
  uart_config_t uart_config = {};
  uart_config.baud_rate = baud_rate;
  uart_config.data_bits = UART_DATA_8_BITS;
  uart_config.parity = UART_PARITY_DISABLE;
  uart_config.stop_bits = UART_STOP_BITS_1;
  uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
  uart_config.rx_flow_ctrl_thresh = 0;
  uart_config.source_clk = UART_SCLK_DEFAULT;

  ESP_ERROR_CHECK(uart_param_config(uart_num_, &uart_config));
  ESP_ERROR_CHECK(uart_driver_install(uart_num_, 1024 * 2, 0, 0, nullptr, 0));
  ESP_ERROR_CHECK(uart_set_rx_full_threshold(uart_num_, 1));
  ESP_ERROR_CHECK(uart_set_rx_timeout(uart_num_, 1));
}

int UartConsole::Read(uint8_t* buffer, int max_len, TickType_t timeout) {
  if (!buffer || max_len <= 0) {
    return 0;
  }

  const TickType_t start_ticks = xTaskGetTickCount();

  while (true) {
    const int buffered = uart_read_bytes(uart_num_, buffer, max_len, 0);
    if (buffered > 0) {
      return buffered;
    }

    uart_dev_t* uart_hw = UART_LL_GET_HW(uart_num_);
    const int fifo_len = static_cast<int>(uart_ll_get_rxfifo_len(uart_hw));
    if (fifo_len > 0) {
      const int to_read = fifo_len < max_len ? fifo_len : max_len;
      for (int i = 0; i < to_read; i++) {
        uart_ll_read_rxfifo(uart_hw, &buffer[i], 1);
      }
      return to_read;
    }

    if (timeout == 0) {
      return 0;
    }

    if (timeout != portMAX_DELAY) {
      const TickType_t elapsed = xTaskGetTickCount() - start_ticks;
      if (elapsed >= timeout) {
        return 0;
      }
    }

    vTaskDelay(1);
  }
}

void UartConsole::Write(const uint8_t* buffer, int len) {
  if (!buffer || len <= 0) {
    return;
  }
  uart_write_bytes(uart_num_, reinterpret_cast<const char*>(buffer), len);
}
