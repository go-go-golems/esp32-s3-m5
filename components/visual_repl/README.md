# visual_repl

Fixed-cell visual terminal model and renderer for `0102-esp32-p4-visual-quickjs-repl`.

Current checkpoint:

- 320×320 RGB565 target.
- 8×16 cells.
- 40 columns × 20 rows.
- The first 19 rows are scrollback/output rows.
- The last row is reserved for the editable prompt/input line.
- Rows carry semantic styles (`system`, `prompt`, `input`, `output`, `error`, `status`) that map to RGB565 foreground/background colors.

The first renderer intentionally keeps the data model simple: one style per row. Later phases should evolve this to line spans so a row can contain mixed prompt/input/error/output styling without changing the LCD primitive API.
