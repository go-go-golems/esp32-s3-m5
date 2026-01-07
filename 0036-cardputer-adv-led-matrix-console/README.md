# Tutorial 0036 — Cardputer-ADV LED Matrix Console (MAX7219) (ESP-IDF 5.4.x)

This firmware is a Phase-1 bring-up tool for driving an external MAX7219-style 8×8 LED matrix from an interactive `esp_console` REPL over **USB Serial/JTAG**.

This setup assumes **4** chained 8×8 modules (a 32×8 “strip”).

## Pins (currently assumed; verify for your hardware)

- SPI SCK: `GPIO40`
- SPI MOSI: `GPIO14`
- SPI CS: `GPIO5` (**verify**; may conflict with on-board peripherals depending on your setup)

## Console

Console backend is **USB Serial/JTAG** (`CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`).

## Commands

- `help`
- `matrix` (prints help)
- `matrix help`
- `matrix init` (re-inits + clears + shows `ids` pattern)
- `matrix clear`
- `matrix test on|off`
- `matrix intensity <0..15>`
- `matrix bright <0..100>` (gamma-mapped brightness, more “human linear”)
- `matrix gamma <1.0..3.0>` (adjust brightness curve; default 2.2)
- `matrix blink on [on_ms] [off_ms]` / `matrix blink off` (continuous on/off with pauses)
- `matrix row <0..7> <0x00..0xff>`
- `matrix row4 <0..7> <b0> <b1> <b2> <b3>` (one byte per 8×8 module)
- `matrix px <0..31> <0..7> <0|1>` (set pixel on the 32×8 strip)
- `matrix pattern rows|cols|diag|checker|off`
- `matrix pattern onehot <0..3>` (turn on exactly one module; helps find module order)
- `matrix reverse on|off` (reverse module order if the strip is wired opposite)

## What you should see

On boot, the firmware auto-initializes the MAX7219 chain and shows the `ids` pattern (each 8×8 module gets a distinct “shape”). If the display shows an old/random pattern after flashing, run `matrix init` to re-program the MAX7219 and clear it.

## Signal integrity

SPI clock is intentionally conservative (`1 MHz`) to improve signal margins on longer wires / breadboards.

## Brightness notes

MAX7219 only has 16 hardware intensity steps (`0..15`), which are not perceptually linear. Use:

- `matrix intensity <0..15>` for raw access
- `matrix bright <0..100>` for a gamma-mapped “more even” brightness ramp (default gamma `2.2`)

## Build / Flash / Monitor

If you already have ESP-IDF sourced:

```bash
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

Recommended tmux workflow:

```bash
./build.sh tmux-flash-monitor
```
