# @file tests/checks/suites/release/test_github_artifacts.py
# @author Luis1454
# @project BLITZAR
# @brief Deterministic tests for GitHub artifact classification and retention policy.

from __future__ import annotations

from datetime import UTC, datetime, timedelta
from pathlib import Path

from python_tools.ci.github_artifacts import Artifact, build_report, classify_artifact


# @brief Documents the artifact class mapping contract.
# @return No return value.
# @note Prefixes must map to one explicit retention authority.
def test_artifact_prefixes_have_explicit_classes() -> None:
    assert classify_artifact("pr-coverage-delta-1") == "pull-request"
    assert classify_artifact("tool-qualification-pr-fast-1") == "pull-request"
    assert classify_artifact("nightly-full-logs-1") == "nightly-evidence"
    assert classify_artifact("gpu-runner-health-1") == "gpu-health"
    assert classify_artifact("release-bundle-v1") == "release-staging"
    assert classify_artifact("desktop-installer-v1") == "release-staging"
    assert classify_artifact("unexpected-output") == "unclassified"


# @brief Documents the stale artifact report contract.
# @return No return value.
# @note The report must expose stale counts without deleting remote state.
def test_report_counts_stale_artifacts_without_side_effects() -> None:
    now = datetime(2026, 8, 16, tzinfo=UTC)
    artifacts = [
        Artifact(1, "pr-coverage-delta-1", 100, now - timedelta(days=8), False),
        Artifact(2, "nightly-full-logs-2", 200, now - timedelta(days=10), False),
        Artifact(3, "release-bundle-v1", 300, now - timedelta(days=31), False),
        Artifact(4, "unexpected-output", 400, now - timedelta(days=100), False),
    ]
    report = build_report(artifacts, now)
    assert report["total_count"] == 4
    assert report["total_bytes"] == 1000
    assert report["classes"]["pull-request"]["stale"] == 1
    assert report["classes"]["nightly-evidence"]["stale"] == 0
    assert report["classes"]["release-staging"]["stale"] == 1
    assert report["classes"]["unclassified"]["stale"] == 0
    assert report["stale_artifacts"][0]["id"] == 1


# @brief Documents the workflow retention declaration contract.
# @return No return value.
# @note Every upload-artifact action must declare its retention explicitly.
def test_upload_artifact_workflows_declare_retention() -> None:
    root = Path(__file__).resolve().parents[4]
    workflow_paths = list((root / ".github" / "workflows").glob("*.yml"))
    for path in workflow_paths:
        content = path.read_text(encoding="utf-8")
        blocks = content.split("uses: actions/upload-artifact@")
        for block in blocks[1:]:
            action_block = block.split("\n      - ", 1)[0]
            assert "retention-days:" in action_block, f"missing retention in {path}"
