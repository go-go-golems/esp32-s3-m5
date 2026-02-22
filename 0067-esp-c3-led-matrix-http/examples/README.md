# 0067 Matrix JS Examples

These JavaScript examples run on the device via `/api/js/eval`.

For the full scripting/runtime API and an end-to-end getting started guide, see:
- `../docs/JS-API-GUIDE.md`

## Files

- `js/01-plasma-ribbon.js` -- dual-sine ribbon interference with spark accents
- `js/02-life-torus.js` -- Conway's Game of Life on toroidal grid with periodic reseeding
- `js/03-comet-trails.js` -- bouncing comets with decaying trail field and dithered brightness
- `js/04-anim-registry-demo.js` -- minimal example of `matrix.anim` register/start/cleanup
- `js/05-nyan-cat.js` -- pixel-art cat with rainbow trail and "MAGIC" text flash
- `js/06-superbowl.js` -- rapid-fire commercial: countdown, starbursts, sparkle rain, stadium wave, strobe finale
- `js/07-zen-garden.js` -- calm ripple pond with breathing stone, drifting firefly, raked sand texture

All animation scripts use the `matrix.anim` registry API.
Each script:
- registers a named animation,
- starts it immediately,
- returns `matrix.anim.status()` as JSON text.

## Run

From repository root:

```bash
BASE_URL=http://192.168.3.119 \
ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/scripts/0067_play_js_example.sh 01-plasma-ribbon
```

You can also pass a full path:

```bash
BASE_URL=http://192.168.3.119 \
ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/scripts/0067_play_js_example.sh \
0067-esp-c3-led-matrix-http/examples/js/02-life-torus.js
```

## Stop

```bash
curl -sS -X POST http://192.168.3.119/api/js/stop
```

Soft reset (keeps VM):

```bash
curl -sS -X POST http://192.168.3.119/api/js/reset
```
