# ESP32-S3-M5 Project History Analysis Scripts

This directory contains scripts for analyzing the git history of the ESP32-S3-M5 project and generating insights.

## Setup

1. **Populate the database**:
   ```bash
   python3 debug-history-scripts/01-populate-database.py
   ```

2. **Extract tutorial information**:
   ```bash
   python3 debug-history-scripts/02-extract-tutorials.py
   ```

3. **Run analysis**:
   ```bash
   python3 debug-history-scripts/03-analyze-project.py
   ```

4. **View live dashboard**:
   ```bash
   ./debug-history-scripts/dashboard.sh
   ```

## Scripts

### `01-populate-database.py`
Populates a SQLite database (`project_history.db`) with:
- Commit metadata (hash, author, date, message)
- File change statistics (insertions, deletions per file)
- Commit-level statistics

### `02-extract-tutorials.py`
Extracts tutorial information from commit messages and populates the `tutorials` table with:
- Tutorial IDs (0001-0019)
- Creation dates
- Last modification dates

### `03-analyze-project.py`
Generates a comprehensive analysis report including:
- Timeline analysis
- Tutorial progression
- Most active files
- File type breakdown
- Commit message patterns
- Directory structure analysis

### `dashboard.sh`
Live-reloading dashboard showing:
- Project statistics (commits, insertions, deletions, unique files)
- Daily activity breakdown
- Tutorial progress
- File type breakdown
- Most active files
- Recent commits

Press `Ctrl+C` to exit the dashboard.

## Database Schema

The SQLite database (`project_history.db`) contains:

- **commits**: Commit metadata and statistics
- **file_changes**: Per-file change tracking
- **tutorials**: Tutorial project tracking

## Usage Example

```bash
# Full workflow
cd esp32-s3-m5
python3 debug-history-scripts/01-populate-database.py
python3 debug-history-scripts/02-extract-tutorials.py
python3 debug-history-scripts/03-analyze-project.py

# View live dashboard (refreshes every 2 seconds)
./debug-history-scripts/dashboard.sh
```

## Notes

- The database is created in the current directory (`project_history.db`)
- The dashboard refreshes every 2 seconds by default (configurable via `REFRESH_INTERVAL`)
- All scripts assume they're run from the `esp32-s3-m5` directory

