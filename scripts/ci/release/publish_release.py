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
    parser.add_argument("--target", default="", help="Branch or commit used when creating a missing tag")
    parser.add_argument("--prerelease", action="store_true", help="Mark as pre-release")
    parser.add_argument("--draft", action="store_true", help="Create as draft release")
    return parser.parse_args()


def _resolve_assets(patterns: list[str]) -> list[str]:
    files: list[str] = []
    for pattern in patterns:
        matched = glob.glob(pattern, recursive=True)
        files.extend(matched)
    return sorted(set(files))


def _repo_flags(repo: str) -> list[str]:
    if repo:
        return ["--repo", repo]
    return []


def _release_exists(tag: str, repo: str) -> bool:
    cmd = ["gh", "release", "view", tag, *_repo_flags(repo)]
    return subprocess.run(cmd, check=False, capture_output=True, text=True).returncode == 0


def _notes_flags(notes_file: str, generate_when_missing: bool) -> list[str]:
    if notes_file:
        notes_path = Path(notes_file)
        if not notes_path.exists():
            print(f"Error: notes file not found: {notes_path}", file=sys.stderr)
            raise FileNotFoundError(notes_path)
        return ["--notes-file", str(notes_path)]
    return ["--generate-notes"] if generate_when_missing else []


def _state_flags(prerelease: bool, draft: bool) -> list[str]:
    flags: list[str] = []
    if prerelease:
        flags.append("--prerelease")
    if draft:
        flags.append("--draft")
    return flags


def _run(cmd: list[str]) -> int:
    print(f"Running: {' '.join(cmd)}")
    result = subprocess.run(cmd, check=False)
    return result.returncode


def main() -> int:
    args = parse_args()
    try:
        create_notes_flags = _notes_flags(args.notes_file, generate_when_missing=True)
        edit_notes_flags = _notes_flags(args.notes_file, generate_when_missing=False)
    except FileNotFoundError:
        return 1
    assets = _resolve_assets(args.assets)
    repo_flags = _repo_flags(args.repo)

    if _release_exists(args.tag, args.repo):
        edit = ["gh", "release", "edit", args.tag, *repo_flags, *edit_notes_flags, *_state_flags(args.prerelease, args.draft)]
        edit_status = _run(edit)
        if edit_status != 0:
            return edit_status
        if assets:
            return _run(["gh", "release", "upload", args.tag, *assets, "--clobber", *repo_flags])
        return 0

    create = ["gh", "release", "create", args.tag, *repo_flags, *create_notes_flags, *_state_flags(args.prerelease, args.draft)]
    if args.target.strip():
        create += ["--target", args.target.strip()]
    create.extend(assets)
    return _run(create)


if __name__ == "__main__":
    raise SystemExit(main())
