#!/usr/bin/env python3
"""Changelog generator from Conventional Commits git history."""
from __future__ import annotations

import re
import subprocess
from dataclasses import dataclass, field
from pathlib import Path

_CONV_RE = re.compile(
    r"^(?P<type>feat|fix|docs|refactor|test|perf|chore|ci|style|build)"
    r"(?:\((?P<scope>[^)]+)\))?(?P<breaking>!)?:\s*(?P<desc>.+)$",
    re.IGNORECASE,
)

_TYPE_HEADERS: dict[str, str] = {
    "feat": "Features",
    "fix": "Bug Fixes",
    "perf": "Performance",
    "refactor": "Refactoring",
    "docs": "Documentation",
    "test": "Tests",
    "ci": "CI",
    "build": "Build",
    "chore": "Chores",
    "style": "Style",
}


@dataclass
class CommitEntry:
    sha: str
    commit_type: str
    scope: str
    breaking: bool
    description: str


@dataclass
class ChangelogData:
    tag: str
    previous_tag: str
    breaking: list[CommitEntry] = field(default_factory=list)
    by_type: dict[str, list[CommitEntry]] = field(default_factory=dict)


def _git_log_range(repo_root: Path, from_ref: str, to_ref: str) -> list[str]:
    """Return commit lines between two refs (exclusive from, inclusive to)."""
    sep = "|||"
    revision_range = f"{from_ref}..{to_ref}" if from_ref else to_ref
    result = subprocess.run(
        ["git", "log", "--no-merges", f"--format=%H{sep}%s", revision_range],
        cwd=str(repo_root),
        capture_output=True,
        text=True,
        check=True,
    )
    return [line for line in result.stdout.splitlines() if line.strip()]


def _latest_previous_tag(repo_root: Path, current_tag: str) -> str:
    """Return the tag just before current_tag, or empty string if none."""
    result = subprocess.run(
        ["git", "tag", "--sort=-version:refname"],
        cwd=str(repo_root),
        capture_output=True,
        text=True,
        check=True,
    )
    tags = [t.strip() for t in result.stdout.splitlines() if t.strip()]
    try:
        idx = tags.index(current_tag)
        return tags[idx + 1] if idx + 1 < len(tags) else ""
    except ValueError:
        return tags[0] if tags else ""


def parse_commits(raw_lines: list[str]) -> list[CommitEntry]:
    """Parse raw git log lines into CommitEntry objects."""
    entries: list[CommitEntry] = []
    for line in raw_lines:
        parts = line.split("|||", 1)
        if len(parts) != 2:
            continue
        sha, subject = parts
        m = _CONV_RE.match(subject.strip())
        if not m:
            continue
        entries.append(
            CommitEntry(
                sha=sha.strip()[:12],
                commit_type=m.group("type").lower(),
                scope=m.group("scope") or "",
                breaking=bool(m.group("breaking")),
                description=m.group("desc").strip(),
            )
        )
    return entries


def generate_changelog(
    repo_root: Path,
    tag: str,
    previous_tag: str = "",
) -> ChangelogData:
    """Generate ChangelogData for the given tag range."""
    prev = previous_tag if previous_tag else _latest_previous_tag(repo_root, tag)
    raw = _git_log_range(repo_root, prev, tag)
    commits = parse_commits(raw)
    data = ChangelogData(tag=tag, previous_tag=prev)
    for commit in commits:
        if commit.breaking:
            data.breaking.append(commit)
        data.by_type.setdefault(commit.commit_type, []).append(commit)
    return data


def render_markdown(data: ChangelogData) -> str:
    """Render a ChangelogData into GitHub-flavoured Markdown."""
    lines: list[str] = [f"# Release {data.tag}\n"]
    if data.previous_tag:
        lines.append(f"Changes since `{data.previous_tag}`.\n")
    if data.breaking:
        lines.append("\n## ⚠ Breaking Changes\n")
        for c in data.breaking:
            scope = f"**{c.scope}**: " if c.scope else ""
            lines.append(f"- {scope}{c.description} (`{c.sha}`)")
    type_order = list(_TYPE_HEADERS.keys())
    for ctype in type_order:
        bucket = data.by_type.get(ctype)
        if not bucket:
            continue
        header = _TYPE_HEADERS[ctype]
        lines.append(f"\n## {header}\n")
        for c in bucket:
            scope = f"**{c.scope}**: " if c.scope else ""
            lines.append(f"- {scope}{c.description} (`{c.sha}`)")
    return "\n".join(lines) + "\n"
