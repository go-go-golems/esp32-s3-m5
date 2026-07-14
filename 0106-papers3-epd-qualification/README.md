# Tutorial 0106 — PaperS3 EPD Qualification Harness

This firmware is the Phase 0 control harness for ticket `ESP-50-PAPERS3-EREADER-PRIMITIVES`. It qualifies PaperS3 display behavior before the reader primitives depend on a particular ESP-IDF/M5GFX/M5Unified combination.

It intentionally contains no reader, storage, layout, or JavaScript code. The console can render deterministic scenes, exercise the small/rotated update paths associated with M5GFX Issue 181, run a mixed-update soak, inspect heap/display state, and test display sleep/wakeup.

## Matrix

```bash
./tools/list_matrix.sh
```

| Cell | ESP-IDF | M5GFX | M5Unified | Purpose |
|---|---|---|---|---|
| A | 5.3.3 | 0.2.15 | 0.2.10 | Exact upstream factory-demo control |
| B | 5.3.3 | 0.2.25 | 0.2.18 | New M5 stack on conservative IDF |
| C | 5.3.4 | 0.2.25 | 0.2.18 | Existing repository S3 baseline |
| D | 5.4.2 | 0.2.25 | 0.2.18 | Upstream's repaired IDF 5.4 path |

Prepare clean component checkouts:

```bash
./tools/prepare_matrix_components.sh
```

Build one cell:

```bash
./tools/build_matrix_cell.sh C
```

Each cell uses its own ignored `build-cell-X/` and `sdkconfig.cell-X`. Exact IDF/component metadata is captured in `build-cell-X/qualification-build.txt`. Copy that file beside the reviewed serial and photographic evidence; build directories remain disposable and ignored.

ESP-IDF 5.3.3 was not installed when the harness was created. Cells A/B correctly fail closed until `~/esp/esp-idf-5.3.3/export.sh` exists; do not silently substitute 5.3.4.

## Direct development build

For development against the current local sibling components, read their Git status first, then:

```bash
source ~/esp/esp-idf-5.3.4/export.sh
idf.py set-target esp32s3
idf.py build
```

The matrix scripts are required for qualification evidence because the sibling M5GFX checkout currently contains unrelated/local debugging patches.

## Flash and console

Use a single owner and preferably a stable `/dev/serial/by-id/...` path. The flash helper refuses an absent or already-owned port and rebuilds the selected cell before flashing:

```bash
./tools/flash_matrix_cell.sh C /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_...-if00
```

Do not run `idf.py monitor` in parallel with the qualification runner. If the board needs a reset after attach, keep the one runner session open and press reset when prompted rather than launching another probe.

The REPL prompt is `epd-qual>`. Commands:

```text
epd help
epd status
epd scene white|black|gray|checker|text
epd waveform quality|text|fast|fastest black|white|gray
epd waveform-compare
epd boundary [0|1|2|3|all]
epd text-soak [iterations]
epd soak [iterations]
epd cycle-sleep [milliseconds]
epd poweroff CONFIRM
```

### Suggested corpus

```text
epd status
epd scene white
epd scene black
epd scene white
epd scene gray
epd scene checker
epd scene text
epd boundary all
epd text-soak 1000
epd soak 1000
epd cycle-sleep 2000
epd status
```

The serial runner executes that corpus in one exclusive session and emits a transcript, JSON result, and visual operator checklist:

```bash
source ~/esp/esp-idf-5.3.4/export.sh
python tools/run_qualification.py \
  --cell C \
  --port /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_...-if00 \
  --output-dir matrix-results/cell-C/$(date +%Y%m%d-%H%M%S)
```

Use `--smoke` for a 10-text/25-mixed-update harness check. A normal run performs 1,000 text updates and 1,000 deterministic mixed updates. Run `epd soak 10000` manually only as a deliberate long test. Soaks are synchronous and print periodic integrity/timing progress.

If black depth or ghosting differs between scenes, run `epd waveform-compare`. It leaves four simultaneous nominal-black columns labeled `QUALITY`, `TEXT`, `FAST`, and `FASTEST`; compare density and retained artifacts before choosing a reader refresh policy. Use `epd waveform <mode> gray` to compare grayscale behavior one waveform at a time. A console `pass` only means the transaction and heap checks completed—the operator's visual disposition remains required.

## Evidence requirements

For every cell preserve:

- build metadata and full serial log;
- board revision and USB identity;
- boot/status heap figures;
- photos of every static scene under comparable lighting;
- boundary and soak completion status;
- display sleep/wakeup result;
- any reset/manual attach behavior.

Do not select a production pin from a successful compile alone.

While attached through `idf.py monitor`, this project configures `Ctrl-A` as the monitor menu key. Rebuild/reflash is **`Ctrl-A`, then `Ctrl-F`** (both are control-key chords); lowercase `f` is not the reflash command. Use `Ctrl-A`, then `Ctrl-H` for monitor help.
