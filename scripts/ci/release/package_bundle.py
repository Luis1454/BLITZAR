#!/usr/bin/env python3
from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from python_tools.ci.release_bundle import ReleaseBundlePackager


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Package Windows release bundle.")
    parser.add_argument("--build-dir", default="build", help="Directory containing built binaries")
    parser.add_argument("--dist-dir", default="dist", help="Output distribution directory")
    parser.add_argument("--tag", default="", help="Archive tag (defaults to GitHub ref/run)")
    parser.add_argument("--tool-manifest", default="", help="Optional generated tool manifest to embed in the bundle")
    parser.add_argument(
        "--artifact-kind",
        choices=("portable", "desktop-installer"),
        default="portable",
        help="Artifact shape to package",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    packager = ReleaseBundlePackager()
    tag = packager.resolve_tag(args.tag)
    tool_manifest = Path(args.tool_manifest) if args.tool_manifest.strip() else None
    archive = packager.package(Path(args.build_dir), Path(args.dist_dir), tag, tool_manifest, args.artifact_kind)
    print(archive.as_posix())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
