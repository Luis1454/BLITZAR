#!/usr/bin/env python3
# File: scripts/ci/release/package_source.py
# Purpose: Automation script for BLITZAR build, release, or operations tasks.

from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from python_tools.ci.release_source import ReleaseSourcePackager


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Package tracked source code for a release.")
    parser.add_argument("--repo-root", default=".", help="Repository root to archive")
    parser.add_argument("--dist-dir", default="dist/source", help="Output directory")
    parser.add_argument("--tag", default="", help="Archive tag (defaults to GitHub ref/run)")
    parser.add_argument("--ref", default="HEAD", help="Git ref to archive")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    packager = ReleaseSourcePackager()
    tag = packager.resolve_tag(args.tag)
    archive = packager.package(Path(args.repo_root), Path(args.dist_dir), tag, args.ref)
    print(archive.as_posix())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
