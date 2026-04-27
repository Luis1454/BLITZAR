# @file tests/checks/suites/core/test_coverage_publication.py
# @author Luis1454
# @project BLITZAR
# @brief Automated verification assets for BLITZAR quality gates.

from __future__ import annotations

from pathlib import Path


# @brief Documents the test readme uses coverage data branch for badges and widget operation contract.
# @param None This contract does not take explicit parameters.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def test_readme_uses_coverage_data_branch_for_badges_and_widget() -> None:
    readme = Path("README.md").read_text(encoding="utf-8")
    assert "raw.githubusercontent.com/Luis1454/BLITZAR/coverage-data/coverage/widget.svg" in readme
    assert "raw.githubusercontent.com%2FLuis1454%2FBLITZAR%2Fcoverage-data%2Fcoverage%2Flines.json" in readme
    assert "luis1454.github.io/BLITZAR/coverage/" not in readme


# @brief Documents the test nightly workflow publishes coverage data branch operation contract.
# @param None This contract does not take explicit parameters.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def test_nightly_workflow_publishes_coverage_data_branch() -> None:
    workflow = Path(".github/workflows/nightly-full.yml").read_text(encoding="utf-8")
    assert "Publish coverage payload branch" in workflow
    assert "bash scripts/ci/nightly/publish_coverage_branch.sh coverage-dashboard" in workflow
    assert "nightly-integration-coverage:" in workflow
    assert "permissions:\n      contents: write\n      statuses: write" in workflow
    assert "Deploy Coverage Pages" not in workflow

