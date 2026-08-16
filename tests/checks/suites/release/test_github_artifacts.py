# @file tests/checks/suites/release/test_github_artifacts.py
# @author Luis1454
# @project BLITZAR
# @brief Deterministic tests for GitHub artifact classification and retention policy.

from __future__ import annotations

from datetime import UTC, datetime, timedelta
from pathlib import Path

from python_tools.ci.github_artifacts import Artifact, build_report, classify_artifact, delete_expired_artifacts


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
    assert classify_artifact("tool-qualification-release-v1") == "release-staging"
    assert classify_artifact("github-pages") == "github-pages"
    assert classify_artifact("unexpected-output") == "unclassified"


# @brief Documents the stale artifact report contract.
# @return No return value.
# @note The report must expose stale and expired counts without deleting remote state.
def test_report_counts_stale_artifacts_without_side_effects() -> None:
    now = datetime(2026, 8, 16, tzinfo=UTC)
    artifacts = [
        Artifact(1, "pr-coverage-delta-1", 100, now - timedelta(days=8), False),
        Artifact(2, "nightly-full-logs-2", 200, now - timedelta(days=10), False),
        Artifact(3, "release-bundle-v1", 300, now - timedelta(days=31), False),
        Artifact(4, "unexpected-output", 400, now - timedelta(days=100), True),
    ]
    report = build_report(artifacts, now)
    assert report["total_count"] == 4
    assert report["total_bytes"] == 1000
    assert report["expired_count"] == 1
    assert report["classes"]["pull-request"]["stale"] == 1
    assert report["classes"]["nightly-evidence"]["stale"] == 0
    assert report["classes"]["release-staging"]["stale"] == 1
    assert report["classes"]["unclassified"]["stale"] == 0
    assert report["classes"]["unclassified"]["expired"] == 1
    assert report["stale_artifacts"][0]["id"] == 1
    assert report["unclassified_artifacts"] == [
        {
            "id": 4,
            "name": "unexpected-output",
            "size": 400,
            "created_at": (now - timedelta(days=100)).isoformat(),
            "expired": True,
        }
    ]


# @brief Documents that cleanup deletes only artifacts already marked expired.
# @param monkeypatch Input fixture used to isolate the GitHub API call.
# @return No return value.
# @note Active artifacts must never be selected by the cleanup operation.
def test_delete_expired_artifacts_does_not_delete_active_items(monkeypatch) -> None:
    calls: list[str] = []

    class Response:
        def __enter__(self):
            return self

        def __exit__(self, *_args):
            return False

    def fake_urlopen(request, timeout):
        calls.append(f"{request.method}:{request.full_url}:{timeout}")
        return Response()

    monkeypatch.setattr("python_tools.ci.github_artifacts.urlopen", fake_urlopen)
    deleted, failures = delete_expired_artifacts(
        "Luis1454/BLITZAR",
        "token",
        [
            Artifact(10, "expired", 1, datetime.now(UTC), True),
            Artifact(11, "active", 1, datetime.now(UTC), False),
        ],
    )

    assert deleted == 1
    assert failures == []
    assert calls == [
        "DELETE:https://api.github.com/repos/Luis1454/BLITZAR/actions/artifacts/10:30"
    ]


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
