#!/usr/bin/env python3
"""
Lightweight "research backfill" script for ticket 065a-ADD-OLED.

It reproduces the two ad-hoc steps that happened during exploration:
1) Find a likely vendor/product page for "GME12864-11" via DuckDuckGo HTML results
2) Fetch a chosen page and summarize key strings (controller, resolution, interface hints)

Notes:
- DuckDuckGo HTML results are not a stable API; treat `search` output as a hint list.
- The goal is a repeatable breadcrumb trail, not perfect scraping.
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from typing import Iterable

try:
    import requests  # type: ignore
except Exception:  # pragma: no cover
    requests = None  # type: ignore


DEFAULT_QUERY = "GME12864-11 OLED"
DEFAULT_VENDOR_URL = (
    "https://goldenmorninglcd.com/oled-display-module/0.96-inch-128x64-ssd1306-gme12864-11/"
)


def _http_get(url: str, timeout_s: float = 30.0) -> str:
    if requests is None:
        raise SystemExit("missing dependency: requests (install or use python with requests available)")
    r = requests.get(url, timeout=timeout_s, headers={"User-Agent": "Mozilla/5.0"})
    r.raise_for_status()
    return r.text


def ddg_search_links(query: str, limit: int) -> list[str]:
    url = "https://duckduckgo.com/html/?q=" + requests.utils.quote(query)  # type: ignore[attr-defined]
    html = _http_get(url)
    out: list[str] = []
    for m in re.finditer(r'<a rel="nofollow" class="result__a" href="(.*?)"', html):
        href = m.group(1)
        if href.startswith("//duckduckgo.com/l/?"):
            href = "https:" + href
        out.append(href)
        if len(out) >= limit:
            break
    return out


@dataclass(frozen=True)
class PageSignal:
    label: str
    pattern: str


SIGNALS: list[PageSignal] = [
    PageSignal("mentions SSD1306", r"ssd1306"),
    PageSignal("mentions 128x64", r"128\s*[x×]\s*64"),
    PageSignal("mentions I2C", r"\bi2c\b"),
    PageSignal("mentions SPI", r"\bspi\b"),
    PageSignal("mentions 0.96", r"\b0\.96\b"),
    PageSignal("mentions OLED", r"\boled\b"),
    PageSignal("mentions SCL", r"\bscl\b"),
    PageSignal("mentions SDA", r"\bsda\b"),
    PageSignal("mentions 0x3C", r"0x3c"),
    PageSignal("mentions 0x3D", r"0x3d"),
]


def summarize_page(html: str) -> dict[str, bool]:
    low = html.lower()
    return {s.label: (re.search(s.pattern, low, flags=re.IGNORECASE) is not None) for s in SIGNALS}


def extract_title(html: str) -> str | None:
    m = re.search(r"<title>(.*?)</title>", html, flags=re.IGNORECASE | re.DOTALL)
    if not m:
        return None
    title = re.sub(r"\s+", " ", m.group(1)).strip()
    return title or None


def cmd_search(args: argparse.Namespace) -> int:
    links = ddg_search_links(args.query, args.limit)
    for i, link in enumerate(links):
        print(f"{i+1:2d}. {link}")
    return 0


def cmd_fetch(args: argparse.Namespace) -> int:
    html = _http_get(args.url)
    title = extract_title(html)
    print(f"url:   {args.url}")
    print(f"title: {title or '(none)'}")
    print("")
    signals = summarize_page(html)
    for k in sorted(signals.keys()):
        print(f"{k:>16}: {'yes' if signals[k] else 'no'}")
    if args.save_html:
        with open(args.save_html, "w", encoding="utf-8") as f:
            f.write(html)
        print("")
        print(f"saved: {args.save_html}")
    return 0


def _build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser()
    sub = p.add_subparsers(dest="cmd", required=True)

    ps = sub.add_parser("search", help="List likely sources via DuckDuckGo HTML results")
    ps.add_argument("--query", default=DEFAULT_QUERY)
    ps.add_argument("--limit", type=int, default=10)
    ps.set_defaults(fn=cmd_search)

    pf = sub.add_parser("fetch", help="Fetch a page and print a small signal summary")
    pf.add_argument("--url", default=DEFAULT_VENDOR_URL)
    pf.add_argument("--save-html", default="")
    pf.set_defaults(fn=cmd_fetch)

    return p


def main(argv: Iterable[str]) -> int:
    p = _build_parser()
    args = p.parse_args(list(argv))
    if args.cmd == "fetch" and args.save_html == "":
        args.save_html = None
    return int(args.fn(args))


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

