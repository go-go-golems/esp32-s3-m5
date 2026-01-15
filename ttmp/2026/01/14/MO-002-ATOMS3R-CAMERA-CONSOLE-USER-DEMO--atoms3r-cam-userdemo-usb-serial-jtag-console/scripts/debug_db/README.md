# 0041 Debug DB

This folder stores the sqlite schema and helper scripts used to track the 0041 debugging process.

## Initialize the database

```bash
python3 init_db.py --db /abs/path/to/debug.sqlite3
```

## Import a log

```bash
python3 import_log.py \
  --db /abs/path/to/debug.sqlite3 \
  --log /abs/path/to/log.txt \
  --run-name "run-2026-01-14" \
  --step-name "Step 1: Power Polarity" \
  --analysis-ref "Step-by-step comparison > Step 1" \
  --file-refs "0041-atoms3r-cam-jtag-serial-test/main/main.c" \
  --function-refs "camera_power_set,camera_sccb_scan"
```

## Dashboard

```bash
python3 dashboard.py --db /abs/path/to/debug.sqlite3
python3 dashboard.py --db /abs/path/to/debug.sqlite3 --run-name "run-2026-01-14"
```

## Notes

- Raw log lines go into `raw_log_lines`.
- Parsed log lines go into `parsed_log_lines`.
- `STEP:` markers in logs are captured as `step_name` on parsed lines.
