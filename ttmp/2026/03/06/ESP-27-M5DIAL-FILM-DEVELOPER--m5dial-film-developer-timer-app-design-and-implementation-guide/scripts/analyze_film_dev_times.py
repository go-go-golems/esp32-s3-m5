#!/usr/bin/env python3

import argparse
import collections
import json
from pathlib import Path


def normalize_temp(entry):
    temp = entry.get("temperature")
    if not isinstance(temp, dict):
        return None
    if "celsius" in temp:
        return float(temp["celsius"])
    raw = temp.get("raw")
    if raw is None:
        return None
    try:
        return float(raw)
    except (TypeError, ValueError):
        return None


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("json_path", type=Path)
    args = parser.parse_args()

    with args.json_path.open() as f:
        data = json.load(f)

    entries = data["entries"]
    films = data["films"]

    category_counts = collections.Counter(e.get("film_category", "<missing>") for e in entries)
    developer_counts = collections.Counter(e.get("developer", "<missing>") for e in entries)
    push_counts = collections.Counter(e.get("push_pull_type", "<missing>") for e in entries)
    temp_counts = collections.Counter()

    for entry in entries:
        temp = normalize_temp(entry)
        if temp is not None:
            temp_counts[temp] += 1

    print("entries", len(entries))
    print("films", len(films))
    print("categories", category_counts.most_common())
    print("top developers", developer_counts.most_common(25))
    print("top push/pull", push_counts.most_common(20))
    print("top temperatures", temp_counts.most_common(20))


if __name__ == "__main__":
    main()
