#!/usr/bin/env python3

import argparse
import json
import statistics
from collections import defaultdict
from pathlib import Path


COMMON_BW_DEVELOPERS = {
    "D-76",
    "HC-110",
    "ID-11",
    "Ilfosol 3",
    "Ilfotec DD-X",
    "Rodinal",
    "TMax Dev",
    "Xtol",
}

STARTER_FILMS = {
    "Arista Premium 100",
    "Arista Premium 400",
    "Fomapan 100",
    "Fomapan 400",
    "Fuji Neopan 100 Acros II",
    "Ilford Delta 100 Pro",
    "Ilford Delta 400 Pro",
    "Ilford Delta 3200 Pro",
    "Ilford FP4+",
    "Ilford HP5+",
    "Ilford Pan F+",
    "Kentmere 100",
    "Kentmere 400",
    "Kodak TMax 100",
    "Kodak TMax 400",
    "Kodak TMax P3200",
    "Kodak Tri-X 320",
    "Kodak Tri-X 400",
    "Tasma Type-17",
}


def normalize_temp_c(entry):
    temp = entry.get("temperature")
    if not isinstance(temp, dict):
        return None
    if "celsius" in temp:
        return round(float(temp["celsius"]), 1)
    raw = temp.get("raw")
    if raw is None:
        return None
    try:
        return round(float(raw), 1)
    except (TypeError, ValueError):
        return None


def choose_time_seconds(entry):
    for key in ("time_35mm", "time_120", "time_sheet"):
        value = entry.get(key)
        if value is not None:
            return int(round(float(value) * 60.0))
    return None


def include_entry(entry):
    film = entry.get("film")
    developer = entry.get("developer")
    category = entry.get("film_category")
    if film not in STARTER_FILMS:
        return False
    if category == "bw":
        return developer in COMMON_BW_DEVELOPERS
    return developer == "C-41"


def build_groups(entries):
    grouped = defaultdict(list)
    for entry in entries:
        if not include_entry(entry):
            continue

        temp_c = normalize_temp_c(entry)
        time_seconds = choose_time_seconds(entry)
        if temp_c is None or time_seconds is None:
            continue

        key = (
            entry["film"],
            entry["developer"],
            entry.get("dilution") or "stock",
            temp_c,
            entry.get("push_pull_type") or "box_speed",
            entry.get("film_category") or "unknown",
        )
        grouped[key].append(entry)
    return grouped


def choose_samples(grouped, limit):
    ranked = sorted(
        grouped.items(),
        key=lambda item: (
            -len(item[1]),
            item[0][5],
            item[0][0],
            item[0][1],
            item[0][2],
            item[0][3],
            item[0][4],
        ),
    )
    return ranked[:limit]


def format_entry(key, bucket):
    film, developer, dilution, temp_c, push_pull_type, film_category = key
    times = [choose_time_seconds(entry) for entry in bucket if choose_time_seconds(entry) is not None]
    median_seconds = int(round(statistics.median(times)))
    lines = [
        f"film={film}",
        f"developer={developer}",
        f"dilution={dilution}",
        f"temp_c={temp_c:.1f}",
        f"push_pull={push_pull_type}",
        f"film_category={film_category}",
        f"source_count={len(bucket)}",
        f"times_seconds={times}",
        f"median_seconds={median_seconds}",
    ]
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("json_path", type=Path)
    parser.add_argument("--limit", type=int, default=5)
    args = parser.parse_args()

    with args.json_path.open() as f:
      data = json.load(f)

    grouped = build_groups(data["entries"])
    for index, (key, bucket) in enumerate(choose_samples(grouped, args.limit), start=1):
        print(f"=== sample {index} ===")
        print(format_entry(key, bucket))


if __name__ == "__main__":
    main()
