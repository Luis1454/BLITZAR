#!/usr/bin/env python3
from __future__ import annotations

import re
from collections.abc import Sequence
from pathlib import Path

from python_tools.checks.github_pr import fetch_github_list, load_event_payload
from python_tools.checks.quality_manifest import QualityManifestLoader
from python_tools.checks.test_inventory import TestInventory
from python_tools.core.base_check import BaseCheck
from python_tools.core.models import CheckContext, CheckResult

TRACEABILITY_PATHS = (
    "engine/include/physics/",
    "engine/src/physics/",
    "runtime/",
    "tests/int/protocol/",
    "tests/int/runtime/",
    "tests/unit/physics/",
)
REQUIREMENT_LINE_RE = re.compile(r"REQ-[A-Z]+-[0-9]{3}")
TRACEABILITY_FILE = "docs/quality/traceability.csv"


class TraceabilityGateCheck(BaseCheck):
    name = "traceability_gate"
    success_message = "Traceability gate passed"
    failure_title = "Traceability gate failed:"

    def _execute(self, context: CheckContext, result: CheckResult) -> None:
        if context.event_name.strip() != "pull_request":
            result.success_message = "traceability gate skipped: not a pull_request event"
            return
        payload = load_event_payload(context.event_path, result)
        if not payload:
            return
        pull_request = payload.get("pull_request", {})
        if not isinstance(pull_request, dict):
            result.add_error("pull_request payload is missing")
            return
        repo = context.options.get("repo", "")
        token = context.options.get("token", "")
        number = int(pull_request.get("number", 0) or 0)
        if not isinstance(repo, str) or not repo.strip() or not isinstance(token, str) or not token.strip() or number <= 0:
            result.add_error("missing repo, token, or pull request number for traceability gate")
            return
        files = self._fetch_files(repo.strip(), number, token, result)
        if not self._touches_traceability_paths(files):
            result.success_message = "traceability gate skipped: no runtime/physics/protocol paths changed"
            return
        body = str(pull_request.get("body", "") or "")
        requirement_ids = self._extract_requirement_ids(body)
        if not requirement_ids:
            result.add_error("PR must list impacted requirement IDs in the 'Requirements impacted' section")
        valid_ids = self._load_requirement_ids(context.root, result)
        for requirement_id in requirement_ids:
            if requirement_id not in valid_ids:
                result.add_error(f"unknown requirement id in PR body: {requirement_id}")
        if TRACEABILITY_FILE not in {str(item.get('filename', '')).strip() for item in files}:
            result.add_error(f"critical-path PR must update {TRACEABILITY_FILE}")

    def _fetch_files(self, repo: str, number: int, token: str, result: CheckResult) -> list[dict[str, object]]:
        return fetch_github_list(
            f"https://api.github.com/repos/{repo}/pulls/{number}/files?per_page=100",
            token,
            result,
            "PR files",
        )

    @staticmethod
    def _touches_traceability_paths(files: Sequence[dict[str, object]]) -> bool:
        for item in files:
            filename = str(item.get("filename", "")).strip()
            if any(filename.startswith(prefix) for prefix in TRACEABILITY_PATHS):
                return True
        return False

    @staticmethod
    def _extract_requirement_ids(body: str) -> list[str]:
        capture = False
        lines: list[str] = []
        for raw in body.splitlines():
            stripped = raw.strip()
            if stripped.startswith("## ") and capture:
                break
            if stripped.startswith("Requirements impacted:") or stripped.startswith("Requirements touched:"):
                capture = True
                remainder = stripped.split(":", 1)[1].strip()
                if remainder:
                    lines.append(remainder)
                continue
            if capture and stripped:
                lines.append(stripped)
        requirement_ids = {match.group(0) for line in lines for match in REQUIREMENT_LINE_RE.finditer(line)}
        return sorted(requirement_ids)

    @staticmethod
    def _load_requirement_ids(root: Path, result: CheckResult) -> set[str]:
        loader = QualityManifestLoader()
        manifest = loader.load(root, result)
        return TestInventory(loader).requirement_ids(manifest, result)

