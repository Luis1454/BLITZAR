#!/usr/bin/env python3
from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from python_tools.ci.release_sbom import ReleaseSbomPackager


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Package a CycloneDX SBOM for release artifacts.")
    parser.add_argument("--artifacts-dir", default="dist/release-bundle", help="Directory containing packaged release artifacts")
    parser.add_argument("--dist-dir", default="dist/release-sbom", help="Output directory for the generated SBOM")
    parser.add_argument("--tag", default="", help="Release tag override")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    packager = ReleaseSbomPackager()
    sbom = packager.package(Path(args.artifacts_dir), Path(args.dist_dir), packager.resolve_tag(args.tag))
    print(sbom.as_posix())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
