#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path

from python_tools.core.models import CheckContext
from python_tools.policies.quality_baseline import QualityBaselineCheck


def _write(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def _seed_required_quality_files(root: Path) -> None:
    _write(root / "docs/quality/README.md", "# Quality\n")
    _write(root / "docs/quality/standards_profile.md", "profile\n")
    _write(root / "docs/quality/fmea.md", "fmea\n")
    _write(root / "docs/quality/tool_qualification.md", "tools\n")
    _write(root / "docs/quality/ivv_plan.md", "ivv\n")
    _write(root / "docs/quality/numerical_validation.md", "numerical\n")
    _write(root / "docs/quality/test_catalog.csv", "test_code,test_id,req_ids,source\n")


def _seed_baseline_payloads(root: Path, test_regex: str) -> None:
    requirements = {
        "system": "test",
        "revision": "2026-03-01",
        "requirements": [{"id": "REQ-COMP-001", "title": "Crosswalk evidence exists", "verification": {"tests": [test_regex], "artifacts": ["docs/quality/nasa_crosswalk.csv"]}}],
    }
    _write(root / "docs/quality/requirements.json", json.dumps(requirements, indent=2) + "\n")
    _write(root / "docs/quality/traceability.csv", "req_id,item_type,item_ref,evidence\n" f"REQ-COMP-001,test_regex,{test_regex},unit-test\n")
    _write(root / "docs/quality/nasa_crosswalk.csv", "control_id,source_standard,repo_artifact,verification\n" "CTRL-001,NPR-7150.2D,docs/quality/README.md,review\n")


def test_quality_baseline_passes_with_valid_minimal_repo(tmp_path: Path) -> None:
    _seed_required_quality_files(tmp_path)
    _seed_baseline_payloads(tmp_path, r"^TST_QLT_REPO_006_GravityQualityBaselineCheck$")
    result = QualityBaselineCheck().run(CheckContext(root=tmp_path))
    assert result.ok
    assert result.errors == []


def test_quality_baseline_fails_when_test_regex_has_no_match(tmp_path: Path) -> None:
    _seed_required_quality_files(tmp_path)
    _seed_baseline_payloads(tmp_path, r"^NO_MATCH$")
    result = QualityBaselineCheck().run(CheckContext(root=tmp_path))
    assert not result.ok
    assert any("did not match any test id" in error for error in result.errors)

