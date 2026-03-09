#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

from python_tools.core.models import CheckContext
from python_tools.policies.repo_policy import RepoPolicyCheck


def write_file(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def run_repo_policy(root: Path, allowlist: Path | None = None) -> tuple[bool, list[str], list[str]]:
    context = CheckContext(root=root, allowlist=allowlist or (root / "allowlist.txt"), target_lines=200, hard_lines=300)
    result = RepoPolicyCheck().run(context)
    return result.ok, result.errors, result.warnings
