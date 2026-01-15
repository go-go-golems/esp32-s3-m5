PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS debug_runs (
    id INTEGER PRIMARY KEY,
    name TEXT UNIQUE NOT NULL,
    ticket TEXT,
    firmware TEXT,
    git_hash TEXT,
    idf_version TEXT,
    device_port TEXT,
    started_at TEXT,
    notes TEXT
);

CREATE TABLE IF NOT EXISTS debug_steps (
    id INTEGER PRIMARY KEY,
    run_id INTEGER NOT NULL,
    name TEXT NOT NULL,
    description TEXT,
    analysis_ref TEXT,
    file_refs TEXT,
    function_refs TEXT,
    started_at TEXT,
    ended_at TEXT,
    result TEXT,
    notes TEXT,
    UNIQUE(run_id, name),
    FOREIGN KEY(run_id) REFERENCES debug_runs(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS log_files (
    id INTEGER PRIMARY KEY,
    run_id INTEGER NOT NULL,
    step_id INTEGER,
    path TEXT NOT NULL,
    sha256 TEXT,
    size_bytes INTEGER,
    created_at TEXT,
    kind TEXT,
    FOREIGN KEY(run_id) REFERENCES debug_runs(id) ON DELETE CASCADE,
    FOREIGN KEY(step_id) REFERENCES debug_steps(id) ON DELETE SET NULL
);

CREATE TABLE IF NOT EXISTS raw_log_lines (
    id INTEGER PRIMARY KEY,
    log_id INTEGER NOT NULL,
    line_no INTEGER NOT NULL,
    raw_text TEXT NOT NULL,
    FOREIGN KEY(log_id) REFERENCES log_files(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS parsed_log_lines (
    id INTEGER PRIMARY KEY,
    log_id INTEGER NOT NULL,
    line_no INTEGER NOT NULL,
    ts TEXT,
    level TEXT,
    tag TEXT,
    message TEXT,
    step_name TEXT,
    raw_text TEXT,
    FOREIGN KEY(log_id) REFERENCES log_files(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS log_metrics (
    log_id INTEGER PRIMARY KEY,
    line_count INTEGER,
    error_count INTEGER,
    warn_count INTEGER,
    info_count INTEGER,
    debug_count INTEGER,
    other_count INTEGER,
    step_change_count INTEGER,
    first_ts TEXT,
    last_ts TEXT,
    FOREIGN KEY(log_id) REFERENCES log_files(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS step_metrics (
    step_id INTEGER PRIMARY KEY,
    line_count INTEGER,
    error_count INTEGER,
    warn_count INTEGER,
    info_count INTEGER,
    debug_count INTEGER,
    other_count INTEGER,
    updated_at TEXT,
    FOREIGN KEY(step_id) REFERENCES debug_steps(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS run_metrics (
    run_id INTEGER PRIMARY KEY,
    line_count INTEGER,
    error_count INTEGER,
    warn_count INTEGER,
    info_count INTEGER,
    debug_count INTEGER,
    other_count INTEGER,
    updated_at TEXT,
    FOREIGN KEY(run_id) REFERENCES debug_runs(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS debug_artifacts (
    id INTEGER PRIMARY KEY,
    run_id INTEGER NOT NULL,
    step_id INTEGER,
    kind TEXT,
    path TEXT,
    notes TEXT,
    created_at TEXT,
    FOREIGN KEY(run_id) REFERENCES debug_runs(id) ON DELETE CASCADE,
    FOREIGN KEY(step_id) REFERENCES debug_steps(id) ON DELETE SET NULL
);

CREATE TABLE IF NOT EXISTS debug_kv (
    id INTEGER PRIMARY KEY,
    scope TEXT NOT NULL,
    scope_id INTEGER,
    key TEXT NOT NULL,
    value TEXT
);

CREATE INDEX IF NOT EXISTS idx_raw_log_lines_log_id ON raw_log_lines(log_id);
CREATE INDEX IF NOT EXISTS idx_parsed_log_lines_log_id ON parsed_log_lines(log_id);
CREATE INDEX IF NOT EXISTS idx_parsed_log_lines_level ON parsed_log_lines(level);
CREATE INDEX IF NOT EXISTS idx_log_files_run_id ON log_files(run_id);
CREATE INDEX IF NOT EXISTS idx_debug_steps_run_id ON debug_steps(run_id);
