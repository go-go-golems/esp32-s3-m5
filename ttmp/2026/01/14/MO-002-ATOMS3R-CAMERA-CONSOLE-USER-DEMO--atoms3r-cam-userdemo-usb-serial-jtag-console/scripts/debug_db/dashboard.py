#!/usr/bin/env python3
import argparse
import sqlite3


def main() -> None:
    parser = argparse.ArgumentParser(description="Dashboard for 0041 debug sqlite db")
    parser.add_argument("--db", required=True, help="Path to sqlite db")
    parser.add_argument("--run-name", default="", help="Filter by run name")
    parser.add_argument("--limit-errors", type=int, default=10, help="Max error lines to show")
    args = parser.parse_args()

    with sqlite3.connect(args.db) as conn:
        conn.row_factory = sqlite3.Row
        if not args.run_name:
            rows = conn.execute(
                "SELECT id, name, ticket, firmware, git_hash, started_at FROM debug_runs ORDER BY started_at DESC"
            ).fetchall()
            if not rows:
                print("No runs found.")
                return
            print("Runs:")
            for row in rows:
                print(f"- {row['name']} (id={row['id']} ticket={row['ticket']} firmware={row['firmware']} git={row['git_hash']})")
            return

        run = conn.execute(
            "SELECT id, name, ticket, firmware, git_hash, started_at FROM debug_runs WHERE name = ?",
            (args.run_name,),
        ).fetchone()
        if not run:
            print(f"Run not found: {args.run_name}")
            return

        print(f"Run: {run['name']} (id={run['id']} ticket={run['ticket']} firmware={run['firmware']} git={run['git_hash']})")
        steps = conn.execute(
            """
            SELECT s.id, s.name, s.result, s.started_at,
                   SUM(CASE WHEN p.level = 'E' THEN 1 ELSE 0 END) AS errors,
                   SUM(CASE WHEN p.level = 'W' THEN 1 ELSE 0 END) AS warns
            FROM debug_steps s
            LEFT JOIN log_files lf ON lf.step_id = s.id
            LEFT JOIN parsed_log_lines p ON p.log_id = lf.id
            WHERE s.run_id = ?
            GROUP BY s.id
            ORDER BY s.started_at ASC
            """,
            (run["id"],),
        ).fetchall()
        print("Steps:")
        for step in steps:
            print(f"- {step['name']} (id={step['id']} result={step['result']} errors={step['errors'] or 0} warns={step['warns'] or 0})")

        print("\nRecent errors:")
        errors = conn.execute(
            """
            SELECT p.ts, p.tag, p.message, p.step_name
            FROM parsed_log_lines p
            JOIN log_files lf ON lf.id = p.log_id
            JOIN debug_steps s ON s.id = lf.step_id
            WHERE s.run_id = ? AND p.level = 'E'
            ORDER BY p.id DESC
            LIMIT ?
            """,
            (run["id"], args.limit_errors),
        ).fetchall()
        if not errors:
            print("(none)")
            return
        for err in errors:
            print(f"- [{err['ts']}] {err['tag']}: {err['message']} (step={err['step_name']})")


if __name__ == "__main__":
    main()
