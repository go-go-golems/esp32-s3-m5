# picocalc_keyboard

Reusable PicoCalc keyboard component extracted from `0099-esp32-p4-picocalc-display-keyboard`.

## Hardware mapping

Same-position RPico socket adapter:

| PicoCalc / RPico net | ESP32-P4 GPIO |
|---|---:|
| GP6 / SDA | GPIO50 |
| GP7 / SCL | GPIO49 |

The keyboard controller is accessed over I2C at address `0x1f` using the status register `0x04` and FIFO register `0x09`.

## Public API

- `picocalc_keyboard_init()` initializes the I2C bus/device.
- `picocalc_keyboard_poll_event()` reads one key event from the FIFO when available.
- `picocalc_keyboard_read_status()` and `picocalc_keyboard_get_diag()` support diagnostics.
- `picocalc_keyboard_key_name()` and `picocalc_keyboard_state_name()` provide human-readable names for diagnostics and key mapping work.

This component intentionally keeps the low-level polling API. Higher-level visual REPL code should translate `picocalc_key_event_t` into semantic editor events such as character, enter, backspace, cursor left/right, and scroll.
