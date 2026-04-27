#!/usr/bin/env python3
# File: tests/checks/suites/policy/test_review_gates_solo.py
# Purpose: Verification coverage for the BLITZAR quality gate.

from __future__ import annotations

from pathlib import Path

from python_tools.core.models import CheckContext
from tests.checks.suites.policy.test_review_gates import (
    FakeIvvGateCheck,
    _event_context,
    _write_pr_event,
)


# Description: Executes the test_ivv_gate_passes_for_solo_owner_waiver operation.
def test_ivv_gate_passes_for_solo_owner_waiver(tmp_path: Path) -> None:
    body = """## IV&V Checklist

- [x] Analyzer evidence attached
- [x] Deterministic test evidence attached
- [x] Solo maintainer waiver recorded
- [x] Deviation recorded or not needed

Analyzer evidence: ruff + clang-tidy
Deterministic test evidence: ctest fast subset
Solo maintainer waiver: true
Deviation: WVR-SOLO-IVV
"""

    # Description: Executes the fetch_items operation.
    def fetch_items(repo: str, number: int, suffix: str, token: str, result):  # noqa: ARG001
        return [{"filename": "runtime/src/client/ClientRuntime.cpp"}] if suffix == "files" else []

    event_path = _write_pr_event(tmp_path, body, user="owner")
    context = CheckContext(root=tmp_path, event_name="pull_request", event_path=str(event_path), options={"repo": "owner/repo", "token": "token"})
    result = FakeIvvGateCheck(fetch_items).run(context)
    assert result.ok


# Description: Executes the test_ivv_gate_rejects_invalid_solo_waiver_for_non_owner operation.
def test_ivv_gate_rejects_invalid_solo_waiver_for_non_owner(tmp_path: Path) -> None:
    body = """## IV&V Checklist

- [x] Analyzer evidence attached
- [x] Deterministic test evidence attached
- [x] Solo maintainer waiver recorded
- [x] Deviation recorded or not needed

Analyzer evidence: ruff + clang-tidy
Deterministic test evidence: ctest fast subset
Solo maintainer waiver: true
Deviation: DEV-SOLO-IVV
"""

    # Description: Executes the fetch_items operation.
    def fetch_items(repo: str, number: int, suffix: str, token: str, result):  # noqa: ARG001
        return [{"filename": "tests/unit/physics/orbit.cpp"}] if suffix == "files" else []

    result = FakeIvvGateCheck(fetch_items).run(_event_context(tmp_path, _write_pr_event(tmp_path, body, user="author")))
    assert any("solo maintainer waiver requires" in error for error in result.errors)
