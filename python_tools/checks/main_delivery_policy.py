#!/usr/bin/env python3
from __future__ import annotations

import re

from python_tools.checks.github_pr import fetch_github_list, load_event_payload
from python_tools.core.base_check import BaseCheck
from python_tools.core.models import CheckContext, CheckResult

BRANCH_RE = re.compile(r"^issue/(?P<issue>\d+)-[a-z0-9]+(?:-[a-z0-9]+)*$")


class MainDeliveryGateCheck(BaseCheck):
    name = "main_delivery_gate"
    success_message = "Main delivery gate passed"
    failure_title = "Main delivery gate failed:"

    def _execute(self, context: CheckContext, result: CheckResult) -> None:
        if context.event_name.strip() != "push":
            result.success_message = "main delivery gate skipped: not a push event"
            return
        payload = load_event_payload(context.event_path, result) if context.event_path else {}
        if not payload:
            return
        ref = str(payload.get("ref", "")).strip()
        if ref != "refs/heads/main":
            result.success_message = f"main delivery gate skipped: ref '{ref or 'unknown'}' is not refs/heads/main"
            return
        repo = str(context.options.get("repo", "")).strip()
        token = str(context.options.get("token", "")).strip()
        sha = str(context.options.get("sha", "")).strip() or str(payload.get("after", "")).strip()
        if not repo or not token or not sha:
            result.add_error("missing repo, token, or commit sha for main delivery gate")
            return
        pulls = self._fetch_associated_pulls(repo, sha, token, result)
        if any(self._is_valid_delivery_pr(item) for item in pulls):
            return
        result.add_error("push to main must come from a merged issue/<N>-<slug> pull request")

    def _fetch_associated_pulls(self, repo: str, sha: str, token: str, result: CheckResult) -> list[dict[str, object]]:
        return fetch_github_list(
            f"https://api.github.com/repos/{repo}/commits/{sha}/pulls",
            token,
            result,
            "associated pull requests",
        )

    @staticmethod
    def _is_valid_delivery_pr(pull_request: dict[str, object]) -> bool:
        base = pull_request.get("base", {})
        head = pull_request.get("head", {})
        base_ref = str(base.get("ref", "")).strip() if isinstance(base, dict) else ""
        head_ref = str(head.get("ref", "")).strip() if isinstance(head, dict) else ""
        merged_at = str(pull_request.get("merged_at", "")).strip()
        return base_ref == "main" and bool(merged_at) and bool(BRANCH_RE.match(head_ref))


class MainDeliverySelfTestCheck(MainDeliveryGateCheck):
    def __init__(self, valid: bool) -> None:
        self._valid = valid

    def _fetch_associated_pulls(self, repo: str, sha: str, token: str, result: CheckResult) -> list[dict[str, object]]:
        del repo, sha, token, result
        if not self._valid:
            return [{"base": {"ref": "main"}, "head": {"ref": "feature/direct-main-push"}, "merged_at": "2026-03-07T00:00:00Z"}]
        return [{"base": {"ref": "main"}, "head": {"ref": "issue/106-enforce-branch-per-issue"}, "merged_at": "2026-03-07T00:00:00Z"}]

    def _execute(self, context: CheckContext, result: CheckResult) -> None:
        fixture = "main_delivery_valid.json" if self._valid else "main_delivery_invalid.json"
        super()._execute(
            CheckContext(
                root=context.root,
                event_name="push",
                event_path=str(context.root / "tests/checks/fixtures" / fixture),
                options={"repo": "owner/repo", "token": "token", "sha": "abc123"},
            ),
            result,
        )

