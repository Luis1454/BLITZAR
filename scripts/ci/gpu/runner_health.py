#!/usr/bin/env python3
# File: scripts/ci/gpu/runner_health.py
# Purpose: Automation script for BLITZAR build, release, or operations tasks.

from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from python_tools.ci.gpu_runner_inventory import GitHubGpuRunnerInventory


# Description: Executes the parse_args operation.
def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Collect GitHub self-hosted GPU runner health.")
    parser.add_argument("--repo", required=True, help="owner/repo")
    parser.add_argument("--label", action="append", default=[], help="Required runner label (repeatable)")
    parser.add_argument("--token", default="", help="GitHub token with runner inventory access")
    parser.add_argument("--api-url", default="https://api.github.com", help="GitHub API base URL")
    parser.add_argument("--output", required=True, help="Output JSON report path")
    parser.add_argument("--enabled", action="store_true", help="Treat GPU lanes as enabled")
    parser.add_argument("--min-online", type=int, default=1, help="Minimum online matching runners")
    parser.add_argument("--min-idle", type=int, default=1, help="Minimum idle matching runners")
    return parser.parse_args()


# Description: Executes the main operation.
def main() -> int:
    args = parse_args()
    collector = GitHubGpuRunnerInventory()
    report = collector.collect(
        repo=args.repo,
        labels=args.label,
        enabled=args.enabled,
        min_online=args.min_online,
        min_idle=args.min_idle,
        token=args.token,
        api_url=args.api_url,
    )
    collector.write(report, str(Path(args.output)))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
