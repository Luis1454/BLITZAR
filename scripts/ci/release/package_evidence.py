#!/usr/bin/env python3
# File: scripts/ci/release/package_evidence.py
# Purpose: Automation script for BLITZAR build, release, or operations tasks.

from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from python_tools.ci.release_evidence_pack import ReleaseEvidencePackager
from python_tools.ci.release_support import (
    build_release_lane_activities,
    build_release_lane_analyzers,
    default_ci_context,
)


# Description: Executes the parse_args operation.
def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Package qualification-oriented release evidence.")
    parser.add_argument("--root", default=".", help="Repository root")
    parser.add_argument("--dist-dir", default="dist/evidence-pack", help="Output directory for evidence pack files")
    parser.add_argument("--tag", default="", help="Archive tag (defaults to GitHub ref/run)")
    parser.add_argument("--profile", default="prod", help="Qualification profile recorded in the pack")
    parser.add_argument("--requirement", action="append", default=[], help="Requirement id to include (defaults to all manifest requirements)")
    parser.add_argument("--evidence-ref", action="append", default=[], help="Additional EVD_* reference to bundle")
    return parser.parse_args()


# Description: Executes the main operation.
def main() -> int:
    args = parse_args()
    packager = ReleaseEvidencePackager()
    ci_context = default_ci_context()
    ci_context["source"] = "release-lane"
    archive = packager.package(
        root=Path(args.root),
        dist_dir=Path(args.dist_dir),
        tag=packager.resolve_tag(args.tag),
        profile=args.profile.strip() or "prod",
        requirements=args.requirement or None,
        verification_activities=build_release_lane_activities(args.profile.strip() or "prod"),
        analyzer_status=build_release_lane_analyzers(),
        ci_context=ci_context,
        extra_evidence_refs=args.evidence_ref,
    )
    print(archive.as_posix())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
