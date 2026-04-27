#!/usr/bin/env python3
# @file tests/checks/suites/policy/test_review_gates_solo.py
# @author Luis1454
# @project BLITZAR
# @brief Automated verification assets for BLITZAR quality gates.

from __future__ import annotations

from pathlib import Path

from python_tools.core.models import CheckContext
from tests.checks.suites.policy.test_review_gates import (
    FakeIvvGateCheck,
    _event_context,
    _write_pr_event,
)


# @brief Documents the test ivv gate passes for solo owner waiver operation contract.
# @param tmp_path Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
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

    # @brief Documents the fetch items operation contract.
    # @param repo Input value used by this contract.
    # @param number Input value used by this contract.
    # @param suffix Input value used by this contract.
    # @param token Input value used by this contract.
    # @param result Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def fetch_items(repo: str, number: int, suffix: str, token: str, result):  # noqa: ARG001
        return [{"filename": "runtime/src/client/ClientRuntime.cpp"}] if suffix == "files" else []

    event_path = _write_pr_event(tmp_path, body, user="owner")
    context = CheckContext(root=tmp_path, event_name="pull_request", event_path=str(event_path), options={"repo": "owner/repo", "token": "token"})
    result = FakeIvvGateCheck(fetch_items).run(context)
    assert result.ok


# @brief Documents the test ivv gate rejects invalid solo waiver for non owner operation contract.
# @param tmp_path Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
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

    # @brief Documents the fetch items operation contract.
    # @param repo Input value used by this contract.
    # @param number Input value used by this contract.
    # @param suffix Input value used by this contract.
    # @param token Input value used by this contract.
    # @param result Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def fetch_items(repo: str, number: int, suffix: str, token: str, result):  # noqa: ARG001
        return [{"filename": "tests/unit/physics/orbit.cpp"}] if suffix == "files" else []

    result = FakeIvvGateCheck(fetch_items).run(_event_context(tmp_path, _write_pr_event(tmp_path, body, user="author")))
    assert any("solo maintainer waiver requires" in error for error in result.errors)
