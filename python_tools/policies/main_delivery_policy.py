#!/usr/bin/env python3
# File: python_tools/policies/main_delivery_policy.py
# Purpose: Python quality and automation support for BLITZAR governance.

from __future__ import annotations

import json
import re
import urllib.request
from pathlib import Path

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
        payload = self._read_event_payload(Path(context.event_path), result) if context.event_path else {}
        if not payload:
            return
        ref = str(payload.get("ref", "")).strip()
        if ref != "refs/heads/main":
            result.success_message = f"main delivery gate skipped: ref '{ref or 'unknown'}' is not refs/heads/main"
            return
        repo = str(context.options.get("repo", "")).strip()
        token = str(context.options.get("token", "")).strip()
        before = str(payload.get("before", "")).strip()
        sha = str(context.options.get("sha", "")).strip() or str(payload.get("after", "")).strip()
        if not repo or not token or not sha:
            result.add_error("missing repo, token, or commit sha for main delivery gate")
            return
        pulls = self._fetch_associated_pulls(repo, sha, token, result)
        if any(self._is_valid_delivery_pr(item) for item in pulls):
            return
        if before:
            commits = self._fetch_compare_commits(repo, before, sha, token, result)
            if commits and self._compare_range_is_valid(repo, commits, token, result):
                return
        result.add_error("push to main must come from a merged issue/<N>-<slug> pull request")

    def _fetch_associated_pulls(self, repo: str, sha: str, token: str, result: CheckResult) -> list[dict[str, object]]:
        request = urllib.request.Request(
            f"https://api.github.com/repos/{repo}/commits/{sha}/pulls",
            headers={
                "Accept": "application/vnd.github+json",
                "Authorization": f"Bearer {token}",
                "X-GitHub-Api-Version": "2022-11-28",
            },
        )
        try:
            with urllib.request.urlopen(request) as response:
                payload = json.loads(response.read().decode("utf-8"))
        except Exception as exc:
            result.add_error(f"failed to query associated pull requests: {exc}")
            return []
        if not isinstance(payload, list):
            result.add_error("unexpected associated pull requests payload shape")
            return []
        return [item for item in payload if isinstance(item, dict)]

    def _fetch_compare_commits(
        self, repo: str, before: str, after: str, token: str, result: CheckResult
    ) -> list[dict[str, object]]:
        request = urllib.request.Request(
            f"https://api.github.com/repos/{repo}/compare/{before}...{after}",
            headers={
                "Accept": "application/vnd.github+json",
                "Authorization": f"Bearer {token}",
                "X-GitHub-Api-Version": "2022-11-28",
            },
        )
        try:
            with urllib.request.urlopen(request) as response:
                payload = json.loads(response.read().decode("utf-8"))
        except Exception as exc:
            result.add_error(f"failed to compare pushed main range: {exc}")
            return []
        commits = payload.get("commits", []) if isinstance(payload, dict) else []
        if not isinstance(commits, list):
            result.add_error("unexpected compare payload shape")
            return []
        return [item for item in commits if isinstance(item, dict)]

    def _compare_range_is_valid(
        self, repo: str, commits: list[dict[str, object]], token: str, result: CheckResult
    ) -> bool:
        found_issue_delivery = False
        for commit in commits:
            sha = str(commit.get("sha", "")).strip()
            if not sha:
                result.add_error("compare payload contains commit without sha")
                return False
            pulls = self._fetch_associated_pulls(repo, sha, token, result)
            if any(self._is_valid_delivery_pr(item) for item in pulls):
                found_issue_delivery = True
                continue
            parents = commit.get("parents", [])
            if isinstance(parents, list) and len(parents) > 1:
                continue
            result.add_error(
                f"commit {sha[:12]} introduced on main is not traceable to a merged issue/<N>-<slug> pull request"
            )
            return False
        if found_issue_delivery:
            return True
        result.add_error("compare range did not contain any commits traceable to merged issue/<N>-<slug> pull requests")
        return False

    @staticmethod
    def _read_event_payload(path: Path, result: CheckResult) -> dict[str, object]:
        if not path.exists():
            result.add_error(f"event payload not found: {path}")
            return {}
        try:
            payload = json.loads(path.read_text(encoding="utf-8"))
        except Exception as exc:
            result.add_error(f"failed to parse event payload: {exc}")
            return {}
        if not isinstance(payload, dict):
            result.add_error("event payload root must be an object")
            return {}
        return payload

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
