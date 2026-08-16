#!/usr/bin/env python3
# @file scripts/ci/audit_github_artifacts.py
# @author Luis1454
# @project BLITZAR
# @brief CLI entry point for the GitHub Actions artifact inventory.

from __future__ import annotations

import argparse
import json
import os
import sys
from datetime import UTC, datetime
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from python_tools.ci.github_artifacts import build_report, fetch_artifacts


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Audit GitHub Actions artifact retention classes.")
    parser.add_argument("--repo", default=os.getenv("GITHUB_REPOSITORY", ""), help="owner/name repository")
    parser.add_argument("--token", default=os.getenv("GITHUB_TOKEN", ""), help="GitHub API token")
    parser.add_argument("--output", type=Path, help="optional JSON report path")
    parser.add_argument("--fail-on-stale", action="store_true", help="return 2 when stale artifacts exist")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if not args.repo or not args.token:
        print("--repo and --token (or GITHUB_REPOSITORY/GITHUB_TOKEN) are required", file=sys.stderr)
        return 2
    report: dict[str, Any] = build_report(fetch_artifacts(args.repo, args.token), datetime.now(UTC))
    serialized = json.dumps(report, indent=2) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(serialized, encoding="utf-8")
    else:
        print(serialized, end="")
    stale = sum(int(item["stale"]) for item in report["classes"].values())
    return 2 if args.fail_on_stale and stale else 0


if __name__ == "__main__":
    raise SystemExit(main())
