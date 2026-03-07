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
        return float(temp["celsius"])
    raw = temp.get("raw")
    if raw is None:
        return None
    try:
        return float(raw)
    except (TypeError, ValueError):
        return None


def choose_time_seconds(entry):
    for key in ("time_35mm", "time_120", "time_sheet"):
        value = entry.get(key)
        if value is not None:
            return int(round(float(value) * 60.0))
    return None


def push_display(push_pull_type):
    if push_pull_type == "box_speed":
        return "Box"
    if push_pull_type.startswith("push+"):
        return f"Push +{push_pull_type[5:]}"
    if push_pull_type.startswith("pull-"):
        return f"Pull -{push_pull_type[5:]}"
    return push_pull_type.replace("_", " ")


def include_entry(entry):
    film = entry.get("film")
    developer = entry.get("developer")
    category = entry.get("film_category")
    if film not in STARTER_FILMS:
        return False
    if category == "bw":
        return developer in COMMON_BW_DEVELOPERS
    return developer == "C-41"


def escape_cpp_string(value):
    return json.dumps(value, ensure_ascii=True)


def emit_cpp(entries, out_cpp):
    films = sorted({entry["film"] for entry in entries})
    developers = sorted({entry["developer"] for entry in entries})
    with out_cpp.open("w", encoding="ascii") as f:
        f.write('#include "generated_film_catalog.h"\n\n')
        f.write("namespace tutorial_0073::generated {\n\n")
        f.write("const FilmCatalogEntry kEntries[] = {\n")
        for entry in entries:
            f.write("    {\n")
            f.write(f"        {escape_cpp_string(entry['film'])},\n")
            f.write(f"        {escape_cpp_string(entry['developer'])},\n")
            f.write(f"        {escape_cpp_string(entry['dilution'])},\n")
            f.write(f"        {entry['temperature_tenths_c']},\n")
            f.write(f"        {escape_cpp_string(entry['push_pull_type'])},\n")
            f.write(f"        {escape_cpp_string(entry['push_pull_display'])},\n")
            f.write(f"        {entry['push_pull_stops_hundredths']},\n")
            f.write(f"        {entry['time_seconds']},\n")
            f.write(f"        {str(entry['has_time_35mm']).lower()},\n")
            f.write(f"        {str(entry['has_time_120']).lower()},\n")
            f.write(f"        {str(entry['has_time_sheet']).lower()},\n")
            f.write(f"        {entry['source_count']},\n")
            f.write(f"        {escape_cpp_string(entry['film_category'])},\n")
            f.write("    },\n")
        f.write("};\n\n")
        f.write(f"const size_t kEntryCount = {len(entries)};\n")
        f.write(f"const size_t kFilmCount = {len(films)};\n")
        f.write(f"const size_t kDeveloperCount = {len(developers)};\n")
        f.write("\n}  // namespace tutorial_0073::generated\n")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("json_path", type=Path)
    parser.add_argument("output_cpp", type=Path)
    args = parser.parse_args()

    with args.json_path.open() as f:
        data = json.load(f)

    grouped = defaultdict(list)
    for entry in data["entries"]:
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
            round(temp_c, 1),
            entry.get("push_pull_type") or "box_speed",
            entry.get("film_category") or "unknown",
        )
        grouped[key].append(entry)

    normalized = []
    for key, bucket in grouped.items():
        film, developer, dilution, temp_c, push_pull_type, film_category = key
        time_values = [choose_time_seconds(entry) for entry in bucket if choose_time_seconds(entry) is not None]
        if not time_values:
            continue
        normalized.append(
            {
                "film": film,
                "developer": developer,
                "dilution": dilution,
                "temperature_tenths_c": int(round(temp_c * 10.0)),
                "push_pull_type": push_pull_type,
                "push_pull_display": push_display(push_pull_type),
                "push_pull_stops_hundredths": int(round(float(bucket[0].get("push_pull_stops", 0.0)) * 100.0)),
                "time_seconds": int(round(statistics.median(time_values))),
                "has_time_35mm": any(entry.get("time_35mm") is not None for entry in bucket),
                "has_time_120": any(entry.get("time_120") is not None for entry in bucket),
                "has_time_sheet": any(entry.get("time_sheet") is not None for entry in bucket),
                "source_count": len(bucket),
                "film_category": film_category,
            }
        )

    normalized.sort(
        key=lambda entry: (
            entry["film_category"],
            entry["film"],
            entry["developer"],
            entry["dilution"],
            entry["temperature_tenths_c"],
            entry["push_pull_stops_hundredths"],
        )
    )
    emit_cpp(normalized, args.output_cpp)
    print(f"generated {len(normalized)} recipe rows -> {args.output_cpp}")


if __name__ == "__main__":
    main()
