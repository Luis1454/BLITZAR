#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path

from python_tools.checks.traceability_gate import TraceabilityGateCheck
from python_tools.core.models import CheckContext


class FakeTraceabilityGateCheck(TraceabilityGateCheck):
    def __init__(self, files: list[dict[str, object]]) -> None:
        self._files = files

    def _fetch_files(self, repo: str, number: int, token: str, result: object) -> list[dict[str, object]]:
        del repo, number, token, result
        return list(self._files)


def _write_payload(root: Path, body: str) -> Path:
    payload = {
        "pull_request": {
            "number": 98,
            "body": body,
        }
    }
    path = root / "event.json"
    path.write_text(json.dumps(payload), encoding="utf-8")
    return path


def _write_requirements(root: Path) -> None:
    path = root / "docs/quality/manifest/requirements.json"
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        json.dumps({"requirements": {"REQ-PROT-001": {}, "REQ-RUN-001": {}, "REQ-PHYS-001": {}}}, indent=2),
        encoding="utf-8",
    )
    (root / "docs/quality/quality_manifest.json").write_text(
        json.dumps(
            {
                "metadata": {"system": "test", "revision": "2026-03-09"},
                "includes": ["manifest/requirements.json"],
            },
            indent=2,
        ),
        encoding="utf-8",
    )


def _context(root: Path, event_path: Path) -> CheckContext:
    return CheckContext(
        root=root,
        event_name="pull_request",
        event_path=str(event_path),
        options={"repo": "owner/repo", "token": "token"},
    )


def test_traceability_gate_passes_for_critical_pr_with_ids_and_csv(tmp_path: Path) -> None:
    _write_requirements(tmp_path)
    event_path = _write_payload(
        tmp_path,
        "Requirements impacted:\n- REQ-PROT-001\n- REQ-RUN-001\n\n## Notes\nBody",
    )
    check = FakeTraceabilityGateCheck(
        [
            {"filename": "runtime/src/frontend/FrontendRuntime.cpp"},
            {"filename": "docs/quality/traceability.csv"},
        ]
    )
    result = check.run(_context(tmp_path, event_path))
    assert result.ok


def test_traceability_gate_fails_without_requirement_ids(tmp_path: Path) -> None:
    _write_requirements(tmp_path)
    event_path = _write_payload(tmp_path, "Requirements impacted:\n\n## Notes\nBody")
    check = FakeTraceabilityGateCheck(
        [
            {"filename": "runtime/src/frontend/FrontendRuntime.cpp"},
            {"filename": "docs/quality/traceability.csv"},
        ]
    )
    result = check.run(_context(tmp_path, event_path))
    assert not result.ok
    assert any("Requirements impacted" in error for error in result.errors)


def test_traceability_gate_fails_when_csv_is_not_updated(tmp_path: Path) -> None:
    _write_requirements(tmp_path)
    event_path = _write_payload(tmp_path, "Requirements impacted:\n- REQ-PHYS-001")
    check = FakeTraceabilityGateCheck([{"filename": "engine/src/physics/cuda/ParticleSystem.cu"}])
    result = check.run(_context(tmp_path, event_path))
    assert not result.ok
    assert any("traceability.csv" in error for error in result.errors)


def test_traceability_gate_skips_non_critical_pr(tmp_path: Path) -> None:
    _write_requirements(tmp_path)
    event_path = _write_payload(tmp_path, "Requirements impacted:\n- REQ-PROT-001")
    check = FakeTraceabilityGateCheck([{"filename": "docs/README_full.md"}])
    result = check.run(_context(tmp_path, event_path))
    assert result.ok
    assert "skipped" in result.success_message
