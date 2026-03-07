#!/usr/bin/env python3
from __future__ import annotations

import json
import re
import urllib.request
from collections.abc import Sequence

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
        payload = self._read_payload(context.event_path, result)
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

    @staticmethod
    def _read_payload(path_text: str, result: CheckResult) -> dict[str, object]:
        if not path_text:
            result.add_error("missing event payload path")
            return {}
        try:
            payload = json.loads(open(path_text, encoding="utf-8").read())
        except Exception as exc:
            result.add_error(f"failed to load event payload: {exc}")
            return {}
        if not isinstance(payload, dict):
            result.add_error("event payload root must be an object")
            return {}
        return payload

    def _fetch_items(self, repo: str, number: int, suffix: str, token: str, result: CheckResult) -> list[dict[str, object]]:
        request = urllib.request.Request(
            f"https://api.github.com/repos/{repo}/pulls/{number}/{suffix}?per_page=100",
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
            result.add_error(f"failed to query PR {suffix}: {exc}")
            return []
        if not isinstance(payload, list):
            result.add_error(f"unexpected PR {suffix} payload shape")
            return []
        return [item for item in payload if isinstance(item, dict)]

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
