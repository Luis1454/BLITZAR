#!/usr/bin/env python3
# @file scripts/ci/nightly/package_numerical_validation.py
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

from python_tools.ci.numerical_validation import NumericalValidationCampaign


# @brief Documents the parse args operation contract.
# @param None This contract does not take explicit parameters.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run the nightly numerical validation campaign.")
    parser.add_argument("--root", default=".", help="Repository root")
    parser.add_argument("--dist-dir", default="dist/numerical-validation", help="Output directory for the report bundle")
    parser.add_argument("--profile", default="gpu-prod", help="Campaign profile from docs/quality/manifest/numerical_campaign.json")
    parser.add_argument("--tool", required=True, help="Path to blitzarNumericalValidationTool executable")
    return parser.parse_args()


# @brief Documents the main operation contract.
# @param None This contract does not take explicit parameters.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
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
