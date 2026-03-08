#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path

from python_tools.core.models import CheckContext
from python_tools.policies.ivv_gate import IvvGateCheck


def _write_payload(path: Path, body: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        json.dumps(
            {
                "pull_request": {
                    "number": 42,
                    "body": body,
                    "user": {"login": "author"},
                }
            }
        ),
        encoding="utf-8",
    )


class FakeIvvGateCheck(IvvGateCheck):
    def __init__(self, fetch_items) -> None:
        super().__init__()
        self._fetch_items_impl = fetch_items

    def _fetch_items(self, repo: str, number: int, suffix: str, token: str, result):
        return self._fetch_items_impl(repo, number, suffix, token, result)


def _context(tmp_path: Path, body: str, fetch_items):
    payload = tmp_path / "event.json"
    _write_payload(payload, body)
    check = FakeIvvGateCheck(fetch_items)
    return check, CheckContext(
        root=tmp_path,
        event_name="pull_request",
        event_path=str(payload),
        options={"repo": "owner/repo", "token": "token"},
    )


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
        if suffix == "files":
            return [{"filename": "runtime/src/frontend/FrontendRuntime.cpp"}]
        return [{"state": "APPROVED", "user": {"login": "reviewer"}}]

    check, context = _context(tmp_path, body, fetch_items)
    result = check.run(context)
    assert result.ok


def test_ivv_gate_fails_without_non_author_approval(tmp_path: Path) -> None:
    body = """## IV&V Checklist

- [x] Analyzer evidence attached
- [x] Deterministic test evidence attached
- [x] Independent reviewer identified
- [x] Deviation recorded or not needed

Independent reviewer: @reviewer
Analyzer evidence: ruff + clang-tidy
Deterministic test evidence: ctest fast subset
Deviation: DEV-QUAL-001
"""

    def fetch_items(repo: str, number: int, suffix: str, token: str, result):  # noqa: ARG001
        if suffix == "files":
            return [{"filename": "tests/unit/physics/orbit.cpp"}]
        return [{"state": "COMMENTED", "user": {"login": "reviewer"}}]

    check, context = _context(tmp_path, body, fetch_items)
    result = check.run(context)
    assert not result.ok
    assert any("APPROVED review" in error for error in result.errors)


def test_ivv_gate_skips_non_critical_pr(tmp_path: Path) -> None:
    body = "Independent reviewer: @reviewer\nAnalyzer evidence: ruff\nDeterministic test evidence: ctest\nDeviation: none\n"

    def fetch_items(repo: str, number: int, suffix: str, token: str, result):  # noqa: ARG001
        if suffix == "files":
            return [{"filename": "docs/quality/README.md"}]
        return []

    check, context = _context(tmp_path, body, fetch_items)
    result = check.run(context)
    assert result.ok
    assert "skipped" in result.success_message
