#include "console/UsbSerialJtagConsole.h"

#include "driver/usb_serial_jtag.h"
#include "esp_check.h"

UsbSerialJtagConsole::UsbSerialJtagConsole() {
  usb_serial_jtag_driver_config_t config = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
  config.rx_buffer_size = 1024;
  config.tx_buffer_size = 1024;
  ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&config));
}

int UsbSerialJtagConsole::Read(uint8_t* buffer, int max_len, TickType_t timeout) {
  if (!buffer || max_len <= 0) {
    return 0;
  }
  return usb_serial_jtag_read_bytes(buffer, static_cast<uint32_t>(max_len), timeout);
}

void UsbSerialJtagConsole::Write(const uint8_t* buffer, int len) {
  if (!buffer || len <= 0) {
    return;
  }

  if (!usb_serial_jtag_is_connected()) {
    return;
  }

  int written = 0;
  while (written < len) {
    const int n = usb_serial_jtag_write_bytes(
        buffer + written, static_cast<size_t>(len - written), portMAX_DELAY);
    if (n <= 0) {
      return;
    }
    written += n;
  }
}

