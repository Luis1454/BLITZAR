#!/usr/bin/env python3
"""CLI: Create or update a GitHub Release with notes and artifact files."""
from __future__ import annotations

import argparse
import glob
import subprocess
import sys
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Publish a GitHub Release.")
    parser.add_argument("--tag", required=True, help="Release tag name (e.g. v1.2.3)")
    parser.add_argument("--notes-file", default="", help="Markdown file with release notes")
    parser.add_argument("--assets", nargs="*", default=[], help="Glob patterns for asset files to attach")
    parser.add_argument("--repo", default="", help="repo: OWNER/NAME (defaults to GH_REPO env / auto-detect)")
    parser.add_argument("--prerelease", action="store_true", help="Mark as pre-release")
    parser.add_argument("--draft", action="store_true", help="Create as draft release")
    return parser.parse_args()


def _resolve_assets(patterns: list[str]) -> list[str]:
    files: list[str] = []
    for pattern in patterns:
        matched = glob.glob(pattern, recursive=True)
        files.extend(matched)
    return sorted(set(files))


def main() -> int:
    args = parse_args()

    cmd: list[str] = ["gh", "release", "create", args.tag]

    if args.repo:
        cmd += ["--repo", args.repo]
    if args.notes_file:
        notes_path = Path(args.notes_file)
        if not notes_path.exists():
            print(f"Error: notes file not found: {notes_path}", file=sys.stderr)
            return 1
        cmd += ["--notes-file", str(notes_path)]
    else:
        cmd += ["--generate-notes"]
    if args.prerelease:
        cmd.append("--prerelease")
    if args.draft:
        cmd.append("--draft")

    assets = _resolve_assets(args.assets)
    cmd.extend(assets)

    print(f"Running: {' '.join(cmd)}")
    result = subprocess.run(cmd, check=False)
    return result.returncode


if __name__ == "__main__":
    raise SystemExit(main())
