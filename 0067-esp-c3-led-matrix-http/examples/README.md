# 0067 Matrix JS Examples

These JavaScript examples run on the device via `/api/js/eval`.

## Files

- `js/01-plasma-ribbon.js`
- `js/02-life-torus.js`
- `js/03-comet-trails.js`

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
