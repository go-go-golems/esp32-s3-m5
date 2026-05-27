#!/usr/bin/env python3
"""
Tab5 Benchmark Analysis Queries

Run pre-built analysis queries against the benchmark SQLite database.
Pass the path to the DB file as the first argument.

Usage:
    python3 02-analyze-results.py bench_results.db
    python3 02-analyze-results.py bench_results.db --query throughput
    python3 02-analyze-results.py bench_results.db --query segments --run-id 1
"""

import argparse
import sqlite3
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).parent


def print_query(db_path: Path, title: str, sql: str, params=()):
    """Run a query and print results as a formatted table."""
    conn = sqlite3.connect(db_path)
    c = conn.cursor()
    c.execute(sql, params)
    rows = c.fetchall()
    cols = [desc[0] for desc in c.description] if c.description else []

    print(f"\n{'=' * 72}")
    print(f"  {title}")
    print(f"{'=' * 72}")
    if not rows:
        print("  (no data)")
        conn.close()
        return

    # Calculate column widths
    widths = [len(col) for col in cols]
    for row in rows:
        for i, val in enumerate(row):
            widths[i] = max(widths[i], len(str(val)))

    # Header
    header = "  ".join(col.ljust(widths[i]) for i, col in enumerate(cols))
    print(header)
    print("  ".join("-" * widths[i] for i in range(len(cols))))

    # Rows
    for row in rows:
        print("  ".join(str(val).ljust(widths[i]) for i, val in enumerate(row)))

    conn.close()


def query_overview(db_path: Path):
    """List all runs."""
    print_query(db_path, "All Benchmark Runs", """
        SELECT id, timestamp, base_url, mode, notes
        FROM runs
        ORDER BY id DESC
    """)


def query_throughput(db_path: Path, run_id: int = None):
    """Upload throughput by payload size and compression."""
    where = f"WHERE ur.run_id = {run_id}" if run_id else ""
    print_query(db_path, "Upload Throughput (avg over repeats)", f"""
        SELECT
            r.mode,
            CASE ur.payload_size
                WHEN 1024 THEN '1KB'
                WHEN 10240 THEN '10KB'
                WHEN 102400 THEN '100KB'
                WHEN 512000 THEN '500KB'
                WHEN 1048576 THEN '1MB'
                WHEN 1843200 THEN '1.8MB'
                ELSE ur.payload_size || 'B'
            END AS size,
            ur.compression,
            COUNT(*) AS n,
            ROUND(AVG(ur.browser_ms), 0) AS browser_ms,
            ROUND(AVG(ur.server_recv_us / 1000.0), 0) AS recv_ms,
            ROUND(AVG(ur.server_decompress_us / 1000.0), 1) AS decomp_ms,
            ROUND(AVG(ur.recv_kbps), 0) AS recv_kbps,
            ROUND(AVG(ur.rssi), 0) AS rssi
        FROM upload_results ur
        JOIN runs r ON ur.run_id = r.id
        {where}
        GROUP BY r.mode, ur.payload_size, ur.compression
        ORDER BY r.mode, ur.payload_size, ur.compression
    """)


def query_download_throughput(db_path: Path, run_id: int = None):
    """Download throughput by payload size."""
    where = f"WHERE dr.run_id = {run_id}" if run_id else ""
    print_query(db_path, "Download Throughput (avg over repeats)", f"""
        SELECT
            r.mode,
            CASE dr.payload_size
                WHEN 1024 THEN '1KB'
                WHEN 10240 THEN '10KB'
                WHEN 102400 THEN '100KB'
                WHEN 512000 THEN '500KB'
                WHEN 1048576 THEN '1MB'
                WHEN 1843200 THEN '1.8MB'
                ELSE dr.payload_size || 'B'
            END AS size,
            COUNT(*) AS n,
            ROUND(AVG(dr.browser_ms), 0) AS browser_ms,
            ROUND(AVG(dr.recv_kbps), 0) AS recv_kbps
        FROM download_results dr
        JOIN runs r ON dr.run_id = r.id
        {where}
        GROUP BY r.mode, dr.payload_size
        ORDER BY r.mode, dr.payload_size
    """)


def query_ping(db_path: Path, run_id: int = None):
    """Ping RTT by payload size."""
    where = f"WHERE pr.run_id = {run_id}" if run_id else ""
    print_query(db_path, "Ping RTT (avg over repeats)", f"""
        SELECT
            r.mode,
            pr.payload_size AS size_bytes,
            COUNT(*) AS n,
            ROUND(AVG(pr.rtt_ms), 1) AS avg_rtt_ms,
            ROUND(MIN(pr.rtt_ms), 1) AS min_rtt_ms,
            ROUND(MAX(pr.rtt_ms), 1) AS max_rtt_ms
        FROM ping_results pr
        JOIN runs r ON pr.run_id = r.id
        {where}
        GROUP BY r.mode, pr.payload_size
        ORDER BY r.mode, pr.payload_size
    """)


def query_segments(db_path: Path, run_id: int, payload_size: int = None):
    """Segment timing analysis for upload results."""
    extra = f" AND ur.payload_size = {payload_size}" if payload_size else ""
    print_query(db_path, "Segment Timing Stats (1.8MB raw upload)", f"""
        SELECT
            ur.id AS upload_id,
            ur.payload_size,
            ur.compression,
            COUNT(s.id) AS seg_count,
            ROUND(AVG(s.delta_us) / 1000.0, 2) AS avg_delta_ms,
            ROUND(MIN(s.delta_us) / 1000.0, 2) AS min_delta_ms,
            ROUND(MAX(s.delta_us) / 1000.0, 2) AS max_delta_ms,
            ROUND(AVG(s.bytes), 0) AS avg_seg_bytes,
            ROUND(MIN(s.bytes), 0) AS min_seg_bytes,
            ROUND(MAX(s.bytes), 0) AS max_seg_bytes,
            SUM(CASE WHEN s.delta_us > 50000 THEN 1 ELSE 0 END) AS stalls_gt_50ms,
            SUM(CASE WHEN s.delta_us > 10000 THEN 1 ELSE 0 END) AS gaps_gt_10ms
        FROM segments s
        JOIN upload_results ur ON s.upload_result_id = ur.id
        WHERE ur.run_id = ?{extra.replace('?', '?')}
        GROUP BY ur.id
        ORDER BY ur.payload_size, ur.compression
    """, (run_id,) if not payload_size else (run_id,))


def query_gap_histogram(db_path: Path, upload_id: int):
    """Distribution of inter-segment gaps for a specific upload."""
    print_query(db_path, f"Inter-Segment Gap Histogram (upload_id={upload_id})", """
        SELECT
            CASE
                WHEN delta_us < 100 THEN '<0.1ms'
                WHEN delta_us < 500 THEN '0.1-0.5ms'
                WHEN delta_us < 1000 THEN '0.5-1ms'
                WHEN delta_us < 5000 THEN '1-5ms'
                WHEN delta_us < 10000 THEN '5-10ms'
                WHEN delta_us < 50000 THEN '10-50ms'
                WHEN delta_us < 100000 THEN '50-100ms'
                ELSE '>100ms'
            END AS gap_range,
            COUNT(*) AS count,
            ROUND(AVG(delta_us) / 1000.0, 2) AS avg_ms
        FROM segments
        WHERE upload_result_id = ? AND delta_us IS NOT NULL
        GROUP BY gap_range
        ORDER BY MIN(delta_us)
    """, (upload_id,))


def query_softap_vs_sta(db_path: Path):
    """Compare SoftAP vs STA throughput for same payload sizes."""
    print_query(db_path, "SoftAP vs STA Throughput Comparison", """
        SELECT
            ur.payload_size,
            ur.compression,
            ROUND(AVG(CASE WHEN r.mode = 'SoftAP' THEN ur.recv_kbps END), 0) AS softap_kbps,
            ROUND(AVG(CASE WHEN r.mode = 'STA' THEN ur.recv_kbps END), 0) AS sta_kbps,
            ROUND(AVG(CASE WHEN r.mode = 'SoftAP' THEN ur.browser_ms END), 0) AS softap_ms,
            ROUND(AVG(CASE WHEN r.mode = 'STA' THEN ur.browser_ms END), 0) AS sta_ms
        FROM upload_results ur
        JOIN runs r ON ur.run_id = r.id
        GROUP BY ur.payload_size, ur.compression
        ORDER BY ur.payload_size, ur.compression
    """)


def query_raw_vs_deflate(db_path: Path):
    """Compare raw vs deflate upload times."""
    print_query(db_path, "Raw vs Deflate Upload Comparison", """
        SELECT
            ur.payload_size,
            ROUND(AVG(CASE WHEN ur.compression = 'raw' THEN ur.browser_ms END), 0) AS raw_browser_ms,
            ROUND(AVG(CASE WHEN ur.compression = 'deflate' THEN ur.browser_ms END), 0) AS deflate_browser_ms,
            ROUND(AVG(CASE WHEN ur.compression = 'raw' THEN ur.server_recv_us / 1000.0 END), 0) AS raw_recv_ms,
            ROUND(AVG(CASE WHEN ur.compression = 'deflate' THEN ur.server_recv_us / 1000.0 END), 0) AS deflate_recv_ms,
            ROUND(AVG(CASE WHEN ur.compression = 'deflate' THEN ur.server_decompress_us / 1000.0 END), 1) AS decomp_ms
        FROM upload_results ur
        GROUP BY ur.payload_size
        ORDER BY ur.payload_size
    """)


def main():
    parser = argparse.ArgumentParser(description="Tab5 Benchmark Analysis")
    parser.add_argument("db", help="Path to SQLite database")
    parser.add_argument("--run-id", type=int, help="Filter to specific run ID")
    parser.add_argument("--query", default="all",
                        choices=["all", "overview", "throughput", "download",
                                 "ping", "segments", "softap-vs-sta",
                                 "raw-vs-deflate", "gaps"],
                        help="Which analysis query to run")
    parser.add_argument("--upload-id", type=int, help="Upload ID for gap histogram")
    args = parser.parse_args()

    db_path = Path(args.db)
    if not db_path.exists():
        print(f"Database not found: {db_path}")
        sys.exit(1)

    queries = {
        "overview": lambda: query_overview(db_path),
        "throughput": lambda: query_throughput(db_path, args.run_id),
        "download": lambda: query_download_throughput(db_path, args.run_id),
        "ping": lambda: query_ping(db_path, args.run_id),
        "segments": lambda: query_segments(db_path, args.run_id or 1),
        "softap-vs-sta": lambda: query_softap_vs_sta(db_path),
        "raw-vs-deflate": lambda: query_raw_vs_deflate(db_path),
        "gaps": lambda: query_gap_histogram(db_path, args.upload_id or 1),
    }

    if args.query == "all":
        for name, fn in queries.items():
            fn()
    else:
        queries[args.query]()


if __name__ == "__main__":
    main()
