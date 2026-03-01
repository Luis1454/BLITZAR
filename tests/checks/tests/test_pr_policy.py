#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path

from python_tools.core.models import CheckContext
from python_tools.policies.pr_policy import PrPolicyCheck, PrPolicySelfTestCheck


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
            }
        },
    )
    result = PrPolicySelfTestCheck().run(CheckContext(root=tmp_path))
    assert result.ok

