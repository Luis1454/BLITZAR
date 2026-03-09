#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path

from python_tools.checks.main_delivery_policy import MainDeliveryGateCheck
from python_tools.checks.pr_policy import PrPolicyCheck, PrPolicySelfTestCheck
from python_tools.core.models import CheckContext, CheckResult


def _write_json(path: Path, payload: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload), encoding="utf-8")


def test_pr_policy_accepts_valid_branch_title_body() -> None:
    context = CheckContext(
        root=Path(".").resolve(),
        event_name="pull_request",
        branch="issue/106-enforce-pr-policy",
        title="Issue #106: Enforce PR policy",
        body="Implements #106\n\nCloses #106",
    )
    result = PrPolicyCheck().run(context)
    assert result.ok
    assert result.errors == []


def test_pr_policy_rejects_issue_mismatch() -> None:
    context = CheckContext(
        root=Path(".").resolve(),
        event_name="pull_request",
        branch="issue/106-enforce-pr-policy",
        title="Issue #106: Enforce PR policy",
        body="Implements #106\n\nCloses #107",
    )
    result = PrPolicyCheck().run(context)
    assert not result.ok
    assert any("issue mismatch" in error for error in result.errors)


def test_pr_policy_self_test_uses_fixtures(tmp_path: Path) -> None:
    _write_json(
        tmp_path / "tests/checks/fixtures/pr_policy_valid.json",
        {
            "pull_request": {
                "title": "Issue #106: Enforce branch-per-issue policy",
                "body": "Implements #106\n\nCloses #106",
                "head": {"ref": "issue/106-enforce-branch-per-issue-policy"},
                "base": {"ref": "main"},
            }
        },
    )
    _write_json(
        tmp_path / "tests/checks/fixtures/pr_policy_invalid.json",
        {
            "pull_request": {
                "title": "Issue #106: Enforce branch-per-issue policy",
                "body": "Implements #106\n\nCloses #107",
                "head": {"ref": "feature/missing-issue-prefix"},
                "base": {"ref": "main"},
            }
        },
    )
    _write_json(tmp_path / "tests/checks/fixtures/main_delivery_valid.json", {"ref": "refs/heads/main", "after": "abc123"})
    _write_json(tmp_path / "tests/checks/fixtures/main_delivery_invalid.json", {"ref": "refs/heads/main", "after": "abc123"})
    result = PrPolicySelfTestCheck().run(CheckContext(root=tmp_path))
    assert result.ok


class FakeMainDeliveryGateCheck(MainDeliveryGateCheck):
    def __init__(self, pulls: list[dict[str, object]]) -> None:
        self._pulls = pulls

    def _fetch_associated_pulls(self, repo: str, sha: str, token: str, result: CheckResult) -> list[dict[str, object]]:
        del repo, sha, token, result
        return list(self._pulls)


def test_main_delivery_gate_accepts_main_push_with_merged_issue_pr(tmp_path: Path) -> None:
    payload = tmp_path / "event.json"
    _write_json(payload, {"ref": "refs/heads/main", "after": "abc123"})
    check = FakeMainDeliveryGateCheck(
        [{"base": {"ref": "main"}, "head": {"ref": "issue/106-enforce-branch-per-issue"}, "merged_at": "2026-03-07T00:00:00Z"}]
    )
    result = check.run(CheckContext(root=tmp_path, event_name="push", event_path=str(payload), options={"repo": "owner/repo", "token": "token"}))
    assert result.ok


def test_main_delivery_gate_rejects_main_push_without_issue_pr_branch(tmp_path: Path) -> None:
    payload = tmp_path / "event.json"
    _write_json(payload, {"ref": "refs/heads/main", "after": "abc123"})
    check = FakeMainDeliveryGateCheck(
        [{"base": {"ref": "main"}, "head": {"ref": "feature/direct-main-push"}, "merged_at": "2026-03-07T00:00:00Z"}]
    )
    result = check.run(CheckContext(root=tmp_path, event_name="push", event_path=str(payload), options={"repo": "owner/repo", "token": "token"}))
    assert not result.ok
    assert any("push to main must come from a merged issue/" in error for error in result.errors)
