# 0048-cardputer-js-web

Device-hosted Web IDE for Cardputer:

- Firmware serves embedded Preact+Zustand UI assets.
- Browser POSTs JavaScript to `/api/js/eval`.
- Firmware evaluates via MicroQuickJS and returns JSON `{ ok, output, error }`.

See the ticket docs:

- `esp32-s3-m5/ttmp/.../0048-CARDPUTER-JS-WEB--cardputer-web-js-ide-preact-microquickjs/`
