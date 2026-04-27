#!/usr/bin/env python3
# File: scripts/ci/nightly/build_dashboard.py
# Purpose: Automation script for BLITZAR build, release, or operations tasks.

from __future__ import annotations

import os
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from python_tools.ci.coverage_dashboard import CoverageDashboardBuilder, CoverageMetrics


def _read_metric(name: str) -> float:
    return float(os.environ[name])


def main() -> int:
    builder = CoverageDashboardBuilder()
    metrics = CoverageMetrics(
        lines=_read_metric("LINES_PCT"),
        functions=_read_metric("FUNCS_PCT"),
        branches=_read_metric("BRANCH_PCT"),
    )
    builder.build(Path("coverage-dashboard"), metrics)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

