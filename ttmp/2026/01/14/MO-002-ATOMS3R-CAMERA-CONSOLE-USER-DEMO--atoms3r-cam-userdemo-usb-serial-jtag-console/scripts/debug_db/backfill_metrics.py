#!/usr/bin/env python3
import argparse
import datetime
import sqlite3


def compute_step_changes(step_names):
    last = None
    changes = 0
    for step in step_names:
        if step is None:
            continue
        if last is None:
            last = step
            continue
        if step != last:
            changes += 1
            last = step
    return changes


def main() -> None:
    parser = argparse.ArgumentParser(description="Backfill log/step/run metrics for existing logs")
    parser.add_argument("--db", required=True, help="Path to sqlite db")
    args = parser.parse_args()

    with sqlite3.connect(args.db) as conn:
        conn.row_factory = sqlite3.Row
        conn.execute("PRAGMA foreign_keys = ON")

        logs = conn.execute("SELECT id, run_id, step_id FROM log_files").fetchall()
        for log in logs:
            line_counts = conn.execute(
                """
                SELECT
                    COUNT(*) AS total,
                    SUM(CASE WHEN level = 'E' THEN 1 ELSE 0 END) AS errors,
                    SUM(CASE WHEN level = 'W' THEN 1 ELSE 0 END) AS warns,
                    SUM(CASE WHEN level = 'I' THEN 1 ELSE 0 END) AS infos,
                    SUM(CASE WHEN level = 'D' THEN 1 ELSE 0 END) AS debugs
                FROM parsed_log_lines
                WHERE log_id = ?
                """,
                (log["id"],),
            ).fetchone()
            other = (line_counts["total"] or 0) - (line_counts["errors"] or 0) - (line_counts["warns"] or 0) - (line_counts["infos"] or 0) - (line_counts["debugs"] or 0)

            step_names = conn.execute(
                "SELECT step_name FROM parsed_log_lines WHERE log_id = ? ORDER BY line_no",
                (log["id"],),
            ).fetchall()
            step_change_count = compute_step_changes([row["step_name"] for row in step_names])

            first_ts = conn.execute(
                "SELECT ts FROM parsed_log_lines WHERE log_id = ? AND ts IS NOT NULL ORDER BY line_no ASC LIMIT 1",
                (log["id"],),
            ).fetchone()
            last_ts = conn.execute(
                "SELECT ts FROM parsed_log_lines WHERE log_id = ? AND ts IS NOT NULL ORDER BY line_no DESC LIMIT 1",
                (log["id"],),
            ).fetchone()

            conn.execute(
                """
                INSERT INTO log_metrics (
                    log_id, line_count, error_count, warn_count, info_count, debug_count, other_count,
                    step_change_count, first_ts, last_ts
                )
                VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                ON CONFLICT(log_id) DO UPDATE SET
                    line_count = excluded.line_count,
                    error_count = excluded.error_count,
                    warn_count = excluded.warn_count,
                    info_count = excluded.info_count,
                    debug_count = excluded.debug_count,
                    other_count = excluded.other_count,
                    step_change_count = excluded.step_change_count,
                    first_ts = excluded.first_ts,
                    last_ts = excluded.last_ts
                """,
                (
                    log["id"],
                    line_counts["total"] or 0,
                    line_counts["errors"] or 0,
                    line_counts["warns"] or 0,
                    line_counts["infos"] or 0,
                    line_counts["debugs"] or 0,
                    other,
                    step_change_count,
                    first_ts["ts"] if first_ts else None,
                    last_ts["ts"] if last_ts else None,
                ),
            )

        step_ids = conn.execute("SELECT DISTINCT step_id FROM log_files WHERE step_id IS NOT NULL").fetchall()
        for row in step_ids:
            step_id = row["step_id"]
            totals = conn.execute(
                """
                SELECT
                    COUNT(*) AS total,
                    SUM(CASE WHEN level = 'E' THEN 1 ELSE 0 END) AS errors,
                    SUM(CASE WHEN level = 'W' THEN 1 ELSE 0 END) AS warns,
                    SUM(CASE WHEN level = 'I' THEN 1 ELSE 0 END) AS infos,
                    SUM(CASE WHEN level = 'D' THEN 1 ELSE 0 END) AS debugs
                FROM parsed_log_lines
                WHERE log_id IN (SELECT id FROM log_files WHERE step_id = ?)
                """,
                (step_id,),
            ).fetchone()
            other = (totals["total"] or 0) - (totals["errors"] or 0) - (totals["warns"] or 0) - (totals["infos"] or 0) - (totals["debugs"] or 0)
            updated_at = datetime.datetime.utcnow().isoformat() + "Z"
            conn.execute(
                """
                INSERT INTO step_metrics (
                    step_id, line_count, error_count, warn_count, info_count, debug_count, other_count, updated_at
                )
                VALUES (?, ?, ?, ?, ?, ?, ?, ?)
                ON CONFLICT(step_id) DO UPDATE SET
                    line_count = excluded.line_count,
                    error_count = excluded.error_count,
                    warn_count = excluded.warn_count,
                    info_count = excluded.info_count,
                    debug_count = excluded.debug_count,
                    other_count = excluded.other_count,
                    updated_at = excluded.updated_at
                """,
                (step_id, totals["total"] or 0, totals["errors"] or 0, totals["warns"] or 0, totals["infos"] or 0, totals["debugs"] or 0, other, updated_at),
            )

        run_ids = conn.execute("SELECT id FROM debug_runs").fetchall()
        for row in run_ids:
            run_id = row["id"]
            totals = conn.execute(
                """
                SELECT
                    COUNT(*) AS total,
                    SUM(CASE WHEN level = 'E' THEN 1 ELSE 0 END) AS errors,
                    SUM(CASE WHEN level = 'W' THEN 1 ELSE 0 END) AS warns,
                    SUM(CASE WHEN level = 'I' THEN 1 ELSE 0 END) AS infos,
                    SUM(CASE WHEN level = 'D' THEN 1 ELSE 0 END) AS debugs
                FROM parsed_log_lines
                WHERE log_id IN (SELECT id FROM log_files WHERE run_id = ?)
                """,
                (run_id,),
            ).fetchone()
            other = (totals["total"] or 0) - (totals["errors"] or 0) - (totals["warns"] or 0) - (totals["infos"] or 0) - (totals["debugs"] or 0)
            updated_at = datetime.datetime.utcnow().isoformat() + "Z"
            conn.execute(
                """
                INSERT INTO run_metrics (
                    run_id, line_count, error_count, warn_count, info_count, debug_count, other_count, updated_at
                )
                VALUES (?, ?, ?, ?, ?, ?, ?, ?)
                ON CONFLICT(run_id) DO UPDATE SET
                    line_count = excluded.line_count,
                    error_count = excluded.error_count,
                    warn_count = excluded.warn_count,
                    info_count = excluded.info_count,
                    debug_count = excluded.debug_count,
                    other_count = excluded.other_count,
                    updated_at = excluded.updated_at
                """,
                (run_id, totals["total"] or 0, totals["errors"] or 0, totals["warns"] or 0, totals["infos"] or 0, totals["debugs"] or 0, other, updated_at),
            )

    print("Backfill complete.")


if __name__ == "__main__":
    main()
