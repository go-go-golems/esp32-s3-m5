## AtomS3R Tutorial 0016 — GROVE GPIO1/GPIO2 Signal Tester (USB Serial/JTAG manual REPL + LCD status) (ESP-IDF 5.4.1)

This project is an **AtomS3R-only** firmware “instrument” to generate and observe signals on the GROVE pins:

- **G1 / GPIO1**
- **G2 / GPIO2**

It uses:

- **Control plane**: a minimal **manual REPL** over **USB Serial/JTAG** (**no** `esp_console`; no tab-completion/history)
- **Status plane**: LCD text UI showing mode/pin/TX pattern or RX counters

### Build (ESP-IDF 5.4.1)

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester && \
./build.sh set-target esp32s3 && \
./build.sh build
```

### Flash + Monitor

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester && \
./build.sh flash monitor
```

### REPL commands (MVP)

- `help`
- `mode tx|rx|idle|uart_tx|uart_rx`
- `pin 1|2`
- `tx high|low|stop`
- `tx square <hz>`
- `tx pulse <width_us> <period_ms>`
- `rx edges rising|falling|both`
- `rx pull none|up|down`
- `rx reset`
- `uart baud <baud>`
- `uart map normal|swapped`
- `uart tx <token> <delay_ms>`
- `uart tx stop`
- `uart rx get [max_bytes]`
- `uart rx clear`
- `status`



