#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path

from python_tools.core.models import CheckResult
from python_tools.policies.deviation_register import DeviationRegister
from python_tools.policies.quality_manifest import QualityManifestLoader


def _write_json(path: Path, payload: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload), encoding="utf-8")


def _write_text(path: Path, content: str = "sample\n") -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def _seed_repo(root: Path, review_by: str = "2026-06-30") -> None:
    _write_json(
        root / "docs/quality/quality_manifest.json",
        {
            "metadata": {"system": "test", "revision": "2026-03-07"},
            "includes": ["manifest/evidence.json", "manifest/requirements.json", "manifest/deviations.json"],
        },
    )
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
                    "paths": ["engine/src/backend/SimulationBackend.cpp"],
                    "requirements": ["REQ-TEST-001"],
                    "artifacts": ["EVD_SAMPLE"],
                }
            }
        },
    )
    _write_text(root / "docs/quality/sample.md")


def test_deviation_register_loads_valid_rows(tmp_path: Path) -> None:
    _seed_repo(tmp_path)
    manifest, errors = QualityManifestLoader().load_with_errors(tmp_path)
    result = CheckResult(name="deviations")
    assert errors == []
    rows = DeviationRegister().load(tmp_path, manifest, {"REQ-TEST-001"}, result)

    assert result.errors == []
    assert rows[0]["id"] == "DEV-QUAL-001"
    assert rows[0]["paths"] == ["engine/src/backend/SimulationBackend.cpp"]


def test_deviation_register_rejects_invalid_review_date(tmp_path: Path) -> None:
    _seed_repo(tmp_path, review_by="2026-99-99")
    manifest, errors = QualityManifestLoader().load_with_errors(tmp_path)
    result = CheckResult(name="deviations")
    assert errors == []
    _ = DeviationRegister().load(tmp_path, manifest, {"REQ-TEST-001"}, result)

    assert any("invalid ISO date in review_by" in error for error in result.errors)
