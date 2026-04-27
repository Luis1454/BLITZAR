#!/usr/bin/env python3
# File: scripts/ci/nightly/package_performance_benchmark.py
# Purpose: Automation script for BLITZAR build, release, or operations tasks.

from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from python_tools.ci.performance_benchmark import PerformanceBenchmarkCampaign


# Description: Executes the parse_args operation.
def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run the nightly performance benchmark campaign.")
    parser.add_argument("--root", default=".", help="Repository root")
    parser.add_argument("--dist-dir", default="dist/performance-benchmark", help="Output directory for the report bundle")
    parser.add_argument("--profile", default="gpu-nightly", help="Campaign profile from docs/quality/manifest/performance_campaign.json")
    parser.add_argument("--tool", required=True, help="Path to gravityPerformanceBenchmarkTool executable")
    return parser.parse_args()


# Description: Executes the main operation.
def main() -> int:
    args = parse_args()
    campaign = PerformanceBenchmarkCampaign()
    archive, report = campaign.run(
        root=Path(args.root),
        dist_dir=Path(args.dist_dir),
        profile=args.profile.strip() or "gpu-nightly",
        tool_path=Path(args.tool),
    )
    print(archive.as_posix())
    return 1 if report.get("status") != "passed" else 0


if __name__ == "__main__":
    raise SystemExit(main())
