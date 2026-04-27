# @file tests/checks/suites/core/test_coverage_dashboard.py
# @author Luis1454
# @project BLITZAR
# @brief Automated verification assets for BLITZAR quality gates.

from __future__ import annotations

import json
from pathlib import Path

from python_tools.ci.coverage_dashboard import CoverageDashboardBuilder, CoverageMetrics


# @brief Documents the test coverage dashboard writes widget and summary operation contract.
# @param tmp_path Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def test_coverage_dashboard_writes_widget_and_summary(tmp_path: Path) -> None:
    builder = CoverageDashboardBuilder()

    output = builder.build(
        tmp_path,
        CoverageMetrics(lines=82.4, functions=77.1, branches=63.8),
    )

    assert output == tmp_path / "index.html"
    widget = (tmp_path / "coverage" / "widget.svg").read_text(encoding="utf-8")
    summary = json.loads((tmp_path / "coverage" / "summary.json").read_text(encoding="utf-8"))

    assert "Coverage Control" in widget
    assert "82.4%" in widget
    assert "77.1%" in widget
    assert "63.8%" in widget
    assert "Lines" in widget
    assert "Functions" in widget
    assert "Branches" in widget
    assert summary["lines_pct"] == 82.4
    assert summary["functions_pct"] == 77.1
    assert summary["branches_pct"] == 63.8


# @brief Documents the test coverage dashboard index links widget operation contract.
# @param tmp_path Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def test_coverage_dashboard_index_links_widget(tmp_path: Path) -> None:
    builder = CoverageDashboardBuilder()

    builder.build(
        tmp_path,
        CoverageMetrics(lines=91.0, functions=88.0, branches=80.0),
    )

    index = (tmp_path / "index.html").read_text(encoding="utf-8")
    assert "coverage/widget.svg" in index
    assert "coverage/summary.json" in index
