#!/usr/bin/env python3
from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from python_tools.ci.numerical_validation import NumericalValidationCampaign


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run the nightly numerical validation campaign.")
    parser.add_argument("--root", default=".", help="Repository root")
    parser.add_argument("--dist-dir", default="dist/numerical-validation", help="Output directory for the report bundle")
    parser.add_argument("--profile", default="gpu-prod", help="Campaign profile from docs/quality/manifest/numerical_campaign.json")
    parser.add_argument("--tool", required=True, help="Path to gravityNumericalValidationTool executable")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    campaign = NumericalValidationCampaign()
    archive, report = campaign.run(
        root=Path(args.root),
        dist_dir=Path(args.dist_dir),
        profile=args.profile.strip() or "gpu-prod",
        tool_path=Path(args.tool),
    )
    print(archive.as_posix())
    return 1 if report.get("status") != "passed" else 0


if __name__ == "__main__":
    raise SystemExit(main())
