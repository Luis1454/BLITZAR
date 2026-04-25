#!/usr/bin/env python3
from __future__ import annotations

import json
import subprocess
from pathlib import Path

from python_tools.core.models import CheckContext, CheckResult
from python_tools.policies.quality_baseline import QualityBaselineCheck
from python_tools.policies.quality_manifest import QualityManifestLoader
from python_tools.policies.test_catalog import TestCatalogCheck
from tests.checks.suites.support.path_specs import TESTS_UNIT_DIR, cpp_file


def _write(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def _write_json(path: Path, payload: object) -> None:
    _write(path, json.dumps(payload) + "\n")


def _init_git_repo(root: Path) -> None:
    subprocess.run(["git", "init"], cwd=root, check=True, capture_output=True, text=True)
    subprocess.run(["git", "config", "user.email", "tests@example.com"], cwd=root, check=True, capture_output=True, text=True)
    subprocess.run(["git", "config", "user.name", "Tests"], cwd=root, check=True, capture_output=True, text=True)


def _seed_minimal_manifest(root: Path, includes: list[str]) -> None:
    _write_json(root / "docs/quality/quality_manifest.json", {"metadata": {"system": "test", "revision": "2026-03-02"}, "includes": includes})


def _seed_catalog_repo(root: Path) -> None:
    _write_json(
        root / "docs/quality/quality_manifest.json",
        {
            "metadata": {"system": "test", "revision": "2026-03-01"},
            "evidence": {"EVD_QLT_MANIFEST": "docs/quality/quality_manifest.json", "EVD_CHECK_MAIN": "tests/checks/check.py"},
            "requirements": {"REQ-TEST-001": {"tests": [".*"], "artifacts": ["EVD_QLT_MANIFEST"]}},
            "crosswalk": {"CTRL-001": {"source_standard": "NPR-7150.2D", "artifact": "EVD_QLT_MANIFEST", "check": "unit"}},
            "test_groups": {},
        },
    )
    _write(root / "tests/checks/check.py", "def main() -> int:\n    return 0\n")


def _seed_required_quality_files(root: Path) -> None:
    for rel, content in {
        "AGENTS.md": "# AGENTS\n",
        "docs/quality/quality-overview.md": "# Quality\n",
        "docs/quality/standards-profile.md": "profile\n",
        "docs/quality/fmea.md": "fmea\n",
        "docs/quality/tool-qualification.md": "tools\n",
        "docs/quality/tool-manifest.md": "tool manifest\n",
        "docs/quality/power-of-10.md": "power of ten\n",
        "docs/quality/production-baseline.md": "prod baseline\n",
        "docs/quality/release-index.md": "release index\n",
        "docs/quality/interface-contracts.md": "contracts\n",
        "docs/quality/ivv-plan.md": "ivv\n",
        "docs/quality/numerical-validation.md": "numerical\n",
        "docs/quality/quality_manifest.json": "{}\n",
        ".github/CODEOWNERS": "* @owner\n",
        ".github/PULL_REQUEST_TEMPLATE.md": "template\n",
        "tests/cmake/targets.cmake": "add_test(NAME TST_QLT_REPO_006_GravityQualityBaselineCheck COMMAND fake)\n",
    }.items():
        _write(root / rel, content)


def _seed_baseline_payloads(root: Path, test_regex: str) -> None:
    _write_json(
        root / "docs/quality/quality_manifest.json",
        {
            "metadata": {"system": "test", "revision": "2026-03-01"},
            "evidence": {
                "EVD_AGENTS": "AGENTS.md",
                "EVD_QLT_MANIFEST": "docs/quality/quality_manifest.json",
                "EVD_QLT_README": "docs/quality/quality-overview.md",
            },
            "policies": {"test_ids": {"repo_quality": {"regex": r"\bTST_QLT_REPO_[0-9]{3}_[A-Za-z0-9_]+\b", "files": ["tests/cmake/targets.cmake"]}}},
            "requirements": {"REQ-COMP-001": {"tests": [test_regex], "artifacts": ["EVD_QLT_MANIFEST"]}},
            "test_groups": {"EVD_CHECK_QUALITY": {"TST-QLT-REPO-006": {"id": "TST_QLT_REPO_006_GravityQualityBaselineCheck", "req_ids": ["REQ-COMP-001"]}}},
            "crosswalk": {"SWE-004": {"source_standard": "NPR-7150.2D", "artifact": "EVD_AGENTS", "check": "review"}},
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
        },
    )
    _write(root / "tests/checks/policy_allowlist.txt", "tests/cmake/targets.cmake\n")


def test_quality_manifest_loader_merges_includes(tmp_path: Path) -> None:
    _write_json(tmp_path / "docs/quality/manifest/evidence.json", {"evidence": {"EVD_A": "a.txt"}})
    _write_json(tmp_path / "docs/quality/manifest/requirements.json", {"requirements": {"REQ-TEST-001": {"tests": [".*"], "artifacts": ["EVD_A"]}}})
    _seed_minimal_manifest(tmp_path, ["manifest/evidence.json", "manifest/requirements.json"])
    result = CheckResult(name="manifest")
    payload = QualityManifestLoader().load(tmp_path, result)
    assert result.errors == []
    assert {"metadata", "evidence", "requirements"} <= set(payload)


def test_quality_manifest_loader_rejects_missing_duplicate_and_cycle(tmp_path: Path) -> None:
    _seed_minimal_manifest(tmp_path, ["manifest/missing.json"])
    result = CheckResult(name="manifest")
    QualityManifestLoader().load(tmp_path, result)
    assert any("missing required quality manifest" in error for error in result.errors)

    _write_json(tmp_path / "docs/quality/manifest/a.json", {"evidence": {"EVD_A": "a.txt"}})
    _write_json(tmp_path / "docs/quality/manifest/b.json", {"evidence": {"EVD_B": "b.txt"}})
    _seed_minimal_manifest(tmp_path, ["manifest/a.json", "manifest/b.json"])
    result = CheckResult(name="manifest")
    QualityManifestLoader().load(tmp_path, result)
    assert any("duplicate top-level key 'evidence'" in error for error in result.errors)

    _seed_minimal_manifest(tmp_path, ["manifest/cycle_a.json"])
    _write_json(tmp_path / "docs/quality/manifest/cycle_a.json", {"includes": ["cycle_b.json"]})
    _write_json(tmp_path / "docs/quality/manifest/cycle_b.json", {"includes": ["cycle_a.json"]})
    result = CheckResult(name="manifest")
    QualityManifestLoader().load(tmp_path, result)
    assert any("include cycle detected" in error for error in result.errors)


def test_test_catalog_passes_with_matching_known_test(tmp_path: Path) -> None:
    _seed_catalog_repo(tmp_path)
    _write(tmp_path / cpp_file(TESTS_UNIT_DIR, "sample"), "TEST(SampleSuite, SampleCase) {}\n")
    payload = json.loads((tmp_path / "docs/quality/quality_manifest.json").read_text(encoding="utf-8"))
    payload["test_groups"] = {"EVD_CHECK_MAIN": {"TST-UNT-SAMP-001": {"id": "SampleSuite.SampleCase", "req_ids": ["REQ-TEST-001"]}}}
    _write_json(tmp_path / "docs/quality/quality_manifest.json", payload)
    result = TestCatalogCheck().run(CheckContext(root=tmp_path, options={"extra_test_ids": set()}))
    assert result.ok
    assert result.errors == []


def test_test_catalog_fails_on_unknown_test_id(tmp_path: Path) -> None:
    _seed_catalog_repo(tmp_path)
    _write(tmp_path / cpp_file(TESTS_UNIT_DIR, "sample"), "TEST(SampleSuite, SampleCase) {}\n")
    payload = json.loads((tmp_path / "docs/quality/quality_manifest.json").read_text(encoding="utf-8"))
    payload["test_groups"] = {"EVD_CHECK_MAIN": {"TST-UNT-SAMP-001": {"id": "MissingSuite.MissingCase", "req_ids": ["REQ-TEST-001"]}}}
    _write_json(tmp_path / "docs/quality/quality_manifest.json", payload)
    result = TestCatalogCheck().run(CheckContext(root=tmp_path, options={"extra_test_ids": set()}))
    assert not result.ok
    assert any("unknown test_id" in error for error in result.errors)


def test_quality_baseline_passes_with_valid_minimal_repo(tmp_path: Path) -> None:
    _init_git_repo(tmp_path)
    _seed_required_quality_files(tmp_path)
    _seed_baseline_payloads(tmp_path, r"^TST_QLT_REPO_006_GravityQualityBaselineCheck$")
    subprocess.run(["git", "add", "AGENTS.md"], cwd=tmp_path, check=True, capture_output=True, text=True)
    result = QualityBaselineCheck().run(CheckContext(root=tmp_path))
    assert result.ok
    assert result.errors == []


def test_test_catalog_fails_when_requirement_test_regex_has_no_match(tmp_path: Path) -> None:
    _seed_required_quality_files(tmp_path)
    _seed_baseline_payloads(tmp_path, r"^NO_MATCH$")
    result = TestCatalogCheck().run(CheckContext(root=tmp_path, options={"extra_test_ids": set()}))
    assert not result.ok
    assert any("did not match any test id" in error for error in result.errors)


def test_quality_baseline_fails_when_agents_is_missing(tmp_path: Path) -> None:
    _init_git_repo(tmp_path)
    _seed_required_quality_files(tmp_path)
    _seed_baseline_payloads(tmp_path, r"^TST_QLT_REPO_006_GravityQualityBaselineCheck$")
    payload = json.loads((tmp_path / "docs/quality/quality_manifest.json").read_text(encoding="utf-8"))
    del payload["evidence"]["EVD_AGENTS"]
    _write_json(tmp_path / "docs/quality/quality_manifest.json", payload)
    result = QualityBaselineCheck().run(CheckContext(root=tmp_path))
    assert not result.ok
    assert any("missing required evidence id: EVD_AGENTS" in error for error in result.errors)

