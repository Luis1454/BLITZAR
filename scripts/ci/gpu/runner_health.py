#!/usr/bin/env python3
# @file scripts/ci/gpu/runner_health.py
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

from python_tools.ci.gpu_runner_inventory import GitHubGpuRunnerInventory


# @brief Documents the parse args operation contract.
# @param None This contract does not take explicit parameters.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
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


# @brief Documents the main operation contract.
# @param None This contract does not take explicit parameters.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
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
