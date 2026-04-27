#!/usr/bin/env python3
# File: tests/checks/suites/policy/test_review_gates.py
# Purpose: Verification coverage for the BLITZAR quality gate.

from __future__ import annotations

import json
from pathlib import Path

from python_tools.core.models import CheckContext, CheckResult
from python_tools.policies.ivv_gate import IvvGateCheck
from python_tools.policies.main_delivery_policy import MainDeliveryGateCheck
from python_tools.policies.pr_policy import PrPolicyCheck, PrPolicySelfTestCheck
from python_tools.policies.traceability_gate import TraceabilityGateCheck


def _write_json(path: Path, payload: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload), encoding="utf-8")


def _write_pr_event(root: Path, body: str, user: str = "author") -> Path:
    path = root / "event.json"
    _write_json(path, {"pull_request": {"number": 42, "body": body, "user": {"login": user}}})
    return path


def _write_pr_payload(root: Path, payload: object) -> Path:
    path = root / "event.json"
    _write_json(path, payload)
    return path


class FakeIvvGateCheck(IvvGateCheck):
    def __init__(self, fetch_items) -> None:
        super().__init__()
        self._fetch_items_impl = fetch_items

    def _fetch_items(self, repo: str, number: int, suffix: str, token: str, result):
        return self._fetch_items_impl(repo, number, suffix, token, result)


class FakeTraceabilityGateCheck(TraceabilityGateCheck):
    def __init__(self, files: list[dict[str, object]]) -> None:
        self._files = files

    def _fetch_files(self, repo: str, number: int, token: str, result: object) -> list[dict[str, object]]:
        del repo, number, token, result
        return list(self._files)


class FakeMainDeliveryGateCheck(MainDeliveryGateCheck):
    def __init__(
        self, pulls_by_sha: dict[str, list[dict[str, object]]] | None = None, compare_commits: list[dict[str, object]] | None = None
    ) -> None:
        self._pulls_by_sha = pulls_by_sha or {}
        self._compare_commits = compare_commits or []

    def _fetch_associated_pulls(self, repo: str, sha: str, token: str, result: CheckResult) -> list[dict[str, object]]:
        del repo, token, result
        return list(self._pulls_by_sha.get(sha, []))

    def _fetch_compare_commits(self, repo: str, before: str, after: str, token: str, result: CheckResult) -> list[dict[str, object]]:
        del repo, before, after, token, result
        return list(self._compare_commits)


def _event_context(root: Path, event_path: Path) -> CheckContext:
    return CheckContext(root=root, event_name="pull_request", event_path=str(event_path), options={"repo": "owner/repo", "token": "token"})


def _write_requirements(root: Path) -> None:
    _write_json(root / "docs/quality/manifest/requirements.json", {"requirements": {"REQ-PROT-001": {}, "REQ-RUN-001": {}, "REQ-PHYS-001": {}}})


def test_pr_policy_accepts_valid_branch_title_body() -> None:
    context = CheckContext(root=Path(".").resolve(), event_name="pull_request", branch="issue/106-enforce-pr-policy", title="Issue #106: Enforce PR policy", body="Implements #106\n\nCloses #106")
    result = PrPolicyCheck().run(context)
    assert result.ok
    assert result.errors == []


def test_pr_policy_accepts_valid_body_with_escaped_newlines() -> None:
    context = CheckContext(
        root=Path(".").resolve(),
        event_name="pull_request",
        branch="issue/106-enforce-pr-policy",
        title="Issue #106: Enforce PR policy",
        body="Implements #106\\n\\nCloses #106",
    )
    result = PrPolicyCheck().run(context)
    assert result.ok
    assert result.errors == []


def test_pr_policy_rejects_issue_mismatch() -> None:
    context = CheckContext(root=Path(".").resolve(), event_name="pull_request", branch="issue/106-enforce-pr-policy", title="Issue #106: Enforce PR policy", body="Implements #106\n\nCloses #107")
    result = PrPolicyCheck().run(context)
    assert not result.ok
    assert any("issue mismatch" in error for error in result.errors)


def test_pr_policy_self_test_uses_fixtures(tmp_path: Path) -> None:
    _write_json(tmp_path / "tests/checks/fixtures/pr_policy_valid.json", {"pull_request": {"title": "Issue #106: Enforce branch-per-issue policy", "body": "Implements #106\n\nCloses #106", "head": {"ref": "issue/106-enforce-branch-per-issue-policy"}, "base": {"ref": "main"}}})
    _write_json(tmp_path / "tests/checks/fixtures/pr_policy_invalid.json", {"pull_request": {"title": "Issue #106: Enforce branch-per-issue policy", "body": "Implements #106\n\nCloses #107", "head": {"ref": "feature/missing-issue-prefix"}, "base": {"ref": "main"}}})
    _write_json(tmp_path / "tests/checks/fixtures/main_delivery_valid.json", {"ref": "refs/heads/main", "after": "abc123"})
    _write_json(tmp_path / "tests/checks/fixtures/main_delivery_invalid.json", {"ref": "refs/heads/main", "after": "abc123"})
    assert PrPolicySelfTestCheck().run(CheckContext(root=tmp_path)).ok


def test_pr_policy_accepts_dependabot_branch_and_autogenerated_body(tmp_path: Path) -> None:
    event_path = _write_pr_payload(
        tmp_path,
        {
            "pull_request": {
                "title": "ci: bump actions/setup-python from 5 to 6",
                "body": "Dependabot will resolve any conflicts with this PR.\n@dependabot rebase",
                "head": {"ref": "dependabot/github_actions/actions/setup-python-6"},
                "base": {"ref": "main"},
                "user": {"login": "app/dependabot"},
            }
        },
    )
    result = PrPolicyCheck().run(CheckContext(root=tmp_path, event_name="pull_request", event_path=str(event_path)))
    assert result.ok


def test_pr_policy_rejects_non_dependabot_branch_spoofing_bot_shape(tmp_path: Path) -> None:
    event_path = _write_pr_payload(
        tmp_path,
        {
            "pull_request": {
                "title": "ci: bump actions/setup-python from 5 to 6",
                "body": "Dependabot footer kept here.",
                "head": {"ref": "feature/manual-bot-spoof"},
                "base": {"ref": "main"},
                "user": {"login": "app/dependabot"},
            }
        },
    )
    result = PrPolicyCheck().run(CheckContext(root=tmp_path, event_name="pull_request", event_path=str(event_path)))
    assert any("dependabot branch must match" in error for error in result.errors)


def test_ivv_gate_passes_for_critical_path_with_non_author_approval(tmp_path: Path) -> None:
    body = """## IV&V Checklist

- [x] Analyzer evidence attached
- [x] Deterministic test evidence attached
- [x] Independent reviewer identified
- [x] Deviation recorded or not needed

Independent reviewer: @reviewer
Analyzer evidence: ruff + clang-tidy
Deterministic test evidence: ctest fast subset
Deviation: none
"""

    def fetch_items(repo: str, number: int, suffix: str, token: str, result):  # noqa: ARG001
        return [{"filename": "runtime/src/client/ClientRuntime.cpp"}] if suffix == "files" else [{"state": "APPROVED", "user": {"login": "reviewer"}}]

    result = FakeIvvGateCheck(fetch_items).run(_event_context(tmp_path, _write_pr_event(tmp_path, body)))
    assert result.ok


def test_ivv_gate_fails_without_non_author_approval_and_skips_non_critical_pr(tmp_path: Path) -> None:
    critical_body = """## IV&V Checklist

- [x] Analyzer evidence attached
- [x] Deterministic test evidence attached
- [x] Independent reviewer identified
- [x] Deviation recorded or not needed

Independent reviewer: @reviewer
Analyzer evidence: ruff + clang-tidy
Deterministic test evidence: ctest fast subset
Deviation: DEV-QUAL-001
"""

    def critical_fetch(repo: str, number: int, suffix: str, token: str, result):  # noqa: ARG001
        return [{"filename": "tests/unit/physics/orbit.cpp"}] if suffix == "files" else [{"state": "COMMENTED", "user": {"login": "reviewer"}}]

    result = FakeIvvGateCheck(critical_fetch).run(_event_context(tmp_path, _write_pr_event(tmp_path, critical_body)))
    assert any("waiting for one GitHub APPROVED review" in error for error in result.errors)

    def docs_fetch(repo: str, number: int, suffix: str, token: str, result):  # noqa: ARG001
        return [{"filename": "docs/quality/quality-overview.md"}] if suffix == "files" else []

    result = FakeIvvGateCheck(docs_fetch).run(_event_context(tmp_path, _write_pr_event(tmp_path, "Independent reviewer: @reviewer\nAnalyzer evidence: ruff\nDeterministic test evidence: ctest\nDeviation: none\n")))
    assert result.ok
    assert "skipped" in result.success_message


def test_traceability_gate_passes_for_critical_pr_with_ids_and_csv(tmp_path: Path) -> None:
    _write_requirements(tmp_path)
    event_path = _write_pr_event(tmp_path, "Requirements impacted:\n- REQ-PROT-001\n- REQ-RUN-001\n\n## Notes\nBody")
    check = FakeTraceabilityGateCheck([{"filename": "runtime/src/client/ClientRuntime.cpp"}, {"filename": "docs/quality/traceability.csv"}])
    assert check.run(_event_context(tmp_path, event_path)).ok


def test_traceability_gate_fails_without_ids_or_csv_and_skips_non_critical_pr(tmp_path: Path) -> None:
    _write_requirements(tmp_path)
    event_path = _write_pr_event(tmp_path, "Requirements impacted:\n\n## Notes\nBody")
    check = FakeTraceabilityGateCheck([{"filename": "runtime/src/client/ClientRuntime.cpp"}, {"filename": "docs/quality/traceability.csv"}])
    result = check.run(_event_context(tmp_path, event_path))
    assert any("Requirements impacted" in error for error in result.errors)

    event_path = _write_pr_event(tmp_path, "Requirements impacted:\n- REQ-PHYS-001")
    result = FakeTraceabilityGateCheck([{"filename": "engine/src/physics/cuda/ParticleSystem.cu"}]).run(_event_context(tmp_path, event_path))
    assert any("traceability.csv" in error for error in result.errors)

    event_path = _write_pr_event(tmp_path, "Requirements impacted:\n- REQ-PROT-001")
    result = FakeTraceabilityGateCheck([{"filename": "README.md"}]).run(_event_context(tmp_path, event_path))
    assert result.ok
    assert "skipped" in result.success_message


def test_main_delivery_gate_accepts_main_push_with_merged_issue_pr(tmp_path: Path) -> None:
    payload = tmp_path / "event.json"
    _write_json(payload, {"ref": "refs/heads/main", "after": "abc123"})
    check = FakeMainDeliveryGateCheck(
        pulls_by_sha={"abc123": [{"base": {"ref": "main"}, "head": {"ref": "issue/106-enforce-branch-per-issue"}, "merged_at": "2026-03-07T00:00:00Z"}]}
    )
    assert check.run(CheckContext(root=tmp_path, event_name="push", event_path=str(payload), options={"repo": "owner/repo", "token": "token"})).ok


def test_main_delivery_gate_rejects_main_push_without_issue_pr_branch(tmp_path: Path) -> None:
    payload = tmp_path / "event.json"
    _write_json(payload, {"ref": "refs/heads/main", "after": "abc123"})
    check = FakeMainDeliveryGateCheck(
        pulls_by_sha={"abc123": [{"base": {"ref": "main"}, "head": {"ref": "feature/direct-main-push"}, "merged_at": "2026-03-07T00:00:00Z"}]}
    )
    result = check.run(CheckContext(root=tmp_path, event_name="push", event_path=str(payload), options={"repo": "owner/repo", "token": "token"}))
    assert any("push to main must come from a merged issue/" in error for error in result.errors)


def test_main_delivery_gate_accepts_traceable_aggregation_merge(tmp_path: Path) -> None:
    payload = tmp_path / "event.json"
    _write_json(payload, {"ref": "refs/heads/main", "before": "oldmain", "after": "merge123"})
    check = FakeMainDeliveryGateCheck(
        pulls_by_sha={"issue123": [{"base": {"ref": "main"}, "head": {"ref": "issue/83-web-adapter"}, "merged_at": "2026-03-15T00:00:00Z"}]},
        compare_commits=[
            {"sha": "merge123", "parents": [{"sha": "oldmain"}, {"sha": "demohead"}]},
            {"sha": "issue123", "parents": [{"sha": "parent"}]},
        ],
    )
    assert check.run(CheckContext(root=tmp_path, event_name="push", event_path=str(payload), options={"repo": "owner/repo", "token": "token"})).ok


def test_main_delivery_gate_rejects_untraceable_commit_in_aggregation_range(tmp_path: Path) -> None:
    payload = tmp_path / "event.json"
    _write_json(payload, {"ref": "refs/heads/main", "before": "oldmain", "after": "merge123"})
    check = FakeMainDeliveryGateCheck(
        compare_commits=[
            {"sha": "merge123", "parents": [{"sha": "oldmain"}, {"sha": "demohead"}]},
            {"sha": "badc0de", "parents": [{"sha": "parent"}]},
        ]
    )
    result = check.run(CheckContext(root=tmp_path, event_name="push", event_path=str(payload), options={"repo": "owner/repo", "token": "token"}))
    assert any("badc0de" in error for error in result.errors)

