# 0099 — ESP32-P4 PicoCalc display + keyboard smoke test

Lean PicoCalc peripheral firmware for the same-position RPico-to-Waveshare ESP32-P4-WIFI6 adapter.

No ESP-Hosted, no Wi-Fi, no HTTP server. This exists to iterate quickly on the PicoCalc keyboard and LCD without compiling the networking stack.

## Physical adapter pin mapping

Keyboard:

| PicoCalc / RPico net | ESP32-P4 GPIO |
|---|---:|
| GP6 / SDA | GPIO50 |
| GP7 / SCL | GPIO49 |

LCD:

| PicoCalc / RPico net | ESP32-P4 GPIO |
|---|---:|
| GP10 / LCD SCK | GPIO3 |
| GP11 / LCD MOSI | GPIO2 |
| GP12 / LCD MISO | not used |
| GP13 / LCD CS | GPIO7 |
| GP14 / LCD DC | GPIO24 |
| GP15 / LCD RST | GPIO25 |

Console remains the Waveshare CH343 USB-UART bridge on UART0 GPIO37/GPIO38.

## Build / flash / monitor

```bash
source ~/esp/esp-idf-5.4.2/export.sh
idf.py set-target esp32p4
idf.py build
PORT=/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00
lsof "$PORT" || true
idf.py -p "$PORT" flash monitor
```

## Console commands

```text
kbd status
kbd poll 10
kbd raw on
kbd raw off
lcd init
lcd speed
lcd speed 40M
lcd speed 75M
lcd speed 80M
lcd bench 10
lcd perf
lcd perf full
lcd perf queued
lcd rectbench 16 16 500
lcd rectbench 80 24 200
lcd cellbench 8 16 1000
lcd rowbench 16 200
lcd scrollbench 16 20
lcd textbench 8 16 20
lcd textqueued 8 16 20
lcd text 8 16
lcd pattern checker
lcd pattern stripes
lcd pattern diagonal
lcd pattern all
lcd fill red|green|blue|white|black
lcd bars
status
```

The older RP2350 PicoCalc firmware defaulted to 75 MHz after testing. This ESP32-P4 firmware explicitly selects the high-speed GPSPI `SPI_CLK_SRC_SPLL` source, defaults to 80 MHz, and reports the ESP-IDF actual SPI frequency with `lcd speed` / `status`. Without SPLL, ESP32-P4's default SPI source is XTAL (40 MHz), which makes ESP-IDF reject SCLK requests above 20 MHz.

The first display-throughput optimization uses a reusable 32 KiB internal DMA-capable fill buffer and sets the SPI bus maximum transfer size to 32 KiB. This reduces a full-screen 320×320 RGB565 fill from roughly 400 small 512-byte pixel transactions to roughly seven large DMA transactions. On the same-position GPIO-matrix LCD wiring at actual 80 MHz, measured full-screen fill improved from about 32 ms to about 21 ms; `lcd bars` improved from about 33 ms to about 26 ms.

Use `lcd pattern checker|stripes|diagonal` for stronger visual/signal-integrity checks than solid color bars. Use `lcd rectbench [w h loops]`, `lcd cellbench [w h loops]`, `lcd rowbench [h loops]`, and `lcd scrollbench [row_h loops]` to measure dirty-rectangle, terminal-cell, row repaint, and full terminal-scroll-style redraw overhead. Use `lcd textbench [cell_w cell_h loops]`, `lcd textqueued [cell_w cell_h loops]`, and `lcd text [cell_w cell_h]` for row-batched pseudo-text rendering benchmarks. Use `lcd perf`, `lcd perf full`, or `lcd perf queued` for repeatable performance suites with comparable metric lines and text render-vs-transfer split timing.
