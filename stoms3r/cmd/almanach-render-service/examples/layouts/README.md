# Almanach CLI Layout Examples

These YAML files are inputs for the `almanach-render-service render`, `inspect`, and `print` CLI verbs.

Run from `stoms3r/cmd/almanach-render-service`:

```bash
make build

./almanach-render-service render \
  --layout examples/layouts/01-minimal.yaml \
  --out /tmp/almanach-minimal.png \
  --output yaml

./almanach-render-service inspect \
  --layout examples/layouts/01-minimal.yaml \
  --output yaml
```

Examples:

- `01-minimal.yaml` — minimal title/date/quote smoke test.
- `02-daily-briefing.yaml` — daily weather, plan, news, and note blocks.
- `03-knowledge-strip.yaml` — word, history, did-you-know, divider, and quote blocks.
- `04-tracker-journal.yaml` — habits, mood, reading, and reflection blocks.
- `05-wrapped-render-request.yaml` — supported `{ layout, render }` wrapper shape.
- `06-paper-shell-preview.yaml` — `.paper-shell` capture for decorative zigzag preview.

Use `.paper-body` for print-oriented output and `.paper-shell` when you explicitly want the decorative zigzag edges included in the preview image.
