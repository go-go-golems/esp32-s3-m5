#!/usr/bin/env python3
import argparse
import sqlite3


def table_exists(conn: sqlite3.Connection, name: str) -> bool:
    row = conn.execute(
        "SELECT name FROM sqlite_master WHERE type='table' AND name = ?",
        (name,),
    ).fetchone()
    return row is not None


def fmt_int(value) -> str:
    return str(value or 0)


def fmt_size(value) -> str:
    if value is None:
        return "0B"
    if value < 1024:
        return f"{value}B"
    if value < 1024 * 1024:
        return f"{value / 1024:.1f}KB"
    return f"{value / (1024 * 1024):.1f}MB"


def print_run_list(conn: sqlite3.Connection) -> None:
    rows = conn.execute(
        """
        SELECT r.id, r.name, r.ticket, r.firmware, r.git_hash, r.started_at,
               (SELECT COUNT(*) FROM debug_steps s WHERE s.run_id = r.id) AS steps,
               (SELECT COUNT(*) FROM log_files l WHERE l.run_id = r.id) AS logs,
               (SELECT COUNT(*) FROM parsed_log_lines p JOIN log_files l ON l.id = p.log_id WHERE l.run_id = r.id AND p.level = 'E') AS errors,
               (SELECT COUNT(*) FROM parsed_log_lines p JOIN log_files l ON l.id = p.log_id WHERE l.run_id = r.id AND p.level = 'W') AS warns
        FROM debug_runs r
        ORDER BY r.started_at DESC
        """
    ).fetchall()
    if not rows:
        print("No runs found.")
        return
    print("Runs:")
    for row in rows:
        print(
            f"- {row['name']} (id={row['id']} ticket={row['ticket']} firmware={row['firmware']} "
            f"git={row['git_hash']} steps={row['steps']} logs={row['logs']} errors={row['errors']} warns={row['warns']})"
        )


def print_run_summary(conn: sqlite3.Connection, run) -> None:
    print("Run summary:")
    print(f"- name: {run['name']}")
    print(f"- id: {run['id']}")
    print(f"- ticket: {run['ticket']}")
    print(f"- firmware: {run['firmware']}")
    print(f"- git_hash: {run['git_hash']}")
    print(f"- idf_version: {run['idf_version']}")
    print(f"- device_port: {run['device_port']}")
    print(f"- started_at: {run['started_at']}")
    if run["notes"]:
        print(f"- notes: {run['notes']}")


def print_run_metrics(conn: sqlite3.Connection, run_id: int) -> None:
    metrics = None
    if table_exists(conn, "run_metrics"):
        metrics = conn.execute(
            """
            SELECT line_count, error_count, warn_count, info_count, debug_count, other_count, updated_at
            FROM run_metrics
            WHERE run_id = ?
            """,
            (run_id,),
        ).fetchone()
    if metrics:
        print("Run metrics:")
        print(
            f"- lines={fmt_int(metrics['line_count'])} errors={fmt_int(metrics['error_count'])} "
            f"warns={fmt_int(metrics['warn_count'])} infos={fmt_int(metrics['info_count'])} "
            f"debugs={fmt_int(metrics['debug_count'])} other={fmt_int(metrics['other_count'])}"
        )
        print(f"- updated_at: {metrics['updated_at']}")
        return

    row = conn.execute(
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
    other = (row["total"] or 0) - (row["errors"] or 0) - (row["warns"] or 0) - (row["infos"] or 0) - (row["debugs"] or 0)
    print("Run metrics:")
    print(
        f"- lines={fmt_int(row['total'])} errors={fmt_int(row['errors'])} warns={fmt_int(row['warns'])} "
        f"infos={fmt_int(row['infos'])} debugs={fmt_int(row['debugs'])} other={fmt_int(other)}"
    )


def print_steps(conn: sqlite3.Connection, run_id: int) -> None:
    steps = conn.execute(
        """
        SELECT s.id, s.name, s.result, s.started_at, s.ended_at,
               sm.line_count, sm.error_count, sm.warn_count, sm.info_count, sm.debug_count, sm.other_count,
               (SELECT COUNT(*) FROM log_files l WHERE l.step_id = s.id) AS logs
        FROM debug_steps s
        LEFT JOIN step_metrics sm ON sm.step_id = s.id
        WHERE s.run_id = ?
        ORDER BY s.started_at ASC
        """,
        (run_id,),
    ).fetchall()
    print("Steps:")
    if not steps:
        print("- (none)")
        return
    for step in steps:
        print(
            f"- {step['name']} (id={step['id']} result={step['result']} logs={step['logs']} "
            f"lines={fmt_int(step['line_count'])} errors={fmt_int(step['error_count'])} warns={fmt_int(step['warn_count'])} "
            f"infos={fmt_int(step['info_count'])} debugs={fmt_int(step['debug_count'])} other={fmt_int(step['other_count'])})"
        )
        if step["started_at"] or step["ended_at"]:
            print(f"  started_at={step['started_at']} ended_at={step['ended_at']}")


def print_log_files(conn: sqlite3.Connection, run_id: int) -> None:
    rows = conn.execute(
        """
        SELECT lf.id, lf.path, lf.sha256, lf.size_bytes, lf.created_at, lf.kind, s.name AS step_name,
               lm.line_count, lm.error_count, lm.warn_count, lm.info_count, lm.debug_count, lm.other_count, lm.step_change_count
        FROM log_files lf
        LEFT JOIN debug_steps s ON s.id = lf.step_id
        LEFT JOIN log_metrics lm ON lm.log_id = lf.id
        WHERE lf.run_id = ?
        ORDER BY lf.created_at ASC
        """,
        (run_id,),
    ).fetchall()
    print("Log files:")
    if not rows:
        print("- (none)")
        return
    for row in rows:
        short_hash = row["sha256"][:12] if row["sha256"] else "unknown"
        metrics = (
            f"lines={fmt_int(row['line_count'])} errors={fmt_int(row['error_count'])} warns={fmt_int(row['warn_count'])} "
            f"infos={fmt_int(row['info_count'])} debugs={fmt_int(row['debug_count'])} other={fmt_int(row['other_count'])} "
            f"step_changes={fmt_int(row['step_change_count'])}"
        )
        print(
            f"- {row['path']} (id={row['id']} step={row['step_name']} kind={row['kind']} "
            f"size={fmt_size(row['size_bytes'])} sha256={short_hash})"
        )
        print(f"  created_at={row['created_at']} {metrics}")


def print_step_name_summary(conn: sqlite3.Connection, run_id: int) -> None:
    rows = conn.execute(
        """
        SELECT p.step_name,
               COUNT(*) AS total,
               SUM(CASE WHEN p.level = 'E' THEN 1 ELSE 0 END) AS errors,
               SUM(CASE WHEN p.level = 'W' THEN 1 ELSE 0 END) AS warns,
               SUM(CASE WHEN p.level = 'I' THEN 1 ELSE 0 END) AS infos,
               SUM(CASE WHEN p.level = 'D' THEN 1 ELSE 0 END) AS debugs
        FROM parsed_log_lines p
        JOIN log_files lf ON lf.id = p.log_id
        WHERE lf.run_id = ?
        GROUP BY p.step_name
        ORDER BY total DESC
        """,
        (run_id,),
    ).fetchall()
    print("Step markers (from parsed logs):")
    if not rows:
        print("- (none)")
        return
    for row in rows:
        other = (row["total"] or 0) - (row["errors"] or 0) - (row["warns"] or 0) - (row["infos"] or 0) - (row["debugs"] or 0)
        print(
            f"- {row['step_name']} (lines={fmt_int(row['total'])} errors={fmt_int(row['errors'])} warns={fmt_int(row['warns'])} "
            f"infos={fmt_int(row['infos'])} debugs={fmt_int(row['debugs'])} other={fmt_int(other)})"
        )


def print_top_tags(conn: sqlite3.Connection, run_id: int, level: str, limit: int) -> None:
    rows = conn.execute(
        """
        SELECT p.tag, COUNT(*) AS total
        FROM parsed_log_lines p
        JOIN log_files lf ON lf.id = p.log_id
        WHERE lf.run_id = ? AND p.level = ? AND p.tag IS NOT NULL
        GROUP BY p.tag
        ORDER BY total DESC
        LIMIT ?
        """,
        (run_id, level, limit),
    ).fetchall()
    label = "errors" if level == "E" else "warnings"
    print(f"Top tags ({label}):")
    if not rows:
        print("- (none)")
        return
    for row in rows:
        print(f"- {row['tag']}: {row['total']}")


def print_recent_lines(conn: sqlite3.Connection, run_id: int, level: str, limit: int) -> None:
    rows = conn.execute(
        """
        SELECT p.ts, p.tag, p.message, p.step_name
        FROM parsed_log_lines p
        JOIN log_files lf ON lf.id = p.log_id
        WHERE lf.run_id = ? AND p.level = ?
        ORDER BY p.id DESC
        LIMIT ?
        """,
        (run_id, level, limit),
    ).fetchall()
    label = "errors" if level == "E" else "warnings"
    print(f"Recent {label}:")
    if not rows:
        print("- (none)")
        return
    for row in rows:
        print(f"- [{row['ts']}] {row['tag']}: {row['message']} (step={row['step_name']})")


def main() -> None:
    parser = argparse.ArgumentParser(description="Verbose dashboard for 0041 debug sqlite db")
    parser.add_argument("--db", required=True, help="Path to sqlite db")
    parser.add_argument("--run-name", default="", help="Filter by run name")
    parser.add_argument("--limit-errors", type=int, default=25, help="Max error lines to show")
    parser.add_argument("--limit-warns", type=int, default=25, help="Max warning lines to show")
    parser.add_argument("--limit-tags", type=int, default=10, help="Max tags to show")
    args = parser.parse_args()

    with sqlite3.connect(args.db) as conn:
        conn.row_factory = sqlite3.Row
        if not args.run_name:
            print_run_list(conn)
            return

        run = conn.execute(
            """
            SELECT id, name, ticket, firmware, git_hash, idf_version, device_port, started_at, notes
            FROM debug_runs
            WHERE name = ?
            """,
            (args.run_name,),
        ).fetchone()
        if not run:
            print(f"Run not found: {args.run_name}")
            return

        print_run_summary(conn, run)
        print_run_metrics(conn, run["id"])
        print_steps(conn, run["id"])
        print_log_files(conn, run["id"])
        print_step_name_summary(conn, run["id"])
        print_top_tags(conn, run["id"], "E", args.limit_tags)
        print_top_tags(conn, run["id"], "W", args.limit_tags)
        print_recent_lines(conn, run["id"], "E", args.limit_errors)
        print_recent_lines(conn, run["id"], "W", args.limit_warns)


if __name__ == "__main__":
    main()
