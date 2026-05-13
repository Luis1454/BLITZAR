#!/usr/bin/env python3
# @file scripts/ci/nightly/build_dashboard.py
# @author Luis1454
# @project BLITZAR
# @brief Build, release, and CI helper automation for BLITZAR workflows.

from __future__ import annotations

import os
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from python_tools.ci.coverage_dashboard import CoverageDashboardBuilder, CoverageMetrics


# @brief Documents the read metric operation contract.
# @param name Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def _read_metric(name: str) -> float:
    return float(os.environ[name])


# @brief Documents the main operation contract.
# @param None This contract does not take explicit parameters.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
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

