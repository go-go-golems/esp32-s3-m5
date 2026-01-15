---
Title: Diary — 0041 Debugging
Ticket: MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO
Status: active
Topics:
    - camera
    - esp32
    - firmware
    - usb
    - console
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0041-atoms3r-cam-jtag-serial-test/main/main.c
      Note: Step marker logs and power sweep instrumentation
    - Path: 0041-atoms3r-cam-jtag-serial-test/sdkconfig
      Note: Enabled CONFIG_SPIRAM for PSRAM allocation
    - Path: ttmp/2026/01/14/MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO--atoms3r-cam-userdemo-usb-serial-jtag-console/analysis/01-camera-init-analysis-userdemo-vs-0041.md
      Note: Debugging plan and analysis context for 0041
    - Path: ttmp/2026/01/14/MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO--atoms3r-cam-userdemo-usb-serial-jtag-console/scripts/debug_db/README.md
      Note: Dashboard usage and metrics documentation
    - Path: ttmp/2026/01/14/MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO--atoms3r-cam-userdemo-usb-serial-jtag-console/scripts/debug_db/backfill_metrics.py
      Note: Backfills metrics tables for existing logs
    - Path: ttmp/2026/01/14/MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO--atoms3r-cam-userdemo-usb-serial-jtag-console/scripts/debug_db/debug.sqlite3
      Note: Debug run data store
    - Path: ttmp/2026/01/14/MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO--atoms3r-cam-userdemo-usb-serial-jtag-console/scripts/debug_db/import_log.py
      Note: Log import utility
    - Path: ttmp/2026/01/14/MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO--atoms3r-cam-userdemo-usb-serial-jtag-console/scripts/debug_db/schema.sql
      Note: Debug sqlite schema
    - Path: ttmp/2026/01/14/MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO--atoms3r-cam-userdemo-usb-serial-jtag-console/tasks.md
      Note: Added SCCB scan investigation task
    - Path: ttmp/2026/01/14/MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO--atoms3r-cam-userdemo-usb-serial-jtag-console/various/debug-logs/step-01-power-sweep-flash.log
      Note: Step 1 flash log
    - Path: ttmp/2026/01/14/MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO--atoms3r-cam-userdemo-usb-serial-jtag-console/various/debug-logs/step-01-power-sweep-monitor-idf5.1.4-user.log
      Note: User-provided Step 1 monitor output (IDF 5.1.4)
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-14T18:58:34-05:00
WhatFor: ""
WhenToUse: ""
---






# Diary

## Goal

Record each debugging step taken to bring up the camera in the 0041 firmware, including code changes, flash/monitor commands, failures, and results.

## Step 1: Create the debugging plan and diary scaffold

I created a dedicated debugging diary and added a new guidebook section to the camera analysis document. This establishes a structured, step-by-step workflow for investigating why the 0041 firmware fails to initialize the camera.

This step does not change firmware behavior; it only sets up the documentation scaffolding and the debugging plan so each future experiment is logged consistently.

### What I did
- Added a new “Debugging plan / guidebook for 0041 bring-up” section to the analysis doc.
- Created this diary for ongoing debugging notes.

### Why
- The user requested a structured debugging plan with commits after each step and a detailed diary.

### What worked
- The plan now provides a staged checklist with references to the analysis and the exact files/functions to touch.

### What didn't work
- N/A.

### What I learned
- N/A.

### What was tricky to build
- N/A.

### What warrants a second pair of eyes
- N/A.

### What should be done in the future
- Update this diary after each debug experiment (power polarity, SCCB scan, PSRAM fallback, timing, capture loop).

### Code review instructions
- Start in `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/14/MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO--atoms3r-cam-userdemo-usb-serial-jtag-console/analysis/01-camera-init-analysis-userdemo-vs-0041.md`.
- Review the “Debugging plan / guidebook for 0041 bring-up” section.

### Technical details
- No commands run; documentation-only step.

## Step 2: Build the sqlite debug database and tooling

I set up a dedicated sqlite schema and helper scripts for tracking debug runs, steps, logs, and artifacts. This gives us a structured place to store full raw logs and parsed lines, plus metadata like step names and file references.

The database now exists in the ticket’s `scripts/` directory, and a small dashboard script is available for summarizing runs and error counts.

**Commit (code):** a1da196 — "0041: debug step 1 power sweep + logs"

### What I did
- Created `scripts/debug_db/schema.sql`, `init_db.py`, `import_log.py`, and `dashboard.py`.
- Initialized `scripts/debug_db/debug.sqlite3`.
- Created a `README.md` with usage examples.
- Created `various/debug-logs/` to store per-step logs.

### Why
- The user requested a structured data store for debugging history, including raw and parsed logs.

### What worked
- The database schema created cleanly and initialization completed without errors.

### What didn't work
- N/A.

### What I learned
- N/A.

### What was tricky to build
- Ensuring the import script tracks step transitions using `STEP:` markers while also storing raw lines verbatim.

### What warrants a second pair of eyes
- Confirm the schema captures all required metadata (especially for later dashboard queries).

### What should be done in the future
- Add or adjust tables as new debugging data needs emerge.

### Code review instructions
- Start in `ttmp/2026/01/14/MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO--atoms3r-cam-userdemo-usb-serial-jtag-console/scripts/debug_db/schema.sql`.
- Review `init_db.py`, `import_log.py`, and `dashboard.py` for correctness.

### Technical details
- Commands: `python3 .../init_db.py --db .../debug.sqlite3`.

## Step 3: Instrument 0041 for step-aware logs and power polarity sweep

I updated the 0041 firmware to emit step markers (`STEP: ...`) and added a power polarity sweep before SCCB scanning and camera init. This creates clear, step-labeled serial output and provides immediate evidence about whether GPIO18 power gating is inverted.

These changes are the first functional debugging experiment and align with Step 1 of the guidebook in the analysis document.

**Commit (code):** a1da196 — "0041: debug step 1 power sweep + logs"

### What I did
- Added `log_step()` and `camera_power_force_level()` helpers.
- Added a polarity sweep (level 0 and level 1) with SCCB scans and extended delays.
- Labeled each phase with a `STEP:` log line for correlation.

### Why
- We need to correlate serial logs to debugging steps and directly test whether the power enable polarity is wrong in 0041.

### What worked
- Code changes compile locally (no build run yet), and step markers are in place.

### What didn't work
- N/A.

### What I learned
- N/A.

### What was tricky to build
- Keeping step markers concise while still covering every phase that needs log correlation.

### What warrants a second pair of eyes
- Confirm the polarity sweep does not accidentally mask the real power enable state for the final init.

### What should be done in the future
- Run tmux flash + monitor and capture logs to validate this step.

### Code review instructions
- Start in `0041-atoms3r-cam-jtag-serial-test/main/main.c`.
- Review `log_step()`, `camera_power_force_level()`, and `app_main()` step markers.

### Technical details
- Files touched: `0041-atoms3r-cam-jtag-serial-test/main/main.c`.

## Step 4: Run Step 1 in tmux and capture logs

I ran the Step 1 flash/monitor workflow inside tmux and captured logs to the ticket. The outputs show an IDF Python environment mismatch during flash and only the monitor header, which indicates the serial connection is not yet producing runtime logs.

This step produced the first stored debug logs and imported them into the sqlite database under a named run so we can query them later.

### What I did
- Started a tmux session with two panes: one for `idf.py flash` and one for `idf.py monitor`, each piping output to a log file.
- Captured logs to `various/debug-logs/step-01-power-sweep-flash.log` and `various/debug-logs/step-01-power-sweep-monitor.log`.
- Imported both logs into `scripts/debug_db/debug.sqlite3` with `import_log.py`.

### Why
- The debugging plan requires tmux-based flashing/monitoring and per-step log capture.

### What worked
- Logs were captured and imported into sqlite; log records were created for run `run-2026-01-14-0041-step1`.

### What didn't work
- Flash output reported: `'/home/manuel/.espressif/python_env/idf5.4_py3.11_env/bin/python' is currently active in the environment while the project was configured with '/home/manuel/.espressif/python_env/idf5.1_py3.11_env/bin/python'. Run 'idf.py fullclean' to start again.`
- Monitor output only showed the monitor header and no runtime logs, which suggests no device output or no active connection to the target.

### What I learned
- The project environment mismatch likely needs a `idf.py fullclean` before flashing if we stay on IDF v5.4.1.

### What was tricky to build
- Capturing both flash and monitor logs while respecting the tmux requirement.

### What warrants a second pair of eyes
- Verify whether the flash command actually attempted to connect to the target after the environment mismatch warning.

### What should be done in the future
- Resolve the IDF Python environment mismatch, then re-run Step 1 to obtain actual device logs.

### Code review instructions
- Review the captured logs in `various/debug-logs/` and the imported data in `scripts/debug_db/debug.sqlite3`.

### Technical details
- tmux command used: `tmux new-session -d -s atoms3r-step1 "bash -lc 'cd .../0041-atoms3r-cam-jtag-serial-test && idf.py -p /dev/ttyACM0 flash |& tee .../step-01-power-sweep-flash.log'"` and `tmux split-window -h -t atoms3r-step1 "bash -lc 'cd .../0041-atoms3r-cam-jtag-serial-test && timeout 15s idf.py -p /dev/ttyACM0 monitor |& tee .../step-01-power-sweep-monitor.log'"`.

## Step 5: Expand dashboard verbosity and backfill metrics

I made the dashboard output significantly more verbose, added metrics tables to the sqlite schema, and introduced a backfill script to compute summary metrics for existing logs. This ensures we can run detailed, structured queries for run-level, step-level, and log-level statistics without re-deriving them on the fly.

I also updated the README so it documents the expanded dashboard output and the new metrics tables.

**Commit (code):** da5c44b — "Debug DB: add metrics + verbose dashboard"

### What I did
- Added `log_metrics`, `step_metrics`, and `run_metrics` tables to `schema.sql`.
- Expanded `import_log.py` to compute and store summary counts when importing logs.
- Added `backfill_metrics.py` to compute metrics for existing logs.
- Re-initialized the sqlite db to add new tables and ran backfill.
- Expanded `dashboard.py` output (run summary, steps, log files, step markers, tag counts, recent errors/warnings).
- Updated `README.md` with new usage notes.

### Why
- The user requested much more verbose dashboards and a richer sqlite-backed data model.

### What worked
- The dashboard now surfaces detailed metrics and log file metadata by default.
- Metrics are stored in sqlite for fast, repeatable queries.

### What didn't work
- N/A.

### What I learned
- Persisted metrics reduce repeated heavy queries and are useful for dashboards that will grow over time.

### What was tricky to build
- Ensuring backfill logic correctly derives step change counts and timestamps from existing parsed logs.

### What warrants a second pair of eyes
- Validate that metrics remain correct when multiple logs are imported into the same step/run.

### What should be done in the future
- Extend the dashboard with export formats (CSV/JSON) if we need to integrate with external dashboards.

### Code review instructions
- Start in `ttmp/2026/01/14/MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO--atoms3r-cam-userdemo-usb-serial-jtag-console/scripts/debug_db/dashboard.py`.
- Review `import_log.py`, `schema.sql`, and `backfill_metrics.py` for correctness.

### Technical details
- Commands: `python3 .../init_db.py --db .../debug.sqlite3`, `python3 .../backfill_metrics.py --db .../debug.sqlite3`.

## Step 6: Import user-provided Step 1 logs (IDF 5.1.4) and record findings

I captured the monitor output you provided from an IDF 5.1.4 build, stored it as a log file in the ticket, and imported it into the sqlite database as a dedicated run. This gave us the first complete device-side trace for Step 1, including SCCB scan results and the exact camera init failure.

The log shows that SCCB scans did not detect any devices, yet the camera probe later detects a GC0308 at address 0x21. The init then fails because PSRAM is disabled, and the driver cannot allocate a 153,600-byte frame buffer in PSRAM. This points to a configuration mismatch between `sdkconfig.defaults` (PSRAM enabled) and the actual build `sdkconfig` (PSRAM disabled).

**Commit (code):** 2623249 — "Debug Step 1: import IDF 5.1.4 monitor log"

### What I did
- Saved the provided monitor output to `various/debug-logs/step-01-power-sweep-monitor-idf5.1.4-user.log`.
- Fixed a syntax error in `import_log.py` and re-imported the log into sqlite.
- Backfilled metrics after import to keep run/step summaries current.

### Why
- We need the full runtime log in the ticket and database so we can query it and tie findings to a specific run.

### What worked
- The log imported successfully, with metrics captured for lines, errors, warnings, and step changes.

### What didn't work
- N/A.

### What I learned
- The camera probe can succeed even when SCCB scan reports no devices, which suggests the scan is not authoritative.
- The immediate failure is a PSRAM allocation error, not SCCB or power gating.

### What was tricky to build
- Ensuring the log import script was fixed before re-running the import to avoid corrupt or partial records.

### What warrants a second pair of eyes
- Confirm whether the PSRAM-disable state is due to `sdkconfig` (build config) or actual hardware availability.

### What should be done in the future
- Enable `CONFIG_SPIRAM` in the active `sdkconfig` and rebuild to confirm the PSRAM failure disappears.

### Code review instructions
- Review the log in `various/debug-logs/step-01-power-sweep-monitor-idf5.1.4-user.log`.
- Check sqlite run `run-2026-01-14-0041-step1-idf514` via the dashboard.

### Technical details
- Import command: `python3 .../import_log.py --run-name run-2026-01-14-0041-step1-idf514 --log .../step-01-power-sweep-monitor-idf5.1.4-user.log`.

## Step 7: Enable PSRAM in active sdkconfig and add SCCB scan task

I flipped `CONFIG_SPIRAM` on in the active `sdkconfig` for the 0041 firmware to address the PSRAM allocation failure seen in the IDF 5.1.4 log. This aligns the runtime configuration with `sdkconfig.defaults` and should unblock frame buffer allocation during `esp_camera_init()`.

I also added a ticket task to investigate the SCCB scan discrepancy (scan reports no devices while the driver probe succeeds), since it remains unresolved but is likely not the root cause.

**Commit (code):** 495f0f0 — "Debug Step 3: enable PSRAM + add SCCB task"

### What I did
- Set `CONFIG_SPIRAM=y` in `0041-atoms3r-cam-jtag-serial-test/sdkconfig`.
- Added a ticket task: “Investigate SCCB scan discrepancy (scan reports no devices but camera probe succeeds)”.

### Why
- The log shows a definitive PSRAM allocation failure, so enabling PSRAM is the next corrective step.

### What worked
- N/A (config change recorded; build/flash pending).

### What didn't work
- N/A.

### What I learned
- The runtime `sdkconfig` is authoritative; `sdkconfig.defaults` alone does not guarantee PSRAM is enabled.

### What was tricky to build
- N/A.

### What warrants a second pair of eyes
- Confirm that enabling PSRAM in `sdkconfig` is sufficient without additional PSRAM-related settings in menuconfig.

### What should be done in the future
- Rebuild/flash under IDF 5.1.4 (direnv) and re-run Step 1 to confirm PSRAM allocation succeeds.

### Code review instructions
- Review `0041-atoms3r-cam-jtag-serial-test/sdkconfig` for the PSRAM flag.
- Review `ttmp/.../tasks.md` for the new SCCB scan task.

### Technical details
- Command used: `docmgr task add --ticket MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO --text \"Investigate SCCB scan discrepancy (scan reports no devices but camera probe succeeds)\"`.
