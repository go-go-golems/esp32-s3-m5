#!/usr/bin/env python3

import argparse
import collections
import json
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

COMMON_C41_DEVELOPERS = {
    "C-41",
    "Cinestill Cs41",
    "Fuji Hunt X-Press",
    "Kodak Flexicolor",
    "Rollei Digibase C-41",
    "Tetenal C-41",
}


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


def include_entry(entry):
    category = entry.get("film_category")
    developer = entry.get("developer", "")
    if category == "bw":
        return developer in COMMON_BW_DEVELOPERS
    if category == "color_negative":
        return developer in COMMON_C41_DEVELOPERS or "c-41" in developer.lower() or "cs41" in developer.lower()
    return False


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("json_path", type=Path)
    args = parser.parse_args()

    with args.json_path.open() as f:
        data = json.load(f)

    entries = data["entries"]
    subset = [e for e in entries if include_entry(e)]

    by_category = collections.Counter(e.get("film_category", "<missing>") for e in subset)
    by_developer = collections.Counter(e.get("developer", "<missing>") for e in subset)
    by_push = collections.Counter(e.get("push_pull_type", "<missing>") for e in subset)
    by_temp = collections.Counter()
    films = collections.Counter(e.get("film", "<missing>") for e in subset)

    for entry in subset:
        temp = normalize_temp(entry)
        if temp is not None:
            by_temp[temp] += 1

    print("subset entries", len(subset))
    print("subset films", len(films))
    print("subset categories", by_category.most_common())
    print("subset developers", by_developer.most_common(20))
    print("subset push/pull", by_push.most_common(20))
    print("subset temperatures", by_temp.most_common(20))
    print("sample films", films.most_common(25))


if __name__ == "__main__":
    main()
