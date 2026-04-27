#!/usr/bin/env python3
# @file scripts/ci/release/package_quality_index.py
# @author Luis1454
# @project BLITZAR
# @brief Build, release, and CI helper automation for BLITZAR workflows.

from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from python_tools.ci.release_quality_index import ReleaseQualityIndexBuilder
from python_tools.ci.release_support import (
    build_release_lane_activities,
    build_release_lane_analyzers,
    default_ci_context,
)


# @brief Documents the parse args operation contract.
# @param None This contract does not take explicit parameters.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Package release quality index.")
    parser.add_argument("--root", default=".", help="Repository root")
    parser.add_argument("--dist-dir", default="dist/release-quality-index", help="Output directory for index files")
    parser.add_argument("--tag", default="", help="Archive tag (defaults to GitHub ref/run)")
    parser.add_argument("--profile", default="prod", help="Qualification profile recorded in the index")
    parser.add_argument("--requirement", action="append", default=[], help="Requirement id to include (defaults to all manifest requirements)")
    return parser.parse_args()


# @brief Documents the main operation contract.
# @param None This contract does not take explicit parameters.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def main() -> int:
    args = parse_args()
    builder = ReleaseQualityIndexBuilder()
    ci_context = default_ci_context()
    ci_context["source"] = "release-lane"
    archive = builder.package(
        root=Path(args.root),
        dist_dir=Path(args.dist_dir),
        tag=builder.resolve_tag(args.tag),
        profile=args.profile.strip() or "prod",
        requirements=args.requirement or None,
        verification_activities=build_release_lane_activities(args.profile.strip() or "prod"),
        analyzer_status=build_release_lane_analyzers(),
        ci_context=ci_context,
    )
    print(archive.as_posix())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
