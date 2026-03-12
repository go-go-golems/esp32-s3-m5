# Script Eval Notes

Use the Go server HTTP endpoint to send JavaScript to a connected device:

- Endpoint: `POST /api/script-eval`
- Required JSON fields:
  - `device_id`
  - `code`
- Optional JSON fields:
  - `request_id`
  - `filename`
  - `timeout_ms`

## Quick inline test

```bash
curl -sS -X POST http://127.0.0.1:18080/api/script-eval \
  -H 'Content-Type: application/json' \
  --data '{"device_id":"m5dial-b76a94","request_id":9001,"filename":"inline","timeout_ms":1000,"code":"lain.mode(\"radio\"); lain.position(35); \"ok\""}'
```

## Run a saved script

```bash
jq -Rs \
  --arg device_id "m5dial-b76a94" \
  --arg filename "scripts/radio-demo.js" \
  '{device_id:$device_id, request_id:9002, filename:$filename, timeout_ms:1000, code:.}' \
  0074-m5dial-web-remote/scripts/radio-demo.js |
curl -sS -X POST http://127.0.0.1:18080/api/script-eval \
  -H 'Content-Type: application/json' \
  --data-binary @-
```

## Queue limit

The current firmware app-command queue is short. Do not send the full station table in one eval yet.

- Safe: mode change, a few `lain.station(...)` calls, `lain.position(...)`, `lain.reveal(...)`
- Unsafe right now: one giant script that enqueues the entire browser station catalog at once

If you need the full catalog, split it across multiple requests.

## Timer examples

Saved timer-based examples:

- `scripts/timer-radio-sweep.js`
- `scripts/timer-reveal-demo.js`

Those rely on the project timer support:

- `setTimeout(fn, ms)`
- `clearTimeout(id)`
- `every(ms, fn)`
- `cancel(handleOrId)`

Write callbacks with classic function syntax, not arrow syntax:

- Use: `function () { ... }`
- Avoid: `() => { ... }`
- Use: `var`
- Avoid: `let` and `const`

Prefer `print(...)` for local serial debugging and `console.log(...)` for remote script logs.
