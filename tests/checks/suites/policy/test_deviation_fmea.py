#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path

import pytest

from python_tools.ci.fmea_status import FmeaStatusSnapshot
from python_tools.core.models import CheckResult
from python_tools.policies.deviation_register import DeviationRegister
from python_tools.policies.fmea_action_register import FmeaActionRegister, FmeaActionRegisterError
from python_tools.policies.quality_manifest import QualityManifestLoader


def _write_json(path: Path, payload: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2), encoding="utf-8")


def _write_text(path: Path, content: str = "sample\n") -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def _seed_deviation_repo(root: Path, review_by: str = "2026-06-30") -> None:
    _write_json(root / "docs/quality/quality_manifest.json", {"metadata": {"system": "test", "revision": "2026-03-07"}, "includes": ["manifest/evidence.json", "manifest/requirements.json", "manifest/deviations.json"]})
    _write_json(root / "docs/quality/manifest/evidence.json", {"evidence": {"EVD_SAMPLE": "docs/quality/sample.md"}})
    _write_json(root / "docs/quality/manifest/requirements.json", {"requirements": {"REQ-TEST-001": {"tests": [".*"], "artifacts": ["EVD_SAMPLE"]}}})
    _write_json(
        root / "docs/quality/manifest/deviations.json",
        {
            "deviations": {
                "DEV-QUAL-001": {
                    "kind": "deviation",
                    "status": "open",
                    "scope": "repository-policy:file-size",
                    "owner": "maintainer",
                    "approver": "quality-review",
                    "introduced_on": "2026-03-07",
                    "review_by": review_by,
                    "rationale": "Temporary split pending.",
                    "mitigation": "Review on every change.",
                    "closure_criteria": "Split file and remove allowlist path.",
                    "paths": ["engine/src/server/SimulationServer.cpp"],
                    "requirements": ["REQ-TEST-001"],
                    "artifacts": ["EVD_SAMPLE"],
                }
            }
        },
    )
    _write_text(root / "docs/quality/sample.md")


def _write_fmea_rows(root: Path, rows: list[dict[str, object]]) -> None:
    _write_json(root / "docs/quality/manifest/fmea_actions.json", {"fmea_actions": rows})


def test_deviation_register_loads_valid_rows_and_rejects_invalid_review_date(tmp_path: Path) -> None:
    _seed_deviation_repo(tmp_path)
    manifest, errors = QualityManifestLoader().load_with_errors(tmp_path)
    result = CheckResult(name="deviations")
    assert errors == []
    rows = DeviationRegister().load(tmp_path, manifest, {"REQ-TEST-001"}, result)
    assert result.errors == []
    assert rows[0]["id"] == "DEV-QUAL-001"
    assert rows[0]["paths"] == ["engine/src/server/SimulationServer.cpp"]

    _seed_deviation_repo(tmp_path, review_by="2026-99-99")
    manifest, errors = QualityManifestLoader().load_with_errors(tmp_path)
    result = CheckResult(name="deviations")
    assert errors == []
    DeviationRegister().load(tmp_path, manifest, {"REQ-TEST-001"}, result)
    assert any("invalid ISO date in review_by" in error for error in result.errors)


def test_fmea_action_register_accepts_linked_medium_risk(tmp_path: Path) -> None:
    _write_fmea_rows(
        tmp_path,
        [
            {"id": "FMEA-001", "owner": "maintainer", "status": "closed", "residual_risk": "Low", "linked_tasks": [], "verification_evidence": ["EVD_TEST"]},
            {"id": "FMEA-002", "owner": "maintainer", "status": "in-progress", "residual_risk": "Medium", "linked_tasks": ["#102"], "verification_evidence": ["EVD_TEST"]},
        ],
    )
    assert [row["id"] for row in FmeaActionRegister().load(tmp_path)] == ["FMEA-001", "FMEA-002"]


@pytest.mark.parametrize(
    ("rows", "expected"),
    [
        ([{"id": "FMEA-002", "owner": "maintainer", "status": "in-progress", "residual_risk": "Medium", "linked_tasks": [], "verification_evidence": ["EVD_TEST"]}], "requires at least one linked task"),
        ([{"id": "FMEA-001", "owner": "maintainer", "status": "closed", "residual_risk": "Low", "linked_tasks": [], "verification_evidence": []}], "require linked verification evidence"),
    ],
)
def test_fmea_action_register_rejects_invalid_rows(tmp_path: Path, rows: list[dict[str, object]], expected: str) -> None:
    _write_fmea_rows(tmp_path, rows)
    with pytest.raises(FmeaActionRegisterError, match=expected):
        FmeaActionRegister().load(tmp_path)


def test_fmea_status_snapshot_packages_summary(tmp_path: Path) -> None:
    _write_fmea_rows(
        tmp_path,
        [
            {"id": "FMEA-001", "owner": "config-maintainer", "status": "closed", "residual_risk": "Low", "linked_tasks": [], "verification_evidence": ["EVD_TEST"]},
            {"id": "FMEA-002", "owner": "runtime-maintainer", "status": "in-progress", "residual_risk": "Medium", "linked_tasks": ["#120"], "verification_evidence": ["EVD_TEST"]},
        ],
    )
    archive = FmeaStatusSnapshot().package(tmp_path, tmp_path / "dist/fmea")
    assert archive.exists()
    snapshot = json.loads((tmp_path / "dist/fmea/fmea_status_snapshot.json").read_text(encoding="utf-8"))
    assert snapshot["summary"] == {"open": 0, "in_progress": 1, "closed": 1}
    assert [row["id"] for row in snapshot["high_medium"]] == ["FMEA-002"]
