#!/usr/bin/env python3
"""Download complete GitHub issue threads as reproducible Markdown evidence."""

from __future__ import annotations

import json
import pathlib
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[1]
WEB = ROOT / "sources" / "web"

ISSUES = [
    ("m5stack", "M5GFX", 119, "17-m5gfx-issue-119-full-thread.md"),
    ("m5stack", "M5GFX", 152, "18-m5gfx-issue-152-full-thread.md"),
    ("m5stack", "M5GFX", 157, "19-m5gfx-issue-157-pushsprite-regression.md"),
    ("m5stack", "M5GFX", 160, "20-m5gfx-issue-160-idf54-stripes.md"),
    ("m5stack", "M5GFX", 166, "21-m5gfx-issue-166-panel-instability.md"),
    ("Xinyuan-LilyGO", "LilyGo-EPD47", 93, "24-lilygo-issue-93-rails-vcom-corruption.md"),
    ("bitbank2", "FastEPD", 29, "25-fastepd-issue-29-4bpp-corruption.md"),
]


def gh_json(path: str, *, paginate: bool = False):
    command = ["gh", "api", path]
    if paginate:
        command.append("--paginate")
    result = subprocess.run(command, check=True, text=True, capture_output=True)
    if not paginate:
        return json.loads(result.stdout)
    documents = []
    decoder = json.JSONDecoder()
    text = result.stdout.lstrip()
    while text:
        value, offset = decoder.raw_decode(text)
        documents.append(value)
        text = text[offset:].lstrip()
    merged = []
    for document in documents:
        merged.extend(document)
    return merged


def clean_markdown(value: str | None, empty: str) -> str:
    if not value:
        return empty
    value = value.replace("\r\n", "\n").replace("\r", "\n")
    return "\n".join(line.rstrip() for line in value.split("\n"))


def render(owner: str, repo: str, number: int) -> str:
    issue = gh_json(f"repos/{owner}/{repo}/issues/{number}")
    comments = gh_json(f"repos/{owner}/{repo}/issues/{number}/comments", paginate=True)
    lines = [
        f"# {owner}/{repo} issue #{number}: {issue['title']}",
        "",
        f"- URL: {issue['html_url']}",
        f"- State: `{issue['state']}`",
        f"- Created: {issue['created_at']}",
        f"- Updated: {issue['updated_at']}",
        f"- Author: `{issue['user']['login']}`",
        "",
        "## Issue body",
        "",
        clean_markdown(issue.get("body"), "_(No body.)_"),
        "",
        "## Comments",
        "",
    ]
    if not comments:
        lines.append("_(No comments.)_")
    for index, comment in enumerate(comments, 1):
        lines.extend(
            [
                f"### Comment {index}: {comment['user']['login']} at {comment['created_at']}",
                "",
                f"Permalink: {comment['html_url']}",
                "",
                clean_markdown(comment.get("body"), "_(No body.)_"),
                "",
            ]
        )
    return "\n".join(lines).rstrip() + "\n"


def main() -> None:
    WEB.mkdir(parents=True, exist_ok=True)
    for owner, repo, number, filename in ISSUES:
        destination = WEB / filename
        destination.write_text(render(owner, repo, number), encoding="utf-8")
        print(destination.relative_to(ROOT))


if __name__ == "__main__":
    main()
