#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""ESP-60 Phase 5: normalize Arduino (M5) and ESP-IDF I2C traces to one schema
and emit the comparison report from design doc 04 Section 12.

Both backends are normalized to: {seq, t_us, elapsed_us, gap_us, backend, phase,
kind, wire_key, logical_key, wlen, rlen, ok, result}.

  Arduino  M5_I2C  txn=.. t_us=.. elapsed_us=.. kind=W key=0x.. wlen=.. rlen=.. ok=.. failure_stage=0x..
  ESP-IDF  I2C_TRACE seq=.. t_us=.. gap_us=.. elapsed_us=.. backend=.. phase=.. attempt=.. kind=.. op=.. logical=.. wire=.. wlen=.. rlen=.. api=.. hint=.. class=.. flags=..

The two captures are from different runs/conditions (Arduino: 4 tags; ESP-IDF:
no tag), so this script does NOT force an event-by-event divergence. Instead it
emits the design S12 outputs in a run-aware form:
  1. Summary    (per backend x phase: events, failures, median/p95/max elapsed)
  2. KeyCoverage (per wire key: counts + failure rate per backend)
  3. FailureTiming (ESP-IDF failures: elapsed/gap distribution + first error)
  4. Divergence (first ESP-IDF failure + neighbourhood vs Arduino same-key success)
"""
from __future__ import annotations
import argparse, gzip, re, statistics, json
from collections import defaultdict
from pathlib import Path

ANSI = re.compile(r"\x1b\[[0-?]*[ -/]*[@-~]")
M5_EVT = re.compile(
    r"M5_I2C txn=(\d+) t_us=(\d+) elapsed_us=(\d+) kind=(\w+) key=0x([0-9A-F]+) "
    r"wlen=(\d+) rlen=(\d+) ok=(\d+) failure_stage=0x([0-9A-F]+)")
M5_PHASE = re.compile(r"M5_PHASE phase=(\w+) ok=(\d+)")
IDF_EVT = re.compile(
    r"I2C_TRACE seq=(\d+) t_us=(\d+) gap_us=(\d+) elapsed_us=(\d+) backend=(\S+) "
    r"phase=(\S+) attempt=(\d+) kind=(\S+) op=(\S+) logical=([0-9A-F]+) wire=([0-9A-F]+) "
    r"wlen=(\d+) rlen=(\d+) api=(\S+) hint=(\S+) class=(\S+) flags=(\S+)")
IDF_STATUS = re.compile(
    r"TRACE_STATUS mode=(\S+) backend=(\S+) recorded=(\d+) failed=(\d+) ring=(\d+)/(\d+) "
    r"overwritten=(\d+)")

def pct(vals, f):
    if not vals: return None
    return sorted(vals)[int(f * (len(vals) - 1))]

def norm_arduino(text):
    out = []
    phase = "unknown"
    for line in text.splitlines():
        m = M5_PHASE.search(line)
        if m: phase = m.group(1); continue
        m = M5_EVT.search(line)
        if m:
            wire = int(m.group(5), 16)
            out.append({
                "backend": "m5-direct", "phase": phase, "seq": int(m.group(1)),
                "t_us": int(m.group(2)), "elapsed_us": int(m.group(3)),
                "gap_us": None, "kind": m.group(4), "wire_key": wire,
                "logical_key": wire & 0x3F, "wlen": int(m.group(6)),
                "rlen": int(m.group(7)), "ok": int(m.group(8)) == 1,
                "result": "ESP_OK" if int(m.group(8)) == 1 else "M5_FAIL",
            })
    return out

def norm_idf(text):
    out = []
    for line in text.splitlines():
        m = IDF_EVT.search(line)
        if m:
            out.append({
                "backend": m.group(5), "phase": m.group(6), "seq": int(m.group(1)),
                "t_us": int(m.group(2)), "gap_us": int(m.group(3)),
                "elapsed_us": int(m.group(4)), "kind": m.group(8),
                "op": m.group(9), "logical_key": int(m.group(10), 16),
                "wire_key": int(m.group(11), 16), "wlen": int(m.group(12)),
                "rlen": int(m.group(13)), "ok": m.group(14) == "ESP_OK",
                "result": m.group(14), "hint": m.group(15), "class": m.group(16),
                "flags": m.group(17),
            })
    return out

def summary(events):
    rows = {}
    grp = defaultdict(list)
    for e in events: grp[(e["backend"], e["phase"])].append(e)
    for (be, ph), evs in sorted(grp.items()):
        el = [e["elapsed_us"] for e in evs if e["elapsed_us"] is not None]
        fails = [e for e in evs if not e["ok"]]
        rows[f"{be}/{ph}"] = {
            "events": len(evs), "failures": len(fails),
            "fail_rate": round(len(fails)/len(evs), 4) if evs else 0,
            "median_us": statistics.median(el) if el else None,
            "p95_us": pct(el, 0.95), "max_us": max(el) if el else None,
        }
    return rows

def key_coverage(events):
    cov = defaultdict(lambda: {"m5-direct": [0, 0], "idf-high": [0, 0]})
    for e in events:
        be = e["backend"]
        if be not in cov[e["wire_key"]]: cov[e["wire_key"]][be] = [0, 0]
        cov[e["wire_key"]][be][0] += 1
        if not e["ok"]: cov[e["wire_key"]][be][1] += 1
    return {f"0x{k:02X}": v for k, v in sorted(cov.items())}

def failure_timing(events):
    fails = [e for e in events if not e["ok"]]
    if not fails: return None
    el = [e["elapsed_us"] for e in fails]
    gaps = [e["gap_us"] for e in fails if e["gap_us"] is not None]
    first = fails[0]
    return {
        "count": len(fails),
        "elapsed_median_us": statistics.median(el), "elapsed_max_us": max(el),
        "gap_median_us": statistics.median(gaps) if gaps else None,
        "first": {"seq": first["seq"], "phase": first["phase"], "op": first.get("op","?"),
                  "wire": f"0x{first['wire_key']:02X}", "logical": f"0x{first['logical_key']:02X}",
                  "elapsed_us": first["elapsed_us"], "result": first["result"],
                  "hint": first.get("hint","?"), "class": first.get("class","?")},
    }

def divergence(idf, arduino):
    fails = [e for e in idf if not e["ok"]]
    if not fails: return {"note": "no ESP-IDF failures to diverge on"}
    f = fails[0]
    # Arduino success rate for the same wire key
    same = [a for a in arduino if a["wire_key"] == f["wire_key"]]
    a_ok = sum(1 for a in same if a["ok"])
    return {
        "first_idf_failure": {"seq": f["seq"], "phase": f["phase"], "op": f.get("op","?"),
                              "wire": f"0x{f['wire_key']:02X}", "elapsed_us": f["elapsed_us"],
                              "result": f["result"]},
        "arduino_same_wire_key": {"events": len(same), "ok": a_ok,
                                  "fail": len(same) - a_ok,
                                  "fail_rate": round((len(same)-a_ok)/len(same), 4) if same else None},
    }

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("arduino", type=Path, help="Arduino capture (.txt or .log.gz)")
    ap.add_argument("espidf", type=Path, help="ESP-IDF capture (.txt)")
    ap.add_argument("--json", type=Path, help="write JSON report here")
    args = ap.parse_args()

    def read_text(p: Path) -> str:
        b = p.read_bytes()
        if p.suffix == ".gz": b = gzip.decompress(b)
        return ANSI.sub("", b.decode("utf-8", "replace"))

    arduino = norm_arduino(read_text(args.arduino))
    idf = norm_idf(read_text(args.espidf))

    report = {
        "arduino_events": len(arduino), "espidf_events": len(idf),
        "summary": summary(arduino + idf),
        "key_coverage": key_coverage(arduino + idf),
        "espidf_failure_timing": failure_timing(idf),
        "divergence": divergence(idf, arduino),
    }

    if args.json:
        args.json.write_text(json.dumps(report, indent=2, default=str) + "\n")

    print(f"# ESP-60 Arduino vs ESP-IDF I2C trace comparison\n")
    print(f"Arduino events: {len(arduino)}  | ESP-IDF events: {len(idf)}\n")
    print("## Summary (backend/phase)\n")
    print(f"{'backend/phase':<28}{'events':>8}{'fails':>7}{'rate':>8}{'med':>8}{'p95':>8}{'max':>8}")
    for k, v in report["summary"].items():
        med = v['median_us']; p95 = v['p95_us']; mx = v['max_us']
        print(f"{k:<28}{v['events']:>8}{v['failures']:>7}{v['fail_rate']:>8}"
              f"{(str(int(med)) if med else '-'):>8}{(str(int(p95)) if p95 else '-'):>8}"
              f"{(str(int(mx)) if mx else '-'):>8}")
    print("\n## Key coverage (wire key -> [total, fail] per backend)\n")
    print(f"{'wire':>6}{'m5 tot':>8}{'m5 fail':>9}{'idf tot':>9}{'idf fail':>10}")
    for k, v in report["key_coverage"].items():
        m = v["m5-direct"]; i = v["idf-high"]
        print(f"{k:>6}{m[0]:>8}{m[1]:>9}{i[0]:>9}{i[1]:>10}")
    print("\n## ESP-IDF failure timing\n")
    ft = report["espidf_failure_timing"]
    if ft:
        print(f"count={ft['count']} elapsed_median_us={ft['elapsed_median_us']} "
              f"elapsed_max_us={ft['elapsed_max_us']} gap_median_us={ft['gap_median_us']}")
        f = ft["first"]
        print(f"first: seq={f['seq']} phase={f['phase']} op={f['op']} wire={f['wire']} "
              f"logical={f['logical']} elapsed_us={f['elapsed_us']} result={f['result']} "
              f"hint={f['hint']} class={f['class']}")
    else:
        print("no ESP-IDF failures")
    print("\n## Divergence (first ESP-IDF failure vs Arduino same-key)\n")
    d = report["divergence"]
    if "first_idf_failure" in d:
        f = d["first_idf_failure"]; a = d["arduino_same_wire_key"]
        print(f"ESP-IDF first failure: seq={f['seq']} phase={f['phase']} wire={f['wire']} "
              f"elapsed_us={f['elapsed_us']} result={f['result']}")
        print(f"Arduino same wire key {f['wire']}: events={a['events']} ok={a['ok']} "
              f"fail={a['fail']} fail_rate={a['fail_rate']}")
    else:
        print(d.get("note", "n/a"))

if __name__ == "__main__":
    main()
