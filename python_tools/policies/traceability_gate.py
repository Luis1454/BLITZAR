#!/usr/bin/env python3
# @file python_tools/policies/traceability_gate.py
# @author Luis1454
# @project BLITZAR
# @brief Python quality and automation support for BLITZAR governance.

from __future__ import annotations

import json
import re
import urllib.request
from collections.abc import Sequence
from pathlib import Path

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


# @brief Defines the traceability gate check type contract.
# @param None This contract does not take explicit parameters.
# @note Keep construction and side effects explicit for deterministic quality gates.
class TraceabilityGateCheck(BaseCheck):
    name = "traceability_gate"
    success_message = "Traceability gate passed"
    failure_title = "Traceability gate failed:"

    # @brief Documents the execute operation contract.
    # @param context Input value used by this contract.
    # @param result Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _execute(self, context: CheckContext, result: CheckResult) -> None:
        if context.event_name.strip() != "pull_request":
            result.success_message = "traceability gate skipped: not a pull_request event"
            return
        payload = self._read_payload(context.event_path, result)
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

    @staticmethod
    # @brief Documents the read payload operation contract.
    # @param path_text Input value used by this contract.
    # @param result Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _read_payload(path_text: str, result: CheckResult) -> dict[str, object]:
        if not path_text:
            result.add_error("missing event payload path")
            return {}
        try:
            payload = json.loads(Path(path_text).read_text(encoding="utf-8"))
        except Exception as exc:
            result.add_error(f"failed to load event payload: {exc}")
            return {}
        if not isinstance(payload, dict):
            result.add_error("event payload root must be an object")
            return {}
        return payload

    # @brief Documents the fetch files operation contract.
    # @param repo Input value used by this contract.
    # @param number Input value used by this contract.
    # @param token Input value used by this contract.
    # @param result Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _fetch_files(self, repo: str, number: int, token: str, result: CheckResult) -> list[dict[str, object]]:
        request = urllib.request.Request(
            f"https://api.github.com/repos/{repo}/pulls/{number}/files?per_page=100",
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
            result.add_error(f"failed to query PR files: {exc}")
            return []
        if not isinstance(payload, list):
            result.add_error("unexpected PR files payload shape")
            return []
        return [item for item in payload if isinstance(item, dict)]

    @staticmethod
    # @brief Documents the touches traceability paths operation contract.
    # @param files Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _touches_traceability_paths(files: Sequence[dict[str, object]]) -> bool:
        for item in files:
            filename = str(item.get("filename", "")).strip()
            if any(filename.startswith(prefix) for prefix in TRACEABILITY_PATHS):
                return True
        return False

    @staticmethod
    # @brief Documents the extract requirement ids operation contract.
    # @param body Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
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
    # @brief Documents the load requirement ids operation contract.
    # @param root Input value used by this contract.
    # @param result Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _load_requirement_ids(root: Path, result: CheckResult) -> set[str]:
        path = root / "docs/quality/manifest/requirements.json"
        try:
            payload = json.loads(path.read_text(encoding="utf-8"))
        except Exception as exc:
            result.add_error(f"failed to load requirement registry: {exc}")
            return set()
        requirements = payload.get("requirements")
        if not isinstance(requirements, dict):
            result.add_error("requirement registry payload is missing 'requirements'")
            return set()
        return {str(key).strip() for key, value in requirements.items() if isinstance(key, str) and isinstance(value, dict)}
