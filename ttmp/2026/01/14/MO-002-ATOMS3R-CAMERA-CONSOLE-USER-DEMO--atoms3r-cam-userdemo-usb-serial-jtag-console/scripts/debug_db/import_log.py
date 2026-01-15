#!/usr/bin/env python3
import argparse
import datetime
import hashlib
import pathlib
import re
import sqlite3
from typing import Optional, Tuple

IDF_RE = re.compile(r"^(?P<level>[IWEVD]) \((?P<ms>\d+)\) (?P<tag>[^:]+): (?P<msg>.*)$")
BRACKET_RE = re.compile(r"^\[(?P<time>\d{2}:\d{2}:\d{2})\] \[(?P<level>\w)\] (?P<msg>.*)$")
STEP_RE = re.compile(r"STEP: (?P<step>.+)$")


def compute_sha256(path: pathlib.Path) -> str:
    hasher = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            hasher.update(chunk)
    return hasher.hexdigest()


def get_or_create_run(conn: sqlite3.Connection, name: str, ticket: str, firmware: str,
                      git_hash: str, idf_version: str, device_port: str, notes: str) -> int:
    row = conn.execute("SELECT id FROM debug_runs WHERE name = ?", (name,)).fetchone()
    if row:
        return row[0]
    started_at = datetime.datetime.utcnow().isoformat() + "Z"
    cur = conn.execute(
        """
        INSERT INTO debug_runs (name, ticket, firmware, git_hash, idf_version, device_port, started_at, notes)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
        """,
        (name, ticket, firmware, git_hash, idf_version, device_port, started_at, notes),
    )
    return cur.lastrowid


def get_or_create_step(conn: sqlite3.Connection, run_id: int, name: str, description: str,
                       analysis_ref: str, file_refs: str, function_refs: str,
                       result: str, notes: str) -> int:
    row = conn.execute(
        "SELECT id FROM debug_steps WHERE run_id = ? AND name = ?",
        (run_id, name),
    ).fetchone()
    if row:
        step_id = row[0]
        conn.execute(
            """
            UPDATE debug_steps
            SET description = COALESCE(?, description),
                analysis_ref = COALESCE(?, analysis_ref),
                file_refs = COALESCE(?, file_refs),
                function_refs = COALESCE(?, function_refs),
                result = COALESCE(?, result),
                notes = COALESCE(?, notes)
            WHERE id = ?
            """,
            (description or None, analysis_ref or None, file_refs or None, function_refs or None,
             result or None, notes or None, step_id),
        )
        return step_id

    started_at = datetime.datetime.utcnow().isoformat() + "Z"
    cur = conn.execute(
        """
        INSERT INTO debug_steps (run_id, name, description, analysis_ref, file_refs, function_refs, started_at, result, notes)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
        """,
        (run_id, name, description, analysis_ref, file_refs, function_refs, started_at, result, notes),
    )
    return cur.lastrowid


def parse_line(raw: str) -> Tuple[Optional[str], Optional[str], Optional[str], Optional[str]]:
    raw = raw.rstrip("\n")
    match = IDF_RE.match(raw)
    if match:
        level = match.group("level")
        tag = match.group("tag").strip()
        message = match.group("msg").strip()
        ts = match.group("ms")
        return ts, f"{level}", tag, message

    match = BRACKET_RE.match(raw)
    if match:
        level = match.group("level")
        message = match.group("msg").strip()
        ts = match.group("time")
        return ts, level, None, message

    return None, None, None, raw.strip()


def main() -> None:
    parser = argparse.ArgumentParser(description="Import a debug log into the sqlite database")
    parser.add_argument("--db", required=True, help="Path to sqlite db")
    parser.add_argument("--log", required=True, help="Path to log file")
    parser.add_argument("--run-name", required=True, help="Debug run name (unique)")
    parser.add_argument("--step-name", required=True, help="Debug step name")
    parser.add_argument("--ticket", default="MO-002-ATOMS3R-CAMERA-CONSOLE-USER-DEMO")
    parser.add_argument("--firmware", default="0041-atoms3r-cam-jtag-serial-test")
    parser.add_argument("--git-hash", default="")
    parser.add_argument("--idf-version", default="")
    parser.add_argument("--device-port", default="")
    parser.add_argument("--step-desc", default="")
    parser.add_argument("--analysis-ref", default="")
    parser.add_argument("--file-refs", default="")
    parser.add_argument("--function-refs", default="")
    parser.add_argument("--result", default="")
    parser.add_argument("--notes", default="")
    parser.add_argument("--kind", default="raw")
    args = parser.parse_args()

    db_path = pathlib.Path(args.db)
    log_path = pathlib.Path(args.log)
    if not log_path.exists():
        raise SystemExit(f"Log file not found: {log_path}")

    sha256 = compute_sha256(log_path)
    size_bytes = log_path.stat().st_size
    created_at = datetime.datetime.utcnow().isoformat() + "Z"

    with sqlite3.connect(db_path) as conn:
        conn.execute("PRAGMA foreign_keys = ON")
        run_id = get_or_create_run(
            conn,
            args.run_name,
            args.ticket,
            args.firmware,
            args.git_hash,
            args.idf_version,
            args.device_port,
            args.notes,
        )
        step_id = get_or_create_step(
            conn,
            run_id,
            args.step_name,
            args.step_desc,
            args.analysis_ref,
            args.file_refs,
            args.function_refs,
            args.result,
            args.notes,
        )

        cur = conn.execute(
            """
            INSERT INTO log_files (run_id, step_id, path, sha256, size_bytes, created_at, kind)
            VALUES (?, ?, ?, ?, ?, ?, ?)
            """,
            (run_id, step_id, str(log_path), sha256, size_bytes, created_at, args.kind),
        )
        log_id = cur.lastrowid

        current_step = args.step_name
        raw_rows = []
        parsed_rows = []
        with log_path.open("r", encoding="utf-8", errors="replace") as handle:
            for idx, line in enumerate(handle, start=1):
                raw_rows.append((log_id, idx, line.rstrip("\n")))
                step_match = STEP_RE.search(line)
                if step_match:
                    current_step = step_match.group("step").strip()

                ts, level, tag, message = parse_line(line)
                parsed_rows.append((log_id, idx, ts, level, tag, message, current_step, line.rstrip("\n")))

        conn.executemany(
            "INSERT INTO raw_log_lines (log_id, line_no, raw_text) VALUES (?, ?, ?)",
            raw_rows,
        )
        conn.executemany(
            """
            INSERT INTO parsed_log_lines (log_id, line_no, ts, level, tag, message, step_name, raw_text)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?)
            """,
            parsed_rows,
        )

    print(f"Imported log: {log_path}\nrun_id={run_id} step_id={step_id} log_id={log_id}")


if __name__ == "__main__":
    main()
