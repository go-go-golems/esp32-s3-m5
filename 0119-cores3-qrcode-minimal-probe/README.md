# 0119 — CoreS3 QRCode minimal probe

A deliberately minimal ESP-IDF 5.3.4 firmware for isolating the CoreS3 +
Module13.2 QRCode electrical/UART path. It is separate from the full `0118`
application and contains no request queues, scanner task, console, protocol
class, saved-setting writes, or mode configuration.

## Fixed wiring

- Module Gateway H2: removed
- Module13.2 external input: 12 V
- Scanner interface selector: UART
- CoreS3 UART1 TX: G13 -> QR_RX (M5-Bus pin 23)
- CoreS3 UART1 RX: G14 <- QR_TX (M5-Bus pin 26)
- PI4IOE5V6408 channel 0: QR engine power enable, high
- PI4IOE5V6408 channel 4: hardware TRIG, idle high
- UART: 115200 8N1
- Console: USB Serial/JTAG

## Exact behavior

1. Initialize M5Unified and the LCD.
2. Preload TRIG high and power low in the expander output latch.
3. Enable both expander outputs, then raise scanner power.
4. Wait one second.
5. Install UART1 at 115200 on G13/G14.
6. Transmit exactly one firmware query: `43 02 C1`.
7. Display and log every received byte, or explicitly report zero bytes.
8. After boot, tap the screen to pulse hardware TRIG low for 100 ms.
9. Continuously display/log any scanner bytes, framed only by 50 ms of UART
   silence.

The firmware does not send factory reset, trigger-mode, lighting, suffix, baud,
or interface-selection commands.

## Build and flash

```bash
source ~/esp/esp-idf-5.3.4/export.sh
idf.py set-target esp32s3   # first build only
idf.py build
idf.py -p /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_30:ED:A0:0B:0F:50-if00 flash monitor
```

Exit the monitor with `Ctrl-]`. Only one process may own the serial device.
