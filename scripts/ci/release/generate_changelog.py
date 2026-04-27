#!/usr/bin/env python3
# @file scripts/ci/release/generate_changelog.py
# @author Luis1454
# @project BLITZAR
# @brief Build, release, and CI helper automation for BLITZAR workflows.

"""CLI: Generate a Markdown changelog from Conventional Commits git history."""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from python_tools.ci.changelog import generate_changelog, render_markdown


# @brief Documents the parse args operation contract.
# @param None This contract does not take explicit parameters.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate changelog from git history.")
    parser.add_argument("--tag", required=True, help="Current release tag (e.g. v1.2.3)")
    parser.add_argument("--previous-tag", default="", help="Previous tag (auto-detected if omitted)")
    parser.add_argument("--repo-root", default=".", help="Path to git repository root")
    parser.add_argument("--output", default="CHANGELOG.md", help="Output file path")
    return parser.parse_args()


# @brief Documents the main operation contract.
# @param None This contract does not take explicit parameters.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def main() -> int:
    args = parse_args()
    repo_root = Path(args.repo_root).resolve()
    data = generate_changelog(repo_root, args.tag, args.previous_tag)
    markdown = render_markdown(data)
    out = Path(args.output)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(markdown, encoding="utf-8")
    print(f"Changelog written to {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
