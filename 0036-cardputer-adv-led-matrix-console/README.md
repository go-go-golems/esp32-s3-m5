# Tutorial 0036 — Cardputer-ADV LED Matrix Console (MAX7219) (ESP-IDF 5.4.x)

This firmware is a Phase-1 bring-up tool for driving an external MAX7219-style 8×8 LED matrix from an interactive `esp_console` REPL over **USB Serial/JTAG**.

This firmware supports up to **16** chained 8×8 modules. Default is **12** modules (a 96×8 strip); override with `matrix chain N`.

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
- `matrix safe on|off` (RAM-based full-on/full-off; prefer this over `matrix test` for wiring bring-up)
- `matrix chain [n]` (get/set chain length; max 16)
- `matrix text <TEXT>` (render 1 character per 8×8 module)
- `matrix intensity <0..15>`
- `matrix spi [hz]` (get/set SPI clock in Hz; useful for marginal wiring)
- `matrix blink on [on_ms] [off_ms]` / `matrix blink off` (continuous on/off with pauses)
- `matrix scroll on <TEXT> [fps] [pause_ms]` / `matrix scroll off` (smooth 1px scroll, defaults `15fps` + `250ms` pauses)
- `matrix scroll wave <TEXT> [fps] [pause_ms]` (scroll + wave)
- `matrix anim drop <TEXT> [fps] [pause_ms]` / `matrix anim wave <TEXT> [fps]` / `matrix anim off` (drop-bounce + wave text animations)
- `matrix anim flip <A|B|C...> [fps] [hold_ms]` (flipboard / split-flap style transitions between texts)
- `matrix flipv on|off` (flip vertically; defaults to `on` for common module orientation)
- `matrix row <0..7> <0x00..0xff>`
- `matrix rowm <0..7> <b0..bN-1>` (set a row with one byte per module)
- `matrix row4 <0..7> <b0> <b1> <b2> <b3>` (compat: set first 4 modules)
- `matrix px <0..W-1> <0..7> <0|1>` (set pixel on the strip, where W = 8×chain_len)
- `matrix pattern rows|cols|diag|checker|off`
- `matrix pattern onehot <0..N-1>` (turn on exactly one module; helps find module order)
- `matrix reverse on|off` (reverse module order if the strip is wired opposite)

## What you should see

On boot, the firmware auto-initializes the MAX7219 chain and shows the `ids` pattern (each 8×8 module gets a distinct “shape”). If the display shows an old/random pattern after flashing, run `matrix init` to re-program the MAX7219 and clear it.

## Signal integrity

SPI clock defaults to a conservative `100 kHz` to improve signal margins on longer wires / breadboards. Use `matrix spi 1000000` (or similar) if you want to speed it up.

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
