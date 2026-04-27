#!/usr/bin/env python3
# File: scripts/ci/release/package_tool_manifest.py
# Purpose: Automation script for BLITZAR build, release, or operations tasks.

from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from python_tools.ci.tool_manifest import ToolManifestCollector


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate tool qualification manifest.")
    parser.add_argument("--output", default="dist/tool-qualification/tool_manifest.json", help="Output manifest path")
    parser.add_argument("--lane", default="manual", help="CI or review lane name")
    parser.add_argument("--profile", default="prod", help="Profile recorded in the manifest")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    collector = ToolManifestCollector()
    manifest = collector.collect(lane=args.lane.strip() or "manual", profile=args.profile.strip() or "prod")
    output = collector.write(manifest, Path(args.output))
    print(output.as_posix())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
