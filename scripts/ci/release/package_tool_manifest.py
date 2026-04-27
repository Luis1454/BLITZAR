#!/usr/bin/env python3
# @file scripts/ci/release/package_tool_manifest.py
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

from python_tools.ci.tool_manifest import ToolManifestCollector


# @brief Documents the parse args operation contract.
# @param None This contract does not take explicit parameters.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate tool qualification manifest.")
    parser.add_argument("--output", default="dist/tool-qualification/tool_manifest.json", help="Output manifest path")
    parser.add_argument("--lane", default="manual", help="CI or review lane name")
    parser.add_argument("--profile", default="prod", help="Profile recorded in the manifest")
    return parser.parse_args()


# @brief Documents the main operation contract.
# @param None This contract does not take explicit parameters.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def main() -> int:
    args = parse_args()
    collector = ToolManifestCollector()
    manifest = collector.collect(lane=args.lane.strip() or "manual", profile=args.profile.strip() or "prod")
    output = collector.write(manifest, Path(args.output))
    print(output.as_posix())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
