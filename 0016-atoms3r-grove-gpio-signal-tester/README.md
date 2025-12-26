## AtomS3R Tutorial 0016 — GROVE GPIO1/GPIO2 Signal Tester (USB Serial/JTAG manual REPL + LCD status) (ESP-IDF 5.4.1)

This project is an **AtomS3R-only** firmware “instrument” to generate and observe signals on the GROVE pins:

- **G1 / GPIO1**
- **G2 / GPIO2**

It uses:

- **Control plane**: a minimal **manual REPL** over **USB Serial/JTAG** (**no** `esp_console`; no tab-completion/history)
- **Status plane**: LCD text UI showing mode/pin/TX pattern or RX counters

### Build (ESP-IDF 5.4.1)

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && \
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester && \
idf.py set-target esp32s3 && idf.py build
```

### Flash + Monitor

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && \
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester && \
idf.py flash monitor
```

### REPL commands (MVP)

- `help`
- `mode tx|rx|idle`
- `pin 1|2`
- `tx high|low|stop`
- `tx square <hz>`
- `tx pulse <width_us> <period_ms>`
- `rx edges rising|falling|both`
- `rx pull none|up|down`
- `rx reset`
- `status`



