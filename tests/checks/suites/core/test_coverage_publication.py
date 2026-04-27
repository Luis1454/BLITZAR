# File: tests/checks/suites/core/test_coverage_publication.py
# Purpose: Verification coverage for the BLITZAR quality gate.

from __future__ import annotations

from pathlib import Path


def test_readme_uses_coverage_data_branch_for_badges_and_widget() -> None:
    readme = Path("README.md").read_text(encoding="utf-8")
    assert "raw.githubusercontent.com/Luis1454/BLITZAR/coverage-data/coverage/widget.svg" in readme
    assert "raw.githubusercontent.com%2FLuis1454%2FBLITZAR%2Fcoverage-data%2Fcoverage%2Flines.json" in readme
    assert "luis1454.github.io/BLITZAR/coverage/" not in readme


def test_nightly_workflow_publishes_coverage_data_branch() -> None:
    workflow = Path(".github/workflows/nightly-full.yml").read_text(encoding="utf-8")
    assert "Publish coverage payload branch" in workflow
    assert "bash scripts/ci/nightly/publish_coverage_branch.sh coverage-dashboard" in workflow
    assert "nightly-integration-coverage:" in workflow
    assert "permissions:\n      contents: write\n      statuses: write" in workflow
    assert "Deploy Coverage Pages" not in workflow

