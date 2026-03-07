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
    _write(root / "docs/quality/tool_manifest.md", "tool manifest\n")
    _write(root / "docs/quality/prod_baseline.md", "prod baseline\n")
    _write(root / "docs/quality/release_index.md", "release index\n")
    _write(root / "docs/quality/interface_contracts.md", "contracts\n")
    _write(root / "docs/quality/ivv_plan.md", "ivv\n")
    _write(root / "docs/quality/numerical_validation.md", "numerical\n")
    _write(root / "docs/quality/quality_manifest.json", "{}\n")
    _write(root / "tests/cmake/targets.cmake", "add_test(NAME TST_QLT_REPO_006_GravityQualityBaselineCheck COMMAND fake)\n")


def _seed_baseline_payloads(root: Path, test_regex: str) -> None:
    manifest = {
        "metadata": {"system": "test", "revision": "2026-03-01"},
        "evidence": {
            "EVD_QLT_MANIFEST": "docs/quality/quality_manifest.json",
            "EVD_QLT_README": "docs/quality/README.md",
        },
        "policies": {
            "test_ids": {
                "repo_quality": {
                    "regex": r"\bTST_QLT_REPO_[0-9]{3}_[A-Za-z0-9_]+\b",
                    "files": ["tests/cmake/targets.cmake"],
                }
            }
        },
        "requirements": {
            "REQ-COMP-001": {
                "tests": [test_regex],
                "artifacts": ["EVD_QLT_MANIFEST"],
            }
        },
        "test_groups": {
            "EVD_CHECK_QUALITY": {
                "TST-QLT-REPO-006": {
                    "id": "TST_QLT_REPO_006_GravityQualityBaselineCheck",
                    "req_ids": ["REQ-COMP-001"],
                }
            }
        },
        "crosswalk": {
            "CTRL-001": {
                "source_standard": "NPR-7150.2D",
                "artifact": "EVD_QLT_README",
                "check": "review",
            }
        },
        "deviations": {
            "DEV-QUAL-001": {
                "kind": "deviation",
                "status": "open",
                "scope": "repository-policy:file-size",
                "owner": "maintainer",
                "approver": "quality-review",
                "introduced_on": "2026-03-01",
                "review_by": "2026-06-30",
                "rationale": "Temporary split pending.",
                "mitigation": "Review on each change.",
                "closure_criteria": "Split file and remove allowlist path.",
                "paths": ["tests/cmake/targets.cmake"],
                "requirements": ["REQ-COMP-001"],
                "artifacts": ["EVD_QLT_MANIFEST"],
            }
        },
    }
    _write(root / "docs/quality/quality_manifest.json", json.dumps(manifest, indent=2) + "\n")
    _write(root / "tests/checks/policy_allowlist.txt", "tests/cmake/targets.cmake\n")


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
