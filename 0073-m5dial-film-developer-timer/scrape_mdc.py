#!/usr/bin/env python3
"""
Massive Dev Chart Scraper
Scrapes film development times from digitaltruth.com/devchart.php
Produces a structured JSON database with push/pull stops computed.
"""

import requests
from bs4 import BeautifulSoup
import json
import time
import re
import math
import sys
from datetime import datetime

BASE_URL = "https://www.digitaltruth.com/devchart.php"
HEADERS = {
    "User-Agent": (
        "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) "
        "AppleWebKit/537.36 (KHTML, like Gecko) "
        "Chrome/120.0.0.0 Safari/537.36"
    ),
    "Referer": "https://www.digitaltruth.com/devchart.php",
}

# ─── Film nominal ISO lookup ───────────────────────────────────────────────────
# Key patterns matched (case-insensitive) → nominal ISO
NOMINAL_ISO_PATTERNS = [
    (r"\bP3200\b",          3200),
    (r"\b3200\b",           3200),
    (r"\b1600\b",           1600),
    (r"\b800\b",            800),
    (r"\b400\b",            400),
    (r"\bTri.?X\b",         400),   # Tri-X is 400
    (r"\bHP5",              400),
    (r"\bBW400",            400),
    (r"\bKentmere 400\b",   400),
    (r"\bStreet.?Candy",    400),
    (r"\b320\b",            320),
    (r"\bTri.?X 320\b",     320),
    (r"\b250\b",            250),
    (r"\b200\b",            200),
    (r"\bGold 200\b",       200),
    (r"\bEktar\b",          100),
    (r"\b100\b",            100),
    (r"\bFP4",              125),
    (r"\bDelta 100\b",      100),
    (r"\bDelta 400\b",      400),
    (r"\bDelta 3200\b",     3200),
    (r"\bTMax 100\b",       100),
    (r"\bTMax 400\b",       400),
    (r"\bTMax P3200\b",     3200),
    (r"\bAcros",            100),
    (r"\bNeopan 400\b",     400),
    (r"\bPan F",            50),
    (r"\b50\b",             50),
    (r"\b25\b",             25),
    (r"\b20\b",             20),
    (r"\bPortra 160\b",     160),
    (r"\bPortra 400\b",     400),
    (r"\bPortra 800\b",     800),
    (r"\bKodacolor",        200),
    (r"\bColorPlus",        200),
    (r"\bUltramax",         400),
    (r"\bGold 100\b",       100),
    (r"\bGold 400\b",       400),
    (r"\bFujicolor 100\b",  100),
    (r"\bFujicolor 200\b",  200),
    (r"\bFujicolor 400\b",  400),
    (r"\bSuperia 100\b",    100),
    (r"\bSuperia 200\b",    200),
    (r"\bSuperia 400\b",    400),
    (r"\bSuperia 800\b",    800),
    (r"\bVelvia 50\b",      50),
    (r"\bVelvia 100\b",     100),
    (r"\bProvia 100\b",     100),
    (r"\bProvia 400\b",     400),
    (r"\bEktachrome 100\b", 100),
    (r"\bEktachrome 400\b", 400),
]


def guess_nominal_iso(film_name: str) -> int | None:
    """Try to infer the box speed from the film name."""
    for pattern, iso in NOMINAL_ISO_PATTERNS:
        if re.search(pattern, film_name, re.IGNORECASE):
            return iso
    return None


def parse_time_minutes(time_str: str) -> float | None:
    """
    Convert a time string to minutes (float).
    Handles: '8.25', '13.75', '1:30', '30', '1h30', 'N/A', '-', '', '*'
    Strips trailing '*' (rotary/continuous agitation flags).
    """
    if not time_str:
        return None
    t = time_str.strip().rstrip("*").strip()
    if not t or t in ("-", "N/A", "n/a", "?", "NR"):
        return None
    # hh:mm format
    m = re.match(r"^(\d+):(\d{2})$", t)
    if m:
        return int(m.group(1)) * 60 + int(m.group(2))
    # "1h30" format
    m = re.match(r"^(\d+)h(\d+)$", t, re.IGNORECASE)
    if m:
        return int(m.group(1)) * 60 + int(m.group(2))
    # plain decimal
    try:
        return float(t)
    except ValueError:
        return None


def parse_temp(temp_str: str) -> dict | None:
    """
    Parse temperature string like '20C' or '68F' into
    {'celsius': 20, 'fahrenheit': 68}.
    """
    if not temp_str:
        return None
    t = temp_str.strip()
    m = re.match(r"^([\d.]+)C$", t, re.IGNORECASE)
    if m:
        c = float(m.group(1))
        return {"celsius": c, "fahrenheit": round(c * 9 / 5 + 32, 1)}
    m = re.match(r"^([\d.]+)F$", t, re.IGNORECASE)
    if m:
        f = float(m.group(1))
        return {"fahrenheit": f, "celsius": round((f - 32) * 5 / 9, 1)}
    return {"raw": t}


def compute_push_pull(shot_iso: int, nominal_iso: int | None) -> dict:
    """Compute push/pull in stops."""
    if nominal_iso is None or nominal_iso <= 0 or shot_iso <= 0:
        return {"stops": None, "type": "unknown"}
    stops = round(math.log2(shot_iso / nominal_iso), 2)
    if stops > 0:
        ptype = f"push_{stops:.2g}".rstrip("0").rstrip(".")
        ptype = f"push+{stops:.2g}".replace("+0.0", "+0")
    elif stops < 0:
        ptype = f"pull{stops:.2g}"
    else:
        ptype = "box_speed"
    return {"stops": stops, "type": ptype}


def fetch_all_films() -> list[str]:
    """Fetch the list of film names from the main dropdown."""
    r = requests.get(BASE_URL, headers=HEADERS, timeout=20)
    r.raise_for_status()
    soup = BeautifulSoup(r.text, "lxml")
    sel = soup.find("select", {"name": "Film"}) or soup.find("select", {"id": "Film"})
    if not sel:
        raise RuntimeError("Could not find Film select element")
    films = []
    for opt in sel.find_all("option"):
        v = opt.get("value", "").strip()
        if v and v not in ("", "searchbox", "All Films"):
            films.append(v)
    return films


def fetch_dev_rows(film: str) -> list[dict]:
    """Fetch all development rows for a given film."""
    params = {"Film": film, "Developer": "", "mdc": "Search"}
    r = requests.get(BASE_URL, params=params, headers=HEADERS, timeout=20)
    r.raise_for_status()
    soup = BeautifulSoup(r.text, "lxml")

    table = soup.find("table", {"class": "mdctable"})
    if not table:
        return []

    rows = table.find_all("tr")[1:]  # skip header
    results = []
    for row in rows:
        cells = row.find_all("td")
        if len(cells) < 8:
            continue

        # Extract notes link (devrow ID for future reference)
        notes_link = cells[8].find("a") if len(cells) > 8 else None
        notes_href = notes_link["href"] if notes_link else None
        devrow_id = None
        if notes_href:
            m = re.search(r"devrow=(\d+)", notes_href)
            if m:
                devrow_id = int(m.group(1))

        # Has asterisk = rotary/continuous agitation flag
        time_35mm_raw = cells[4].get_text(strip=True)
        time_120_raw  = cells[5].get_text(strip=True)
        time_sheet_raw = cells[6].get_text(strip=True)

        row_data = {
            "film":       cells[0].get_text(strip=True),
            "developer":  cells[1].get_text(strip=True),
            "dilution":   cells[2].get_text(strip=True) or None,
            "shot_iso":   None,
            "time_35mm":  parse_time_minutes(time_35mm_raw),
            "time_120":   parse_time_minutes(time_120_raw),
            "time_sheet": parse_time_minutes(time_sheet_raw),
            "rotary_35mm":  time_35mm_raw.endswith("*"),
            "rotary_120":   time_120_raw.endswith("*"),
            "rotary_sheet": time_sheet_raw.endswith("*"),
            "temperature":  parse_temp(cells[7].get_text(strip=True)),
            "has_notes":    bool(notes_link),
            "devrow_id":    devrow_id,
        }

        iso_str = cells[3].get_text(strip=True)
        try:
            row_data["shot_iso"] = int(iso_str)
        except (ValueError, TypeError):
            row_data["shot_iso"] = None

        results.append(row_data)
    return results


def build_database(films: list[str], delay: float = 0.5) -> list[dict]:
    """Scrape all films and return flat list of entries with push/pull info."""
    all_entries = []
    for i, film in enumerate(films):
        print(f"  [{i+1}/{len(films)}] {film}", flush=True)
        try:
            rows = fetch_dev_rows(film)
        except Exception as e:
            print(f"    ERROR: {e}", file=sys.stderr)
            time.sleep(delay * 2)
            continue

        # Determine nominal ISO for this film
        nominal_iso = guess_nominal_iso(film)
        # Also try to infer from rows: most common ISO around base speed
        if nominal_iso is None and rows:
            isos = [r["shot_iso"] for r in rows if r["shot_iso"]]
            if isos:
                # Use the most common ISO
                from collections import Counter
                nominal_iso = Counter(isos).most_common(1)[0][0]

        for row in rows:
            pp = compute_push_pull(row["shot_iso"] or 0, nominal_iso)
            entry = {
                **row,
                "nominal_iso": nominal_iso,
                "push_pull_stops": pp["stops"],
                "push_pull_type": pp["type"],
            }
            all_entries.append(entry)

        if rows:
            print(f"    → {len(rows)} entries, nominal ISO: {nominal_iso}")
        else:
            print(f"    → no data")

        time.sleep(delay)

    return all_entries


def build_json_db(entries: list[dict]) -> dict:
    """Wrap entries in a metadata envelope."""
    return {
        "meta": {
            "source": "Massive Dev Chart — digitaltruth.com",
            "scraped_at": datetime.utcnow().isoformat() + "Z",
            "total_entries": len(entries),
            "schema_version": "1.0",
            "schema": {
                "film":            "Film name as listed in Massive Dev Chart",
                "developer":       "Developer name",
                "dilution":        "Dilution ratio (e.g. '1+1', 'stock', '1+100')",
                "shot_iso":        "ISO film was exposed at",
                "nominal_iso":     "Film's box speed (inferred from name or most common ISO)",
                "push_pull_stops": "Push/pull in stops (positive = push, negative = pull)",
                "push_pull_type":  "Human-readable push/pull label",
                "time_35mm":       "Development time in minutes for 35mm format",
                "time_120":        "Development time in minutes for 120 format",
                "time_sheet":      "Development time in minutes for sheet film",
                "rotary_35mm":     "True if 35mm time is for rotary/continuous agitation",
                "rotary_120":      "True if 120 time is for rotary/continuous agitation",
                "rotary_sheet":    "True if sheet time is for rotary/continuous agitation",
                "temperature":     "Development temperature {celsius, fahrenheit}",
                "has_notes":       "True if this entry has additional notes on MDC",
                "devrow_id":       "MDC internal row ID (use with ?devrow=ID to fetch notes)",
            }
        },
        "entries": entries,
    }


if __name__ == "__main__":
    print("=== Massive Dev Chart Scraper ===")
    print("Fetching film list...")
    films = fetch_all_films()
    print(f"Found {len(films)} films in dropdown\n")

    print("Scraping development times...")
    entries = build_database(films, delay=0.4)

    print(f"\nTotal entries collected: {len(entries)}")

    db = build_json_db(entries)

    out_path = "/sessions/eloquent-intelligent-rubin/mnt/outputs/film_dev_times.json"
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(db, f, indent=2, ensure_ascii=False)

    print(f"Saved → {out_path}")

    # Also save the scraper script itself
    import shutil
    shutil.copy(__file__, "/sessions/eloquent-intelligent-rubin/mnt/outputs/scrape_mdc.py")
    print("Saved → scrape_mdc.py")
