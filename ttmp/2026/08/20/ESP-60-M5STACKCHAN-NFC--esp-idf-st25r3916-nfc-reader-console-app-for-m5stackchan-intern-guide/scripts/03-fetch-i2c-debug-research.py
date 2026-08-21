#!/usr/bin/env python3
"""Preserve selected ESP-IDF GitHub issues as readable Markdown.

Requires an authenticated `gh` CLI. The issue body and every public comment are
retrieved from the GitHub API so the ticket retains the evidence used by the
I2C debugging guide.
"""

from __future__ import annotations

import datetime as dt
import json
from pathlib import Path
import subprocess

REPO = "espressif/esp-idf"
ISSUES = {
    13136: "06-esp-idf-issue-13136-i2c-failure-notification.md",
    14030: "07-esp-idf-issue-14030-nack-invalid-state.md",
    17556: "08-esp-idf-issue-17556-defined-operations-invalid-state.md",
    17720: "09-esp-idf-issue-17720-nack-stop-watchdog.md",
}


def gh_json(endpoint: str):
    result = subprocess.run(
        ["gh", "api", endpoint, "--paginate"],
        check=True,
        capture_output=True,
        text=True,
    )
    return json.loads(result.stdout)


def render_issue(number: int) -> str:
    issue = gh_json(f"repos/{REPO}/issues/{number}")
    comments = gh_json(f"repos/{REPO}/issues/{number}/comments")
    labels = ", ".join(label["name"] for label in issue.get("labels", [])) or "none"
    lines = [
        f"# {issue['title']}",
        "",
        f"- **Canonical URL:** {issue['html_url']}",
        f"- **Repository:** `{REPO}`",
        f"- **Issue:** #{number}",
        f"- **State at retrieval:** {issue['state']}",
        f"- **Created:** {issue['created_at']}",
        f"- **Updated:** {issue['updated_at']}",
        f"- **Retrieved:** {dt.date.today().isoformat()}",
        f"- **Labels:** {labels}",
        "",
        "> [!note] Source snapshot",
        "> This file preserves an external issue discussion as research evidence. Claims in comments are reports from participants, not automatically verified facts. Consult the canonical issue for later updates.",
        "",
        "## Issue body",
        "",
        issue.get("body") or "(empty)",
        "",
        "## Comments",
        "",
    ]
    if not comments:
        lines.append("(No comments at retrieval time.)")
    for index, comment in enumerate(comments, start=1):
        lines.extend(
            [
                f"### Comment {index}: {comment['user']['login']}",
                "",
                f"- **Created:** {comment['created_at']}",
                f"- **Updated:** {comment['updated_at']}",
                f"- **URL:** {comment['html_url']}",
                "",
                comment.get("body") or "(empty)",
                "",
            ]
        )
    return "\n".join(lines).rstrip() + "\n"


def main() -> None:
    script_dir = Path(__file__).resolve().parent
    web_dir = script_dir.parent / "sources" / "web"
    web_dir.mkdir(parents=True, exist_ok=True)
    for number, filename in ISSUES.items():
        destination = web_dir / filename
        destination.write_text(render_issue(number), encoding="utf-8")
        print(destination)


if __name__ == "__main__":
    main()
