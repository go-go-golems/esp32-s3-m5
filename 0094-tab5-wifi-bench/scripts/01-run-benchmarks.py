#!/usr/bin/env python3
"""
Tab5 WiFi/HTTP Benchmark Runner

Runs the full benchmark matrix against the Tab5 benchmark firmware,
stores results in SQLite, and provides analysis queries.

Usage:
    python3 01-run-benchmarks.py --base-url http://192.168.4.1
    python3 01-run-benchmarks.py --base-url http://192.168.0.26
    python3 01-run-benchmarks.py --base-url http://192.168.4.1 --quick
"""

import argparse
import json
import sqlite3
import sys
import time
import zlib
from pathlib import Path

try:
    import requests
except ImportError:
    print("pip install requests")
    sys.exit(1)

SCRIPT_DIR = Path(__file__).parent
DB_PATH = SCRIPT_DIR / "bench_results.db"

SIZES = [1024, 10 * 1024, 100 * 1024, 500 * 1024, 1024 * 1024, 1843200]
SIZE_LABELS = {1024: "1KB", 10 * 1024: "10KB", 100 * 1024: "100KB",
               500 * 1024: "500KB", 1024 * 1024: "1MB", 1843200: "1.8MB"}
COMPRESSIONS = ["raw", "deflate"]
PING_SIZES = [64, 256, 1024, 4096, 16384]
REPEATS = 3


def create_db(db_path: Path):
    """Create the SQLite database with benchmark tables."""
    conn = sqlite3.connect(db_path)
    c = conn.cursor()
    c.execute("""
        CREATE TABLE IF NOT EXISTS runs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp TEXT DEFAULT (datetime('now')),
            base_url TEXT NOT NULL,
            mode TEXT NOT NULL,
            firmware_version TEXT,
            notes TEXT
        )
    """)
    c.execute("""
        CREATE TABLE IF NOT EXISTS upload_results (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            run_id INTEGER NOT NULL,
            payload_size INTEGER NOT NULL,
            compression TEXT NOT NULL,
            repeat_num INTEGER NOT NULL,
            browser_ms REAL,
            server_recv_us INTEGER,
            server_decompress_us INTEGER,
            server_total_us INTEGER,
            recv_kbps REAL,
            segment_count INTEGER,
            payload_bytes INTEGER,
            decompressed_bytes INTEGER,
            rssi INTEGER,
            free_heap INTEGER,
            free_spiram INTEGER,
            FOREIGN KEY (run_id) REFERENCES runs(id)
        )
    """)
    c.execute("""
        CREATE TABLE IF NOT EXISTS download_results (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            run_id INTEGER NOT NULL,
            payload_size INTEGER NOT NULL,
            repeat_num INTEGER NOT NULL,
            browser_ms REAL,
            recv_kbps REAL,
            payload_bytes INTEGER,
            FOREIGN KEY (run_id) REFERENCES runs(id)
        )
    """)
    c.execute("""
        CREATE TABLE IF NOT EXISTS ping_results (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            run_id INTEGER NOT NULL,
            payload_size INTEGER NOT NULL,
            repeat_num INTEGER NOT NULL,
            rtt_ms REAL,
            FOREIGN KEY (run_id) REFERENCES runs(id)
        )
    """)
    c.execute("""
        CREATE TABLE IF NOT EXISTS segments (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            upload_result_id INTEGER NOT NULL,
            seg_num INTEGER NOT NULL,
            bytes INTEGER NOT NULL,
            time_us INTEGER NOT NULL,
            delta_us INTEGER,
            FOREIGN KEY (upload_result_id) REFERENCES upload_results(id)
        )
    """)
    # Indexes for common queries
    c.execute("CREATE INDEX IF NOT EXISTS idx_upload_run ON upload_results(run_id)")
    c.execute("CREATE INDEX IF NOT EXISTS idx_upload_size ON upload_results(payload_size)")
    c.execute("CREATE INDEX IF NOT EXISTS idx_segments_upload ON segments(upload_result_id)")
    conn.commit()
    conn.close()


def start_run(base_url: str, mode: str, notes: str = "") -> int:
    conn = sqlite3.connect(DB_PATH)
    c = conn.cursor()
    c.execute("INSERT INTO runs (base_url, mode, notes) VALUES (?, ?, ?)",
              (base_url, mode, notes))
    run_id = c.lastrowid
    conn.commit()
    conn.close()
    return run_id


def generate_payload(size: int) -> bytes:
    """Generate an incrementing byte pattern — compressible but not trivial."""
    return bytes(i & 0xFF for i in range(size))


def run_upload_bench(base_url: str, payload_size: int, compression: str,
                     repeat_num: int, run_id: int):
    """Run a single upload benchmark and store results in SQLite."""
    payload = generate_payload(payload_size)
    url = f"{base_url}/api/bench/upload?size={payload_size}"
    headers = {}

    if compression == "deflate":
        compressed = zlib.compress(payload, 6)  # zlib format (RFC 1950)
        body = compressed
        headers["Content-Encoding"] = "deflate"
        comp_label = f"{len(compressed)} bytes compressed"
    else:
        body = payload
        comp_label = f"{payload_size} bytes raw"

    label = SIZE_LABELS.get(payload_size, f"{payload_size}")
    print(f"  upload: {label} {compression} (repeat {repeat_num}) [{comp_label}]...", end=" ", flush=True)

    t0 = time.perf_counter()
    resp = requests.post(url, data=body, headers=headers, timeout=120)
    t1 = time.perf_counter()
    browser_ms = (t1 - t0) * 1000

    if resp.status_code != 200:
        print(f"HTTP {resp.status_code}")
        return

    data = resp.json()
    if not data.get("ok"):
        print(f"error: {data.get('error', 'unknown')}")
        return

    server_recv_us = data.get("timing_us", {}).get("recv")
    server_decompress_us = data.get("timing_us", {}).get("decompress")
    server_total_us = data.get("timing_us", {}).get("total")
    recv_kbps = data.get("recv_throughput_kbps", 0)
    segments = data.get("segments", [])
    system = data.get("system", {})

    conn = sqlite3.connect(DB_PATH)
    c = conn.cursor()
    c.execute("""
        INSERT INTO upload_results
        (run_id, payload_size, compression, repeat_num,
         browser_ms, server_recv_us, server_decompress_us, server_total_us,
         recv_kbps, segment_count, payload_bytes, decompressed_bytes,
         rssi, free_heap, free_spiram)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    """, (run_id, payload_size, compression, repeat_num,
          browser_ms, server_recv_us, server_decompress_us, server_total_us,
          recv_kbps, len(segments), data.get("payload_bytes"),
          data.get("decompressed_bytes"),
          system.get("rssi"), system.get("free_heap"), system.get("free_spiram")))
    upload_id = c.lastrowid

    # Insert segments with delta timing
    prev_time = None
    for i, seg in enumerate(segments):
        delta = seg["t"] - prev_time if prev_time is not None else None
        c.execute("""
            INSERT INTO segments (upload_result_id, seg_num, bytes, time_us, delta_us)
            VALUES (?, ?, ?, ?, ?)
        """, (upload_id, i, seg["b"], seg["t"], delta))
        prev_time = seg["t"]

    conn.commit()
    conn.close()

    print(f"browser {browser_ms:.0f}ms, server_recv {server_recv_us/1000:.0f}ms, "
          f"{recv_kbps:.0f} kbps, {len(segments)} segs")


def run_download_bench(base_url: str, payload_size: int, repeat_num: int, run_id: int):
    """Run a single download benchmark and store results in SQLite."""
    url = f"{base_url}/api/bench/download?size={payload_size}"
    label = SIZE_LABELS.get(payload_size, f"{payload_size}")

    print(f"  download: {label} (repeat {repeat_num})...", end=" ", flush=True)

    t0 = time.perf_counter()
    resp = requests.get(url, timeout=120)
    content = resp.content
    t1 = time.perf_counter()
    browser_ms = (t1 - t0) * 1000

    recv_kbps = (len(content) * 8) / (browser_ms / 1000) / 1000 if browser_ms > 0 else 0

    conn = sqlite3.connect(DB_PATH)
    c = conn.cursor()
    c.execute("""
        INSERT INTO download_results
        (run_id, payload_size, repeat_num, browser_ms, recv_kbps, payload_bytes)
        VALUES (?, ?, ?, ?, ?, ?)
    """, (run_id, payload_size, repeat_num, browser_ms, recv_kbps, len(content)))
    conn.commit()
    conn.close()

    print(f"browser {browser_ms:.0f}ms, {recv_kbps:.0f} kbps, {len(content)} bytes")


def run_ping_bench(base_url: str, payload_size: int, repeat_num: int, run_id: int):
    """Run a single ping benchmark and store results in SQLite."""
    url = f"{base_url}/api/bench/ping"
    payload = generate_payload(payload_size)

    print(f"  ping: {payload_size}B (repeat {repeat_num})...", end=" ", flush=True)

    t0 = time.perf_counter()
    resp = requests.post(url, data=payload, timeout=30)
    t1 = time.perf_counter()
    rtt_ms = (t1 - t0) * 1000

    conn = sqlite3.connect(DB_PATH)
    c = conn.cursor()
    c.execute("""
        INSERT INTO ping_results (run_id, payload_size, repeat_num, rtt_ms)
        VALUES (?, ?, ?, ?)
    """, (run_id, payload_size, repeat_num, rtt_ms))
    conn.commit()
    conn.close()

    print(f"RTT {rtt_ms:.1f}ms")


def main():
    parser = argparse.ArgumentParser(description="Tab5 WiFi/HTTP Benchmark Runner")
    parser.add_argument("--base-url", required=True, help="Base URL of the Tab5 benchmark server")
    parser.add_argument("--quick", action="store_true", help="Run fewer sizes and repeats")
    parser.add_argument("--upload-only", action="store_true", help="Only run upload benchmarks")
    parser.add_argument("--download-only", action="store_true", help="Only run download benchmarks")
    parser.add_argument("--ping-only", action="store_true", help="Only run ping benchmarks")
    parser.add_argument("--repeats", type=int, default=REPEATS, help="Number of repeats per test")
    args = parser.parse_args()

    base_url = args.base_url.rstrip("/")
    mode = "SoftAP" if "4.1" in base_url else "STA"
    sizes = [1024, 100 * 1024, 1843200] if args.quick else SIZES
    repeats = 1 if args.quick else args.repeats

    # Create DB
    create_db(DB_PATH)

    # Health check
    print(f"Connecting to {base_url} ({mode})...")
    try:
        resp = requests.get(f"{base_url}/api/health", timeout=5)
        if resp.status_code != 200 or not resp.json().get("ok"):
            print(f"Health check failed: HTTP {resp.status_code}")
            sys.exit(1)
    except requests.ConnectionError as e:
        print(f"Cannot connect: {e}")
        sys.exit(1)
    print("Health check OK")

    # Start a run
    run_id = start_run(base_url, mode, notes=f"repeats={repeats}")
    print(f"Run ID: {run_id}\n")

    # Upload benchmarks
    if not args.download_only and not args.ping_only:
        print("=== Upload Benchmarks ===")
        for compression in COMPRESSIONS:
            for size in sizes:
                for r in range(1, repeats + 1):
                    run_upload_bench(base_url, size, compression, r, run_id)
                    time.sleep(0.5)
        print()

    # Download benchmarks
    if not args.upload_only and not args.ping_only:
        print("=== Download Benchmarks ===")
        for size in sizes:
            for r in range(1, repeats + 1):
                run_download_bench(base_url, size, r, run_id)
                time.sleep(0.5)
        print()

    # Ping benchmarks
    if not args.upload_only and not args.download_only:
        print("=== Ping Benchmarks ===")
        ping_sizes = [64, 1024] if args.quick else PING_SIZES
        for size in ping_sizes:
            for r in range(1, repeats + 1):
                run_ping_bench(base_url, size, r, run_id)
                time.sleep(0.3)
        print()

    print(f"Done. Results in {DB_PATH} (run_id={run_id})")


if __name__ == "__main__":
    main()
