#!/usr/bin/env python3
from __future__ import annotations

import re

from python_tools.checks.github_pr import load_event_payload
from python_tools.checks.main_delivery_policy import MainDeliverySelfTestCheck
from python_tools.core.base_check import BaseCheck
from python_tools.core.models import CheckContext, CheckResult

BRANCH_RE = re.compile(r"^issue/(?P<issue>\d+)-[a-z0-9]+(?:-[a-z0-9]+)*$")
TITLE_RE = re.compile(r"^Issue\s+#(?P<issue>\d+):\s+\S")
IMPLEMENTS_RE = re.compile(r"\bImplements\s+#(?P<issue>\d+)\b", re.IGNORECASE)
BODY_RE = re.compile(r"\bCloses\s+#(?P<issue>\d+)\b", re.IGNORECASE)


class PrPolicyCheck(BaseCheck):
    name = "pr_policy"
    success_message = "PR policy check passed"
    failure_title = "PR policy check failed:"

    def _execute(self, context: CheckContext, result: CheckResult) -> None:
        event_name = context.event_name.strip()
        if event_name != "pull_request":
            result.success_message = f"pr policy check skipped: event '{event_name or 'unknown'}' is not pull_request"
            return

        branch = context.branch.strip()
        title = context.title.strip()
        body = context.body
        base = ""
        if not (branch and title and body):
            if not context.event_path:
                result.add_error("missing --event-path for pull_request event")
                return
            payload = load_event_payload(context.event_path, result)
            pull_request = payload.get("pull_request", {}) if isinstance(payload, dict) else {}
            if not isinstance(pull_request, dict):
                result.add_error("pull_request payload is missing")
                return
            head = pull_request.get("head", {})
            base_ref = pull_request.get("base", {})
            branch_ref = head.get("ref", "") if isinstance(head, dict) else ""
            main_ref = base_ref.get("ref", "") if isinstance(base_ref, dict) else ""
            branch = branch or str(branch_ref).strip()
            base = str(main_ref).strip()
            title = title or str(pull_request.get("title", "")).strip()
            body = body or str(pull_request.get("body", ""))
        if base and base != "main":
            result.add_error(f"pull request base must be main, got '{base}'")
        branch_issue = self._extract_branch_issue(branch, result)
        title_issue = self._extract_issue("PR title", title, TITLE_RE, result)
        implements_issue = self._extract_issue("PR body", body, IMPLEMENTS_RE, result)
        body_issue = self._extract_issue("PR body", body, BODY_RE, result)
        self._check_issue_mismatch(branch_issue, title_issue, implements_issue, body_issue, result)

    def _extract_branch_issue(self, branch: str, result: CheckResult) -> int | None:
        match = BRANCH_RE.match(branch or "")
        if not match:
            result.add_error("branch must match issue/<N>-<slug> with lowercase kebab-case slug")
            return None
        return int(match.group("issue"))

    def _extract_issue(self, label: str, text: str, regex: re.Pattern[str], result: CheckResult) -> int | None:
        match = regex.search(text or "")
        if not match:
            result.add_error(f"{label} does not match expected format")
            return None
        return int(match.group("issue"))

    def _check_issue_mismatch(
        self,
        branch_issue: int | None,
        title_issue: int | None,
        implements_issue: int | None,
        body_issue: int | None,
        result: CheckResult,
    ) -> None:
        if branch_issue is not None and title_issue is not None and branch_issue != title_issue:
            result.add_error(f"issue mismatch: branch #{branch_issue} vs title #{title_issue}")
        if branch_issue is not None and implements_issue is not None and branch_issue != implements_issue:
            result.add_error(f"issue mismatch: branch #{branch_issue} vs implements #{implements_issue}")
        if branch_issue is not None and body_issue is not None and branch_issue != body_issue:
            result.add_error(f"issue mismatch: branch #{branch_issue} vs body #{body_issue}")
        if title_issue is not None and implements_issue is not None and title_issue != implements_issue:
            result.add_error(f"issue mismatch: title #{title_issue} vs implements #{implements_issue}")
        if title_issue is not None and body_issue is not None and title_issue != body_issue:
            result.add_error(f"issue mismatch: title #{title_issue} vs body #{body_issue}")
        if implements_issue is not None and body_issue is not None and implements_issue != body_issue:
            result.add_error(f"issue mismatch: implements #{implements_issue} vs body #{body_issue}")


class PrPolicySelfTestCheck(BaseCheck):
    name = "pr_policy_self_test"
    success_message = "PR policy check self-test passed"
    failure_title = "PR policy check self-test failed:"

    def _execute(self, context: CheckContext, result: CheckResult) -> None:
        fixtures_dir = context.root / "tests/checks/fixtures"
        valid_context = CheckContext(
            root=context.root,
            event_name="pull_request",
            event_path=str(fixtures_dir / "pr_policy_valid.json"),
        )
        invalid_context = CheckContext(
            root=context.root,
            event_name="pull_request",
            event_path=str(fixtures_dir / "pr_policy_invalid.json"),
        )
        valid_result = PrPolicyCheck().run(valid_context)
        invalid_result = PrPolicyCheck().run(invalid_context)
        if not valid_result.ok:
            details = "; ".join(valid_result.errors) if valid_result.errors else "unknown error"
            result.add_error(f"[valid-payload] expected success, got failure: {details}")
        if invalid_result.ok:
            result.add_error("[invalid-payload] expected failure, got success")
        delivery_valid = MainDeliverySelfTestCheck(valid=True).run(CheckContext(root=context.root))
        delivery_invalid = MainDeliverySelfTestCheck(valid=False).run(CheckContext(root=context.root))
        if not delivery_valid.ok:
            details = "; ".join(delivery_valid.errors) if delivery_valid.errors else "unknown error"
            result.add_error(f"[main-delivery-valid] expected success, got failure: {details}")
        if delivery_invalid.ok:
            result.add_error("[main-delivery-invalid] expected failure, got success")

