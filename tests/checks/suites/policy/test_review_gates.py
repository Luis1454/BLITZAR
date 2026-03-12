#!/usr/bin/env python3
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
    def __init__(self, pulls: list[dict[str, object]]) -> None:
        self._pulls = pulls

    def _fetch_associated_pulls(self, repo: str, sha: str, token: str, result: CheckResult) -> list[dict[str, object]]:
        del repo, sha, token, result
        return list(self._pulls)


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
        return [{"filename": "runtime/src/frontend/FrontendRuntime.cpp"}] if suffix == "files" else [{"state": "APPROVED", "user": {"login": "reviewer"}}]

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
    assert any("APPROVED review" in error for error in result.errors)

    def docs_fetch(repo: str, number: int, suffix: str, token: str, result):  # noqa: ARG001
        return [{"filename": "docs/quality/README.md"}] if suffix == "files" else []

    result = FakeIvvGateCheck(docs_fetch).run(_event_context(tmp_path, _write_pr_event(tmp_path, "Independent reviewer: @reviewer\nAnalyzer evidence: ruff\nDeterministic test evidence: ctest\nDeviation: none\n")))
    assert result.ok
    assert "skipped" in result.success_message


def test_traceability_gate_passes_for_critical_pr_with_ids_and_csv(tmp_path: Path) -> None:
    _write_requirements(tmp_path)
    event_path = _write_pr_event(tmp_path, "Requirements impacted:\n- REQ-PROT-001\n- REQ-RUN-001\n\n## Notes\nBody")
    check = FakeTraceabilityGateCheck([{"filename": "runtime/src/frontend/FrontendRuntime.cpp"}, {"filename": "docs/quality/traceability.csv"}])
    assert check.run(_event_context(tmp_path, event_path)).ok


def test_traceability_gate_fails_without_ids_or_csv_and_skips_non_critical_pr(tmp_path: Path) -> None:
    _write_requirements(tmp_path)
    event_path = _write_pr_event(tmp_path, "Requirements impacted:\n\n## Notes\nBody")
    check = FakeTraceabilityGateCheck([{"filename": "runtime/src/frontend/FrontendRuntime.cpp"}, {"filename": "docs/quality/traceability.csv"}])
    result = check.run(_event_context(tmp_path, event_path))
    assert any("Requirements impacted" in error for error in result.errors)

    event_path = _write_pr_event(tmp_path, "Requirements impacted:\n- REQ-PHYS-001")
    result = FakeTraceabilityGateCheck([{"filename": "engine/src/physics/cuda/ParticleSystem.cu"}]).run(_event_context(tmp_path, event_path))
    assert any("traceability.csv" in error for error in result.errors)

    event_path = _write_pr_event(tmp_path, "Requirements impacted:\n- REQ-PROT-001")
    result = FakeTraceabilityGateCheck([{"filename": "docs/README_full.md"}]).run(_event_context(tmp_path, event_path))
    assert result.ok
    assert "skipped" in result.success_message


def test_main_delivery_gate_accepts_main_push_with_merged_issue_pr(tmp_path: Path) -> None:
    payload = tmp_path / "event.json"
    _write_json(payload, {"ref": "refs/heads/main", "after": "abc123"})
    check = FakeMainDeliveryGateCheck([{"base": {"ref": "main"}, "head": {"ref": "issue/106-enforce-branch-per-issue"}, "merged_at": "2026-03-07T00:00:00Z"}])
    assert check.run(CheckContext(root=tmp_path, event_name="push", event_path=str(payload), options={"repo": "owner/repo", "token": "token"})).ok


def test_main_delivery_gate_rejects_main_push_without_issue_pr_branch(tmp_path: Path) -> None:
    payload = tmp_path / "event.json"
    _write_json(payload, {"ref": "refs/heads/main", "after": "abc123"})
    check = FakeMainDeliveryGateCheck([{"base": {"ref": "main"}, "head": {"ref": "feature/direct-main-push"}, "merged_at": "2026-03-07T00:00:00Z"}])
    result = check.run(CheckContext(root=tmp_path, event_name="push", event_path=str(payload), options={"repo": "owner/repo", "token": "token"}))
    assert any("push to main must come from a merged issue/" in error for error in result.errors)
