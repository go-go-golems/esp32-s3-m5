# Matrix Diagnostic Sequence (Visual)

Use this when JS animations appear to run but the LED matrix does not change.

## Prereqs

- Device reachable at `http://192.168.3.119` (adjust if needed).
- Use helper:

```bash
PLAY=ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/scripts/0067_play_js_example.sh
```

## Steps

1. `diag/00-env-status.js`  
Expected: JSON output with `width:96`, `height:8`, status object.

2. `diag/01-all-on.js`  
Expected visual: every LED lit.

3. `diag/02-all-off.js`  
Expected visual: every LED off.

4. `diag/03-single-pixel.js`  
Expected visual: exactly one pixel lit at top-left logical origin.

5. `diag/04-border.js`  
Expected visual: rectangular border around whole 96x8 matrix.

6. `diag/05-checkerboard.js`  
Expected visual: alternating checker pattern.

7. `diag/06-walk-dot.js`  
Expected visual: one dot moving left-to-right continuously.

8. `diag/07-text-test.js`  
Expected visual: static `TEST`.

9. `diag/08-scroll-test.js`  
Expected visual: scrolling `HELLO 123`.

10. `diag/09-wave-test.js`  
Expected visual: wavy scrolling text.

11. `diag/10-stop-reset.js`  
Expected visual: matrix cleared/off.

## Manual run example

```bash
$PLAY 0067-esp-c3-led-matrix-http/examples/js/diag/01-all-on.js
$PLAY 0067-esp-c3-led-matrix-http/examples/js/diag/02-all-off.js
```

## If visuals still fail

- Confirm non-JS matrix path:

```bash
curl -sS -X POST http://192.168.3.119/api/matrix/text \
  -H 'content-type: application/json' \
  --data '{"text":"TEST"}'
```

- Check runtime status:

```bash
curl -sS http://192.168.3.119/api/js/status
curl -sS http://192.168.3.119/api/matrix/status
```
