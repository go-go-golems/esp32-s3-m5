# Scripts (0066b)

These are convenience scripts for building/flashing and quick web smoke-testing while working on:

- `0066-cardputer-adv-ledchain-gfx-sim` (firmware)
- Ticket `0066b-cardputer-adv-keyboard-ui-screens--cardputer-adv-keyboard-matrix-ui-screens`

## Build

```bash
ttmp/2026/01/24/0066b-cardputer-adv-keyboard-ui-screens--cardputer-adv-keyboard-matrix-ui-screens/scripts/01-build-0066.sh
```

## Flash

```bash
PORT=/dev/ttyACM0 \
  ttmp/2026/01/24/0066b-cardputer-adv-keyboard-ui-screens--cardputer-adv-keyboard-matrix-ui-screens/scripts/02-flash-0066.sh
```

## Monitor (TTY required)

```bash
PORT=/dev/ttyACM0 \
  ttmp/2026/01/24/0066b-cardputer-adv-keyboard-ui-screens--cardputer-adv-keyboard-matrix-ui-screens/scripts/03-monitor-0066.sh
```

## Web smoke

```bash
HOST=http://192.168.4.1 \
  ttmp/2026/01/24/0066b-cardputer-adv-keyboard-ui-screens--cardputer-adv-keyboard-matrix-ui-screens/scripts/04-web-smoke.sh
```

