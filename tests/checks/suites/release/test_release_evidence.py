#!/usr/bin/env python3
from __future__ import annotations

import json
import zipfile
from pathlib import Path

import pytest

from python_tools.ci.release_evidence_pack import ReleaseEvidencePackager, ReleaseEvidencePackError
from python_tools.ci.release_quality_index import ReleaseQualityIndexBuilder, ReleaseQualityIndexError
from python_tools.ci.release_support import build_release_lane_activities, build_release_lane_analyzers


def _write_json(path: Path, payload: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload), encoding="utf-8")


def _write_text(path: Path, content: str = "sample\n") -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def _seed_repo(root: Path) -> None:
    _write_json(root / "docs/quality/quality_manifest.json", {"metadata": {"system": "test", "revision": "2026-03-07"}, "includes": ["manifest/evidence.json", "manifest/requirements.json", "manifest/deviations.json"]})
    _write_json(
        root / "docs/quality/manifest/evidence.json",
        {
            "evidence": {
                "EVD_CI_RELEASE_LANE": ".github/workflows/release-lane.yml",
                "EVD_CI_SECURITY_CODEQL": ".github/workflows/security-codeql.yml",
                "EVD_CI_DEPENDENCY_REVIEW": ".github/workflows/dependency-review.yml",
                "EVD_GITHUB_DEPENDABOT": ".github/dependabot.yml",
                "EVD_QLT_DEVIATION_REGISTER": "docs/quality/manifest/deviations.json",
                "EVD_QLT_EVIDENCE_PACK_FORMAT": "docs/quality/evidence_pack.md",
                "EVD_QLT_MANIFEST": "docs/quality/quality_manifest.json",
                "EVD_QLT_PROD_BASELINE": "docs/quality/prod_baseline.md",
                "EVD_QLT_README": "docs/quality/README.md",
                "EVD_QLT_RELEASE_INDEX_FORMAT": "docs/quality/release_index.md",
                "EVD_QLT_STANDARDS_PROFILE": "docs/quality/standards_profile.md",
                "EVD_SCRIPT_RELEASE_PACKAGE_BUNDLE": "scripts/ci/release/package_bundle.py",
                "EVD_SCRIPT_RELEASE_PACKAGE_EVIDENCE": "scripts/ci/release/package_evidence.py",
                "EVD_SCRIPT_RELEASE_PACKAGE_INDEX": "scripts/ci/release/package_quality_index.py",
                "EVD_SCRIPT_RELEASE_PACKAGE_SBOM": "scripts/ci/release/package_sbom.py",
                "EVD_SAMPLE": "docs/quality/sample_requirement.md",
            }
        },
    )
    _write_json(
        root / "docs/quality/manifest/requirements.json",
        {
            "requirements": {
                "REQ-TEST-001": {"tests": ["^TST_ONE$"], "artifacts": ["EVD_SAMPLE"]},
                "REQ-TEST-002": {"tests": ["^TST_TWO$"], "artifacts": ["EVD_SAMPLE"]},
            }
        },
    )
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
                    "review_by": "2026-06-30",
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
    for rel in [
        ".github/workflows/release-lane.yml",
        ".github/workflows/security-codeql.yml",
        ".github/workflows/dependency-review.yml",
        ".github/dependabot.yml",
        "docs/quality/README.md",
        "docs/quality/standards_profile.md",
        "docs/quality/evidence_pack.md",
        "docs/quality/prod_baseline.md",
        "docs/quality/release_index.md",
        "docs/quality/sample_requirement.md",
        "scripts/ci/release/package_bundle.py",
        "scripts/ci/release/package_evidence.py",
        "scripts/ci/release/package_quality_index.py",
        "scripts/ci/release/package_sbom.py",
    ]:
        _write_text(root / rel)


def _read_json_from_archive(archive: Path, member: str) -> tuple[dict[str, object], list[str]]:
    with zipfile.ZipFile(archive) as bundle:
        return json.loads(bundle.read(member)), bundle.namelist()


def test_release_evidence_packager_packages_selected_requirements(tmp_path: Path) -> None:
    _seed_repo(tmp_path)
    archive = ReleaseEvidencePackager().package(
        root=tmp_path,
        dist_dir=tmp_path / "dist/evidence-pack",
        tag="rc-1",
        profile="prod",
        requirements=["REQ-TEST-001"],
        verification_activities=build_release_lane_activities("prod"),
        analyzer_status=build_release_lane_analyzers(),
        ci_context={"source": "unit-test"},
    )
    payload, names = _read_json_from_archive(archive, "release_evidence_pack.json")
    assert payload["tag"] == "rc-1"
    assert payload["profile"] == "prod"
    assert payload["requirement_ids"] == ["REQ-TEST-001"]
    assert payload["ci_context"] == {"source": "unit-test"}
    assert isinstance(payload["open_exceptions"], list)
    assert payload["open_exceptions"][0]["id"] == "DEV-QUAL-001"
    assert payload["open_exceptions"][0]["paths"] == ["engine/src/backend/SimulationBackend.cpp"]
    assert "evidence/docs/quality/sample_requirement.md" in names
    assert "README.md" in names


def test_release_evidence_packager_defaults_and_rejects_unknown_requirements(tmp_path: Path) -> None:
    _seed_repo(tmp_path)
    archive = ReleaseEvidencePackager().package(root=tmp_path, dist_dir=tmp_path / "dist/evidence-pack", tag="rc-2", profile="prod")
    payload, _ = _read_json_from_archive(archive, "release_evidence_pack.json")
    assert payload["requirement_ids"] == ["REQ-TEST-001", "REQ-TEST-002"]

    with pytest.raises(ReleaseEvidencePackError, match="unknown requirement id"):
        ReleaseEvidencePackager().package(
            root=tmp_path,
            dist_dir=tmp_path / "dist/evidence-pack",
            tag="rc-3",
            profile="prod",
            requirements=["REQ-UNKNOWN-999"],
        )


def test_release_quality_index_packages_selected_requirements(tmp_path: Path) -> None:
    _seed_repo(tmp_path)
    archive = ReleaseQualityIndexBuilder().package(
        root=tmp_path,
        dist_dir=tmp_path / "dist/release-quality-index",
        tag="rc-1",
        profile="prod",
        requirements=["REQ-TEST-001"],
        verification_activities=build_release_lane_activities("prod"),
        analyzer_status=build_release_lane_analyzers(),
        ci_context={"source": "unit-test"},
    )
    payload, names = _read_json_from_archive(archive, "release_quality_index.json")
    assert isinstance(payload["summary"], dict)
    assert isinstance(payload["open_deviations"], list)
    assert payload["tag"] == "rc-1"
    assert payload["summary"]["requirements"] == 1
    assert payload["summary"]["open_deviations"] == 1
    assert payload["requirement_ids"] == ["REQ-TEST-001"]
    assert payload["open_deviations"][0]["id"] == "DEV-QUAL-001"
    assert payload["ci_context"] == {"source": "unit-test"}
    assert "README.md" in names


def test_release_quality_index_defaults_and_rejects_unknown_requirements(tmp_path: Path) -> None:
    _seed_repo(tmp_path)
    archive = ReleaseQualityIndexBuilder().package(root=tmp_path, dist_dir=tmp_path / "dist/release-quality-index", tag="rc-2", profile="prod")
    payload, _ = _read_json_from_archive(archive, "release_quality_index.json")
    assert isinstance(payload["summary"], dict)
    assert payload["summary"]["requirements"] == 2

    with pytest.raises(ReleaseQualityIndexError, match="unknown requirement id"):
        ReleaseQualityIndexBuilder().package(
            root=tmp_path,
            dist_dir=tmp_path / "dist/release-quality-index",
            tag="rc-3",
            profile="prod",
            requirements=["REQ-UNKNOWN-999"],
        )
