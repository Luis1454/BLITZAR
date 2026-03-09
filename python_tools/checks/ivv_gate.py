#!/usr/bin/env python3
from __future__ import annotations

import re
from collections.abc import Sequence

from python_tools.checks.github_pr import fetch_github_list, load_event_payload
from python_tools.core.base_check import BaseCheck
from python_tools.core.models import CheckContext, CheckResult

CRITICAL_PATHS = (
    "docs/backend_protocol.md",
    "docs/quality/numerical_validation.md",
    "engine/include/physics/",
    "engine/src/physics/",
    "runtime/",
    "tests/int/protocol/",
    "tests/int/runtime/",
    "tests/unit/physics/",
)
DEVIATION_RE = re.compile(r"^(none|DEV-[A-Z0-9-]+|WVR-[A-Z0-9-]+)$", re.IGNORECASE)
CHECKLIST_MARKERS = (
    "- [x] Analyzer evidence attached",
    "- [x] Deterministic test evidence attached",
    "- [x] Independent reviewer identified",
    "- [x] Deviation recorded or not needed",
)


class IvvGateCheck(BaseCheck):
    name = "ivv_gate"
    success_message = "IV&V gate passed"
    failure_title = "IV&V gate failed:"

    def _execute(self, context: CheckContext, result: CheckResult) -> None:
        if context.event_name.strip() != "pull_request":
            result.success_message = "ivv gate skipped: not a pull_request event"
            return
        payload = load_event_payload(context.event_path, result)
        if not payload:
            return
        pull_request = payload.get("pull_request", {})
        if not isinstance(pull_request, dict):
            result.add_error("pull_request payload is missing")
            return
        author = str(pull_request.get("user", {}).get("login", "")).strip()
        body = str(pull_request.get("body", "") or "")
        repo = context.options.get("repo", "")
        token = context.options.get("token", "")
        number = int(pull_request.get("number", 0) or 0)
        if not isinstance(repo, str) or not repo.strip() or not isinstance(token, str) or not token.strip() or number <= 0:
            result.add_error("missing repo, token, or pull request number for IV&V gate")
            return
        files = self._fetch_items(repo.strip(), number, "files", token, result)
        if not self._touches_critical_paths(files):
            result.success_message = "IV&V gate skipped: no protocol/runtime/physics paths changed"
            return
        self._check_template(author, body, result)
        reviews = self._fetch_items(repo.strip(), number, "reviews", token, result)
        self._check_non_author_approval(author, reviews, result)

    def _fetch_items(self, repo: str, number: int, suffix: str, token: str, result: CheckResult) -> list[dict[str, object]]:
        return fetch_github_list(
            f"https://api.github.com/repos/{repo}/pulls/{number}/{suffix}?per_page=100",
            token,
            result,
            f"PR {suffix}",
        )

    @staticmethod
    def _touches_critical_paths(files: Sequence[dict[str, object]]) -> bool:
        for item in files:
            filename = str(item.get("filename", "")).strip()
            if any(filename == prefix or filename.startswith(prefix) for prefix in CRITICAL_PATHS):
                return True
        return False

    def _check_template(self, author: str, body: str, result: CheckResult) -> None:
        for marker in CHECKLIST_MARKERS:
            if marker not in body:
                result.add_error(f"missing PR checklist confirmation: {marker}")
        reviewer = self._extract_line(body, "Independent reviewer")
        analyzer = self._extract_line(body, "Analyzer evidence")
        deterministic = self._extract_line(body, "Deterministic test evidence")
        deviation = self._extract_line(body, "Deviation")
        if not reviewer.startswith("@") or reviewer.lower() == f"@{author.lower()}":
            result.add_error("independent reviewer must be a non-author GitHub handle")
        if not analyzer:
            result.add_error("analyzer evidence field must be filled")
        if not deterministic:
            result.add_error("deterministic test evidence field must be filled")
        if not DEVIATION_RE.match(deviation):
            result.add_error("deviation field must be 'none' or a DEV-/WVR- identifier")

    @staticmethod
    def _extract_line(body: str, label: str) -> str:
        prefix = f"{label}:"
        for raw in body.splitlines():
            if raw.startswith(prefix):
                return raw[len(prefix) :].strip()
        return ""

    @staticmethod
    def _check_non_author_approval(author: str, reviews: Sequence[dict[str, object]], result: CheckResult) -> None:
        for review in reviews:
            state = str(review.get("state", "")).upper()
            user = review.get("user", {})
            reviewer = str(user.get("login", "")).strip() if isinstance(user, dict) else ""
            if state == "APPROVED" and reviewer and reviewer.lower() != author.lower():
                return
        result.add_error("critical-path PR requires at least one APPROVED review from a non-author reviewer")

